#include "novu/ui/FilePickerDialog.h"

#include "novu/ui/FilePicker.h"
#include "novu/ui/FilePickerPlaces.h"
#include "novu/ui/Popover.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/WindowShell.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QStandardPaths>
#include <QSet>
#include <QStorageInfo>
#include <QVBoxLayout>

namespace novu::ui {
namespace {

// The entry that goes back up.
//
// FilePicker lists what a location holds and nothing else, and the tree is the
// only way out of a folder - which works until someone drills three levels into
// a drive whose tree nodes they never expanded. A parent entry in the listing
// is how every file manager answers this, and it costs the picker nothing: it
// is just another folder row, and the driver knows what it means.
const QString kParentEntry = QStringLiteral("..");

QString formattedSize(qint64 bytes)
{
    // QLocale, so a size reads the way the rest of the person's system writes
    // numbers. FilePicker takes the string already formatted precisely because
    // it has no business deciding this.
    return QLocale::system().formattedDataSize(bytes);
}

struct Listing
{
    QVector<FilePicker::Entry> entries;
    QStringList folderPaths;   //!< parallel to the folder entries, in order
};

// What one directory holds, folders first, each sorted by name.
//
// Hidden entries are left out. A picker is for choosing a file to work with,
// and a listing that leads with forty dotfiles buries the one thing the person
// came for; anyone who needs one can type its name.
Listing listDirectory(const QDir& dir, const QStringList& patterns, bool foldersOnly = false)
{
    Listing listing;

    const QFileInfo self(dir.absolutePath());
    if (!self.isRoot()) {
        FilePicker::Entry up;
        up.name = kParentEntry;
        up.kind = FilePicker::EntryKind::Folder;
        up.type = QObject::tr("Parent folder");
        listing.entries.append(up);
        listing.folderPaths.append(QFileInfo(dir.absolutePath()).dir().absolutePath());
    }

    const QFileInfoList folders =
        dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo& info : folders) {
        FilePicker::Entry entry;
        entry.name = info.fileName();
        entry.kind = FilePicker::EntryKind::Folder;
        entry.dateModified = info.lastModified().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
        entry.type = QObject::tr("Folder");
        listing.entries.append(entry);
        listing.folderPaths.append(info.absoluteFilePath());
    }

    // Choosing a folder: a file is not an answer, so it is not offered.
    if (foldersOnly) {
        return listing;
    }

    // No patterns means everything: "show me all files" is a real answer and
    // not a missing filter.
    const QFileInfoList files = patterns.isEmpty()
        ? dir.entryInfoList(QDir::Files, QDir::Name | QDir::IgnoreCase)
        : dir.entryInfoList(patterns, QDir::Files, QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo& info : files) {
        FilePicker::Entry entry;
        entry.name = info.fileName();
        entry.kind = FilePicker::EntryKind::File;
        entry.dateModified = info.lastModified().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
        const QString suffix = info.suffix();
        entry.type = suffix.isEmpty() ? QObject::tr("File")
                                      : QObject::tr("%1 file").arg(suffix.toUpper());
        entry.size = formattedSize(info.size());
        listing.entries.append(entry);
    }
    return listing;
}

// The system's own places: always there, and not the person's to rename or
// remove. The ones they pinned themselves come from FilePickerPlaces.
QStringList systemPlaces()
{
    const QVector<QStandardPaths::StandardLocation> locations = {
        QStandardPaths::DesktopLocation,
        QStandardPaths::DocumentsLocation,
        QStandardPaths::DownloadLocation,
        QStandardPaths::HomeLocation,
    };
    QStringList places;
    for (const auto location : locations) {
        const QString path =
            FilePickerPlaces::normalised(QStandardPaths::writableLocation(location));
        if (!path.isEmpty() && !places.contains(path) && QFileInfo(path).isDir()) {
            places << path;
        }
    }
    return places;
}

QString nameOf(const QString& path)
{
    const QString name = QDir(path).dirName();
    return name.isEmpty() ? path : name;
}

// The window, and everything that drives it.
//
// A QObject so the signal connections have an owner, and a stack object so that
// owner is exactly the duration of the call. It owns the window rather than the
// other way round.
class Session : public QObject
{
public:
    Session(QWidget* parent, const QString& title, const QString& note,
            const QString& startPath, const QStringList& patterns, bool saving,
            QString defaultSuffix, bool folders = false)
        : m_dialog(new Dialog(title, parent))
        , m_picker(new FilePicker(m_dialog))
        , m_patterns(patterns)
        , m_saving(saving)
        , m_folders(folders)
        , m_defaultSuffix(std::move(defaultSuffix))
    {
        m_dialog->setHeading(title, note, Dialog::Tone::Neutral,
                             saving ? Glyph::Save : Glyph::Folder);
        // No inset set here any more: Dialog carries the modal's own padding
        // from its constructor, so adding one here would double it.
        m_dialog->contentLayout()->addWidget(m_picker, 1);
        // FilePicker carries its own Cancel and confirm button, so the
        // dialog's footer is deliberately never created - two button rows in
        // one window would be the picker disagreeing with its own host.
        m_picker->setConfirmLabel(saving ? tr("Save")
                                         : folders ? tr("Select folder") : tr("Open"));

        fillRoots();

        connect(m_picker, &FilePicker::locationActivated, this, &Session::navigate);
        connect(m_picker, &FilePicker::locationMenuRequested, this, &Session::locationMenu);
        connect(m_picker, &FilePicker::entryMenuRequested, this, &Session::entryMenu);
        connect(m_picker, &FilePicker::locationRenamed, this, &Session::renameFavourite);
        connect(m_picker, &FilePicker::entryActivated, this, &Session::activateEntry);
        connect(m_picker, &FilePicker::selectionChanged, this, [this] { refreshConfirm(); });
        connect(m_picker, &FilePicker::cancelled, m_dialog, &QDialog::reject);
        connect(m_picker, &FilePicker::confirmed, this, &Session::confirm);
        connect(m_picker, &FilePicker::newFolderRequested, this, &Session::newFolder);

        // Where a file is about to be written or a folder chosen, the folder
        // the person wants is often one that does not exist yet. Opening a
        // file never needs one.
        m_picker->setNewFolderVisible(saving || folders);

        openStart(startPath);
        m_dialog->resize(880, 560);
    }

    // The dialog is owned here rather than by the caller's widget: a picker is
    // a question with an answer, and leaving the window parented to the screen
    // behind it would keep one alive per call for the life of that screen.
    // Session lives on the stack for exactly as long as the question is open.
    ~Session() override { delete m_dialog; }

    QString run()
    {
        return m_dialog->exec() == QDialog::Accepted ? m_chosen : QString();
    }

private:
    // `startPath` is allowed to be either a directory to open or a file to
    // start on, because both callers are natural: "open in Documents" and
    // "save as this particular path".
    void openStart(const QString& startPath)
    {
        // No start given is the common case for opening - and it used to mean
        // the process's working directory, which is wherever the application
        // happened to be launched from. Where the person last chose a file is
        // nearly always where the next one is.
        if (startPath.trimmed().isEmpty()) {
            const QString last = FilePickerPlaces::lastDirectory();
            navigate(last.isEmpty()
                         ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                         : last);
            refreshConfirm();
            return;
        }
        QFileInfo info(startPath);
        QString directory;
        QString name;
        // A name with no folder of its own - "trades.csv" - is a suggestion
        // for what to call the file, not a path relative to wherever the
        // process was started. It goes where the last one went.
        const QString slashed = QDir::fromNativeSeparators(startPath.trimmed());
        if (!QDir::isAbsolutePath(slashed) && !slashed.contains(QLatin1Char('/'))) {
            const QString last = FilePickerPlaces::lastDirectory();
            navigate(last.isEmpty()
                         ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                         : last);
            m_picker->setSelectedName(slashed);
            refreshConfirm();
            return;
        }
        if (info.isDir()) {
            directory = info.absoluteFilePath();
        } else {
            directory = info.absolutePath();
            name = info.fileName();
        }
        if (directory.isEmpty() || !QFileInfo(directory).isDir()) {
            // A path that has gone away - a removed drive, a deleted folder -
            // opens at home rather than showing an empty window about a place
            // that no longer exists.
            directory = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        }
        navigate(directory);
        if (!name.isEmpty()) {
            m_picker->setSelectedName(name);
        }
        refreshConfirm();
    }

    void navigate(const QString& path)
    {
        const QDir dir(path);
        if (!dir.exists()) {
            return;
        }
        m_current = dir.absolutePath();
        const Listing listing = listDirectory(dir, m_patterns, m_folders);
        m_folderPaths = listing.folderPaths;
        m_entries = listing.entries;

        // The tree is filled as it is walked, which is what FilePicker's own
        // lazy contract asks for: a node's children are added when the node is
        // opened, not guessed at ahead of time.
        //
        // Guarded on both sides. The branch down to here has to exist before a
        // child can hang off it - a window opened directly on a deep path has
        // never shown that chain - and a folder's children are added once, or
        // navigating out and back in would list every one of them twice.
        ensureBranch(m_current);
        if (!m_populated.contains(m_current)) {
            m_populated.insert(m_current);
            for (int i = 0; i < listing.folderPaths.size(); ++i) {
                if (listing.entries.at(i).name == kParentEntry) {
                    continue;
                }
                const QString child = listing.folderPaths.at(i);
                m_picker->addLocation(m_current, child, listing.entries.at(i).name,
                                      Glyph::Folder);
                m_known.insert(child);
            }
        }
        m_picker->setCurrentLocation(m_current);
        m_picker->setEntries(m_entries);
        m_picker->setLocationExpanded(m_current, true);
        refreshConfirm();
    }

    // Where the tree starts: the folders the person pinned, the system's own
    // places, then the volumes.
    //
    // Pinned ones first. They are there because the person put them there,
    // which makes them the likeliest place the file is - and a tree that opens
    // on a drive root makes the common case the long way round.
    //
    // Order matters for more than reading: a tree id belongs to whichever node
    // claimed it first (TreeView drops a duplicate), so a pinned folder has to
    // be added before the drive it lives on is walked down to it.
    void fillRoots()
    {
        m_favourites.clear();
        m_fixed.clear();
        QStringList added;

        for (const FilePickerPlaces::Favourite& favourite : FilePickerPlaces::favourites()) {
            // Kept, not shown, while its folder is missing - an unplugged
            // drive's favourite comes back with the drive.
            if (added.contains(favourite.path) || !QFileInfo(favourite.path).isDir()) {
                continue;
            }
            added << favourite.path;
            m_favourites.insert(favourite.path);
            m_picker->addLocation({}, favourite.path, favourite.label, Glyph::Star);
        }

        for (const QString& path : systemPlaces()) {
            if (added.contains(path)) {
                continue;
            }
            added << path;
            m_fixed.insert(path);
            m_picker->addLocation({}, path, nameOf(path), Glyph::Star);
        }

        for (const QStorageInfo& volume : QStorageInfo::mountedVolumes()) {
            if (!volume.isValid() || !volume.isReady()) {
                continue;
            }
            const QString path = FilePickerPlaces::normalised(volume.rootPath());
            if (path.isEmpty() || added.contains(path)) {
                continue;
            }
            added << path;
            // The volume's own name when it has one, and its path when it
            // does not - an unnamed volume is still somewhere to go.
            const QString name = volume.displayName().isEmpty()
                ? path : QStringLiteral("%1 (%2)").arg(volume.displayName(), path);
            m_picker->addLocation({}, path, name, Glyph::Folder);
        }

        for (const QString& root : added) {
            m_known.insert(root);
        }
    }

    // Pinning or unpinning changes which node owns a path, and the tree has no
    // way to move a node - so it is rebuilt, and the listing re-opened where
    // it was. What was typed in the name field survives it.
    void rebuildTree()
    {
        const QString typed = m_picker->selectedName();
        m_picker->clearLocations();
        m_known.clear();
        m_populated.clear();
        fillRoots();
        navigate(m_current);
        m_picker->setSelectedName(typed);
        refreshConfirm();
    }

    bool canPin(const QString& path) const
    {
        return !path.isEmpty() && !m_favourites.contains(path) && !m_fixed.contains(path)
            && QFileInfo(path).isDir();
    }

    void locationMenu(const QString& id, const QPoint& globalPosition)
    {
        if (m_favourites.contains(id)) {
            showMenu(id, globalPosition, /*pinned=*/true);
        } else if (canPin(id)) {
            showMenu(id, globalPosition, /*pinned=*/false);
        }
        // A system place, or a click on no row: nothing to offer, and an
        // empty menu would be a menu that says so.
    }

    void entryMenu(int row, const QPoint& globalPosition)
    {
        if (row < 0 || row >= m_entries.size() || row >= m_folderPaths.size()) {
            return;   // a file, or below the last row
        }
        const FilePicker::Entry& entry = m_entries.at(row);
        if (entry.kind != FilePicker::EntryKind::Folder || entry.name == kParentEntry) {
            return;
        }
        const QString path = FilePickerPlaces::normalised(m_folderPaths.at(row));
        if (m_favourites.contains(path)) {
            showMenu(path, globalPosition, /*pinned=*/true);
        } else if (canPin(path)) {
            showMenu(path, globalPosition, /*pinned=*/false);
        }
    }

    void showMenu(const QString& path, const QPoint& globalPosition, bool pinned)
    {
        auto* menu = new Popover(m_dialog);
        if (pinned) {
            menu->addItem(QStringLiteral("rename"), tr("Rename"), Glyph::Edit);
            menu->setMeta(QStringLiteral("rename"), QStringLiteral("F2"));
            menu->addItem(QStringLiteral("unpin"), tr("Remove from favourites"), Glyph::Trash);
        } else {
            menu->addItem(QStringLiteral("pin"), tr("Add to favourites"), Glyph::Star);
        }
        connect(menu, &Popover::closed, menu, &QObject::deleteLater);
        connect(menu, &Popover::triggered, this, [this, path](const QString& key) {
            if (key == QLatin1String("pin")) {
                if (FilePickerPlaces::addFavourite(path)) {
                    rebuildTree();
                }
            } else if (key == QLatin1String("unpin")) {
                if (FilePickerPlaces::removeFavourite(path)) {
                    rebuildTree();
                }
            } else if (key == QLatin1String("rename")) {
                m_picker->renameLocation(path);
            }
        });
        menu->popupAt(globalPosition);
    }

    // Only a favourite has a name of its own to change. F2 still opens the
    // editor on any row - that is the tree's gesture, not this dialog's - and
    // on anything else the row simply keeps its name.
    void renameFavourite(const QString& id, const QString& label)
    {
        if (id == m_created) {
            renameCreated(label);
            return;
        }
        if (m_favourites.contains(id) && FilePickerPlaces::renameFavourite(id, label)) {
            m_picker->setLocationLabel(id, label.trimmed());
        }
    }

    // NEW FOLDER, the way a file manager does it: the folder exists the moment
    // the button is pressed, as "New folder", and its name is then edited in
    // place. Escape keeps the name it was given - the folder is already there,
    // and a picker that deleted it again would be deleting something on disk
    // behind the person's back.
    void newFolder()
    {
        const QDir dir(m_current);
        if (!dir.exists()) {
            return;
        }
        QString name = tr("New folder");
        for (int n = 2; QFileInfo::exists(dir.filePath(name)); ++n) {
            name = tr("New folder (%1)").arg(n);
        }
        if (!dir.mkdir(name)) {
            Dialog::reportError(m_dialog, tr("Could not create the folder"),
                                tr("%1 could not be created in %2.")
                                    .arg(name, QDir::toNativeSeparators(m_current)));
            return;
        }
        m_created = FilePickerPlaces::normalised(dir.filePath(name));
        showCreated();
        m_picker->renameLocation(m_created);
    }

    // Only the folder made in this window can be renamed here: renaming a
    // folder that was already on disk is a file manager's job, not a
    // picker's.
    void renameCreated(const QString& label)
    {
        const QString name = label.trimmed();
        const QFileInfo created(m_created);
        if (name.isEmpty() || name == created.fileName()) {
            return;
        }
        if (name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\'))
            || name == QLatin1String(".") || name == kParentEntry) {
            Dialog::reportError(m_dialog, tr("Could not rename the folder"),
                                tr("A folder name cannot contain / or \\."));
            m_picker->renameLocation(m_created);
            return;
        }
        QDir parent = created.dir();
        if (QFileInfo::exists(parent.filePath(name))) {
            Dialog::reportError(m_dialog, tr("Could not rename the folder"),
                                tr("%1 already exists here.").arg(name));
            m_picker->renameLocation(m_created);
            return;
        }
        if (!parent.rename(created.fileName(), name)) {
            Dialog::reportError(m_dialog, tr("Could not rename the folder"),
                                tr("%1 could not be renamed to %2.")
                                    .arg(created.fileName(), name));
            return;
        }
        m_created = FilePickerPlaces::normalised(parent.filePath(name));
        showCreated();
    }

    // The new folder in the tree and the listing, selected - and, when it is a
    // folder being chosen, the answer the confirm button gives.
    void showCreated()
    {
        // The tree has no way to move or re-sort a node, so it is rebuilt,
        // which also lists the folder the new one is in afresh.
        rebuildTree();
        const QString name = QFileInfo(m_created).fileName();
        m_picker->selectEntry(name);
        if (m_folders) {
            m_picker->setSelectedName(name);
        }
        refreshConfirm();
    }

    // Walks up until it reaches somewhere the tree already knows - a root, or a
    // folder opened earlier - and then adds the chain back down, so the tree
    // always shows where the listing is.
    void ensureBranch(const QString& path)
    {
        QStringList chain;
        QString walk = path;
        while (!walk.isEmpty() && !m_known.contains(walk)) {
            chain.prepend(walk);
            const QString parent = QFileInfo(walk).dir().absolutePath();
            if (parent == walk || parent.isEmpty()) {
                // A root nothing was listed under: QStorageInfo did not report
                // it, which happens for a mount the system hides. It becomes a
                // root of its own rather than being left unreachable.
                walk.clear();
                break;
            }
            walk = parent;
        }
        QString parentId = m_known.contains(walk) ? walk : QString();
        for (const QString& node : chain) {
            const QString name = QDir(node).dirName();
            m_picker->addLocation(parentId, node, name.isEmpty() ? node : name,
                                  Glyph::Folder);
            m_known.insert(node);
            parentId = node;
        }
    }

    void activateEntry(int row)
    {
        if (row < 0 || row >= m_entries.size()) {
            return;
        }
        if (m_entries.at(row).kind == FilePicker::EntryKind::Folder) {
            if (row < m_folderPaths.size()) {
                navigate(m_folderPaths.at(row));
            }
            return;
        }
        // Opening a file is the same act as pressing the button on it.
        confirm(m_entries.at(row).name);
    }

    // Saving accepts a name that does not exist yet; opening does not. Without
    // this an Open window would hand back a path to nothing and the caller
    // would report a read error for what was really a typo.
    void refreshConfirm()
    {
        // Choosing a folder always has an answer: the one that is open.
        if (m_folders) {
            m_picker->setConfirmEnabled(QFileInfo(m_current).isDir());
            return;
        }
        const QString name = m_picker->selectedName().trimmed();
        if (name.isEmpty()) {
            m_picker->setConfirmEnabled(false);
            return;
        }
        m_picker->setConfirmEnabled(
            m_saving || QFileInfo(QDir(m_current).filePath(name)).isFile());
    }

    void confirm(const QString& rawName)
    {
        QString name = rawName.trimmed();

        if (m_folders) {
            // The folder selected in the listing, a path typed in, or - with
            // nothing selected - the folder that is open. ".." is a way out,
            // not an answer, so it means the open folder too.
            QString path = m_current;
            if (!name.isEmpty() && name != kParentEntry) {
                const QFileInfo info(name);
                const QString candidate = info.isAbsolute()
                    ? info.absoluteFilePath() : QDir(m_current).absoluteFilePath(name);
                if (QFileInfo(candidate).isDir()) {
                    path = candidate;
                }
            }
            if (!QFileInfo(path).isDir()) {
                return;
            }
            m_chosen = QDir::cleanPath(path);
            FilePickerPlaces::setLastDirectory(m_chosen);
            m_dialog->accept();
            return;
        }

        if (name.isEmpty()) {
            return;
        }

        // A name typed with a path in it is honoured - people paste paths - and
        // resolved against where the window currently is when it is relative.
        QFileInfo info(name);
        QString path = info.isAbsolute() ? info.absoluteFilePath()
                                         : QDir(m_current).absoluteFilePath(name);

        if (QFileInfo(path).isDir()) {
            // Not a choice, a destination.
            navigate(path);
            m_picker->setSelectedName(QString());
            return;
        }

        if (m_saving) {
            if (QFileInfo(path).suffix().isEmpty() && !m_defaultSuffix.isEmpty()) {
                path += QLatin1Char('.') + m_defaultSuffix;
            }
            if (QFileInfo::exists(path)
                && !Dialog::confirm(
                       m_dialog, tr("Replace this file?"),
                       tr("%1 already exists. Saving will overwrite it.")
                           .arg(QFileInfo(path).fileName()),
                       tr("Replace"), tr("Cancel"), true)) {
                return;
            }
        } else if (!QFileInfo(path).isFile()) {
            return;
        }

        m_chosen = path;
        FilePickerPlaces::setLastDirectory(QFileInfo(path).absolutePath());
        m_dialog->accept();
    }

    Dialog* m_dialog = nullptr;
    FilePicker* m_picker = nullptr;
    QStringList m_patterns;
    bool m_saving = false;
    bool m_folders = false;
    QString m_defaultSuffix;
    QString m_current;
    QStringList m_folderPaths;
    QVector<FilePicker::Entry> m_entries;
    // Nodes the tree holds, and nodes whose children it already holds. Two
    // different questions: a folder can be in the tree without having been
    // opened.
    QSet<QString> m_known;
    QSet<QString> m_populated;
    // Roots the person pinned, and roots the system provides. Kept apart
    // because only the first may be renamed or removed.
    QSet<QString> m_favourites;
    QSet<QString> m_fixed;
    // The folder made with New folder in this window, the only one it renames.
    QString m_created;
    QString m_chosen;
};

QStringList patternsOf(const QVector<FilePickerDialog::Filter>& filters)
{
    QStringList patterns;
    for (const FilePickerDialog::Filter& filter : filters) {
        for (const QString& pattern : filter.patterns) {
            // "*" is "everything", and mixing it with a narrower pattern means
            // everything - so the whole filter collapses rather than leaving a
            // list that claims to narrow and does not.
            if (pattern.trimmed() == QLatin1String("*")) {
                return {};
            }
            if (!patterns.contains(pattern)) {
                patterns << pattern;
            }
        }
    }
    return patterns;
}

// What the window says it is listing, so a filtered view never looks like an
// empty folder.
QString noteOf(const QVector<FilePickerDialog::Filter>& filters, const QStringList& patterns)
{
    if (patterns.isEmpty()) {
        return QObject::tr("All files.");
    }
    QStringList labels;
    for (const FilePickerDialog::Filter& filter : filters) {
        if (!filter.label.trimmed().isEmpty()) {
            labels << filter.label.trimmed();
        }
    }
    if (labels.isEmpty()) {
        return QObject::tr("Showing %1").arg(patterns.join(QStringLiteral(", ")));
    }
    return QObject::tr("Showing %1 (%2)")
        .arg(labels.join(QStringLiteral(", ")), patterns.join(QStringLiteral(", ")));
}

// "*.csv" -> "csv". What a typed name gets when it carries no extension.
QString suffixOf(const QVector<FilePickerDialog::Filter>& filters)
{
    for (const FilePickerDialog::Filter& filter : filters) {
        for (const QString& pattern : filter.patterns) {
            const int dot = pattern.lastIndexOf(QLatin1Char('.'));
            if (dot >= 0 && dot + 1 < pattern.size()) {
                const QString suffix = pattern.mid(dot + 1);
                if (!suffix.contains(QLatin1Char('*'))) {
                    return suffix;
                }
            }
        }
    }
    return {};
}

} // namespace

QString FilePickerDialog::open(QWidget* parent, const QString& title,
                               const QString& startPath, const QVector<Filter>& filters)
{
    const QStringList patterns = patternsOf(filters);
    Session session(parent, title, noteOf(filters, patterns), startPath, patterns,
                    /*saving=*/false, QString());
    return session.run();
}

QString FilePickerDialog::save(QWidget* parent, const QString& title,
                               const QString& suggestedPath, const QVector<Filter>& filters)
{
    const QStringList patterns = patternsOf(filters);
    Session session(parent, title, noteOf(filters, patterns), suggestedPath, patterns,
                    /*saving=*/true, suffixOf(filters));
    return session.run();
}

QString FilePickerDialog::chooseFolder(QWidget* parent, const QString& title,
                                       const QString& startPath)
{
    Session session(parent, title, QObject::tr("Folders only."), startPath, {},
                    /*saving=*/false, QString(), /*folders=*/true);
    return session.run();
}

} // namespace novu::ui
