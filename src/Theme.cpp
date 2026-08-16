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

} // namespace novu::ui
