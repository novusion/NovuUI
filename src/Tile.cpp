#include "novu/ui/Tile.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

namespace novu::ui {

namespace {

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
    // A badged tile is one that cannot be chosen yet; the badge says why.
    setEnabled(m_badge.isEmpty());
}

void Tile::setBadge(const QString& badgeText)
{
    m_badge = badgeText;
    setEnabled(m_badge.isEmpty());
    updateGeometry();
    update();
}

QSize Tile::sizeHint() const
{
    // Measured, not fixed: two text lines plus the profile's own padding, so a
    // longer exchange name or a larger type scale still fits.
    const ThemeProfile& profile = themeFor(this);
    const int padding = profile.space(SpacingRole::FormRowGap);

    const QFontMetrics titleMetrics(roleFont(this, TypeRole::CardTitle, true));
    const QFontMetrics subtitleMetrics(subtitleFont(this));

    int width = qMax(titleMetrics.horizontalAdvance(m_title),
                     subtitleMetrics.horizontalAdvance(m_subtitle));
    if (!m_badge.isEmpty()) {
        width += QFontMetrics(roleFont(this, TypeRole::Caption, true))
                     .horizontalAdvance(m_badge) + padding * 3;
    }
    const int height = titleMetrics.height() + subtitleMetrics.height() + padding * 2 + 4;
    return {width + padding * 2, height};
}

void Tile::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const ThemeColors& c = profile.colors;
    const int padding = profile.space(SpacingRole::FormRowGap);
    const int radius = resolveRadius(profile.radius(SurfaceRole::Button), height());
    const bool on = isChecked();
    const bool unavailable = !m_badge.isEmpty();

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QColor fill = c.panelBgAlt;
    if (on) {
        fill = c.accent;
        fill.setAlphaF(0.10f);
    }
    p.setBrush(fill);
    p.setPen(QPen(on ? c.accent : c.border, 1));
    p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);

    const QFontMetrics titleMetrics(roleFont(this, TypeRole::CardTitle, true));
    const int badgeWidth = unavailable ? 60 : 0;
    const int textWidth = width() - padding * 2 - badgeWidth;

    p.setPen(unavailable ? c.textDisabled : (on ? c.textPrimary : c.textSecondary));
    p.setFont(roleFont(this, TypeRole::CardTitle, true));
    p.drawText(QRect(padding, padding - 2, textWidth, titleMetrics.height()),
               Qt::AlignLeft | Qt::AlignVCenter, m_title);

    p.setPen(c.textDisabled);
    p.setFont(subtitleFont(this));
    p.drawText(QRect(padding, padding + titleMetrics.height() - 2, textWidth,
                     titleMetrics.height()),
               Qt::AlignLeft | Qt::AlignVCenter, m_subtitle);

    if (unavailable) {
        const QRect badgeRect(width() - badgeWidth - padding + 8, padding - 2, 48, 20);
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
