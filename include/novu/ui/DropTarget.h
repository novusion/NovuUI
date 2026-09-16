#pragma once

#include <QRect>

#include "novu/ui/StyleSpec.h"

class QPainter;
class QWidget;

namespace novu::ui {

// Where a drag would land if it were released now.
//
// Two outcomes, and never both at once. The approved design states both the
// rule and the reason:
//
//     /* Two outcomes, so two different highlights, and never both at once:
//        a row lights when the folder will be bound to that workspace, the
//        region lights when it will make a new one. */
//     .trow.drop-on { background:rgba(var(--acc-rgb),.20);
//                     box-shadow:inset 0 0 0 1px var(--acc); }
//     .trow.drop-on .tname { color:var(--acc); font-weight:600; }
//     .tree.drop-on { background:rgba(var(--acc-rgb),.07);
//                     box-shadow:inset 0 0 0 1.5px rgba(var(--acc-rgb),.55);
//                     border-radius:var(--r-md); }
//
// The two weights are the point: the row is emphatic because it names one
// exact destination, the region is faint because it means "anywhere in here".
// A person mid-drag reads which of the two is lit and knows what will happen.
// Lighting both says nothing at all, which is why DropHighlight makes it
// unrepresentable rather than merely discouraged.
enum class DropZone
{
    None,
    Row,      // onto one item - it joins that thing
    Region,   // onto the surface itself - it becomes a new thing
};

// The drag state of one widget.
//
// A holder rather than a base class: the widgets that accept drops already
// derive from something - ItemStrip, QTreeView, a plain QWidget - and Qt hands
// all of them the same four events. So this owns the state and the invariant,
// and the widget keeps its own inheritance.
//
// Every mutator answers "did the picture change", so a dragMoveEvent that
// crosses thirty pixels inside one row repaints once rather than thirty times:
//
//     void Tree::dragMoveEvent(QDragMoveEvent* e) {
//         if (m_drop.setRow(indexAt(e->position().toPoint()))) update();
//         e->acceptProposedAction();
//     }
//     void Tree::dragLeaveEvent(QDragLeaveEvent*) { if (m_drop.clear()) update(); }
//     void Tree::dropEvent(QDropEvent* e) { const int row = m_drop.row();
//                                           m_drop.clear(); update(); ... }
class DropHighlight
{
public:
    DropZone zone() const { return m_zone; }

    // The lit row, or -1 when the zone is not Row. Never a stale index: moving
    // to the region or clearing resets it, so a caller cannot read a row out of
    // a state that has none.
    int row() const { return m_zone == DropZone::Row ? m_row : -1; }

    // A negative index is "not over a row", which is what a hit test returns at
    // the bottom of a list - so the common call is setRow(indexAt(pos)) with no
    // branch at the call site. It clears rather than lighting row -1.
    bool setRow(int index);
    bool setRegion();
    bool clear();

    bool isRowLit(int index) const { return m_zone == DropZone::Row && m_row == index; }
    bool isRegionLit() const { return m_zone == DropZone::Region; }
    bool isActive() const { return m_zone != DropZone::None; }

private:
    DropZone m_zone = DropZone::None;
    int m_row = -1;
};

// What each highlight is made of, as tokens. Public so the contract is
// testable without comparing pixels: a test asks for the spec and reads
// "accent@20" back, and the same spec is what the painter uses.
StyleSpec dropRowSpec();
StyleSpec dropRegionSpec();

// The ink a row's own label switches to while it is the target. The design
// bolds it as well - `stripItemChosen`-style painters should ask for
// dropLabelWeight() rather than hard-coding 600.
ColorToken dropLabelInk();
int dropLabelWeight();

// Drawn inset, the way `box-shadow: inset` is: the outline sits inside `rect`
// so a lit row does not grow by two pixels and shift the one below it.
void paintDropRow(QPainter* painter, const QRect& rect, const QWidget* context);
void paintDropRegion(QPainter* painter, const QRect& rect, const QWidget* context);

} // namespace novu::ui
