#pragma once

#include "novu/ui/Icon.h"
#include "novu/ui/StatusDot.h"
#include "novu/ui/StyleSpec.h"
#include "novu/ui/Theme.h"

#include <QSet>
#include <QString>
#include <QVector>
#include <QWidget>

namespace novu::ui {

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
class NavList : public QWidget
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
    // One indent below the preceding item. Depth stops at one on purpose — the
    // design's note is that a sidebar must show two levels *without turning
    // into a file browser*.
    int addChild(const QString& key, const QString& label,
                 Glyph glyph = Glyph::None);
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

private:
    struct Row
    {
        QString key;
        QString label;
        Glyph glyph = Glyph::None;
        bool section = false;
        bool child = false;
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
    };

    int rowHeight(const Row& row) const;
    int sectionHeight(int index) const;
    // The width a row's own furniture reserves before the label sees any
    // space: indentation, twisty, glyph and a leading status dot on the left;
    // a trailing status dot and the badge or count pill, sized to its own
    // text, on the right. paintEvent() and minimumSizeHint() both read these
    // rather than each carrying its own copy of "which elements does this row
    // have" - the two disagreeing is how a badge ended up drawn where the
    // label's own box said it still had room.
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
    int m_current = -1;
    int m_hovered = -1;
    bool m_plainCounts = false;
    bool m_accentCurrentGlyph = false;
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
    StatusPlacement m_statusPlacement = StatusPlacement::Leading;
};

} // namespace novu::ui
