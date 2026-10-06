#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/StyleSpec.h"

#include <QSplitter>
#include <QSplitterHandle>
#include <QHash>
#include <QWidget>

namespace novu::ui {

// The handle between two panels, and the arrows that put one away.
//
// Ported from NovuExplorer's `NovuSplitterHandle`, which is itself the shape
// NovuVoice settled on and NovuAI copied — three implementations of one idea,
// which is the definition of something that belongs here.
//
// ── what Qt gives you, and why it is not enough ──────────────────────────
//
// QSplitter's default handle is a raised bevel with dots in the middle. It is
// the one piece of platform chrome that survived every previous attempt at a
// Novu look, because no application styled it — so every splitter in every
// window announced itself as a Qt splitter. A stylesheet fixes the *colour* and
// nothing else: it cannot put a control on the handle, and the collapse arrows
// are the reason this component exists.
//
// ── the arrows ───────────────────────────────────────────────────────────
//
// A thirty-pixel zone at each end of the handle is a button rather than a drag
// target. The one nearer a panel collapses that panel, and points the other way
// once it is collapsed, so the same triangle says both "put this away" and
// "bring it back". Between the zones the handle drags as a splitter handle
// should.
//
// That split is why press and release both have to land in the same zone: a
// press that begins in the arrow and ends in the middle is a drag the user
// thought better of, not a collapse.
class NOVUUI_API SplitterHandle : public QSplitterHandle
{
    Q_OBJECT

public:
    SplitterHandle(Qt::Orientation orientation, QSplitter* parent);

    // Which panels are currently on screen. The arrows point according to this,
    // so a caller that collapses a panel some other way must say so or the
    // triangle lies.
    void setPanelsVisible(bool leading, bool trailing);

    // Turns the arrows off entirely: a plain drag handle. For a splitter whose
    // panels are not optional.
    bool hasCollapseArrows() const { return m_arrows; }
    void setCollapseArrows(bool arrows);

    // The two hit zones, for a test that wants to press one without guessing.
    QRect leadingArrowRect() const;
    QRect trailingArrowRect() const;

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;

signals:
    // The arrow was pressed. `leading` says which end. The handle does not
    // collapse anything itself — Splitter below does, and an application that
    // wants different behaviour connects to this instead.
    void collapseRequested(bool leading);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    bool event(QEvent* event) override;

private:
    // Along the handle, in its own orientation: y for a vertical handle between
    // side-by-side panels, x for a horizontal one.
    int positionAlong(const QPoint& point) const;
    int lengthAlong() const;
    bool inLeadingZone(const QPoint& point) const;
    bool inTrailingZone(const QPoint& point) const;
    void updateCursor(const QPoint& point);

    bool m_arrows = true;
    bool m_leadingVisible = true;
    bool m_trailingVisible = true;
    bool m_hovered = false;
    // Which zone the pointer and the press are in. -1 none, 0 leading,
    // 1 trailing.
    int m_hoveredZone = -1;
    int m_pressedZone = -1;
};

// A QSplitter that builds the handle above and knows how to collapse.
//
// Collapsing is done by setting a size to zero rather than by hiding the
// widget, because a hidden widget loses its place in the splitter's size list
// and comes back at whatever the layout decides. Restoring puts back the size
// the panel had when it was collapsed, which is the behaviour a user expects
// and the one a naive implementation gets wrong.
class NOVUUI_API Splitter : public QSplitter
{
    Q_OBJECT

public:
    explicit Splitter(Qt::Orientation orientation, QWidget* parent = nullptr);
    explicit Splitter(QWidget* parent = nullptr);

    bool hasCollapseArrows() const { return m_arrows; }
    void setCollapseArrows(bool arrows);

    // By index, so a splitter of three panels can collapse the middle one.
    bool isPanelCollapsed(int index) const;
    void setPanelCollapsed(int index, bool collapsed);
    void togglePanel(int index);

signals:
    void panelCollapsedChanged(int index, bool collapsed);

protected:
    QSplitterHandle* createHandle() override;

private:
    void syncHandles();

    bool m_arrows = true;
    // The size each panel had when it was collapsed, by index. Zero means it
    // was never collapsed.
    QHash<int, int> m_restoreSizes;
};

// A thin bar that folds a fixed-height strip away.
//
// NovuExplorer's `NovuCollapseDivider`, and its note is the reason it is not a
// splitter: *resizing a fixed-height toolbar strip by dragging makes no sense —
// this is purely a collapse toggle styled to match.* A workspace tab row above
// a category tab row wants to be put away, not resized, and a QSplitter that
// offers a drag the layout will immediately undo is a control that lies.
class NOVUUI_API CollapseDivider : public QWidget
{
    Q_OBJECT

public:
    // `target` is what folds away. The divider does not own it.
    explicit CollapseDivider(QWidget* target, QWidget* parent = nullptr);

    bool isCollapsed() const { return m_collapsed; }
    void setCollapsed(bool collapsed);
    void toggle();

    QSize sizeHint() const override;

signals:
    void collapsedChanged(bool collapsed);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool event(QEvent* event) override;

private:
    QWidget* m_target = nullptr;
    bool m_collapsed = false;
    bool m_hovered = false;
};

} // namespace novu::ui
