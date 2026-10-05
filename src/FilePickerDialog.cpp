#include "novu/ui/FilePickerDialog.h"

#include "novu/ui/FilePicker.h"
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
Listing listDirectory(const QDir& dir, const QStringList& patterns)
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

// Where the tree starts: the person's own places, then the volumes.
//
// Favourites first and marked as such, because that is where a file almost
// always is, and a tree that opens on C:\ makes the common case the long way
// round.
QStringList fillRoots(FilePicker* picker)
{
    const QVector<QPair<QStandardPaths::StandardLocation, Glyph>> favourites = {
        {QStandardPaths::DesktopLocation, Glyph::Star},
        {QStandardPaths::DocumentsLocation, Glyph::Star},
        {QStandardPaths::DownloadLocation, Glyph::Star},
        {QStandardPaths::HomeLocation, Glyph::Star},
    };
    QStringList added;
    for (const auto& favourite : favourites) {
        const QString path = QStandardPaths::writableLocation(favourite.first);
        if (path.isEmpty() || added.contains(path) || !QFileInfo(path).isDir()) {
            continue;
        }
        added << path;
        picker->addLocation({}, path, QDir(path).dirName().isEmpty()
                                          ? path : QDir(path).dirName(),
                            favourite.second);
    }

    for (const QStorageInfo& volume : QStorageInfo::mountedVolumes()) {
        if (!volume.isValid() || !volume.isReady()) {
            continue;
        }
        const QString path = volume.rootPath();
        if (path.isEmpty() || added.contains(path)) {
            continue;
        }
        added << path;
        // The volume's own name when it has one, and its path when it does not -
        // an unnamed volume is still somewhere to go.
        const QString name = volume.displayName().isEmpty()
            ? path : QStringLiteral("%1 (%2)").arg(volume.displayName(), path);
        picker->addLocation({}, path, name, Glyph::Folder);
    }
    return added;
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
            QString defaultSuffix)
        : m_dialog(new Dialog(title, parent))
        , m_picker(new FilePicker(m_dialog))
        , m_patterns(patterns)
        , m_saving(saving)
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
        m_picker->setConfirmLabel(saving ? tr("Save") : tr("Open"));

        const QStringList roots = fillRoots(m_picker);
        for (const QString& root : roots) {
            m_known.insert(root);
        }

        connect(m_picker, &FilePicker::locationActivated, this, &Session::navigate);
        connect(m_picker, &FilePicker::entryActivated, this, &Session::activateEntry);
        connect(m_picker, &FilePicker::selectionChanged, this, [this] { refreshConfirm(); });
        connect(m_picker, &FilePicker::cancelled, m_dialog, &QDialog::reject);
        connect(m_picker, &FilePicker::confirmed, this, &Session::confirm);

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
        QFileInfo info(startPath);
        QString directory;
        QString name;
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
        const Listing listing = listDirectory(dir, m_patterns);
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
        m_dialog->accept();
    }

    Dialog* m_dialog = nullptr;
    FilePicker* m_picker = nullptr;
    QStringList m_patterns;
    bool m_saving = false;
    QString m_defaultSuffix;
    QString m_current;
    QStringList m_folderPaths;
    QVector<FilePicker::Entry> m_entries;
    // Nodes the tree holds, and nodes whose children it already holds. Two
    // different questions: a folder can be in the tree without having been
    // opened.
    QSet<QString> m_known;
    QSet<QString> m_populated;
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

} // namespace novu::ui
