#pragma once

#include "novu/ui/Icon.h"
#include "novu/ui/StatusDot.h"
#include "novu/ui/StyleSpec.h"

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
    int addChild(const QString& key, const QString& label);
    // `.rail-sec` — a caption, not a row. Not selectable and not counted.
    // A section may carry a key, which is what makes it addressable: a
    // collapsible section, or one with an action on its heading, needs to
    // be named the way a row is.
    int addSection(const QString& title, const QString& key = QString());
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
    void setStatus(const QString& key, StatusDot::Tone tone);
    void clearStatus(const QString& key);
    void setRowEnabled(const QString& key, bool enabled);

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

    // A row with a twisty. Expanding is the caller's job — the list says what
    // was asked for and does not hide anything itself, because what a twisty
    // reveals is usually rows fetched on demand.
    void setExpandable(const QString& key, bool expandable);
    // A section collapses everything under it, up to the next section.
    void setSectionCollapsible(const QString& key, bool collapsible);
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

    bool isCollapsed() const { return m_collapsed; }
    void setCollapsed(bool collapsed);

    // Geometry, so a test can click a row or its twisty without guessing.
    QRect rowRect(int index) const;
    QRect twistyRect(int index) const;
    int indexAt(const QPoint& position) const;

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

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool event(QEvent* event) override;

private:
    struct Row
    {
        QString key;
        QString label;
        Glyph glyph = Glyph::None;
        bool section = false;
        bool child = false;
        bool enabled = true;
        bool visible = true;
        bool expandable = false;
        bool expanded = false;
        int count = -1;
        CountTone countTone = CountTone::Normal;
        // Invalid tone plus this flag, rather than an optional, because a row
        // with no dot and a row with a Neutral dot are different drawings.
        bool hasStatus = false;
        StatusDot::Tone status = StatusDot::Tone::Neutral;
        // Sections only.
        Glyph action = Glyph::None;
        QString actionTip;
    };

    int rowHeight(const Row& row) const;
    int sectionHeight() const;
    void applySectionVisibility();
    int sectionTop() const;
    int sectionBottom() const;
    // The next selectable row in `step` direction, skipping captions and
    // disabled rows. -1 when there is none.
    int neighbour(int from, int step) const;
    Row* row(const QString& key);
    const Row* row(const QString& key) const;

    QVector<Row> m_rows;
    int m_current = -1;
    int m_hovered = -1;
    bool m_hoveredTwisty = false;
    // Where the press landed, and in which zone. A click is press and release
    // in the same place; a release elsewhere is a cancelled gesture.
    int m_pressed = -1;
    bool m_pressedTwisty = false;
    bool m_collapsed = false;
    bool m_pressedAction = false;
    bool m_hoveredAction = false;
    StatusPlacement m_statusPlacement = StatusPlacement::Leading;
};

} // namespace novu::ui
