#include "novu/ui/FloatingPanel.h"

#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Tooltip.h"

#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPainterPath>
#include <QRadialGradient>
#include <QResizeEvent>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

// `.fw-bar { height:40px; padding:0 6px 0 11px; gap:9px }`.
constexpr int kBarHeight = 40;
constexpr int kBarPaddingLeft = 11;
constexpr int kBarPaddingRight = 6;
constexpr int kBarGap = 9;
constexpr int kBarGlyph = 15;

// `.fw-bar .ibtn { width:26px; height:22px }` with an 11px glyph.
constexpr int kActionWidth = 26;
constexpr int kActionHeight = 22;
constexpr int kActionGlyph = 11;
constexpr int kActionGap = 5;

// `.floatwin { min-width:380px; min-height:200px }`.
constexpr int kMinWidth = 380;
constexpr int kMinHeight = 200;

// `.floatwin .fw-grip { width:14px; height:14px }`.
constexpr int kGrip = 14;

// `.fw-task { padding:7px 10px 7px 12px; gap:9px }`.
constexpr int kTaskPaddingV = 7;
constexpr int kTaskPaddingLeft = 12;
constexpr int kTaskPaddingRight = 10;

// `.floatwin::before` — a soft accent wash down from the top edge, 90px deep.
constexpr int kWashHeight = 90;
constexpr double kWashAlpha = 0.09;

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
    // Transparent to the mouse everywhere a panel is not: the application
    // underneath keeps working, which is the whole difference between this and
    // a modal scrim.
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);

    if (host) {
        host->installEventFilter(this);
        setGeometry(host->rect());
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
    }
    if (watched == m_host && event->type() == QEvent::ChildAdded) {
        // Anything added to the host after the layer would otherwise paint over
        // it, and the panels would disappear behind a page.
        raise();
    }
    return QWidget::eventFilter(watched, event);
}

void FloatingLayer::addPanel(FloatingPanel* panel)
{
    if (!panel) {
        return;
    }
    panel->setParent(this);
    panel->show();
    raisePanel(panel);
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

    m_body = new QWidget(this);
    auto* layout = new QVBoxLayout(m_body);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

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

void FloatingPanel::setBodyWidget(QWidget* widget)
{
    if (!widget || widget == m_body) {
        return;
    }
    m_body->deleteLater();
    m_body = widget;
    m_body->setParent(this);
    m_body->setVisible(!m_rolled);
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
        m_body->setVisible(false);
        if (m_task) {
            m_task->setVisible(false);
        }
        resize(width(), barHeight());
    } else {
        m_rolled = false;
        m_body->setVisible(true);
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

QRect FloatingPanel::actionRect(int index) const
{
    if (index < 0 || index >= m_actions.size()) {
        return QRect();
    }
    // Laid out right to left from the close button, so adding an action never
    // moves the close - the one control a user reaches for without looking.
    const int fromRight = m_actions.size() - index;
    const int x = closeRect().left() - fromRight * (kActionWidth + kActionGap);
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
        m_body->setGeometry(0, top, width(), 0);
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
    m_body->setGeometry(0, top, width(), qMax(0, bottom - top));
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
        emit closeRequested();
        return;
    }
    for (int i = 0; i < m_actions.size(); ++i) {
        if (actionRect(i).contains(position)) {
            m_drag = Drag::None;
            emit barActionTriggered(m_actions.at(i).key);
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
        } else if (barRect().contains(position) && hovered < 0 && !onClose) {
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
    spec.radius = SurfaceRole::Dialog;
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

    // `.fw-bar` — the title-bar gradient, the same two tokens the window and
    // the status bar use.
    const QRect bar = barRect();
    QLinearGradient ground(0, 0, 0, bar.height());
    ground.setColorAt(0.0, profile.window.titleBarBg);
    ground.setColorAt(1.0, profile.window.titleBarBgBottom);
    painter.fillRect(bar, ground);

    // `.floatwin::before` — a soft accent wash from the top edge. Only on the
    // front panel: it is what says which one has the keyboard.
    if (m_front) {
        QRadialGradient wash(QPointF(width() / 2.0, 0.0), width() * 0.7);
        QColor inner = profile.colors.accent;
        inner.setAlphaF(kWashAlpha);
        QColor outer = profile.colors.accent;
        outer.setAlphaF(0.0);
        wash.setColorAt(0.0, inner);
        wash.setColorAt(1.0, outer);
        painter.fillRect(QRect(0, 0, width(), kWashHeight), wash);
    }

    painter.setPen(QPen(profile.colors.border, 1));
    painter.drawLine(0, bar.bottom(), width(), bar.bottom());
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
