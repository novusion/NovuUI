#pragma once

#include "novu/ui/StyleSpec.h"

#include <QString>
#include <QWidget>

namespace novu::ui {

// A horizontal run of painted, mutually exclusive items, and the interaction
// that goes with it.
//
// Segmented said this would happen: *That makes this the library's first
// painted control with per-item hit testing, and the hit testing lives here for
// exactly as long as it is the only one. The moment a second component needs
// it — the module bar in the title bar is the one coming — it moves out into a
// shared primitive rather than being written a second time.*
//
// ModuleBar is that second component, so here it is.
//
// ── what this owns, and what it does not ──────────────────────────────────
//
// It owns the *behaviour*: where an item is, which one the pointer is over,
// which one the press landed in, that a release somewhere else is a cancelled
// gesture rather than a click, that arrow keys move within one tab stop, and
// that a glyph-only item still gets a tooltip even though there is no child
// widget to carry one.
//
// It owns no content and no colour. There is no item struct here, because a
// base class that held one would have to be kept in step with whatever the
// subclass holds — a picker's segments carry an attention flag, a module bar's
// carry a count, and merging those into one struct is how a shared base grows
// every field either subclass ever needed. Instead the subclass answers four
// questions about its own items, and keeps them in whatever shape suits it.
//
// A subclass therefore implements the pure virtuals below and paints. It gets
// hit testing, press ownership, hover, keyboard and tooltips for nothing, and
// cannot disagree with the other strips about what a click is.
class ItemStrip : public QWidget
{
    Q_OBJECT

public:
    int currentIndex() const { return m_current; }
    // Sets the state. Silent, because a setter sets.
    void setCurrentIndex(int index);
    // Asks for the state to change, and says so. What a click goes through.
    void choose(int index);

    // Where an item is, for a test that wants to click one without guessing at
    // the padding.
    QRect itemRect(int index) const;
    int indexAt(const QPoint& position) const;

protected:
    explicit ItemStrip(QWidget* parent = nullptr);

    // ── what a subclass must answer ──────────────────────────────────────
    virtual int stripCount() const = 0;
    virtual int stripItemWidth(int index) const = 0;
    // Called when a click or a key has chosen an item. The subclass updates
    // whatever it keeps and emits its own signal - this class has none, because
    // a base class signal would arrive alongside the subclass's and every
    // caller would have to know which to connect to.
    virtual void stripItemChosen(int index) = 0;

    // ── what it may override ─────────────────────────────────────────────
    virtual bool stripItemEnabled(int) const { return true; }
    virtual QString stripItemTooltip(int) const { return QString(); }
    // The inset before the first item, the gap between items, and the band the
    // items occupy. Defaults are a flush strip filling the widget's height.
    virtual int stripLead() const { return 0; }
    virtual int stripGap() const { return 0; }
    virtual int stripTop() const { return 0; }
    virtual int stripItemHeight() const { return height(); }

    // ── what it may read while painting ──────────────────────────────────
    int hoveredIndex() const { return m_hovered; }
    int pressedIndex() const { return m_pressed; }
    // Total width of every item plus the gaps and the lead on both sides.
    int stripWidth() const;

    // Clears hover and press state. A subclass calls this when it rebuilds its
    // items, or the indices are stale by exactly one removal.
    void resetStripState();

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    // The next enabled item in `step` direction. -1 when there is none, so
    // arrowing past the end stays where it is rather than landing on a
    // disabled item.
    int neighbour(int from, int step) const;

    int m_current = -1;
    int m_hovered = -1;
    int m_pressed = -1;
    // Which item's tooltip is currently installed, so the swap on the way out
    // is one remove rather than a remove on every mouse move.
    int m_tipIndex = -1;
};

} // namespace novu::ui
