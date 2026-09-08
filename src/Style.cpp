#include "novu/ui/Style.h"

#include "novu/ui/ThemeManager.h"

#include <QApplication>
#include <QPainter>
#include <QPolygonF>
#include <QStyleOption>
#include <QWidget>

namespace novu::ui {

namespace {

const ThemeProfile& profileFor(const QWidget* widget)
{
    // An item-view indicator is drawn with the view as the widget, but a style
    // may also be asked to draw with no widget at all - a QStyleOption built by
    // a delegate for measurement, for instance.
    return widget ? themeFor(widget) : ThemeManager::instance().defaultProfile();
}

// The one drawing of a Novu check indicator: an outline that never goes away
// and a mark inside it.
//
// Not a filled box. A solid accent square reads as a colour swatch rather than
// as something the user ticked, and it is what every one of these applications
// was showing - either because a stylesheet cannot draw a shape inside the
// indicator, or because the indicator was left to the platform entirely.
void paintIndicator(QPainter* painter, const QStyleOption* option, const QWidget* widget)
{
    const ThemeProfile& profile = profileFor(widget);
    const ThemeColors& colors = profile.colors;

    const bool enabled = option->state & QStyle::State_Enabled;
    const bool hovered = option->state & QStyle::State_MouseOver;
    const bool on = option->state & QStyle::State_On;
    const bool partial = option->state & QStyle::State_NoChange;

    // Square, so the mark inside it is not squashed on one axis. An item view
    // hands over the cell's whole indicator column, which is not square.
    const int side = qMin(option->rect.width(), option->rect.height());
    QRectF box(0, 0, side, side);
    box.moveCenter(QRectF(option->rect).center());
    // Half-pixel inset, so a one-pixel outline lands on the pixel rather than
    // straddling it and coming out as two rows of grey.
    box.adjust(0.5, 0.5, -0.5, -0.5);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QColor outline = withAlpha(colors.textDisabled, 0.60);
    if (!enabled) {
        outline = withAlpha(colors.textDisabled, 0.35);
    } else if (on || partial || hovered) {
        // Hover and checked are the accent; resting is not. borderHover is the
        // hover colour and in NovuTrade it is green, so using it at rest made
        // every unchecked box look as though the pointer were already on it.
        outline = colors.accent;
    }

    const int radius = profile.radii.xs;
    painter->setPen(QPen(outline, profile.border(BorderRole::Input)));
    painter->setBrush(colors.panelBgAlt);
    painter->drawRoundedRect(box, radius, radius);

    if (!on && !partial) {
        painter->restore();
        return;
    }

    QPen mark(enabled ? colors.accent : colors.textDisabled);
    // The strong border weight, not the hairline: at this size a one-pixel tick
    // disappears against the outline it sits inside.
    mark.setWidthF(qMax(2, profile.borders.strong));
    mark.setCapStyle(Qt::RoundCap);
    mark.setJoinStyle(Qt::RoundJoin);
    painter->setPen(mark);
    painter->setBrush(Qt::NoBrush);

    // Normalised inside the box, so the mark keeps its proportions at any
    // indicator size and on any display scale.
    const auto at = [&box](qreal x, qreal y) {
        return QPointF(box.left() + box.width() * x, box.top() + box.height() * y);
    };

    if (partial) {
        // A tristate parent says "some of them", which is neither a tick nor a
        // cross.
        painter->drawLine(at(0.26, 0.50), at(0.74, 0.50));
    } else if (widget
               && widget->property(prop::mark).toString() == QLatin1String("cross")) {
        painter->drawLine(at(0.28, 0.28), at(0.72, 0.72));
        painter->drawLine(at(0.72, 0.28), at(0.28, 0.72));
    } else {
        // Two strokes, with the long arm rising past the box's own centre - a
        // tick drawn symmetrically reads as a check mark only at large sizes.
        painter->drawPolyline(QPolygonF({at(0.24, 0.52), at(0.42, 0.71), at(0.76, 0.30)}));
    }

    painter->restore();
}

} // namespace

NovuStyle::NovuStyle(QStyle* base)
    : QProxyStyle(base)
{
}

void NovuStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                              QPainter* painter, const QWidget* widget) const
{
    switch (element) {
    case PE_IndicatorCheckBox:          // QCheckBox, checkable QGroupBox
    case PE_IndicatorItemViewItemCheck: // a checkable cell in a table or tree
        paintIndicator(painter, option, widget);
        return;
    default:
        break;
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

int NovuStyle::pixelMetric(PixelMetric metric, const QStyleOption* option,
                           const QWidget* widget) const
{
    switch (metric) {
    case PM_IndicatorWidth:
    case PM_IndicatorHeight:
        // The icon scale, not a number of this style's own. Even, so the mark
        // centres on a whole pixel.
        return profileFor(widget).icons.medium;
    case PM_CheckBoxLabelSpacing:
        return profileFor(widget).spacing.sm;
    default:
        break;
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
}

NovuStyle* installNovuStyle()
{
    auto* app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) {
        return nullptr;
    }
    if (auto* existing = qobject_cast<NovuStyle*>(app->style())) {
        return existing;
    }
    // Wraps whatever is there rather than replacing it: an application that
    // chose Fusion keeps Fusion for everything this style does not override.
    // QApplication::setStyle() takes ownership, and the previous style is
    // retained as the proxy's base, so nothing is deleted out from under it.
    auto* style = new NovuStyle;
    app->setStyle(style);
    return style;
}

} // namespace novu::ui
