#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/Icon.h"

#include <QFont>
#include "novu/ui/StatusDot.h"
#include "novu/ui/StyleSpec.h"
#include "novu/ui/Theme.h"

#include <QSet>
#include <QString>
#include <QVector>
#include <QWidget>
#include <QPointer>

class QLineEdit;
class QMimeData;
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;
class QResizeEvent;

namespace novu::ui {

// The type a rail's section headings are set in - NOW, TRADING, MODULE.
//
// Exported because a heading does not always live inside the list. NovuSpace
// pins its account control under the navigation, in the sidebar's own actions
// column, and its caption has to sit in the same column of words as the ones
// above it. Copying the three lines that build this font is how two headings
// start looking almost alike.
QFont navSectionFont(const QWidget* context);

// The rows in a sidebar: the design's `.navbtn`, `.navbtn.child` and
// `.rail-sec`, as one painted list.
//
// ── why one widget ────────────────────────────────────────────────────────
//
// A sidebar is twenty to sixty rows, each of which the HTML draws as a button
// containing an icon, a label, a twisty, a status dot and a count pill. Built
// literally that is up to three hundred QWidgets for a panel whose entire job
// is to say which page you are on. Every one of them takes a theme change, a
// polish pass and a paint event, and a stylesheet aimed at any one of them
// cascades into the other four.
//
// So the list is one widget and the rows are data. Hit testing is per row and
// per *zone* within a row — the twisty is a separate target from the label,
// because expanding a workspace and opening it are different actions on the
// same line, exactly as they are in a file tree.
//
// ── what a row can carry ──────────────────────────────────────────────────
//
// All of it is the design's, and none of it is new:
//
//     glyph        `.navbtn svg`
//     label        `.navbtn .lbl`, elided
//     count        `.navbtn .cnt`, with `.alert` and `.bad` tones
//     status dot   `.navbtn .wsdot` — active / idle / warn / error / archived
//     twisty       `.navbtn .tw`, rotating 90 degrees when open
//     child depth  `.navbtn.child`, one indent and a square marker
//     disabled     `.navbtn[disabled]`
//     section      `.rail-sec`, a tracked caption between groups
//
// ── collapsed ─────────────────────────────────────────────────────────────
//
// The design collapses the whole rail to icons by hiding the label, the count
// and the twisty and centring what is left. That is a property of the list, not
// of each row, so it is one call here rather than a flag on every entry.
class NOVUUI_API NavList : public QWidget
{
    Q_OBJECT

public:
    // The tone of a count pill. Not a SemanticRole because only three of the
    // five are ever drawn here, and the design is explicit about why: *a count
    // that needs a person is amber. This is the one place a status colour
    // appears in the chrome.*
    enum class CountTone { Normal, Alert, Bad };

    // Which side of the row the status dot sits on. The NovuSpace design
    // puts it before the label; NovuAI puts it at the trailing edge, in
    // front of the count. One drawing, two placements.
    enum class StatusPlacement { Leading, Trailing };

    explicit NavList(QWidget* parent = nullptr);

    // `key` is the caller's own identifier and what every signal carries. It is
    // never shown, so it can stay stable while a label is translated.
    int addItem(const QString& key, const QString& label, Glyph glyph = Glyph::None);
    // One level in from the preceding item, and deeper on request.
    //
    // The rail is a tree and every level steps by the same amount, so a row's
    // label always sits to the right of its parent's. `depth` counts from the
    // item the row hangs under: 1 is a child, 2 a grandchild.
    //
    // An earlier note here said depth stopped at one so that a sidebar would
    // not turn into a file browser. The Workspaces tree in the approved design
    // *is* a file browser, and stopping at one is what made a workspace and the
    // documents inside it sit at the same apparent level.
    int addChild(const QString& key, const QString& label,
                 Glyph glyph = Glyph::None, int depth = 1);
    // `.rail-sec` — a caption, not a row. Not selectable and not counted.
    // A section may carry a key, which is what makes it addressable: a
    // collapsible section, or one with an action on its heading, needs to
    // be named the way a row is.
    int addSection(const QString& title, const QString& key = QString());

    // A caption's own index. Separate from indexOfKey(), which skips captions
    // so that setCurrentKey() cannot land on one.
    int indexOfSection(const QString& key) const;
    void clear();

    int count() const { return m_rows.size(); }
    int indexOfKey(const QString& key) const;
    QString keyAt(int index) const;
    QStringList keys() const;

    QString currentKey() const;
    int currentIndex() const { return m_current; }
    // Silent.
    void setCurrentKey(const QString& key);
    void setCurrentIndex(int index);
    // Asks, and says so. What a click goes through.
    void choose(int index);

    void setLabel(const QString& key, const QString& label);
    void setGlyph(const QString& key, Glyph glyph);
    // -1 removes the pill. 0 draws a zero, which is a different statement.
    void setCount(const QString& key, int count, CountTone tone = CountTone::Normal);

    // A word where a count would go: `Missing`, `Draft`, `Beta`. The approved
    // sidebars label a row by its condition as often as by a number, and a
    // caller with only setCount() has to fake it -- usually by putting the
    // word in the label, where it then elides with the name.
    void setBadge(const QString& key, const QString& text,
                  CountTone tone = CountTone::Normal);
    void setStatus(const QString& key, StatusDot::Tone tone);
    void clearStatus(const QString& key);
    void setRowEnabled(const QString& key, bool enabled);
    // Additional presentation-only inset for a concrete tree/block. This is
    // deliberately per row: direct rows in another section keep their base
    // axis instead of every sidebar item being shifted globally.
    void setIndentOffset(const QString& key, int pixels);
    void setTreeGuides(bool enabled);
    // A colour marker on a row: a 3px bar, 16px tall, before its glyph -
    // TreeView's marker. An invalid colour takes it away.
    void setMarker(const QString& key, const QColor& color);
    QColor marker(const QString& key) const;
    // The Novu sidebar, as TreeView::setMarkerColumn() draws it: a caption is
    // the root and each level hangs one treeStep under it; a row is chevron,
    // colour bar (its place kept, coloured or not), glyph, name; a workspace's
    // name is DemiBold; glyphs at the profile's treeIconOpacity; counts are
    // TreeView's chips and stand in one column. NovuAI, NovuSpace and
    // NovuExplorer draw this one sidebar. Off, the rail is what it was.
    void setMarkerColumn(bool reserved);
    bool hasMarkerColumn() const { return m_markerColumn; }
    // Captions that head what is under them rather than label it: larger,
    // semibold, in the primary ink, with more room and a hairline above. For a
    // rail long enough that a quiet caption gets lost among its rows. Off by
    // default: the other rails keep the design's quiet `.rail-sec`.
    void setProminentSections(bool prominent);

    // ── renaming a row in place ───────────────────────────────────────────
    //
    // Opens an editor over the row's label, focused and with its text
    // selected - "New collection" typed over in one gesture. Enter or leaving
    // the field submits; Escape abandons. The list decides nothing about the
    // text: inlineEditCommitted() hands it to the caller, which either lets the
    // editor close or calls rejectInlineEdit() from inside that handler to keep
    // it open with a reason.
    //
    // Survives clear() and a rebuild, because a rail is rebuilt from data on
    // every chrome change: the editor follows its key, and goes away only when
    // the key does. Returns false when no such row exists.
    bool beginInlineEdit(const QString& key, const QString& text = QString());
    // From inside an inlineEditCommitted() handler: the text was not accepted.
    // Enter keeps the editor open with `message` beside it; focus leaving
    // abandons instead, so a rejection never holds focus hostage.
    void rejectInlineEdit(const QString& message);
    void cancelInlineEdit();
    bool isInlineEditing() const;
    QString inlineEditKey() const { return m_editKey; }

    // ── dropping onto rows ────────────────────────────────────────────────
    //
    // Which rows accept a drop, and of what. Only a drag carrying `mimeType`
    // is accepted, and only over one of `keys`: every other row, a caption and
    // empty space refuse it. The hovered target is highlighted, and a drop
    // reports the payload with rowDropped(); the list moves nothing itself.
    // Kept apart from the rows, so a rebuild of the rail keeps its targets. An
    // empty `keys` turns drops off.
    void setDropTargets(const QString& mimeType, const QStringList& keys);

    // What a row says, for a caller filtering the list by it.
    QString label(const QString& key) const;

    // Hides a row without removing it.
    //
    // For a filter: a settings window with forty pages grows a search box, and
    // rebuilding the list on every keystroke moves the current page out from
    // under whoever is typing. A hidden row keeps its place, its key and its
    // selection, and takes no height - so it is skipped by the layout, by hit
    // testing and by the arrow keys, all three for free.
    bool isRowVisible(const QString& key) const;
    void setRowVisible(const QString& key, bool visible);
    bool isRowEnabled(const QString& key) const;

    // A row with a twisty. The rows added under it with addChild() belong to
    // it, and the list shows and hides them itself.
    //
    // It used to be the caller's job, on the argument that what a twisty
    // reveals is usually fetched on demand. In practice every application
    // reimplemented the same fold — rebuilding the whole list from a set of
    // open keys on each click — and each got it slightly different, which is
    // how one shared rail ended up behaving four ways. A caller that really
    // does fetch on demand still can: expandToggled() fires before anything
    // is drawn, and rows added while open appear open.
    void setExpandable(const QString& key, bool expandable);
    // A section collapses everything under it, up to the next section.
    void setSectionCollapsible(const QString& key, bool collapsible);
    // Opens or shuts a caption from code, as the twisty does by hand.
    void setSectionExpanded(const QString& key, bool expanded);

    // `.sb-head .n` -- how much the section holds, beside its name. A caption
    // that only names a group leaves the caller to fake the count with a
    // second label that then does not line up with the caption's baseline.
    void setSectionBadge(const QString& key, const QString& text);
    // An affordance at the trailing edge of a section heading - the `+`
    // the design puts on WORKSPACES. Reports intent, does nothing itself.
    void setSectionAction(const QString& key, Glyph glyph,
                          const QString& tooltip = QString());
    QRect sectionActionRect(int index) const;
    void setExpanded(const QString& key, bool expanded);
    bool isExpanded(const QString& key) const;

    // Hides labels, counts and twisties and centres the glyphs.
    StatusPlacement statusPlacement() const { return m_statusPlacement; }
    void setStatusPlacement(StatusPlacement placement);

    // Dense library/filter rails use bare trailing numbers and keep the
    // selected row's glyph in the product accent, as in NovuAI Documents.
    void setPlainCounts(bool plain) { m_plainCounts = plain; updateGeometry(); update(); }
    void setAccentCurrentGlyph(bool accent) { m_accentCurrentGlyph = accent; update(); }

    //! Draw row icons solid rather than as outlines, where the glyph has a
    //! solid form. Off by default: a rail of outlines is what the other
    //! modules draw, and this must not change under them.
    //!
    //! Only glyphs that say they fill are affected - glyphFillsCleanly() for
    //! a folder, glyphFillsAsSilhouette() for the workspace cube - so a glyph
    //! that would turn into a blob is left as an outline.
    void setFilledGlyphs(bool filled) { m_filledGlyphs = filled; update(); }
    bool hasPlainCounts() const { return m_plainCounts; }
    bool accentsCurrentGlyph() const { return m_accentCurrentGlyph; }

    bool isCollapsed() const { return m_collapsed; }
    void setCollapsed(bool collapsed);

    // Geometry, so a test can click a row or its twisty without guessing.
    QRect rowRect(int index) const;
    QRect twistyRect(int index) const;
    int indexAt(const QPoint& position) const;
    // The width this one row's own furniture needs - its indent, twisty,
    // glyph, status dot and badge, plus enough for a label to still read as
    // one. What minimumSizeHint() takes the maximum of, exposed by index so
    // a resize test can ask "does every row still fit" at any width without
    // reaching into the class to find out.
    int rowMinimumWidth(int index) const;

    // The contract for one row, and for the list's own surface.
    StyleSpec styleSpec() const;
    StyleSpec rowStyleSpec(int index) const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    // A row was chosen. Sections and disabled rows never emit.
    void activated(const QString& key);
    void currentChanged(const QString& key);
    // The twisty was clicked. `expanded` is the state it has been set to.
    void expandToggled(const QString& key, bool expanded);
    void sectionActionTriggered(const QString& key);
    // A row was asked for its own menu - right-clicked, or the context-menu
    // key pressed while it is current. The list opens nothing itself: what a
    // row can do is the caller's, and a sidebar that grew its own menu would
    // be four different menus in four products.
    void contextMenuRequested(const QString& key, const QPoint& globalPosition);
    // The inline editor was submitted (Enter, or focus leaving it) or
    // abandoned (Escape, or the row disappearing).
    void inlineEditCommitted(const QString& key, const QString& text);
    void inlineEditCancelled(const QString& key);
    // Something carrying the drop-target MIME type was dropped on `key`.
    void rowDropped(const QString& key, const QByteArray& payload);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    bool event(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    struct Row
    {
        QString key;
        QString label;
        Glyph glyph = Glyph::None;
        bool section = false;
        // Whether this row folds under the one above it and draws a leaf
        // marker. Kept separate from `depth`: this one is about the
        // disclosure, `depth` is about where the row sits.
        bool child = false;
        // How many levels in the row is drawn. Captions are level 0 and every
        // row under one starts at 1, because a caption is the parent of the
        // rows beneath it. Every level is worth the same number of pixels,
        // which is what makes the rail read as one tree rather than as a list
        // with some rows nudged.
        int depth = 0;
        int indentOffset = 0;
        bool enabled = true;
        // What the caller asked for (setRowVisible, a filter) and what is
        // actually drawn. They are separate because a row can be hidden by
        // three independent things at once — a filter, a shut caption above
        // it, a shut parent above that — and folding a section used to
        // overwrite the other two on its way past.
        bool hiddenByFilter = false;
        bool visible = true;
        bool expandable = false;
        bool expanded = false;
        int count = -1;
        CountTone countTone = CountTone::Normal;
        QString badge;
        QString sectionBadge;
        // Invalid tone plus this flag, rather than an optional, because a row
        // with no dot and a row with a Neutral dot are different drawings.
        bool hasStatus = false;
        StatusDot::Tone status = StatusDot::Tone::Neutral;
        // Sections only.
        Glyph action = Glyph::None;
        QString actionTip;
        QColor marker;
    };

    // Where the parts of a row's lead sit with a marker column - one answer
    // for paint, hit tests, size hints and the inline editor.
    struct Lead
    {
        int twisty = 0;
        int bar = 0;
        int glyph = 0;
        int status = -1;   // a leading status dot, when the list reserves one
        int name = 0;
    };
    Lead leadOf(const Row& row) const;
    QFont fontFor(const Row& row) const;

    // Inline editing. One editor for the whole list - only one row is ever
    // being renamed - positioned over the row's label band.
    void scheduleInlineEditorPlacement();
    void placeInlineEditor();
    void finishInlineEdit(bool submit, bool fromFocusOut);
    int dropTargetAt(const QPoint& position, const QMimeData* mime) const;
    QString m_dropMime;
    QSet<QString> m_dropKeys;
    int m_dropHover = -1;
    QPointer<QLineEdit> m_editor;
    QString m_editKey;
    bool m_editRejected = false;
    bool m_editFinishing = false;
    bool m_editPlacementQueued = false;

    int rowHeight(const Row& row) const;
    int sectionHeight(int index) const;
    // The width a row's own furniture reserves before the label sees any
    // space: indentation, twisty, glyph and a leading status dot on the left;
    // a trailing status dot and the badge or count pill, sized to its own
    // text, on the right. paintEvent() and minimumSizeHint() both read these
    // rather than each carrying its own copy of "which elements does this row
    // have" - the two disagreeing is how a badge ended up drawn where the
    // label's own box said it still had room.
    // Where a row's content starts: the rail padding plus one step per level.
    // One answer, because three places used to compute it and the tree only
    // lines up if they agree.
    int rowIndent(const Row& row) const;
    // The furniture every row in this list reserves room for, whether or not
    // it carries any.
    //
    // A twisty and a folder icon are wider than a nesting step, so a parent
    // that has both used to push its own label past its children's - the rail
    // stepped left as it went deeper. Reserving the slots list-wide instead
    // means a level is the only thing that moves a label, and a list with no
    // glyphs anywhere reserves nothing and is unchanged.
    struct LeadReserve { bool twisty = false; bool glyph = false; bool status = false; };
    LeadReserve leadReserve() const;

    int leadingZoneWidth(const Row& row) const;
    int trailingZoneWidth(const Row& row) const;
    // The row's own floor: leading + trailing + just enough for a label to
    // read as a label rather than a bare ellipsis. A container that honours
    // this can never hand the row less room than its badge and its glyph
    // both need at once.
    int rowMinimumWidth(const Row& row) const;
    // One pass that decides what is drawn: filter, then caption, then parent.
    // Every call that can change any of the three ends here.
    void applyDisclosure();
    void rememberFold(const Row& row);
    int sectionTop(int index) const;
    int sectionBottom() const;
    // The next selectable row in `step` direction, skipping captions and
    // disabled rows. -1 when there is none.
    int neighbour(int from, int step) const;
    Row* row(const QString& key);
    int indexOfRow(const Row& row) const;
    Row* sectionRow(const QString& key);
    const Row* sectionRow(const QString& key) const;
    const Row* row(const QString& key) const;

    QVector<Row> m_rows;
    // Which captions are shut, by key. Survives clear(), because an
    // application refreshes its rail by rebuilding it and a fold that does
    // not survive that is a fold nobody can use. See addSection().
    QSet<QString> m_shutSections;
    // 1 once a caption has been added, because everything after one hangs
    // under it. Reset by clear().
    int m_sectionDepth = 0;
    int m_current = -1;
    int m_hovered = -1;
    bool m_plainCounts = false;
    bool m_accentCurrentGlyph = false;
    bool m_filledGlyphs = false;
    bool m_hoveredTwisty = false;
    // Where the press landed, and in which zone. A click is press and release
    // in the same place; a release elsewhere is a cancelled gesture.
    int m_pressed = -1;
    bool m_pressedTwisty = false;
    bool m_collapsed = false;
    // `:focus-visible`, not `:focus`. The ring is for someone driving the
    // list from the keyboard; after a click the row is already marked by
    // the selection itself, and the design draws no outline round it.
    // `:focus-visible` - the same shared helper ModuleBar and TabBar carry,
    // so the rule for which QFocusEvent reason counts as the keyboard lives
    // once rather than as three copies free to disagree.
    FocusRing m_ring;
    bool m_pressedAction = false;
    bool m_hoveredAction = false;
    bool m_treeGuides = false;
    bool m_markerColumn = false;
    bool m_prominentSections = false;
    StatusPlacement m_statusPlacement = StatusPlacement::Leading;
};

} // namespace novu::ui
