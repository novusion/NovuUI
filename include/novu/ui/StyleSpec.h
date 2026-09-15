#pragma once

#include "novu/ui/Theme.h"
#include "novu/ui/ThemeProfile.h"

#include <QColor>
#include <QFont>
#include <QString>

class QWidget;

namespace novu::ui {

// What a component *means*, separated from what that looks like right now.
//
// Two problems, one answer.
//
// The first is correctness. A component that resolves the palette once and
// keeps the result is wrong the moment the profile changes, and it is wrong
// silently: the widget still paints, just in the old product's colours. Keeping
// the *role* and resolving on demand makes a theme switch a repaint rather than
// a rebuild, and makes "did this survive a theme change" a property of the
// design rather than of whether someone remembered an event handler.
//
// The second is testability. A self-painted component has no stylesheet to
// read back and no child widget to query, so the only way to check it is to
// photograph it — and a screenshot test tells you *that* something changed,
// never *what*. Splitting the declaration from the resolution gives two things
// a test can assert on directly:
//
//     spec.background.describe()  == "surfaceHover"          // the contract
//     resolve(spec, w).background == QColor("#29323D")        // this profile
//
// Neither needs a pixel.
//
// Components therefore hold a StyleSpec (or derive one from their variant) and
// never hold a QColor that came out of a profile.

// A colour in the profile, named rather than taken.
enum class ColorRole {
    Transparent,

    WindowBg,
    PanelBg,
    PanelBgAlt,
    HeaderBg,
    SurfaceHover,
    SurfaceActive,

    Border,
    BorderSubtle,
    BorderHover,

    TextPrimary,
    TextSecondary,
    TextDisabled,
    TextOnAccent,

    Accent,
    AccentLight,

    // The semantic layer: which status is carried in `status`, so that
    // "the danger plate" is one token rather than a second enum of twenty.
    StatusText,
    StatusFill,
    StatusBorder,
    StatusHover,
    StatusPressed,
};

QString colorRoleName(ColorRole role);
QString semanticRoleName(SemanticRole role);

// One colour, as a reference. `alpha` multiplies whatever the role resolves to,
// which is how the design's `rgba(var(--rl), .12)` is said here without a
// second token per opacity.
struct ColorToken
{
    ColorRole role = ColorRole::Transparent;
    SemanticRole status = SemanticRole::Neutral;
    double alpha = 1.0;

    QColor resolve(const ThemeProfile& profile) const;

    // A stable name for the token, for tests and for the gallery's inspector:
    // "accent", "accent@12", "status.danger.fill". Never a colour.
    QString describe() const;

    bool isTransparent() const { return role == ColorRole::Transparent || alpha <= 0.0; }

    bool operator==(const ColorToken& other) const
    {
        return role == other.role && status == other.status
            && qFuzzyCompare(alpha + 1.0, other.alpha + 1.0);
    }
    bool operator!=(const ColorToken& other) const { return !(*this == other); }
};

// Spelled out so a call site reads as the design does.
ColorToken token(ColorRole role, double alpha = 1.0);
ColorToken statusToken(SemanticRole status, ColorRole role, double alpha = 1.0);

// Everything a painted control needs to know about its own appearance, as
// references. A component with no hover state simply leaves those tokens
// transparent.
struct StyleSpec
{
    ColorToken background;
    ColorToken foreground;
    ColorToken border;

    ColorToken hoverBackground;
    ColorToken hoverForeground;
    ColorToken hoverBorder;

    ColorToken checkedBackground;
    ColorToken checkedForeground;
    ColorToken checkedBorder;

    ColorToken disabledForeground;
    ColorToken disabledBorder;

    SurfaceRole radius = SurfaceRole::Button;
    // Steps the radius down without claiming to be a different kind of
    // surface. A small button is still a button; it is drawn one step tighter,
    // and `SurfaceRole::ButtonSmall` would be a surface that does not exist.
    // -1 means "use `radius`"; anything else is a RadiusStep.
    int radiusStepOverride = -1;
    BorderRole borderWidth = BorderRole::Input;
    TypeRole type = TypeRole::Body;

    // Weight overrides the role's own when non-zero — a Primary button is bold
    // where the Body role is not, and that is a property of the variant rather
    // than of the type scale.
    int weight = 0;
};

// The same thing, for one profile, at one size. Produced on demand; never
// stored on a widget.
struct ResolvedStyle
{
    QColor background;
    QColor foreground;
    QColor border;

    QColor hoverBackground;
    QColor hoverForeground;
    QColor hoverBorder;

    QColor checkedBackground;
    QColor checkedForeground;
    QColor checkedBorder;

    QColor disabledForeground;
    QColor disabledBorder;

    // Already run through resolveRadius(), so a pill is a number rather than
    // the marker.
    int radius = 0;
    int borderWidth = 1;
    QFont font;
};

// `height` is what a Pill radius resolves against; pass the widget's own height
// where it has one, and 0 where the caller only wants the colours.
ResolvedStyle resolve(const StyleSpec& spec, const QWidget* context, int height = 0);

} // namespace novu::ui
