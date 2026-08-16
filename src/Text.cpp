#include "novu/ui/Text.h"

#include "novu/ui/ThemeManager.h"

#include <QLabel>
#include <QWidget>

namespace novu::ui {

QColor inkColor(const QWidget* context, Ink ink)
{
    const ThemeColors& c = themeFor(context).colors;
    switch (ink) {
    case Ink::Secondary: return c.textSecondary;
    case Ink::Muted:     return c.textDisabled;
    case Ink::Accent:    return c.accent;
    case Ink::Success:   return c.success;
    case Ink::Warning:   return c.warning;
    case Ink::Danger:    return c.danger;
    case Ink::Info:      return c.accentLight;
    case Ink::Default:
    default:             return c.textPrimary;
    }
}

QFont roleFont(const QWidget* context, TypeRole role, bool bold)
{
    const ThemeProfile& profile = themeFor(context);
    const TypeStyle style = profile.text(role);

    QFont f(profile.type.family);
    f.setPixelSize(style.size);
    f.setWeight(bold ? QFont::Bold : static_cast<QFont::Weight>(style.weight));
    return f;
}

QFont roleMonoFont(const QWidget* context, TypeRole role, bool bold)
{
    const ThemeProfile& profile = themeFor(context);
    QFont f = roleFont(context, role, bold);
    f.setFamily(profile.type.monoFamily);
    return f;
}

QFont iconFont(int pixelSize)
{
    QFont f(QStringLiteral("Segoe Fluent Icons"));
    f.setFamilies({QStringLiteral("Segoe Fluent Icons"),
                   QStringLiteral("Segoe MDL2 Assets")});
    f.setPixelSize(pixelSize);
    return f;
}

QLabel* label(const QString& text, QWidget* parent, TypeRole role, Ink ink, bool bold)
{
    auto* lb = new QLabel(text, parent);
    lb->setFont(roleFont(parent, role, bold));
    setInk(lb, inkColor(parent, ink));
    return lb;
}

QLabel* caption(const QString& text, QWidget* parent)
{
    return label(text.toUpper(), parent, TypeRole::Caption, Ink::Muted, true);
}

void applyText(QLabel* lb, TypeRole role, Ink ink, bool bold)
{
    if (!lb) {
        return;
    }
    lb->setFont(roleFont(lb, role, bold));
    setInk(lb, ink);
}

void setInk(QLabel* lb, Ink ink)
{
    if (!lb) {
        return;
    }
    setInk(lb, inkColor(lb, ink));
}

void setInk(QLabel* lb, const QColor& color)
{
    if (!lb) {
        return;
    }
    QPalette pal = lb->palette();
    pal.setColor(QPalette::WindowText, color);
    lb->setPalette(pal);
}

} // namespace novu::ui
