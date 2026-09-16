#include "novu/ui/DropTarget.h"

#include <QPainter>
#include <QWidget>

#include "novu/ui/ThemeProfile.h"

namespace novu::ui {

bool DropHighlight::setRow(int index)
{
    if (index < 0) {
        return clear();
    }
    if (m_zone == DropZone::Row && m_row == index) {
        return false;
    }
    m_zone = DropZone::Row;
    m_row = index;
    return true;
}

bool DropHighlight::setRegion()
{
    if (m_zone == DropZone::Region) {
        return false;
    }
    m_zone = DropZone::Region;
    // Dropped rather than kept: row() would otherwise have to defend itself
    // against a state it cannot be read from, and a stale index is the kind of
    // thing that resurfaces a release later.
    m_row = -1;
    return true;
}

bool DropHighlight::clear()
{
    if (m_zone == DropZone::None) {
        return false;
    }
    m_zone = DropZone::None;
    m_row = -1;
    return true;
}

StyleSpec dropRowSpec()
{
    StyleSpec spec;
    spec.background = token(ColorRole::Accent, 0.20);
    spec.border = token(ColorRole::Accent);
    spec.foreground = dropLabelInk();
    spec.borderWidth = BorderRole::Input;   // 1px, as `inset 0 0 0 1px`
    // The design states no radius on `.trow.drop-on` - the highlight takes the
    // row's own. A tree row is tight, so the scale's small step, said as a step
    // rather than a number so a denser profile moves it.
    spec.radius = SurfaceRole::Input;
    spec.radiusStepOverride = static_cast<int>(RadiusStep::Small);
    spec.weight = dropLabelWeight();
    return spec;
}

StyleSpec dropRegionSpec()
{
    StyleSpec spec;
    spec.background = token(ColorRole::Accent, 0.07);
    spec.border = token(ColorRole::Accent, 0.55);
    // 1.5px in the design. Borders are whole pixels here, and the heavier of
    // the two reads as intended: the region outline is the thing a person is
    // looking for when no row is lit.
    spec.borderWidth = BorderRole::Focus;
    // `border-radius: var(--r-md)`, which is the scale's medium step.
    spec.radius = SurfaceRole::Input;
    spec.radiusStepOverride = static_cast<int>(RadiusStep::Medium);
    return spec;
}

ColorToken dropLabelInk()
{
    return token(ColorRole::Accent);
}

int dropLabelWeight()
{
    return 600;
}

namespace {

// `box-shadow: inset` draws inside the box. Qt strokes centred on the path, so
// the rect is pulled in by half the pen - without it a lit row bleeds a pixel
// over each neighbour and the list appears to shift while a drag passes down
// it.
void paintInset(QPainter* painter, const QRect& rect, const QWidget* context,
                const StyleSpec& spec)
{
    if (!painter || rect.isEmpty()) {
        return;
    }
    const ResolvedStyle style = resolve(spec, context, rect.height());
    const qreal pen = style.borderWidth;
    const QRectF outline = QRectF(rect).adjusted(pen / 2.0, pen / 2.0, -pen / 2.0, -pen / 2.0);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(style.background);
    painter->drawRoundedRect(QRectF(rect), style.radius, style.radius);

    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(style.border, pen));
    const qreal r = qMax(0.0, style.radius - pen / 2.0);
    painter->drawRoundedRect(outline, r, r);
    painter->restore();
}

} // namespace

void paintDropRow(QPainter* painter, const QRect& rect, const QWidget* context)
{
    paintInset(painter, rect, context, dropRowSpec());
}

void paintDropRegion(QPainter* painter, const QRect& rect, const QWidget* context)
{
    paintInset(painter, rect, context, dropRegionSpec());
}

} // namespace novu::ui
