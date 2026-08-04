#pragma once

#include <QColor>
#include <QString>

namespace novu::ui {

// Design tokens from docs/NOVU_DESIGN_SYSTEM.md (NovuSpace Dark, the
// canonical theme). Widgets must read colours from here — the design system
// forbids hardcoded hex inside widgets so a theme switch stays possible.
struct ThemeColors
{
    QColor windowBg{QStringLiteral("#161A1F")};
    QColor panelBg{QStringLiteral("#1D2228")};
    QColor panelBgAlt{QStringLiteral("#21272E")};
    QColor headerBg{QStringLiteral("#1A1F25")};
    QColor surfaceHover{QStringLiteral("#2A3038")};
    QColor surfaceActive{QStringLiteral("#2F353F")};

    QColor border{QStringLiteral("#232830")};
    QColor borderSubtle{QStringLiteral("#1F242A")};
    QColor borderHover{QStringLiteral("#2F343A")};

    QColor textPrimary{QStringLiteral("#E3E8F0")};
    QColor textSecondary{QStringLiteral("#A8B2C3")};
    QColor textDisabled{QStringLiteral("#6E7A8E")};
    QColor textOnAccent{QStringLiteral("#FFFFFF")};

    QColor accent{QStringLiteral("#287873")};
    QColor accentLight{QStringLiteral("#3DA9FC")};

    QColor success{QStringLiteral("#98C379")};
    QColor warning{QStringLiteral("#C9A26D")};
    QColor danger{QStringLiteral("#FF5555")};
};

// Spacing / metric tokens. Components consume these; pages never do arithmetic
// with them to size a widget.
namespace metrics {
inline constexpr int paddingSmall = 4;
inline constexpr int paddingMedium = 8;
inline constexpr int paddingLarge = 12;
inline constexpr int paddingXLarge = 16;

inline constexpr int spacingSmall = 6;
inline constexpr int spacingMedium = 10;
inline constexpr int spacingLarge = 16;

// Flat design: the system specifies 0px radius on most elements. Pills are
// the documented exception.
inline constexpr int radiusFlat = 0;
inline constexpr int radiusPill = 10;

inline constexpr int fontSizeSmall = 11;
inline constexpr int fontSizeNormal = 12;
inline constexpr int fontSizeMedium = 13;
} // namespace metrics

// The single theme instance every component reads from.
const ThemeColors& theme();
void setTheme(const ThemeColors& colors);

QString rgba(const QColor& color, double alpha);

} // namespace novu::ui
