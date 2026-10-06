#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/Icon.h"
#include "novu/ui/StyleSpec.h"

#include <QPointer>
#include <QString>
#include <QVector>
#include <QWidget>

class QHBoxLayout;
class QScrollArea;
class QVBoxLayout;

namespace novu::ui {

class FloatingPanel;

// The surface floating panels live on: the design's `.floatlayer`.
//
// It covers its host entirely, follows its size, and is transparent to the
// mouse everywhere a panel is not — so the application underneath keeps working
// while a panel is open, which is the whole difference between this and a
// modal.
//
// It also owns stacking. A panel that is clicked comes to the front, and the
// front panel is the focused one. Left to each panel that logic is written
// three times and disagrees about what "front" means.
class NOVUUI_API FloatingLayer : public QWidget
{
    Q_OBJECT

public:
    // Covers `host` and follows it. The host keeps ownership.
    explicit FloatingLayer(QWidget* host);

    // The usual case: one layer per host, created on demand. Returns the
    // existing one if there is already one on this host, because two layers
    // over one window is two stacking orders.
    static FloatingLayer* forHost(QWidget* host);

    // Adds a panel and brings it to the front. The layer takes ownership.
    void addPanel(FloatingPanel* panel);
    void raisePanel(FloatingPanel* panel);
    QVector<FloatingPanel*> panels() const;
    FloatingPanel* frontPanel() const;
    int panelCount() const;

    // Lays the panels out so none is entirely behind another. For the "open
    // four of these at once" case, which is otherwise four panels at the same
    // default position.
    void cascade();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void updateInputRegion();

    QPointer<QWidget> m_host;
};

// A panel that floats inside the application window.
//
// The approved NovuAI design's `.floatwin`: a 40px grab bar carrying a mark, a
// title and a subtitle, a body, an optional task strip along the bottom, a
// resize grip, and a roll-up that leaves only the bar.
//
// ── not a window ─────────────────────────────────────────────────────────
//
// This is deliberately *not* WindowShell's ToolWindow, and the two are not
// alternatives. A ToolWindow is a window of the desktop: it can be dragged to
// another screen, it appears in the task bar, and the compositor draws its
// shadow. A FloatingPanel cannot leave the application window — which is the
// point. It is a detached view of something inside that window, it is clipped
// to it, it scrolls with nothing, and closing the application takes it with no
// ceremony.
//
// NovusionUI's `FloatingWindow` is the first kind; this is the second. Naming
// them apart is most of what stops an application reaching for the wrong one.
//
// ── what it owns ─────────────────────────────────────────────────────────
//
// Dragging by the bar, resizing from the grip, clamping to the layer so a panel
// cannot be dragged somewhere it cannot be dragged back from, rolling up, and
// coming to the front when touched. A caller fills `body()` and says what the
// bar reads.
class NOVUUI_API FloatingPanel : public QWidget
{
    Q_OBJECT

public:
    FloatingPanel(Glyph glyph, const QString& title, QWidget* parent = nullptr);

    QString title() const { return m_title; }
    void setTitle(const QString& title);
    // The second line in the bar — `.fw-s`. A path, a model name, a session id.
    QString subtitle() const { return m_subtitle; }
    void setSubtitle(const QString& subtitle);
    void setGlyph(Glyph glyph);

    QWidget* bodyWidget() const { return m_body; }

    // `<button data-fw-min>` and `<button data-fw-max>` - the other two
    // window controls the design puts on a floating window, in that order
    // before the close. A panel that carries only a close box is a popover
    // wearing a title bar: these are *windows*, and the two things one can do
    // to a window other than close it are get it out of the way and make it
    // as big as the space allows.
    QRect minimiseRect() const;
    QRect maximiseRect() const;
    // The third of the same set. Public for the same reason the other two are:
    // where a window control *is* belongs to the window, not to its painter.
    QRect closeRect() const;
    bool isMaximised() const { return !m_restore.isNull(); }
    // Fills the layer inset by 40, or puts the window back where it was.
    void toggleMaximised();


    // `.fw-foot{display:flex;align-items:center;gap:8px;padding:11px 14px;
    // border-top:1px solid var(--hair);flex-shrink:0}` — the row the window's
    // own buttons stand in.
    //
    // A row of its own because `.fw-body` scrolls and this must not: a Create
    // button that scrolls out of the window with the form above it is a window
    // whose action cannot be reached. It costs nothing until something is put
    // in it.
    QHBoxLayout* footer() const;
    QVBoxLayout* body() const;
    void setBodyWidget(QWidget* widget);

    // `.fw-task` — the strip along the bottom saying what this panel is doing.
    // Created on first call; a panel with nothing to report never builds it.
    QWidget* taskStrip();
    QHBoxLayout* task();
    void setTaskText(const QString& text, const QString& meta = {});

    // A button in the bar, left of the close. For what this panel can do with
    // itself — reattach, pop out, pin.
    void addBarAction(Glyph glyph, const QString& tooltip, const QString& key);

    // `.floatwin.rolled` — only the bar. Not the same as closing: a rolled
    // panel keeps its size and its contents and unrolls to exactly what it was.
    bool isRolled() const { return m_rolled; }
    void setRolled(bool rolled);

    // Ask to close, from anywhere, without answering inside the event that
    // asked. The bar's own close button and Escape both go through this; a
    // caller with its own Close button should too, rather than emitting the
    // signal by hand. See the implementation for why it matters.
    void requestClose();

    bool isResizable() const { return m_resizable; }
    void setResizable(bool resizable);

    // Whether this panel is the front one. Set by the layer, read for painting:
    // the design gives the focused panel an accent mark and a brighter caption.
    bool isFront() const { return m_front; }
    void setFront(bool front);

    // Where a caller wants it, in layer coordinates. Clamped on every move, so
    // a panel can never end up with its bar off the top of the host.
    void placeAt(const QPoint& position, const QSize& size = QSize());

    QRect barRect() const;
    QRect gripRect() const;

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void closeRequested();
    // The bar's minimise: the design hides the window outright.
    void minimiseRequested();
    void rolledChanged(bool rolled);
    void barActionTriggered(const QString& key);
    void movedOrResized();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    bool event(QEvent* event) override;

private:
    struct BarAction
    {
        Glyph glyph = Glyph::None;
        QString tooltip;
        QString key;
    };

    enum class Drag { None, Move, Resize };

    int barHeight() const;
    QRect actionRect(int index) const;
    void relayout();
    void clampIntoParent();

    QString m_title;
    QString m_subtitle;
    QString m_taskText;
    QString m_taskMeta;
    Glyph m_glyph = Glyph::None;
    QVector<BarAction> m_actions;

    QWidget* m_body = nullptr;
    // The body's own viewport. `.fw-body{flex:1;min-height:0;overflow:auto}`:
    // a form taller than the window scrolls rather than squashing its fields
    // into each other, which is what a three-row note field did to every
    // field above it.
    QScrollArea* m_scroll = nullptr;
    QWidget* m_foot = nullptr;
    QWidget* m_task = nullptr;

    bool m_rolled = false;
    bool m_hoveredMinimise = false;
    bool m_hoveredMaximise = false;
    // Where it was before it filled the layer. Null means it is not maximised,
    // which is also what tells the button which way it goes.
    QRect m_restore;
    bool m_resizable = true;
    bool m_front = false;

    Drag m_drag = Drag::None;
    QPoint m_dragOrigin;
    QRect m_dragGeometry;
    int m_hoveredAction = -1;
    bool m_hoveredClose = false;
    // The height the panel had before it was rolled, so unrolling restores it
    // rather than falling back to a size hint.
    int m_unrolledHeight = 0;
};

} // namespace novu::ui
