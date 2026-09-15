#include "novu/ui/Segmented.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QPainter>

namespace novu::ui {

namespace {

// The design's `.chipset`: 3px of air inside the tray, 2px between segments,
// and 4px/11px inside a segment.
constexpr int kTrayPadding = 3;
constexpr int kSegmentGap = 2;
constexpr int kSegmentPaddingH = 11;
constexpr int kSegmentPaddingV = 4;

} // namespace

Segmented::Segmented(QWidget* parent)
    : ItemStrip(parent)
{
    // The cursor, the focus policy and mouse tracking are ItemStrip's - every
    // strip needs the same three, and a subclass that forgot one was a strip
    // whose hover silently stopped working.
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

Segmented::Segmented(const QStringList& labels, QWidget* parent, int current)
    : Segmented(parent)
{
    for (const QString& text : labels) {
        addSegment(text);
    }
    setCurrentIndex(current);
}

int Segmented::addSegment(const QString& text)
{
    Segment segment;
    segment.text = text;
    m_segments.append(segment);
    if (currentIndex() < 0) {
        setCurrentIndex(0);
    }
    refresh();
    return m_segments.size() - 1;
}

int Segmented::addSegment(Glyph glyph, const QString& tooltip)
{
    Segment segment;
    segment.glyph = glyph;
    segment.tooltip = tooltip;
    m_segments.append(segment);
    if (currentIndex() < 0) {
        setCurrentIndex(0);
    }
    refresh();
    return m_segments.size() - 1;
}

void Segmented::clear()
{
    m_segments.clear();
    resetStripState();
    refresh();
}

QString Segmented::segmentText(int index) const
{
    return index >= 0 && index < m_segments.size() ? m_segments.at(index).text : QString();
}

void Segmented::setSegmentAttention(int index, bool attention)
{
    if (index < 0 || index >= m_segments.size() || m_segments[index].attention == attention) {
        return;
    }
    m_segments[index].attention = attention;
    update();
}

// ItemStrip has decided an item was chosen; this is where the picker says so.
void Segmented::stripItemChosen(int index)
{
    emit currentIndexChanged(index);
}

QString Segmented::stripItemTooltip(int index) const
{
    return index >= 0 && index < m_segments.size() ? m_segments.at(index).tooltip
                                                   : QString();
}

int Segmented::stripLead() const
{
    return kTrayPadding;
}

int Segmented::stripGap() const
{
    return kSegmentGap;
}

int Segmented::stripTop() const
{
    return kTrayPadding;
}

int Segmented::stripItemHeight() const
{
    return height() - 2 * kTrayPadding;
}

int Segmented::stripItemWidth(int index) const
{
    if (index < 0 || index >= m_segments.size()) {
        return 0;
    }
    return segmentWidth(m_segments.at(index));
}

void Segmented::setControlSize(ControlSize size)
{
    if (m_size == size) {
        return;
    }
    m_size = size;
    refresh();
}

void Segmented::refresh()
{
    updateGeometry();
    update();
}

StyleSpec Segmented::styleSpec() const
{
    // The tray: an inset well, the same one an input sits in, so a picker and a
    // field in the same toolbar read as the same depth.
    StyleSpec spec;
    spec.radius = SurfaceRole::Input;
    spec.borderWidth = BorderRole::Input;
    spec.type = m_size == ControlSize::Small ? TypeRole::Eyebrow : TypeRole::Caption;
    spec.background = token(ColorRole::PanelBg);
    spec.border = token(ColorRole::Border);
    spec.foreground = token(ColorRole::TextDisabled);
    spec.disabledForeground = token(ColorRole::TextDisabled);
    spec.disabledBorder = token(ColorRole::BorderSubtle);
    return spec;
}

StyleSpec Segmented::segmentStyleSpec(int index) const
{
    // One segment. Off is a bare word on the tray; on is a plate lifted out of
    // it. The lift is the whole signal, so the plate is the *raised* surface
    // rather than a tint - a tinted segment reads as a status, and this is a
    // choice.
    StyleSpec spec;
    spec.radius = SurfaceRole::Button;
    spec.radiusStepOverride = static_cast<int>(RadiusStep::Small);
    spec.borderWidth = BorderRole::Input;
    spec.type = m_size == ControlSize::Small ? TypeRole::Eyebrow : TypeRole::Caption;

    spec.background = token(ColorRole::Transparent);
    spec.border = token(ColorRole::Transparent);
    spec.foreground = token(ColorRole::TextDisabled);
    spec.hoverForeground = token(ColorRole::TextSecondary);

    spec.checkedBackground = token(ColorRole::PanelBgAlt);
    spec.checkedForeground = token(ColorRole::TextPrimary);
    spec.checkedBorder = token(ColorRole::Transparent);

    // The one the design marks: current or not, it says the choice has a cost.
    if (index >= 0 && index < m_segments.size() && m_segments.at(index).attention) {
        spec.foreground = statusToken(SemanticRole::Warning, ColorRole::StatusText);
        spec.checkedForeground = statusToken(SemanticRole::Warning, ColorRole::StatusText);
        spec.hoverForeground = statusToken(SemanticRole::Warning, ColorRole::StatusText);
    }
    return spec;
}

ResolvedStyle Segmented::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

int Segmented::segmentWidth(const Segment& segment) const
{
    if (segment.glyph != Glyph::None) {
        return glyphSizeFor(this, m_size) + 2 * kSegmentPaddingH;
    }
    const QFontMetrics fm(resolve(segmentStyleSpec(-1), this).font);
    return fm.horizontalAdvance(segment.text) + 2 * kSegmentPaddingH;
}

QSize Segmented::sizeHint() const
{
    const QFontMetrics fm(resolve(segmentStyleSpec(-1), this).font);
    const int height = qMax(themeFor(this).controlHeight(m_size),
                            fm.height() + 2 * kSegmentPaddingV + 2 * kTrayPadding);
    // stripWidth() is the lead, the gaps and every item, which is exactly what
    // this used to add up by hand.
    return QSize(stripWidth(), height);
}

bool Segmented::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::ParentChange) {
        refresh();
    }
    return QWidget::event(event);
}

void Segmented::paintEvent(QPaintEvent*)
{
    const ResolvedStyle tray = resolvedStyle();
    const ThemeProfile& profile = themeFor(this);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const double border = tray.borderWidth;
    const QRectF well = QRectF(rect()).adjusted(border / 2.0, border / 2.0,
                                                -border / 2.0, -border / 2.0);
    painter.setPen(QPen(isEnabled() ? tray.border : tray.disabledBorder, border));
    painter.setBrush(tray.background);
    painter.drawRoundedRect(well, tray.radius, tray.radius);

    if (hasFocus()) {
        QColor ring = profile.colors.accent;
        ring.setAlphaF(0.75);
        painter.setPen(QPen(ring, profile.border(BorderRole::Focus)));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1),
                                tray.radius, tray.radius);
    }

    for (int i = 0; i < m_segments.size(); ++i) {
        const Segment& segment = m_segments.at(i);
        const ResolvedStyle style = resolve(segmentStyleSpec(i), this, height());
        const QRect box = segmentRect(i);
        const bool on = i == currentIndex();

        QColor ink = on ? style.checkedForeground : style.foreground;
        if (!on && i == hoveredIndex() && isEnabled()) {
            ink = style.hoverForeground;
        }
        if (!isEnabled()) {
            ink.setAlphaF(ink.alphaF() * 0.42);
        }

        if (on) {
            QColor plate = style.checkedBackground;
            if (!isEnabled()) {
                plate.setAlphaF(plate.alphaF() * 0.42);
            }
            painter.setPen(Qt::NoPen);
            painter.setBrush(plate);
            painter.drawRoundedRect(QRectF(box), style.radius, style.radius);
        } else if (i == pressedIndex()) {
            // The press is visible before the release decides anything, so a
            // gesture being cancelled is something the user can see happening.
            QColor plate = style.checkedBackground;
            plate.setAlphaF(plate.alphaF() * 0.5);
            painter.setPen(Qt::NoPen);
            painter.setBrush(plate);
            painter.drawRoundedRect(QRectF(box), style.radius, style.radius);
        }

        if (segment.glyph != Glyph::None) {
            const int size = glyphSizeFor(this, m_size);
            const QRectF glyphBox(box.center().x() - size / 2.0 + 0.5,
                                  box.center().y() - size / 2.0 + 0.5, size, size);
            paintGlyph(&painter, segment.glyph, glyphBox, ink);
        } else {
            painter.setFont(style.font);
            painter.setPen(ink);
            painter.drawText(box, Qt::AlignCenter, segment.text);
        }
    }
}

} // namespace novu::ui
