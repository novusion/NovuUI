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

constexpr int kSegmentGap = 2;
constexpr int kSegmentPaddingH = 11;
constexpr int kSegmentPaddingV = 4;
constexpr int kCountGap = 5;

QPair<QString, QString> splitCount(const QString& text)
{
    const int separator = text.lastIndexOf(QStringLiteral("  "));
    if (separator < 0) return {text, QString()};
    const QString count = text.mid(separator).trimmed();
    bool numeric = !count.isEmpty();
    for (const QChar ch : count) numeric = numeric && ch.isDigit();
    return numeric ? QPair<QString, QString>{text.left(separator).trimmed(), count}
                   : QPair<QString, QString>{text, QString()};
}

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
    return themeFor(this).controls.segmentedPadding;
}

int Segmented::stripGap() const
{
    return kSegmentGap;
}

int Segmented::stripTop() const
{
    return themeFor(this).controls.segmentedPadding;
}

int Segmented::stripItemHeight() const
{
    return height() - 2 * themeFor(this).controls.segmentedPadding;
}

int Segmented::stripItemWidth(int index) const
{
    if (index < 0 || index >= m_segments.size()) {
        return 0;
    }
    if (m_fill) {
        // Fill distributes the SURPLUS, never the content.
        //
        // An even division of the tray is right only while every segment is
        // wider than its own text. Four segments in a 340px rail - Work,
        // Project, Code, Assets - are not: an even share handed each of them
        // less than its label needed, and because a segment's text is painted
        // rather than laid out, the labels ran into each other instead of
        // being clipped. A control that reads "ProjeCodAssets" is worse than
        // one that is too wide.
        //
        // So: natural widths first. What is left over after all of them is
        // divided evenly, and when there is nothing left over each segment
        // keeps its own width - the strip is then wider than its tray, which
        // sizeHint() already reports so a parent can give it the room or
        // scroll it.
        const int count = m_segments.size();
        const int gaps = (count - 1) * stripGap();
        const int usable = qMax(0, width() - 2 * themeFor(this).controls.segmentedPadding - gaps);
        int natural = 0;
        for (const Segment& segment : m_segments) {
            natural += segmentWidth(segment);
        }
        if (natural >= usable) {
            return segmentWidth(m_segments.at(index));
        }
        const int surplus = usable - natural;
        return segmentWidth(m_segments.at(index)) + surplus / count
            + (index < surplus % count ? 1 : 0);
    }
    return segmentWidth(m_segments.at(index));
}

void Segmented::setFill(bool fill)
{
    if (m_variant == Variant::RailSwitcher && !fill) {
        return;
    }
    if (m_fill == fill) {
        return;
    }
    m_fill = fill;
    refresh();
}

void Segmented::setControlSize(ControlSize size)
{
    if (m_variant == Variant::RailSwitcher && size != ControlSize::Medium) {
        return;
    }
    if (m_size == size) {
        return;
    }
    m_size = size;
    refresh();
}

void Segmented::setVariant(Variant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    if (variant == Variant::RailSwitcher) {
        m_size = ControlSize::Medium;
        m_fill = true;
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    } else {
        m_fill = false;
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
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
    // A picker is a select, not a text field: the approved designs shape the
    // two apart (`.seg` and `.select` are both pills where `.omni` is a box),
    // and Select falls back to Input for any profile that has not said so.
    spec.radius = SurfaceRole::Select;
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
    // `.chip{border-radius:7px}` inside `.chipset{border-radius:10px}` - the
    // plate lifted out of the tray is drawn at its own, smaller radius, not
    // the tray's Select radius (that read as a near-pill, not a chip).
    spec.radius = SurfaceRole::Segment;
    spec.borderWidth = BorderRole::Input;
    // `.chip{font-size:11.5px;font-weight:500;line-height:1.3}` at every size:
    // the Caption role, which is that size, and nothing else. A small chipset
    // used to resolve to Eyebrow, which is the *mono, uppercase, tracked*
    // caption that names a group - so `Day Week Month Custom` came out as
    // `DAY WEEK MONTH CUSTOM` in a face the sheet never puts on a control.
    // The control size decides the padding and the height, not the type.
    spec.type = TypeRole::Caption;
    // The Caption role carries the body weight, and a chipset drawn at 400
    // reads lighter than every other control on the same toolbar.
    spec.weight = 500;

    spec.background = token(ColorRole::Transparent);
    spec.border = token(ColorRole::Transparent);
    spec.foreground = token(ColorRole::TextDisabled);
    spec.hoverForeground = token(ColorRole::TextSecondary);

    // `.seg button.on{background:var(--card-h)}` -- the active surface, which
    // is what this is: PanelBgAlt names a second panel, not a state, and a
    // segment that reads as a panel does not read as lifted.
    spec.checkedBackground = token(ColorRole::SurfaceActive);
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
    const auto parts = splitCount(segment.text);
    int content = fm.horizontalAdvance(parts.first);
    if (!parts.second.isEmpty()) {
        QFont countFont = roleMonoFont(this, TypeRole::Mono);
        countFont.setPixelSize(themeFor(this).type.xxs);
        content += kCountGap + QFontMetrics(countFont).horizontalAdvance(parts.second);
    }
    return content + 2 * kSegmentPaddingH;
}

QSize Segmented::sizeHint() const
{
    const QFontMetrics fm(resolve(segmentStyleSpec(-1), this).font);
    const int height = themeFor(this).controls.segmentedHeight>0?themeFor(this).controls.segmentedHeight:qMax(themeFor(this).controlHeight(m_size),
                            fm.height() + 2 * kSegmentPaddingV + 2 * themeFor(this).controls.segmentedPadding);
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

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const double border = tray.borderWidth;
    const QRectF well = QRectF(rect()).adjusted(border / 2.0, border / 2.0,
                                                -border / 2.0, -border / 2.0);
    painter.setPen(QPen(isEnabled() ? tray.border : tray.disabledBorder, border));
    painter.setBrush(tray.background);
    painter.drawRoundedRect(well, tray.radius, tray.radius);

    // No accent ring around the tray on focus: the current segment's own
    // raised surface already says which one is on, and a second, group-wide
    // outline on top of it was reading as its own, unrelated colour rather
    // than as a focus cue.
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
            const auto parts = splitCount(segment.text);
            painter.setFont(style.font);
            const int labelWidth = QFontMetrics(style.font).horizontalAdvance(parts.first);
            if (parts.second.isEmpty()) {
                painter.setPen(ink);
                painter.drawText(box, Qt::AlignCenter, parts.first);
            } else {
                QFont countFont = roleMonoFont(this, TypeRole::Mono);
                countFont.setPixelSize(themeFor(this).type.xxs);
                const int countWidth = QFontMetrics(countFont).horizontalAdvance(parts.second);
                const int totalWidth = labelWidth + kCountGap + countWidth;
                const int left = box.center().x() - totalWidth / 2;
                painter.setPen(ink);
                painter.drawText(QRect(left, box.top(), labelWidth, box.height()),
                                 Qt::AlignVCenter | Qt::AlignLeft, parts.first);
                painter.setFont(countFont);
                // A count belongs to the selected face just as much as its
                // label does. The frozen NovuAI task switcher uses the accent
                // for the active number ("To Do 3") and keeps the remaining
                // counts quiet, so the user can read both selection and scale
                // without turning every number into a badge.
                QColor countInk = on
                    ? themeFor(this).colors.accent
                    : (themeFor(this).colors.textTertiary.isValid()
                           ? themeFor(this).colors.textTertiary
                           : themeFor(this).colors.textDisabled);
                if (!isEnabled()) countInk.setAlphaF(countInk.alphaF() * 0.42);
                painter.setPen(countInk);
                painter.drawText(QRect(left + labelWidth + kCountGap, box.top(),
                                       countWidth, box.height()),
                                 Qt::AlignVCenter | Qt::AlignLeft, parts.second);
            }
        }
    }
}

} // namespace novu::ui
