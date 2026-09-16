#include "novu/ui/Swatch.h"

#include "novu/ui/Icon.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

namespace novu::ui {

namespace {

// The design's `.color-pills`: 6px between swatches, both ways.
constexpr int kGap = 6;
// The tick and the diagonal both sit inside the swatch, so neither needs room
// of its own.
constexpr double kMarkScale = 0.55;

} // namespace

Swatch::Swatch(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

QList<QColor> tagPalette(const QWidget* context)
{
    // The family's named hues, which is what a tag palette *is* in the approved
    // design: NovuExplorer states this exact set, in this order, as the choices
    // in its folder-colour picker.
    //
    // It used to be the profile's chart series plus the four status colours -
    // so tagging was done in the product accent and in the colours that mean
    // "failed" and "warning". A tag is a label a person picks, not a status the
    // application reports, and the two must not be confusable.
    Q_UNUSED(context);
    QList<QColor> out;
    out.reserve(familyHues().size());
    for (const FamilyHue& hue : familyHues()) {
        out.append(hue.color);
    }
    return out;
}

void Swatch::setColors(const QList<QColor>& colors, bool withNone, bool withCustom)
{
    m_colors = colors;
    m_withNone = withNone;
    m_withCustom = withCustom;
    m_hovered = -1;
    m_pressed = -1;
    updateGeometry();
    update();
}

int Swatch::count() const
{
    return m_colors.size() + (m_withNone ? 1 : 0) + (m_withCustom ? 1 : 0);
}

bool Swatch::isNoneAt(int index) const
{
    return m_withNone && index == 0;
}

bool Swatch::isCustomAt(int index) const
{
    return m_withCustom && index == count() - 1;
}

QColor Swatch::colorAt(int index) const
{
    if (isNoneAt(index) || isCustomAt(index)) {
        return {};
    }
    const int offset = index - (m_withNone ? 1 : 0);
    return offset >= 0 && offset < m_colors.size() ? m_colors.at(offset) : QColor();
}

void Swatch::setCurrent(const QColor& color)
{
    if (m_current == color) {
        return;
    }
    m_current = color;
    update();
}

void Swatch::setMultiSelect(bool multi)
{
    if (m_multi == multi) {
        return;
    }
    m_multi = multi;
    update();
}

void Swatch::setChosen(const QList<QColor>& colors)
{
    m_chosen = colors;
    update();
}

void Swatch::setSwatchSize(int px)
{
    px = qMax(8, px);
    if (m_size == px) {
        return;
    }
    m_size = px;
    updateGeometry();
    update();
}

int Swatch::columns(int width) const
{
    return qMax(1, (width + kGap) / (m_size + kGap));
}

QRect Swatch::swatchRect(int index) const
{
    if (index < 0 || index >= count()) {
        return {};
    }
    const int perRow = columns(width());
    const int row = index / perRow;
    const int column = index % perRow;
    return QRect(column * (m_size + kGap), row * (m_size + kGap), m_size, m_size);
}

int Swatch::indexAt(const QPoint& position) const
{
    for (int i = 0; i < count(); ++i) {
        if (swatchRect(i).contains(position)) {
            return i;
        }
    }
    return -1;
}

QSize Swatch::sizeHint() const
{
    const int n = count();
    if (n == 0) {
        return {0, 0};
    }
    return QSize(n * m_size + (n - 1) * kGap, m_size);
}

int Swatch::heightForWidth(int width) const
{
    const int n = count();
    if (n == 0) {
        return 0;
    }
    const int perRow = columns(width);
    const int rows = (n + perRow - 1) / perRow;
    return rows * m_size + (rows - 1) * kGap;
}

StyleSpec Swatch::styleSpec() const
{
    StyleSpec spec;
    spec.radius = SurfaceRole::Button;
    spec.radiusStepOverride = static_cast<int>(RadiusStep::Small);
    spec.borderWidth = BorderRole::Card;
    spec.background = token(ColorRole::PanelBgAlt);
    spec.border = token(ColorRole::BorderHover);
    spec.foreground = token(ColorRole::TextDisabled);
    // The ring round the chosen one, and the tick inside it.
    spec.checkedBorder = token(ColorRole::TextPrimary);
    spec.checkedForeground = token(ColorRole::TextOnAccent);
    return spec;
}

ResolvedStyle Swatch::resolvedStyle() const
{
    return resolve(styleSpec(), this, m_size);
}

void Swatch::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || !isEnabled()) {
        QWidget::mousePressEvent(event);
        return;
    }
    m_pressed = indexAt(event->position().toPoint());
    if (m_pressed >= 0) {
        event->accept();
        update();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void Swatch::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const int index = indexAt(event->position().toPoint());
    // Press and release in the same swatch, or the gesture was cancelled.
    if (m_pressed >= 0 && index == m_pressed) {
        if (isCustomAt(index)) {
            emit customRequested();
        } else {
            const QColor color = colorAt(index);
            if (m_multi) {
                if (m_chosen.contains(color)) {
                    m_chosen.removeAll(color);
                } else if (color.isValid()) {
                    m_chosen.append(color);
                } else {
                    // The "none" swatch in a filter clears it: "any of these"
                    // with nothing chosen is the same as no filter.
                    m_chosen.clear();
                }
                emit chosenChanged(m_chosen);
            } else {
                setCurrent(color);
            }
            emit picked(color);
        }
    }
    m_pressed = -1;
    update();
    event->accept();
}

void Swatch::mouseMoveEvent(QMouseEvent* event)
{
    const int under = indexAt(event->position().toPoint());
    if (under != m_hovered) {
        m_hovered = under;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void Swatch::leaveEvent(QEvent* event)
{
    m_hovered = -1;
    m_pressed = -1;
    update();
    QWidget::leaveEvent(event);
}

bool Swatch::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void Swatch::paintEvent(QPaintEvent*)
{
    const ResolvedStyle style = resolvedStyle();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    for (int i = 0; i < count(); ++i) {
        const QRect box = swatchRect(i);
        const QColor color = colorAt(i);
        const bool none = isNoneAt(i);
        const bool custom = isCustomAt(i);

        const bool on = m_multi ? (none ? m_chosen.isEmpty() : m_chosen.contains(color))
                                : (none ? !m_current.isValid() : m_current == color);

        QPainterPath shape;
        shape.addRoundedRect(QRectF(box), style.radius, style.radius);

        if (custom) {
            // The multi-hued one, so it cannot be mistaken for a colour that is
            // in the palette already.
            QLinearGradient hues(box.topLeft(), box.bottomRight());
            const QList<QColor> series = tagPalette(this);
            for (int s = 0; s < series.size(); ++s) {
                hues.setColorAt(static_cast<double>(s) / qMax(1, series.size() - 1),
                                series.at(s));
            }
            painter.fillPath(shape, hues);
        } else if (none) {
            painter.fillPath(shape, style.background);
        } else {
            painter.fillPath(shape, color);
        }

        // The outline: the chosen one gets the strong ink, the hovered one the
        // control outline, everything else the surface rule.
        QColor outline = style.border;
        double weight = style.borderWidth;
        if (on) {
            outline = style.checkedBorder;
            weight = style.borderWidth + 1;
        } else if (i == m_hovered && isEnabled()) {
            outline = style.checkedBorder;
        }
        painter.setPen(QPen(outline, weight));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(shape);

        const double mark = box.width() * kMarkScale;
        const QRectF markBox(box.center().x() - mark / 2.0 + 0.5,
                             box.center().y() - mark / 2.0 + 0.5, mark, mark);
        if (none) {
            // A diagonal, the way every palette says "no colour".
            painter.setPen(QPen(style.foreground, style.borderWidth + 0.5));
            painter.drawLine(box.topRight() + QPoint(-4, 4),
                             box.bottomLeft() + QPoint(4, -4));
        } else if (on && !custom) {
            // The tick goes *on* the colour, so it has to be the ink that reads
            // on a filled plate rather than the page's own.
            paintGlyph(&painter, Glyph::Check, markBox, style.checkedForeground);
        }
    }
}

} // namespace novu::ui
