#include "novu/ui/Theme.h"

namespace novu::ui {

namespace {
ThemeColors g_theme;
}

const ThemeColors& theme()
{
    return g_theme;
}

void setTheme(const ThemeColors& colors)
{
    g_theme = colors;
}

QString rgba(const QColor& color, double alpha)
{
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(alpha, 0, 'f', 3);
}

QColor withAlpha(const QColor& color, double alpha)
{
    QColor out = color;
    out.setAlphaF(static_cast<float>(alpha));
    return out;
}

const SemanticHues& sharedSemanticHues()
{
    static const SemanticHues hues;
    return hues;
}

SemanticColor semanticColorFrom(const QColor& hue)
{
    // The alphas are the ones already in use, unified rather than invented:
    // 0.16 is what Badge and the scope sheet tinted a plate with, and 0.40 is
    // the border Button drew around a Danger action. Hover and pressed sit
    // between them at even steps, so a tinted control answers the pointer the
    // way every other tinted control does.
    SemanticColor c;
    c.text = hue;
    c.fill = withAlpha(hue, 0.16);
    c.border = withAlpha(hue, 0.40);
    c.hover = withAlpha(hue, 0.24);
    c.pressed = withAlpha(hue, 0.32);
    return c;
}

} // namespace novu::ui
