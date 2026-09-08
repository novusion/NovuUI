#pragma once

#include <QColor>
#include <QString>

namespace novu::ui {

// The Novu application palette.
//
// One palette, every application. Not "the defaults a profile starts from" -
// the actual colours NovuAI, NovuSpace, NovuVoice, NovuExplorer and NovuHub all
// wear. An application does not get a look of its own; it gets the Novu look.
//
// NovuTrade is the single exception, and it is a deliberate one: its slate/
// emerald character is the product, not a drift. Everything else that had its
// own palette had it by accident - NovuExplorer's was already byte-for-byte
// this one, NovuAI's differed only in text and accent, NovuVoice's had drifted
// to cyan, NovuHub's was the same values uncollected in a stylesheet string.
//
// The values are NovuAI's, adopted as the reference. Widgets must read colours
// from here - the design system forbids hardcoded hex inside a widget so a
// theme switch stays possible.
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

    QColor textPrimary{QStringLiteral("#D4D9E0")};
    QColor textSecondary{QStringLiteral("#8B929A")};
    QColor textDisabled{QStringLiteral("#545B64")};
    QColor textOnAccent{QStringLiteral("#FFFFFF")};

    // The application accent - the one colour that is *about* the product
    // rather than about a meaning, and it is shared anyway: Novu is one product
    // family, not five. A softer teal than the shell's old #287873, which is
    // the visible half of this unification.
    QColor accent{QStringLiteral("#3D8B8B")};
    QColor accentLight{QStringLiteral("#4A9E9E")};

    // Status hues. Defaults are the shared semantic palette - see
    // sharedSemanticHues() below - so a profile that says nothing about status
    // is already consistent with every other product. A profile overrides one
    // of these only when its *surface* forces it to, never to have a look of
    // its own.
    QColor success{QStringLiteral("#2EA043")};
    QColor warning{QStringLiteral("#D29922")};
    QColor danger{QStringLiteral("#F85149")};
    // Info used to have no colour at all: every component that needed one
    // reached for accentLight, which is the *application* accent. An Info badge
    // was therefore sky blue in NovuTrade, gold in Warm Gray and violet in Dark
    // Purple - the same status wearing three identities.
    QColor info{QStringLiteral("#58A6FF")};
};

// The status hues every Novusion application shares.
//
// Application accent and semantic status are two different layers. A product
// owns its background, its surfaces, its accent and its selection styling;
// it does not own what "failed" looks like. Success, Warning, Danger and Info
// mean the same thing in every Novu window, so they look the same in every
// Novu window.
//
// The values are NovuAI's, adopted as the reference: decent rather than
// saturated, but still clean enough that none of them reads as disabled. The
// palette they replaced had drifted into four versions of the same four
// meanings - an olive #98C379 in the shell, a mint #3CCB7F in NovuVoice, a
// darker #16A34A in NovuTrade.
struct SemanticHues
{
    QColor success{QStringLiteral("#2EA043")};
    QColor warning{QStringLiteral("#D29922")};
    QColor danger{QStringLiteral("#F85149")};
    QColor info{QStringLiteral("#58A6FF")};
};

const SemanticHues& sharedSemanticHues();

// What a component actually needs from a status colour.
//
// Five values, and no more than five: every one of them is used by something
// shipping. Before this existed, Badge derived "the hue at 0.16", Button
// derived "the hue at 0.14 with a 0.40 border" and the scope sheet derived "the
// hue at 0.16" again - three recipes for one idea, which is how a Danger button
// and a Danger badge stopped matching.
struct SemanticColor
{
    QColor text;      // the word itself, on a plain surface
    QColor fill;      // the tinted plate behind it
    QColor border;
    QColor hover;
    QColor pressed;
};

enum class SemanticRole { Neutral, Success, Warning, Danger, Info };

struct SemanticPalette
{
    // Neutral is not shared and cannot be: it is the absence of a status, so it
    // is drawn in the profile's own secondary ink. One fixed grey would be
    // invisible on the light profile and wrong on every dark one.
    SemanticColor neutral;
    SemanticColor success;
    SemanticColor warning;
    SemanticColor danger;
    SemanticColor info;

    const SemanticColor& role(SemanticRole role) const
    {
        switch (role) {
        case SemanticRole::Success: return success;
        case SemanticRole::Warning: return warning;
        case SemanticRole::Danger:  return danger;
        case SemanticRole::Info:    return info;
        case SemanticRole::Neutral: break;
        }
        return neutral;
    }
};

// The five values, built from one hue. The single recipe that replaced three.
SemanticColor semanticColorFrom(const QColor& hue);

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

// The same tint as a colour rather than as a string, for the components that
// hand it to a painter instead of to a style sheet. Without it every caller
// writes the same three-line copy-and-set-alpha dance.
QColor withAlpha(const QColor& color, double alpha);

} // namespace novu::ui
