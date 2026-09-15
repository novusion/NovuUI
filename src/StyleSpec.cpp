#include "novu/ui/StyleSpec.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QWidget>

namespace novu::ui {

namespace {

const SemanticColor& statusColor(const ThemeProfile& profile, SemanticRole status)
{
    return profile.semantic.role(status);
}

} // namespace

QString colorRoleName(ColorRole role)
{
    switch (role) {
    case ColorRole::Transparent:   return QStringLiteral("transparent");
    case ColorRole::WindowBg:      return QStringLiteral("windowBg");
    case ColorRole::PanelBg:       return QStringLiteral("panelBg");
    case ColorRole::PanelBgAlt:    return QStringLiteral("panelBgAlt");
    case ColorRole::HeaderBg:      return QStringLiteral("headerBg");
    case ColorRole::SurfaceHover:  return QStringLiteral("surfaceHover");
    case ColorRole::SurfaceActive: return QStringLiteral("surfaceActive");
    case ColorRole::Border:        return QStringLiteral("border");
    case ColorRole::BorderSubtle:  return QStringLiteral("borderSubtle");
    case ColorRole::BorderHover:   return QStringLiteral("borderHover");
    case ColorRole::TextPrimary:   return QStringLiteral("textPrimary");
    case ColorRole::TextSecondary: return QStringLiteral("textSecondary");
    case ColorRole::TextDisabled:  return QStringLiteral("textDisabled");
    case ColorRole::TextOnAccent:  return QStringLiteral("textOnAccent");
    case ColorRole::Accent:        return QStringLiteral("accent");
    case ColorRole::AccentLight:   return QStringLiteral("accentLight");
    case ColorRole::StatusText:    return QStringLiteral("text");
    case ColorRole::StatusFill:    return QStringLiteral("fill");
    case ColorRole::StatusBorder:  return QStringLiteral("border");
    case ColorRole::StatusHover:   return QStringLiteral("hover");
    case ColorRole::StatusPressed: return QStringLiteral("pressed");
    }
    return QStringLiteral("transparent");
}

QString semanticRoleName(SemanticRole role)
{
    switch (role) {
    case SemanticRole::Success: return QStringLiteral("success");
    case SemanticRole::Warning: return QStringLiteral("warning");
    case SemanticRole::Danger:  return QStringLiteral("danger");
    case SemanticRole::Info:    return QStringLiteral("info");
    case SemanticRole::Neutral: break;
    }
    return QStringLiteral("neutral");
}

QColor ColorToken::resolve(const ThemeProfile& profile) const
{
    const ThemeColors& c = profile.colors;

    QColor base;
    switch (role) {
    case ColorRole::Transparent:   return QColor(Qt::transparent);
    case ColorRole::WindowBg:      base = c.windowBg; break;
    case ColorRole::PanelBg:       base = c.panelBg; break;
    case ColorRole::PanelBgAlt:    base = c.panelBgAlt; break;
    case ColorRole::HeaderBg:      base = c.headerBg; break;
    case ColorRole::SurfaceHover:  base = c.surfaceHover; break;
    case ColorRole::SurfaceActive: base = c.surfaceActive; break;
    case ColorRole::Border:        base = c.border; break;
    case ColorRole::BorderSubtle:  base = c.borderSubtle; break;
    case ColorRole::BorderHover:   base = c.borderHover; break;
    case ColorRole::TextPrimary:   base = c.textPrimary; break;
    case ColorRole::TextSecondary: base = c.textSecondary; break;
    case ColorRole::TextDisabled:  base = c.textDisabled; break;
    case ColorRole::TextOnAccent:  base = c.textOnAccent; break;
    case ColorRole::Accent:        base = c.accent; break;
    case ColorRole::AccentLight:   base = c.accentLight; break;
    case ColorRole::StatusText:    base = statusColor(profile, status).text; break;
    case ColorRole::StatusFill:    base = statusColor(profile, status).fill; break;
    case ColorRole::StatusBorder:  base = statusColor(profile, status).border; break;
    case ColorRole::StatusHover:   base = statusColor(profile, status).hover; break;
    case ColorRole::StatusPressed: base = statusColor(profile, status).pressed; break;
    }

    if (alpha >= 1.0) {
        return base;
    }
    // Multiplies rather than replaces: the semantic plate already carries an
    // alpha of its own, and a token asking for half of it means half of what it
    // is, not a flat 0.5.
    return withAlpha(base, base.alphaF() * alpha);
}

QString ColorToken::describe() const
{
    const bool isStatus = role == ColorRole::StatusText || role == ColorRole::StatusFill
        || role == ColorRole::StatusBorder || role == ColorRole::StatusHover
        || role == ColorRole::StatusPressed;

    QString name = isStatus
        ? QStringLiteral("status.%1.%2").arg(semanticRoleName(status), colorRoleName(role))
        : colorRoleName(role);

    if (alpha < 1.0) {
        name += QStringLiteral("@%1").arg(qRound(alpha * 100));
    }
    return name;
}

ColorToken token(ColorRole role, double alpha)
{
    ColorToken t;
    t.role = role;
    t.alpha = alpha;
    return t;
}

ColorToken statusToken(SemanticRole status, ColorRole role, double alpha)
{
    ColorToken t;
    t.role = role;
    t.status = status;
    t.alpha = alpha;
    return t;
}

ResolvedStyle resolve(const StyleSpec& spec, const QWidget* context, int height)
{
    const ThemeProfile& profile = themeFor(context);

    ResolvedStyle out;
    out.background = spec.background.resolve(profile);
    out.foreground = spec.foreground.resolve(profile);
    out.border = spec.border.resolve(profile);
    out.hoverBackground = spec.hoverBackground.resolve(profile);
    out.hoverForeground = spec.hoverForeground.resolve(profile);
    out.hoverBorder = spec.hoverBorder.resolve(profile);
    out.checkedBackground = spec.checkedBackground.resolve(profile);
    out.checkedForeground = spec.checkedForeground.resolve(profile);
    out.checkedBorder = spec.checkedBorder.resolve(profile);
    out.disabledForeground = spec.disabledForeground.resolve(profile);
    out.disabledBorder = spec.disabledBorder.resolve(profile);

    out.borderWidth = profile.border(spec.borderWidth);
    const int radius = spec.radiusStepOverride >= 0
        ? profile.radii.value(static_cast<RadiusStep>(spec.radiusStepOverride))
        : profile.radius(spec.radius);
    out.radius = resolveRadius(radius, height);

    out.font = roleFont(context, spec.type);
    if (spec.weight > 0) {
        out.font.setWeight(static_cast<QFont::Weight>(spec.weight));
    }
    return out;
}

} // namespace novu::ui
