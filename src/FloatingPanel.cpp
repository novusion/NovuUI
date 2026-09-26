#include "novu/ui/FloatingPanel.h"

#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Tooltip.h"

#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPainterPath>
#include <QResizeEvent>
#include <QRegion>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

// `.fw-bar{gap:9px;padding:0 5px 0 12px;height:36px}`. The in-app window
// keeps the reference geometry while using an 18px identity glyph so it has
// the same visual weight as the title beside it.
// This was 40/11/6/15 - a title bar
// four pixels taller than the design draws it, its icon inset one pixel short
// on the left and one long on the right, and its own glyph a pixel under
// size: exactly a title bar that reads as "crammed against the window edge"
// even though every individual number looked plausible on its own.
constexpr int kBarHeight = 36;
constexpr int kBarPaddingLeft = 12;
constexpr int kBarPaddingRight = 5;
constexpr int kBarGap = 9;
constexpr int kBarGlyph = 18;

// `.fw-bar .ibtn { width:26px; height:22px }`; the 13px marks remain compact
// but no longer disappear against a high-density display.
constexpr int kActionWidth = 26;
constexpr int kActionHeight = 22;
constexpr int kActionGlyph = 13;
constexpr int kActionGap = 5;

// `.floatwin { min-width:380px; min-height:200px }`.
constexpr int kMinWidth = 380;
// `.floatwin .fw-body{padding:14px 16px;gap:13px}`.
constexpr int kBodyPaddingH = 16;
constexpr int kBodyPaddingV = 14;
constexpr int kBodyGap = 13;

// `.fw-foot{padding:11px 14px;gap:8px}`.
constexpr int kFootPaddingH = 14;
constexpr int kFootPaddingV = 11;
constexpr int kFootGap = 8;
constexpr int kMinHeight = 200;

// `.floatwin .fw-grip { width:14px; height:14px }`.
constexpr int kGrip = 14;

// `.fw-task { padding:7px 10px 7px 12px; gap:9px }`.
constexpr int kTaskPaddingV = 7;
constexpr int kTaskPaddingLeft = 12;
constexpr int kTaskPaddingRight = 10;

// How far a cascade offsets each new panel.
constexpr int kCascadeStep = 28;

QFont titleFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::CardTitle, true);
    font.setPixelSize(profile.type.md);
    return font;
}

QFont subtitleFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Mono);
    font.setPixelSize(qMax(8, profile.type.xxs));
    return font;
}

QFont taskFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Body);
    font.setPixelSize(profile.type.sm);
    return font;
}

} // namespace

// == FloatingLayer ==========================================================

FloatingLayer::FloatingLayer(QWidget* host)
    : QWidget(host)
    , m_host(host)
{
    setObjectName(QStringLiteral("novuFloatingLayer"));
    // A transparent-for-mouse parent also makes every child transparent in
    // Qt. That would draw the windows correctly while silently disabling the
    // title-bar drag and every control in their bodies. The layer therefore
    // uses an input/paint mask made from its panel rectangles instead: points
    // outside the mask fall through to the application, points inside are
    // delivered to the panel and its children.
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);

    if (host) {
        host->installEventFilter(this);
        setGeometry(host->rect());
        updateInputRegion();
        raise();
        show();
    }
}

FloatingLayer* FloatingLayer::forHost(QWidget* host)
{
    if (!host) {
        return nullptr;
    }
    // Two layers over one window is two stacking orders, and a panel added to
    // the second would sit behind every panel in the first for no visible
    // reason.
    if (auto* existing = host->findChild<FloatingLayer*>(
            QStringLiteral("novuFloatingLayer"), Qt::FindDirectChildrenOnly)) {
        return existing;
    }
    return new FloatingLayer(host);
}

bool FloatingLayer::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_host && event->type() == QEvent::Resize) {
        setGeometry(m_host->rect());
        for (FloatingPanel* panel : panels()) {
            // Keeps a panel reachable when the window shrinks under it.
            panel->placeAt(panel->pos());
        }
        updateInputRegion();
    }
    if (watched == m_host && event->type() == QEvent::ChildAdded) {
        // Anything added to the host after the layer would otherwise paint over
        // it, and the panels would disappear behind a page.
        raise();
    }
    if (qobject_cast<FloatingPanel*>(watched)
        && (event->type() == QEvent::Move || event->type() == QEvent::Resize
            || event->type() == QEvent::Show || event->type() == QEvent::Hide)) {
        updateInputRegion();
    }
    if (qobject_cast<FloatingPanel*>(watched) && event->type() == QEvent::Destroy) {
        // The object is still present in QObject::children() during Destroy;
        // rebuild after Qt has removed it so its old rectangle becomes
        // clickable application space again.
        QTimer::singleShot(0, this, [this] { updateInputRegion(); });
    }
    return QWidget::eventFilter(watched, event);
}

void FloatingLayer::updateInputRegion()
{
    QRegion input;
    for (FloatingPanel* panel : panels()) {
        if (!panel->isHidden())
            input += panel->geometry();
    }
    setMask(input);
}

void FloatingLayer::addPanel(FloatingPanel* panel)
{
    if (!panel) {
        return;
    }
    panel->setParent(this);
    panel->installEventFilter(this);
    panel->show();
    updateInputRegion();
    raisePanel(panel);
    // The panel is what the user is about to act on, not whatever had focus
    // in the window behind it. Without this, opening a panel over a selected
    // table row left the row's own selection and focus ring exactly where
    // they were - the new window looked focused (the accent wash says so) but
    // Escape, Tab and every other key still went to the row underneath it.
    panel->setFocus(Qt::PopupFocusReason);
}

QVector<FloatingPanel*> FloatingLayer::panels() const
{
    QVector<FloatingPanel*> result;
    for (QObject* child : children()) {
        if (auto* panel = qobject_cast<FloatingPanel*>(child)) {
            result.append(panel);
        }
    }
    return result;
}

int FloatingLayer::panelCount() const
{
    return panels().size();
}

FloatingPanel* FloatingLayer::frontPanel() const
{
    const QVector<FloatingPanel*> all = panels();
    return all.isEmpty() ? nullptr : all.last();
}

void FloatingLayer::raisePanel(FloatingPanel* panel)
{
    if (!panel || panel->parentWidget() != this) {
        return;
    }
    panel->raise();
    // Front is a property of exactly one panel, so it is set here rather than
    // by each panel deciding for itself - which is how two panels both end up
    // drawing the focused treatment.
    for (FloatingPanel* other : panels()) {
        other->setFront(other == panel);
    }
}

void FloatingLayer::cascade()
{
    int step = 0;
    for (FloatingPanel* panel : panels()) {
        panel->placeAt(QPoint(kCascadeStep + step * kCascadeStep,
                              kCascadeStep + step * kCascadeStep));
        ++step;
    }
}

// == FloatingPanel ==========================================================

FloatingPanel::FloatingPanel(Glyph glyph, const QString& title, QWidget* parent)
    : QWidget(parent)
    , m_title(title)
    , m_glyph(glyph)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_NoMousePropagation);
    // So Escape reaches keyPressEvent even before anything inside has focus.
    // A panel that cannot take focus is a panel the keyboard cannot close.
    setFocusPolicy(Qt::StrongFocus);

    m_body = new QWidget(this);
    auto* layout = new QVBoxLayout(m_body);
    // `.floatwin .fw-body{padding:14px 16px;gap:13px}`. It was flush and
    // gapless, so every field in a compose or an editor sat against the
    // panel's own edge and against the field above it.
    layout->setContentsMargins(kBodyPaddingH, kBodyPaddingV, kBodyPaddingH, kBodyPaddingV);
    layout->setSpacing(kBodyGap);

    // `overflow:auto` - the body is a viewport, not a box that gives way.
    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->viewport()->setAutoFillBackground(false);
    m_scroll->setWidget(m_body);

    // `.fw-foot` - empty, and costing nothing, until a caller puts a button
    // in it.
    m_foot = new QWidget(this);
    auto* footRow = new QHBoxLayout(m_foot);
    footRow->setContentsMargins(kFootPaddingH, kFootPaddingV, kFootPaddingH, kFootPaddingV);
    footRow->setSpacing(kFootGap);
    m_foot->setVisible(false);
    m_foot->installEventFilter(this);

    resize(kMinWidth + 60, kMinHeight + 80);
    m_unrolledHeight = height();
}

void FloatingPanel::setTitle(const QString& title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    update();
}

void FloatingPanel::setSubtitle(const QString& subtitle)
{
    if (m_subtitle == subtitle) {
        return;
    }
    m_subtitle = subtitle;
    update();
}

void FloatingPanel::setGlyph(Glyph glyph)
{
    if (m_glyph == glyph) {
        return;
    }
    m_glyph = glyph;
    update();
}

QVBoxLayout* FloatingPanel::body() const
{
    return qobject_cast<QVBoxLayout*>(m_body->layout());
}

QHBoxLayout* FloatingPanel::footer() const
{
    // Asking for it is what makes it exist: a panel with no buttons draws no
    // rule across its bottom edge. And the body above it gives up the height
    // straight away, rather than at whatever the next resize happens to be.
    const bool appearing = m_foot->isHidden();
    m_foot->setVisible(true);
    if (appearing)
        const_cast<FloatingPanel*>(this)->relayout();
    return qobject_cast<QHBoxLayout*>(m_foot->layout());
}

void FloatingPanel::setBodyWidget(QWidget* widget)
{
    if (!widget || widget == m_body) {
        return;
    }
    m_body->deleteLater();
    m_body = widget;
    m_scroll->setWidget(m_body);
    m_scroll->setVisible(!m_rolled);
    relayout();
}

QWidget* FloatingPanel::taskStrip()
{
    if (!m_task) {
        m_task = new QWidget(this);
        auto* layout = new QHBoxLayout(m_task);
        layout->setContentsMargins(kTaskPaddingLeft, kTaskPaddingV,
                                   kTaskPaddingRight, kTaskPaddingV);
        layout->setSpacing(kBarGap);
        m_task->setVisible(!m_rolled);
        relayout();
    }
    return m_task;
}

QHBoxLayout* FloatingPanel::task()
{
    return qobject_cast<QHBoxLayout*>(taskStrip()->layout());
}

void FloatingPanel::setTaskText(const QString& text, const QString& meta)
{
    m_taskText = text;
    m_taskMeta = meta;
    taskStrip();
    relayout();
    update();
}

void FloatingPanel::addBarAction(Glyph glyph, const QString& tooltip, const QString& key)
{
    BarAction action;
    action.glyph = glyph;
    action.tooltip = tooltip;
    action.key = key;
    m_actions.append(action);
    update();
}

void FloatingPanel::setRolled(bool rolled)
{
    if (m_rolled == rolled) {
        return;
    }
    if (rolled) {
        // Remembered rather than recomputed: a panel the user had made tall
        // must come back tall, not at whatever its contents now suggest.
        m_unrolledHeight = height();
        m_rolled = true;
        m_scroll->setVisible(false);
        if (m_task) {
            m_task->setVisible(false);
        }
        resize(width(), barHeight());
    } else {
        m_rolled = false;
        m_scroll->setVisible(true);
        if (m_task) {
            m_task->setVisible(true);
        }
        resize(width(), qMax(kMinHeight, m_unrolledHeight));
    }
    clampIntoParent();
    relayout();
    update();
    emit rolledChanged(m_rolled);
}

void FloatingPanel::setResizable(bool resizable)
{
    if (m_resizable == resizable) {
        return;
    }
    m_resizable = resizable;
    update();
}

void FloatingPanel::setFront(bool front)
{
    if (m_front == front) {
        return;
    }
    m_front = front;
    update();
}

int FloatingPanel::barHeight() const
{
    return kBarHeight;
}

QRect FloatingPanel::barRect() const
{
    return QRect(0, 0, width(), barHeight());
}

QRect FloatingPanel::closeRect() const
{
    return QRect(width() - kBarPaddingRight - kActionWidth,
                 (barHeight() - kActionHeight) / 2, kActionWidth, kActionHeight);
}

// Right to left from the close: close, maximise, minimise - so the close box
// stays where it always is and the other two grow inwards from it.
QRect FloatingPanel::maximiseRect() const
{
    const QRect close = closeRect();
    return QRect(close.left() - kActionWidth - kActionGap, close.top(),
                 kActionWidth, kActionHeight);
}

QRect FloatingPanel::minimiseRect() const
{
    const QRect max = maximiseRect();
    return QRect(max.left() - kActionWidth - kActionGap, max.top(),
                 kActionWidth, kActionHeight);
}

QRect FloatingPanel::actionRect(int index) const
{
    if (index < 0 || index >= m_actions.size()) {
        return QRect();
    }
    // Laid out right to left from the close button, so adding an action never
    // moves the close - the one control a user reaches for without looking.
    const int fromRight = m_actions.size() - index;
    const int x = minimiseRect().left() - fromRight * (kActionWidth + kActionGap);
    return QRect(x, (barHeight() - kActionHeight) / 2, kActionWidth, kActionHeight);
}

QRect FloatingPanel::gripRect() const
{
    if (!m_resizable || m_rolled) {
        return QRect();
    }
    return QRect(width() - kGrip - 2, height() - kGrip - 2, kGrip, kGrip);
}

void FloatingPanel::relayout()
{
    const int top = barHeight();
    if (m_rolled) {
        m_scroll->setGeometry(0, top, width(), 0);
        m_foot->setVisible(false);
        return;
    }
    int bottom = height();
    if (m_task && !m_taskText.isEmpty()) {
        const int taskHeight = QFontMetrics(taskFont(this)).height() + 2 * kTaskPaddingV;
        m_task->setGeometry(0, height() - taskHeight, width(), taskHeight);
        m_task->setVisible(true);
        bottom -= taskHeight;
    } else if (m_task) {
        m_task->setVisible(false);
    }
    // The footer keeps its height and the body takes the rest, which is what
    // `flex:1` on the body and `flex-shrink:0` on the foot say.
    // isHidden(), not !isVisible(): the panel is laid out before its window
    // is shown, and a footer that only counts once everything above it is on
    // screen is a footer the first layout puts nowhere.
    if (!m_foot->isHidden()) {
        const int footHeight = m_foot->sizeHint().height();
        m_foot->setGeometry(0, bottom - footHeight, width(), footHeight);
        bottom -= footHeight;
    }
    m_scroll->setGeometry(0, top, width(), qMax(0, bottom - top));
}

// `Object.assign(w.el.style, { left:"40px", top:"40px",
// width:(innerWidth - 80)+"px", height:(innerHeight - 80)+"px" })`, and the
// stored `dataset.restore` on the way back.
void FloatingPanel::toggleMaximised()
{
    QWidget* layer = parentWidget();
    if (!layer) {
        return;
    }
    if (isMaximised()) {
        setGeometry(m_restore);
        m_restore = QRect();
    } else {
        m_restore = geometry();
        constexpr int kMaximisedInset = 40;
        setGeometry(kMaximisedInset, kMaximisedInset,
                    qMax(kMinWidth, layer->width() - 2 * kMaximisedInset),
                    qMax(kMinHeight, layer->height() - 2 * kMaximisedInset));
    }
    m_unrolledHeight = height();
    update();
    emit movedOrResized();
}

void FloatingPanel::clampIntoParent()
{
    QWidget* layer = parentWidget();
    if (!layer) {
        return;
    }
    // The bar must stay reachable. Clamping the whole panel inside the host
    // would stop a user pushing a large panel partly off the edge, which is a
    // reasonable thing to want; clamping the *bar* keeps it draggable back.
    const int minX = -width() + kBarPaddingLeft + kBarGlyph + kBarGap + 40;
    const int maxX = layer->width() - kBarPaddingLeft - kBarGlyph;
    const int maxY = layer->height() - barHeight();
    move(qBound(minX, x(), qMax(minX, maxX)), qBound(0, y(), qMax(0, maxY)));
}

void FloatingPanel::placeAt(const QPoint& position, const QSize& size)
{
    if (size.isValid() && !size.isEmpty()) {
        resize(qMax(kMinWidth, size.width()),
               m_rolled ? barHeight() : qMax(kMinHeight, size.height()));
        if (!m_rolled) {
            m_unrolledHeight = height();
        }
    }
    move(position);
    clampIntoParent();
    relayout();
}

QSize FloatingPanel::sizeHint() const
{
    return QSize(kMinWidth + 60, kMinHeight + 80);
}

QSize FloatingPanel::minimumSizeHint() const
{
    return QSize(kMinWidth, m_rolled ? barHeight() : kMinHeight);
}

void FloatingPanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    relayout();
}

bool FloatingPanel::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        relayout();
        update();
    }
    return QWidget::event(event);
}

void FloatingPanel::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const QPoint position = event->position().toPoint();

    // Touching a panel anywhere brings it forward. Doing it on press rather
    // than on click means a drag starts on the panel the user grabbed.
    if (auto* layer = qobject_cast<FloatingLayer*>(parentWidget())) {
        layer->raisePanel(this);
    }

    if (closeRect().contains(position)) {
        m_drag = Drag::None;
        requestClose();
        return;
    }
    // `data-fw-min` - out of the way, and still reachable.
    //
    // The sheet's own handler sets `display:none`, which on a page is
    // recoverable because the window is a singleton its opener can ask for
    // again. Here it would be an orphan: nothing in the application knows the
    // panel exists any more, and there is no taskbar to find it in. So this
    // rolls the window up to its title bar - the same gesture, the same
    // result of getting it out of the way, and the bar stays on the layer to
    // bring it back with.
    if (minimiseRect().contains(position)) {
        m_drag = Drag::None;
        setRolled(!m_rolled);
        emit minimiseRequested();
        return;
    }
    // `data-fw-max` fills the layer inset by 40 and remembers where it was,
    // or puts it back if it is already filling it.
    if (maximiseRect().contains(position)) {
        m_drag = Drag::None;
        toggleMaximised();
        return;
    }
    for (int i = 0; i < m_actions.size(); ++i) {
        if (actionRect(i).contains(position)) {
            m_drag = Drag::None;
            // Queued for the same reason close is - see requestClose(). A bar
            // action is exactly the kind of control that asks a question.
            const QString key = m_actions.at(i).key;
            QMetaObject::invokeMethod(
                this, [this, key] { emit barActionTriggered(key); }, Qt::QueuedConnection);
            return;
        }
    }

    if (!gripRect().isNull() && gripRect().contains(position)) {
        m_drag = Drag::Resize;
    } else if (barRect().contains(position)) {
        m_drag = Drag::Move;
    } else {
        m_drag = Drag::None;
        QWidget::mousePressEvent(event);
        return;
    }
    m_dragOrigin = event->globalPosition().toPoint();
    m_dragGeometry = geometry();
    event->accept();
}

// Escape closes, which is what every window in the design does and what
// nothing in this one did: a panel with the keyboard focus and no Cancel
// button in reach was a window that could only be left with the mouse.
// The footer's height is whatever the caller put in it, and the caller puts it
// in *after* asking for the row - so the panel re-lays itself out when the row
// says its contents changed rather than trusting the height it had when it was
// empty.
bool FloatingPanel::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_foot && event->type() == QEvent::LayoutRequest) {
        relayout();
    }
    return QWidget::eventFilter(watched, event);
}

// Asking to close is never answered inside the event that asked.
//
// A panel is closed from a mouse press, and a caller is entitled to answer
// `closeRequested` with a question of its own - "this message has not been
// sent, keep it?". A modal dialog runs a nested event loop, and starting one
// from inside mousePressEvent leaves the panel holding the implicit mouse grab
// Qt gives a widget between press and release: the release goes to the dialog,
// the panel never gets it, and when the loop unwinds the grab is still out.
// From then on nothing in the window responds to the mouse - which is the
// whole application wedged by a window that would not close.
//
// So the signal is posted rather than emitted. The press finishes, the grab is
// released, and the slot runs on the next turn of the loop with the window in a
// state that can host a dialog.
void FloatingPanel::requestClose()
{
    QMetaObject::invokeMethod(
        this, [this] { emit closeRequested(); }, Qt::QueuedConnection);
}

void FloatingPanel::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        event->accept();
        requestClose();
        return;
    }
    QWidget::keyPressEvent(event);
}

void FloatingPanel::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint position = event->position().toPoint();

    if (m_drag == Drag::None) {
        int hovered = -1;
        for (int i = 0; i < m_actions.size(); ++i) {
            if (actionRect(i).contains(position)) {
                hovered = i;
                break;
            }
        }
        const bool onClose = closeRect().contains(position);
        const bool onMinimise = minimiseRect().contains(position);
        const bool onMaximise = maximiseRect().contains(position);
        if (onMinimise != m_hoveredMinimise || onMaximise != m_hoveredMaximise) {
            m_hoveredMinimise = onMinimise;
            m_hoveredMaximise = onMaximise;
            update();
        }
        if (hovered != m_hoveredAction || onClose != m_hoveredClose) {
            // The bar's buttons are painted, so their tips are swapped onto the
            // panel as the pointer crosses between them.
            if (hovered != m_hoveredAction) {
                if (hovered >= 0 && !m_actions.at(hovered).tooltip.isEmpty()) {
                    Tooltip::install(this, m_actions.at(hovered).tooltip);
                } else {
                    Tooltip::remove(this);
                }
            }
            m_hoveredAction = hovered;
            m_hoveredClose = onClose;
            update();
        }

        if (!gripRect().isNull() && gripRect().contains(position)) {
            setCursor(Qt::SizeFDiagCursor);
        } else if (barRect().contains(position) && hovered < 0 && !onClose
                   && !onMinimise && !onMaximise) {
            setCursor(Qt::OpenHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPoint delta = event->globalPosition().toPoint() - m_dragOrigin;
    if (m_drag == Drag::Move) {
        move(m_dragGeometry.topLeft() + delta);
        clampIntoParent();
    } else {
        resize(qMax(kMinWidth, m_dragGeometry.width() + delta.x()),
               qMax(kMinHeight, m_dragGeometry.height() + delta.y()));
        m_unrolledHeight = height();
    }
    emit movedOrResized();
    event->accept();
}

void FloatingPanel::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_drag != Drag::None) {
        m_drag = Drag::None;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void FloatingPanel::mouseDoubleClickEvent(QMouseEvent* event)
{
    // Double-clicking the bar rolls up, the way it has in every window manager
    // that offers it. Nowhere else does, because a double-click in a body is
    // the body's.
    if (event->button() == Qt::LeftButton && barRect().contains(event->position().toPoint())
        && !closeRect().contains(event->position().toPoint())) {
        setRolled(!m_rolled);
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void FloatingPanel::leaveEvent(QEvent* event)
{
    if (m_hoveredAction != -1 || m_hoveredClose) {
        m_hoveredAction = -1;
        m_hoveredClose = false;
        Tooltip::remove(this);
        update();
    }
    setCursor(Qt::ArrowCursor);
    QWidget::leaveEvent(event);
}

StyleSpec FloatingPanel::styleSpec() const
{
    StyleSpec spec;
    spec.background = token(ColorRole::WindowBg);
    spec.border = m_front ? token(ColorRole::Accent, 0.5) : token(ColorRole::Border);
    spec.foreground = m_front ? token(ColorRole::TextPrimary)
                              : token(ColorRole::TextSecondary);
    // `.floatwin{border-radius:13px}` - its own role, because a window drawn
    // inside a window is neither the frame nor a modal.
    spec.radius = SurfaceRole::FloatingWindow;
    spec.borderWidth = BorderRole::Card;
    spec.type = TypeRole::CardTitle;
    return spec;
}

ResolvedStyle FloatingPanel::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

void FloatingPanel::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const ResolvedStyle style = resolvedStyle();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF frame = QRectF(rect()).adjusted(style.borderWidth / 2.0,
                                                 style.borderWidth / 2.0,
                                                 -style.borderWidth / 2.0,
                                                 -style.borderWidth / 2.0);
    painter.setPen(QPen(style.border, style.borderWidth));
    painter.setBrush(style.background);
    painter.drawRoundedRect(frame, style.radius, style.radius);

    // Clip the chrome to the rounded frame, or the bar's square bottom edge
    // and the wash both bleed past the corners.
    QPainterPath clip;
    clip.addRoundedRect(frame, style.radius, style.radius);
    painter.save();
    painter.setClipPath(clip);

    // The bar carries no fill of its own - the frame's single flat
    // background already covers it, so the panel reads as one surface with
    // the bar, the body and the footer separated only by the hairlines
    // below, not by a shade change between them.
    const QRect bar = barRect();

    // Front vs. behind is the border's own accent tint (see styleSpec()) -
    // a flat colour, not a second wash painted over the one flat surface
    // the whole panel is supposed to read as.

    // `.fw-bar{border-bottom:1px solid var(--hair2)}`. The stronger hairline
    // keeps the draggable title bar legible without turning it into another
    // filled surface.
    painter.setPen(QPen(profile.colors.borderHover, 1));
    paintHairline(&painter, QPoint(0, bar.bottom()), QPoint(width() - 1, bar.bottom()),
                  painter.pen().color(), qMax(1, painter.pen().width()));

    // `.fw-foot{border-top:1px solid var(--hair)}` - the rule belongs to the
    // footer, so it is drawn wherever the footer ended up rather than at a
    // height computed twice.
    if (m_foot && !m_foot->isHidden()) {
        paintHairline(&painter, QPoint(0, m_foot->geometry().top()),
                      QPoint(width() - 1, m_foot->geometry().top()),
                      profile.colors.border, 1);
    }
    painter.restore();

    // ── the bar's contents ───────────────────────────────────────────────
    int x = kBarPaddingLeft;
    if (m_glyph != Glyph::None) {
        paintGlyph(&painter, m_glyph,
                   QRectF(x, (bar.height() - kBarGlyph) / 2.0, kBarGlyph, kBarGlyph),
                   m_front ? profile.colors.accentLight : profile.colors.textDisabled);
        x += kBarGlyph + kBarGap;
    }

    int rightEdge = closeRect().left() - kActionGap;
    for (int i = m_actions.size() - 1; i >= 0; --i) {
        const QRect box = actionRect(i);
        if (i == m_hoveredAction) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(profile.colors.surfaceHover);
            painter.drawRoundedRect(QRectF(box), profile.radii.small, profile.radii.small);
        }
        const double inset = (kActionWidth - kActionGlyph) / 2.0;
        paintGlyph(&painter, m_actions.at(i).glyph,
                   QRectF(box).adjusted(inset, (kActionHeight - kActionGlyph) / 2.0,
                                        -inset, -(kActionHeight - kActionGlyph) / 2.0),
                   i == m_hoveredAction ? profile.colors.textPrimary
                                        : profile.colors.textDisabled);
        rightEdge = box.left() - kActionGap;
    }

    // `<path d="M5 12h14"/>` and `<rect x="5.5" y="5.5" width="13" height="13"
    // rx="2"/>` - a dash and a square, the window controls' own marks.
    const auto paintBarButton = [&](const QRect& box, Glyph glyph, bool hot) {
        if (hot) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(profile.colors.surfaceHover);
            painter.drawRoundedRect(QRectF(box), profile.radii.small, profile.radii.small);
        }
        const double inset = (kActionWidth - kActionGlyph) / 2.0;
        paintGlyph(&painter, glyph,
                   QRectF(box).adjusted(inset, (kActionHeight - kActionGlyph) / 2.0,
                                        -inset, -(kActionHeight - kActionGlyph) / 2.0),
                   hot ? profile.colors.textPrimary : profile.colors.textDisabled);
    };
    paintBarButton(minimiseRect(), Glyph::WindowMinimise, m_hoveredMinimise);
    paintBarButton(maximiseRect(),
                   isMaximised() ? Glyph::WindowRestore : Glyph::WindowMaximise, m_hoveredMaximise);
    rightEdge = minimiseRect().left() - kActionGap;

    const QRect close = closeRect();
    if (m_hoveredClose) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(profile.window.closeHoverBg);
        painter.drawRoundedRect(QRectF(close), profile.radii.small, profile.radii.small);
    }
    const double closeInset = (kActionWidth - kActionGlyph) / 2.0;
    paintGlyph(&painter, Glyph::Close,
               QRectF(close).adjusted(closeInset, (kActionHeight - kActionGlyph) / 2.0,
                                      -closeInset, -(kActionHeight - kActionGlyph) / 2.0),
               m_hoveredClose ? profile.window.closeHoverGlyph : profile.colors.textDisabled);

    // ── title and subtitle ───────────────────────────────────────────────
    const QFont titles = titleFont(this);
    const QFontMetrics titleMetrics(titles);
    const int available = qMax(0, rightEdge - x - kBarGap);

    if (m_subtitle.isEmpty()) {
        painter.setFont(titles);
        painter.setPen(m_front ? profile.colors.textPrimary : profile.colors.textSecondary);
        painter.drawText(QRect(x, 0, available, bar.height()),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         titleMetrics.elidedText(m_title, Qt::ElideRight, available));
    } else {
        const QFont subs = subtitleFont(this);
        const QFontMetrics subMetrics(subs);
        const int block = titleMetrics.height() + 1 + subMetrics.height();
        int y = (bar.height() - block) / 2;

        painter.setFont(titles);
        painter.setPen(m_front ? profile.colors.textPrimary : profile.colors.textSecondary);
        painter.drawText(QRect(x, y, available, titleMetrics.height()),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         titleMetrics.elidedText(m_title, Qt::ElideRight, available));
        y += titleMetrics.height() + 1;

        painter.setFont(subs);
        painter.setPen(profile.colors.textDisabled);
        painter.drawText(QRect(x, y, available, subMetrics.height()),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         subMetrics.elidedText(m_subtitle, Qt::ElideMiddle, available));
    }

    if (m_rolled) {
        return;
    }

    // ── the task strip ───────────────────────────────────────────────────
    if (m_task && !m_taskText.isEmpty()) {
        const QRect strip = m_task->geometry();
        painter.setPen(QPen(profile.colors.borderSubtle, 1));
        painter.drawLine(strip.left(), strip.top(), strip.right(), strip.top());

        const QFont font = taskFont(this);
        const QFontMetrics fm(font);
        painter.setFont(font);

        int metaWidth = 0;
        if (!m_taskMeta.isEmpty()) {
            metaWidth = fm.horizontalAdvance(m_taskMeta);
            painter.setPen(profile.colors.textDisabled);
            painter.drawText(QRect(strip.right() - kTaskPaddingRight - metaWidth,
                                   strip.top(), metaWidth, strip.height()),
                             Qt::AlignRight | Qt::AlignVCenter, m_taskMeta);
            metaWidth += kBarGap;
        }
        painter.setPen(profile.colors.textSecondary);
        const QRect text(strip.left() + kTaskPaddingLeft, strip.top(),
                         qMax(0, strip.width() - kTaskPaddingLeft - kTaskPaddingRight
                                  - metaWidth),
                         strip.height());
        painter.drawText(text, Qt::AlignLeft | Qt::AlignVCenter,
                         fm.elidedText(m_taskText, Qt::ElideRight, text.width()));
    }

    // ── the grip ─────────────────────────────────────────────────────────
    const QRect grip = gripRect();
    if (!grip.isNull()) {
        // `.fw-grip` — two diagonal strokes at half opacity. Drawn rather than
        // styled because a background-image gradient is two more things to keep
        // in step with the hairline colour.
        painter.setPen(QPen(withAlpha(profile.colors.border, 0.9), 1.4));
        painter.drawLine(grip.left() + 5, grip.bottom(), grip.right(), grip.top() + 5);
        painter.drawLine(grip.left() + 10, grip.bottom(), grip.right(), grip.top() + 10);
    }
}

} // namespace novu::ui
