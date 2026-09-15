#include "novu/ui/Splitter.h"

#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QEnterEvent>
#include <QEvent>
#include <QHideEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPolygonF>

namespace novu::ui {

namespace {

// The grab band. Wider than the line it draws, because a one-pixel target is
// one a user has to aim at.
constexpr int kHandleThickness = 8;

// The arrow zone at each end, and the shortest handle that gets arrows at all.
// Below this the two zones would meet in the middle and there would be no drag
// band left between them.
constexpr int kArrowZone = 30;
constexpr int kMinimumForArrows = 60;

// How far the arrow sits from its end of the handle, and how big it is.
constexpr int kArrowInset = 16;
constexpr double kArrowHalf = 3.5;

// The collapse divider is thinner: it has no drag to make room for.
constexpr int kDividerThickness = 10;
constexpr double kDividerArrowHalf = 4.0;

} // namespace

// == SplitterHandle =========================================================

SplitterHandle::SplitterHandle(Qt::Orientation orientation, QSplitter* parent)
    : QSplitterHandle(orientation, parent)
{
    // Most of the handle is deliberately left unpainted so the panels' own
    // backgrounds show through; only the centre line and the arrows are drawn.
    setAttribute(Qt::WA_StyledBackground, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAutoFillBackground(false);
    setMouseTracking(true);
    setCursor(orientation == Qt::Horizontal ? Qt::SplitHCursor : Qt::SplitVCursor);
}

void SplitterHandle::setPanelsVisible(bool leading, bool trailing)
{
    if (m_leadingVisible == leading && m_trailingVisible == trailing) {
        return;
    }
    m_leadingVisible = leading;
    m_trailingVisible = trailing;
    update();
}

void SplitterHandle::setCollapseArrows(bool arrows)
{
    if (m_arrows == arrows) {
        return;
    }
    m_arrows = arrows;
    update();
}

QSize SplitterHandle::sizeHint() const
{
    return orientation() == Qt::Horizontal ? QSize(kHandleThickness, 1)
                                           : QSize(1, kHandleThickness);
}

int SplitterHandle::positionAlong(const QPoint& point) const
{
    return orientation() == Qt::Horizontal ? point.y() : point.x();
}

int SplitterHandle::lengthAlong() const
{
    return orientation() == Qt::Horizontal ? height() : width();
}

bool SplitterHandle::inLeadingZone(const QPoint& point) const
{
    return m_arrows && lengthAlong() >= kMinimumForArrows
        && positionAlong(point) < kArrowZone;
}

bool SplitterHandle::inTrailingZone(const QPoint& point) const
{
    return m_arrows && lengthAlong() >= kMinimumForArrows
        && positionAlong(point) > lengthAlong() - kArrowZone;
}

QRect SplitterHandle::leadingArrowRect() const
{
    if (!m_arrows || lengthAlong() < kMinimumForArrows) {
        return QRect();
    }
    return orientation() == Qt::Horizontal ? QRect(0, 0, width(), kArrowZone)
                                           : QRect(0, 0, kArrowZone, height());
}

QRect SplitterHandle::trailingArrowRect() const
{
    if (!m_arrows || lengthAlong() < kMinimumForArrows) {
        return QRect();
    }
    return orientation() == Qt::Horizontal
        ? QRect(0, height() - kArrowZone, width(), kArrowZone)
        : QRect(width() - kArrowZone, 0, kArrowZone, height());
}

void SplitterHandle::updateCursor(const QPoint& point)
{
    const bool onArrow = inLeadingZone(point) || inTrailingZone(point);
    setCursor(onArrow ? Qt::PointingHandCursor
                      : (orientation() == Qt::Horizontal ? Qt::SplitHCursor
                                                         : Qt::SplitVCursor));
}

void SplitterHandle::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    const QPoint position = event->position().toPoint();
    m_hoveredZone = inLeadingZone(position) ? 0 : (inTrailingZone(position) ? 1 : -1);
    updateCursor(position);
    update();
    QSplitterHandle::enterEvent(event);
}

void SplitterHandle::leaveEvent(QEvent* event)
{
    m_hovered = false;
    m_hoveredZone = -1;
    setCursor(orientation() == Qt::Horizontal ? Qt::SplitHCursor : Qt::SplitVCursor);
    update();
    QSplitterHandle::leaveEvent(event);
}

void SplitterHandle::hideEvent(QHideEvent* event)
{
    // Minimising the window swallows the release, which leaves the press state
    // stuck and the next click behaving as the end of a drag nobody started.
    m_hovered = false;
    m_hoveredZone = -1;
    m_pressedZone = -1;
    update();
    QSplitterHandle::hideEvent(event);
}

void SplitterHandle::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    const QPoint position = event->position().toPoint();
    if (inLeadingZone(position)) {
        m_pressedZone = 0;
        update();
        return;
    }
    if (inTrailingZone(position)) {
        m_pressedZone = 1;
        update();
        return;
    }
    m_pressedZone = -1;
    QSplitterHandle::mousePressEvent(event);
}

void SplitterHandle::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    if (m_pressedZone < 0) {
        QSplitterHandle::mouseReleaseEvent(event);
        return;
    }

    const QPoint position = event->position().toPoint();
    const int released = inLeadingZone(position) ? 0 : (inTrailingZone(position) ? 1 : -1);
    // Press and release in the same zone, or nothing happened: a press that
    // began on the arrow and ended in the drag band is a gesture the user
    // thought better of.
    if (released == m_pressedZone) {
        emit collapseRequested(m_pressedZone == 0);
    }
    m_pressedZone = -1;
    update();
}

void SplitterHandle::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint position = event->position().toPoint();
    if (m_pressedZone >= 0) {
        // A press that started on an arrow never becomes a drag, however far
        // the pointer travels.
        const int zone = inLeadingZone(position) ? 0 : (inTrailingZone(position) ? 1 : -1);
        if (zone != m_hoveredZone) {
            m_hoveredZone = zone;
            update();
        }
        return;
    }

    const int zone = inLeadingZone(position) ? 0 : (inTrailingZone(position) ? 1 : -1);
    if (zone != m_hoveredZone) {
        m_hoveredZone = zone;
        updateCursor(position);
        update();
    }
    QSplitterHandle::mouseMoveEvent(event);
}

bool SplitterHandle::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        update();
    }
    return QSplitterHandle::event(event);
}

StyleSpec SplitterHandle::styleSpec() const
{
    StyleSpec spec;
    // Nothing behind it: the panels' own grounds meet at the line, and a plate
    // here would draw a strip of a third colour between them.
    spec.background = token(ColorRole::Transparent);
    spec.border = token(ColorRole::BorderSubtle);
    spec.hoverBorder = token(ColorRole::BorderHover);
    spec.foreground = token(ColorRole::BorderHover);
    spec.hoverForeground = token(ColorRole::Accent);
    spec.radius = SurfaceRole::Toolbar;
    spec.borderWidth = BorderRole::Divider;
    spec.type = TypeRole::Body;
    return spec;
}

ResolvedStyle SplitterHandle::resolvedStyle() const
{
    return resolve(styleSpec(), this, lengthAlong());
}

void SplitterHandle::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const ResolvedStyle style = resolvedStyle();
    const bool horizontal = orientation() == Qt::Horizontal;

    QPainter painter(this);
    // Deliberately off: the centre line is one device pixel and antialiasing it
    // spreads it over two, which at this width reads as a smudge.
    painter.setRenderHint(QPainter::Antialiasing, false);

    const int centre = horizontal ? width() / 2 : height() / 2;
    painter.setPen(m_hovered ? style.hoverBorder : style.border);
    if (horizontal) {
        painter.drawLine(centre, 0, centre, height());
    } else {
        painter.drawLine(0, centre, width(), centre);
    }

    if (!m_arrows || lengthAlong() < kMinimumForArrows) {
        return;
    }

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);

    // Each arrow points away from the panel it would collapse, and turns round
    // once that panel is gone — so one triangle says both "put this away" and
    // "bring it back".
    const struct { int zone; int along; bool visible; } arrows[] = {
        {0, kArrowInset, m_leadingVisible},
        {1, lengthAlong() - kArrowInset, m_trailingVisible},
    };

    for (const auto& arrow : arrows) {
        const bool hot = m_hovered && m_hoveredZone == arrow.zone;
        const bool held = m_pressedZone == arrow.zone;
        QColor ink = held ? profile.colors.accent
                          : (hot ? profile.colors.accent
                                 : (m_hovered ? profile.colors.textSecondary
                                              : style.foreground));
        painter.setBrush(ink);

        // Leading collapses towards the start, so it points that way while the
        // panel is there and the other way once it is not.
        const bool pointsToStart = arrow.zone == 0 ? arrow.visible : !arrow.visible;
        const double tip = pointsToStart ? arrow.along - kArrowHalf : arrow.along + kArrowHalf;
        const double base = pointsToStart ? arrow.along + kArrowHalf : arrow.along - kArrowHalf;

        QPolygonF triangle;
        if (horizontal) {
            triangle << QPointF(centre - kArrowHalf, base)
                     << QPointF(centre + kArrowHalf, base)
                     << QPointF(centre, tip);
        } else {
            triangle << QPointF(base, centre - kArrowHalf)
                     << QPointF(base, centre + kArrowHalf)
                     << QPointF(tip, centre);
        }
        painter.drawPolygon(triangle);
    }
}

// == Splitter ===============================================================

Splitter::Splitter(Qt::Orientation orientation, QWidget* parent)
    : QSplitter(orientation, parent)
{
    setChildrenCollapsible(true);
    setHandleWidth(kHandleThickness);
}

Splitter::Splitter(QWidget* parent)
    : Splitter(Qt::Horizontal, parent)
{
}

void Splitter::setCollapseArrows(bool arrows)
{
    if (m_arrows == arrows) {
        return;
    }
    m_arrows = arrows;
    syncHandles();
}

QSplitterHandle* Splitter::createHandle()
{
    auto* handle = new SplitterHandle(orientation(), this);
    handle->setCollapseArrows(m_arrows);
    connect(handle, &SplitterHandle::collapseRequested, this, [this, handle](bool leading) {
        // The handle knows which end was pressed; the splitter has to work out
        // which panel that is, because a handle's index is one more than the
        // panel before it.
        const int index = indexOf(handle);
        togglePanel(leading ? index - 1 : index);
    });
    return handle;
}

bool Splitter::isPanelCollapsed(int index) const
{
    const QList<int> current = sizes();
    return index >= 0 && index < current.size() && current.at(index) == 0;
}

void Splitter::setPanelCollapsed(int index, bool collapsed)
{
    QList<int> current = sizes();
    if (index < 0 || index >= current.size()) {
        return;
    }
    if ((current.at(index) == 0) == collapsed) {
        return;
    }

    // The neighbour that gains or gives back the space. The one after, unless
    // this is the last panel.
    const int neighbour = index + 1 < current.size() ? index + 1 : index - 1;
    if (neighbour < 0) {
        return;
    }

    if (collapsed) {
        // Remembered, so restoring puts the panel back at the width the user
        // chose rather than at whatever the layout would decide.
        m_restoreSizes.insert(index, current.at(index));
        current[neighbour] += current.at(index);
        current[index] = 0;
    } else {
        const int restore = qMax(1, m_restoreSizes.value(index, widget(index)
                                                                    ? widget(index)
                                                                          ->sizeHint()
                                                                          .width()
                                                                    : 200));
        const int taken = qMin(restore, qMax(0, current.at(neighbour) - 1));
        current[neighbour] -= taken;
        current[index] = taken;
    }

    setSizes(current);
    syncHandles();
    emit panelCollapsedChanged(index, collapsed);
}

void Splitter::togglePanel(int index)
{
    setPanelCollapsed(index, !isPanelCollapsed(index));
}

void Splitter::syncHandles()
{
    for (int i = 1; i < count(); ++i) {
        auto* handle = qobject_cast<SplitterHandle*>(this->handle(i));
        if (!handle) {
            continue;
        }
        handle->setCollapseArrows(m_arrows);
        handle->setPanelsVisible(!isPanelCollapsed(i - 1), !isPanelCollapsed(i));
    }
}

// == CollapseDivider ========================================================

CollapseDivider::CollapseDivider(QWidget* target, QWidget* parent)
    : QWidget(parent)
    , m_target(target)
{
    setFixedHeight(kDividerThickness);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

QSize CollapseDivider::sizeHint() const
{
    return QSize(120, kDividerThickness);
}

void CollapseDivider::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed) {
        return;
    }
    m_collapsed = collapsed;
    if (m_target) {
        m_target->setVisible(!collapsed);
    }
    update();
    emit collapsedChanged(m_collapsed);
}

void CollapseDivider::toggle()
{
    setCollapsed(!m_collapsed);
}

void CollapseDivider::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void CollapseDivider::leaveEvent(QEvent* event)
{
    m_hovered = false;
    update();
    QWidget::leaveEvent(event);
}

void CollapseDivider::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    toggle();
}

bool CollapseDivider::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        update();
    }
    return QWidget::event(event);
}

void CollapseDivider::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    const int centre = height() / 2;
    painter.setPen(m_hovered ? profile.colors.borderHover : profile.colors.borderSubtle);
    painter.drawLine(0, centre, width(), centre);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_hovered ? profile.colors.accent : profile.colors.borderHover);

    const double x = width() / 2.0;
    QPolygonF triangle;
    if (m_collapsed) {
        // Pointing down: click to bring it back.
        triangle << QPointF(x - kDividerArrowHalf, centre - 2)
                 << QPointF(x + kDividerArrowHalf, centre - 2)
                 << QPointF(x, centre + kDividerArrowHalf);
    } else {
        triangle << QPointF(x - kDividerArrowHalf, centre + 2)
                 << QPointF(x + kDividerArrowHalf, centre + 2)
                 << QPointF(x, centre - kDividerArrowHalf);
    }
    painter.drawPolygon(triangle);
}

} // namespace novu::ui
