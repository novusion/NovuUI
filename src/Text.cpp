#include "novu/ui/Text.h"

#include "novu/ui/ThemeManager.h"

#include <QLabel>
#include <QWidget>

namespace novu::ui {

QColor inkColor(const QWidget* context, Ink ink)
{
    const ThemeProfile& profile = themeFor(context);
    const ThemeColors& c = profile.colors;
    const SemanticPalette& s = profile.semantic;
    switch (ink) {
    case Ink::Secondary: return c.textSecondary;
    case Ink::Muted:     return c.textDisabled;
    case Ink::Accent:    return c.accent;
    // Through the semantic palette, not the raw hues: status text and a status
    // badge are the same statement, so they resolve through the same table.
    // Info was accentLight here as well - the application accent standing in
    // for a status.
    case Ink::Success:   return s.success.text;
    case Ink::Warning:   return s.warning.text;
    case Ink::Danger:    return s.danger.text;
    case Ink::Info:      return s.info.text;
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

    // The palette alone is not enough. The scope sheet carries
    // `QLabel { color: ... }` so that a label with no ink of its own is not
    // left with the light system palette on a dark surface - and a stylesheet
    // colour outranks a palette one. Every ink set inside a themed subtree was
    // therefore being dropped: a loss printed in exactly the same colour as a
    // profit, and an inactive window title stayed at full strength.
    //
    // A widget's own sheet outranks its ancestors', so one declaration on the
    // label restores the role. It is a single property and no selector, so
    // nothing else about the label - background, border, tone rules - changes.
    const QString sheet = QStringLiteral("color: %1;")
        .arg(color.alpha() == 255 ? color.name() : rgba(color, color.alphaF()));
    // Guarded: setInk() is called again on every polish and every theme change,
    // and re-setting an identical sheet re-polishes the widget for nothing.
    if (lb->styleSheet() != sheet) {
        lb->setStyleSheet(sheet);
    }
}

} // namespace novu::ui
