#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/Icon.h"

#include <QPoint>
#include <QString>
#include <QVector>
#include <QWidget>

class QLineEdit;

namespace novu::ui {

class Button;
class Splitter;
class TableView;
class TreeView;

// Navigation on the left, what a location holds on the right, the choice
// made at the bottom - the shape every open and save dialog in these designs
// is built from, in NovuUI's own style rather than the platform's.
//
// It is not a QFileDialog with a stylesheet, and it does no filesystem I/O of
// its own: a caller populates the tree with whatever locations it wants shown
// (drives, favourites, folders opened so far), and hands over a fresh list of
// entries whenever the current location changes. FilePicker only ever reports
// intent - which location was opened, which row was activated, what the
// confirmed name is - the same boundary every NovuUI shell keeps between
// presentation and what it presents.
//
// Built from three existing components rather than three new ones: TreeView
// for the tree, TableView for the listing, Splitter between them.
class NOVUUI_API FilePicker : public QWidget
{
    Q_OBJECT

public:
    enum class EntryKind { Folder, File };

    struct Entry
    {
        QString name;
        EntryKind kind = EntryKind::File;
        QString dateModified;
        QString type;
        // Already formatted ("1,53 KB") - FilePicker does not know what unit
        // a caller's platform counts in. Empty reads as nothing rather than
        // as zero, which is what a folder's own size cell is.
        QString size;
        // Glyph::None falls back to Folder or File by kind, so a caller that
        // has nothing more specific than "this is a folder" still gets an
        // icon.
        Glyph glyph = Glyph::None;
    };

    explicit FilePicker(QWidget* parent = nullptr);

    // ── the tree ─────────────────────────────────────────────────────────
    // `id` is the caller's own - a path, most often - and is never shown.
    // `parentId` empty adds a root (a drive, or a pinned favourite).
    //
    // `expandable` defaults true: a drive or a folder is not known to be a
    // leaf until it has actually been listed, and FilePicker's own tree is
    // filled lazily - locationActivated() asks for a location's contents,
    // and the caller answers with more addLocation() calls once it has
    // them, the same way NavList's expandToggled() already works. Without
    // this every location added ahead of being opened had no children yet
    // and so drew with no chevron at all - indistinguishable from an empty
    // drive. Pass false only for something that really is always a leaf.
    void addLocation(const QString& parentId, const QString& id,
                     const QString& label, Glyph glyph = Glyph::Folder,
                     bool expandable = true);
    void setLocationExpanded(const QString& id, bool expanded);
    void clearLocations();
    // What a location is called, changed in place - a favourite the person
    // renamed. The id, and so everything under it, stays where it is.
    void setLocationLabel(const QString& id, const QString& label);
    // Starts the tree's own in-row rename on `id`. What the new name MEANS is
    // the caller's: it arrives as locationRenamed(), and the row keeps its old
    // label until the caller answers with setLocationLabel().
    void renameLocation(const QString& id);

    QString currentLocation() const { return m_currentLocation; }
    // Silent - does not itself ask for the folder's contents. A caller
    // driving the tree by hand (restoring a session, revealing a path) sets
    // this and then calls setEntries() once it has the listing, the same
    // two steps setCurrentLocation()/setEntries() always are.
    void setCurrentLocation(const QString& id);

    // ── the content ──────────────────────────────────────────────────────
    // Replaces every row. Called once per location change - FilePicker holds
    // no filesystem state of its own to diff the new list against.
    void setEntries(const QVector<Entry>& entries);
    void clearEntries();

    // ── the choice ───────────────────────────────────────────────────────
    QString selectedName() const;
    void setSelectedName(const QString& name);

    // "Select" / "Open" / "Save" - what the primary action means changes
    // with the caller's own mode; the shape does not.
    void setConfirmLabel(const QString& text);
    void setConfirmEnabled(bool enabled);

    TreeView* tree() const { return m_tree; }
    TableView* table() const { return m_table; }
    Splitter* splitter() const { return m_splitter; }

signals:
    // A location in the tree was opened. A caller fetches its contents and
    // answers with setCurrentLocation() + setEntries() - FilePicker does not
    // assume the fetch will succeed, so it does not move there itself.
    void locationActivated(const QString& id);
    // A row in the table was opened (double-click or Enter). A folder drills
    // in and a file confirms the pick, but only the caller knows which this
    // row was, so both read the same signal and decide from `kind`.
    void entryActivated(int row);
    void selectionChanged(const QString& name);
    void confirmed(const QString& name);
    void cancelled();
    // A right-click on a location in the tree, with that location already
    // made current. Empty `id` when the click landed on no row.
    void locationMenuRequested(const QString& id, const QPoint& globalPosition);
    // A right-click on a row of the listing. -1 when it landed below the last
    // row. Which rows a menu applies to is the caller's call, the same way
    // entryActivated() leaves folder-or-file to it.
    void entryMenuRequested(int row, const QPoint& globalPosition);
    // The in-row rename finished with a new, non-empty name. Not emitted for
    // an abandoned rename or an unchanged one.
    void locationRenamed(const QString& id, const QString& label);

private:
    void handleLocationActivated(const QString& id);
    void handleTableActivated(int row);
    void handleSelectionChanged();

    TreeView* m_tree = nullptr;
    Splitter* m_splitter = nullptr;
    TableView* m_table = nullptr;
    QLineEdit* m_nameField = nullptr;
    Button* m_cancelButton = nullptr;
    Button* m_confirmButton = nullptr;
    QVector<Entry> m_entries;
    QString m_currentLocation;
};

} // namespace novu::ui
