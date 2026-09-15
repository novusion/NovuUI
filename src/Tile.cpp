#include "novu/ui/Tile.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QEnterEvent>
#include <QEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QTextOption>

namespace novu::ui {

namespace {

// `.rcard .rt { gap:7px }` and its 15px mark.
constexpr int kGlyphSide = 15;
constexpr int kGlyphGap = 7;

// `.rcard .rd { margin-top:4px; line-height:1.45 }`.
constexpr int kDescriptionGap = 4;
constexpr double kDescriptionLeading = 1.45;

constexpr int kBadgeWidth = 60;

// The subtitle sits one step below Caption on the profile's type scale. No
// TypeRole resolves to it, so the step is named here — once, because
// sizeHint() and paintEvent() must measure and draw with the same font or the
// tile clips its own second line.
QFont subtitleFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Caption);
    font.setPixelSize(profile.type.xs);
    return font;
}

int wrappedHeight(const QFontMetrics& fm, const QString& text, int width, double leading)
{
    if (text.isEmpty() || width <= 0) {
        return 0;
    }
    const QRect bounds = fm.boundingRect(QRect(0, 0, width, 10000),
                                         Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                                         text);
    const int lines = qMax(1, qRound(double(bounds.height()) / qMax(1, fm.height())));
    return qRound(fm.height() * leading) * lines;
}

} // namespace

Tile::Tile(const QString& title, const QString& subtitle, QWidget* parent,
           bool checked, const QString& badgeText)
    : QPushButton(parent)
    , m_title(title)
    , m_subtitle(subtitle)
    , m_badge(badgeText)
{
    setCheckable(true);
    setChecked(checked);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    // A badged tile is one that cannot be chosen yet; the badge says why.
    setEnabled(m_badge.isEmpty());
}

void Tile::setTitle(const QString& title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    updateGeometry();
    update();
}

void Tile::setSubtitle(const QString& subtitle)
{
    if (m_subtitle == subtitle) {
        return;
    }
    m_subtitle = subtitle;
    updateGeometry();
    update();
}

void Tile::setBadge(const QString& badgeText)
{
    m_badge = badgeText;
    setEnabled(m_badge.isEmpty());
    updateGeometry();
    update();
}

void Tile::setGlyph(Glyph glyph)
{
    if (m_glyph == glyph) {
        return;
    }
    m_glyph = glyph;
    updateGeometry();
    update();
}

void Tile::setTone(Tone tone)
{
    if (m_tone == tone) {
        return;
    }
    m_tone = tone;
    update();
}

void Tile::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    update();
    QPushButton::enterEvent(event);
}

void Tile::leaveEvent(QEvent* event)
{
    m_hovered = false;
    update();
    QPushButton::leaveEvent(event);
}

bool Tile::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QPushButton::event(event);
}

StyleSpec Tile::styleSpec() const
{
    StyleSpec spec;
    spec.radius = SurfaceRole::Button;
    spec.borderWidth = BorderRole::Card;
    spec.type = TypeRole::CardTitle;

    // `.rcard { background:var(--bg) }` - the window colour, darker than the
    // card it sits in, so an unchosen option reads as a well rather than as a
    // raised plate competing with the chosen one.
    spec.background = token(ColorRole::WindowBg);
    spec.border = token(ColorRole::Border);
    spec.foreground = token(ColorRole::TextSecondary);

    // `.rcard:hover { border-color:var(--text3) }` - the outline only. A fill
    // change on hover reads as a selection, which is the state next door.
    spec.hoverBorder = token(ColorRole::TextDisabled);
    spec.hoverForeground = token(ColorRole::TextPrimary);

    if (m_tone == Tone::Risky) {
        spec.checkedBackground = statusToken(SemanticRole::Warning, ColorRole::StatusFill);
        spec.checkedBorder = statusToken(SemanticRole::Warning, ColorRole::StatusBorder);
        spec.checkedForeground = statusToken(SemanticRole::Warning, ColorRole::StatusText);
    } else {
        spec.checkedBackground = token(ColorRole::Accent, 0.11);
        spec.checkedBorder = token(ColorRole::Accent, 0.65);
        spec.checkedForeground = token(ColorRole::TextPrimary);
    }

    spec.disabledForeground = token(ColorRole::TextDisabled);
    spec.disabledBorder = token(ColorRole::BorderSubtle);
    return spec;
}

ResolvedStyle Tile::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

int Tile::descriptionHeight(int width) const
{
    const ThemeProfile& profile = themeFor(this);
    const int padding = profile.space(SpacingRole::FormRowGap);
    const int badgeWidth = m_badge.isEmpty() ? 0 : kBadgeWidth;
    return wrappedHeight(QFontMetrics(subtitleFont(this)), m_subtitle,
                         qMax(0, width - 2 * padding - badgeWidth), kDescriptionLeading);
}

int Tile::heightForWidth(int width) const
{
    const ThemeProfile& profile = themeFor(this);
    const int padding = profile.space(SpacingRole::FormRowGap);
    const QFontMetrics titleMetrics(roleFont(this, TypeRole::CardTitle, true));

    int height = titleMetrics.height();
    if (!m_subtitle.isEmpty()) {
        height += kDescriptionGap + descriptionHeight(width);
    }
    return height + 2 * padding + 2;
}

QSize Tile::sizeHint() const
{
    // Measured, not fixed: two text lines plus the profile's own padding, so a
    // longer exchange name or a larger type scale still fits.
    const ThemeProfile& profile = themeFor(this);
    const int padding = profile.space(SpacingRole::FormRowGap);

    const QFontMetrics titleMetrics(roleFont(this, TypeRole::CardTitle, true));
    const QFontMetrics subtitleMetrics(subtitleFont(this));

    int titleWidth = titleMetrics.horizontalAdvance(m_title);
    if (m_glyph != Glyph::None) {
        titleWidth += kGlyphSide + kGlyphGap;
    }
    int width = qMax(titleWidth, subtitleMetrics.horizontalAdvance(m_subtitle));
    if (!m_badge.isEmpty()) {
        width += QFontMetrics(roleFont(this, TypeRole::Caption, true))
                     .horizontalAdvance(m_badge) + padding * 3;
    }
    width += padding * 2;
    return QSize(width, heightForWidth(width));
}

QSize Tile::minimumSizeHint() const
{
    const int width = 160;
    return QSize(width, heightForWidth(width));
}

void Tile::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const ThemeColors& c = profile.colors;
    const ResolvedStyle style = resolvedStyle();
    const int padding = profile.space(SpacingRole::FormRowGap);
    const int radius = resolveRadius(profile.radius(SurfaceRole::Button), height());
    const bool on = isChecked();
    const bool unavailable = !m_badge.isEmpty();

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QColor fill = on ? style.checkedBackground : style.background;
    QColor outline = on ? style.checkedBorder
                        : (m_hovered && isEnabled() ? style.hoverBorder : style.border);
    if (unavailable) {
        outline = style.disabledBorder;
    }

    p.setBrush(fill);
    p.setPen(QPen(outline, style.borderWidth));
    p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);

    const QFont titleFont = roleFont(this, TypeRole::CardTitle, true);
    const QFontMetrics titleMetrics(titleFont);
    const int badgeWidth = unavailable ? kBadgeWidth : 0;
    const int textWidth = width() - padding * 2 - badgeWidth;

    QColor titleInk = on ? style.checkedForeground
                         : (m_hovered && isEnabled() ? style.hoverForeground
                                                     : c.textPrimary);
    if (unavailable) {
        titleInk = style.disabledForeground;
    }

    int left = padding;
    const int titleTop = padding - 2;
    if (m_glyph != Glyph::None) {
        paintGlyph(&p, m_glyph,
                   QRectF(left, titleTop + (titleMetrics.height() - kGlyphSide) / 2.0,
                          kGlyphSide, kGlyphSide),
                   titleInk);
        left += kGlyphSide + kGlyphGap;
    }

    p.setPen(titleInk);
    p.setFont(titleFont);
    const int titleWidth = qMax(0, textWidth - (left - padding));
    p.drawText(QRect(left, titleTop, titleWidth, titleMetrics.height()),
               Qt::AlignLeft | Qt::AlignVCenter,
               titleMetrics.elidedText(m_title, Qt::ElideRight, titleWidth));

    if (!m_subtitle.isEmpty()) {
        QTextOption wrap(Qt::AlignLeft | Qt::AlignTop);
        wrap.setWrapMode(QTextOption::WordWrap);
        p.setPen(c.textDisabled);
        p.setFont(subtitleFont(this));
        p.drawText(QRectF(padding, titleTop + titleMetrics.height() + kDescriptionGap,
                          textWidth, descriptionHeight(width())),
                   m_subtitle, wrap);
    }

    if (unavailable) {
        const QRect badgeRect(width() - kBadgeWidth - padding + 8, padding - 2, 48, 20);
        QColor tint = c.warning;
        tint.setAlphaF(0.15f);
        p.setBrush(tint);
        p.setPen(Qt::NoPen);
        // Through resolveRadius(), never raw. The Badge role is the pill
        // marker — a deliberately huge number meaning "half of this box's
        // height". Handed straight to drawRoundedRect() Qt does not clamp it
        // and draws a degenerate shape, so the "Soon" badge came out as a
        // tapered wedge instead of a pill. StyleSheet.h documents exactly
        // this trap; this call site was not going through it.
        const int badgeRadius = resolveRadius(profile.radius(SurfaceRole::Badge),
                                              badgeRect.height());
        p.drawRoundedRect(badgeRect, badgeRadius, badgeRadius);
        p.setPen(c.warning);
        p.setFont(roleFont(this, TypeRole::Caption, true));
        p.drawText(badgeRect, Qt::AlignCenter, m_badge);
    }
}

} // namespace novu::ui
