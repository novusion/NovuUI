#pragma once

#include "novu/ui/Theme.h"

#include <QColor>
#include <QHash>
#include <QList>
#include <QString>

namespace novu::ui {

// A radius scale, not a radius. The design system owns the *steps*; each
// profile owns their values. A component never names a number — it names a
// step, or better, a role (see SurfaceRole below), so the same component is
// flat in one product and rounded in another without a single conditional.
enum class RadiusStep { None, XS, Small, Medium, Large, XL, Pill };

struct RadiusScale
{
    int none = 0;
    int xs = 2;
    int small = 4;
    int medium = 8;
    int large = 12;
    int xl = 16;
    // Resolved against the widget's own height at paint time, so a pill stays
    // a pill at any size. Any value large enough works as the marker.
    int pill = 9999;

    int value(RadiusStep step) const
    {
        switch (step) {
        case RadiusStep::None:   return none;
        case RadiusStep::XS:     return xs;
        case RadiusStep::Small:  return small;
        case RadiusStep::Medium: return medium;
        case RadiusStep::Large:  return large;
        case RadiusStep::XL:     return xl;
        case RadiusStep::Pill:   return pill;
        }
        return none;
    }
};

// What a component *is*, not how round it looks. Components ask for a role;
// the profile decides which step that role maps to. This is the indirection
// that lets NovuSpace say "cards are barely rounded" and NovuTrade say "cards
// are rounded-xl" without either statement living in a component.
enum class SurfaceRole {
    Card,
    Dialog,
    Button,
    Input,
    Badge,
    Toolbar,
    Table,
    Overlay,
    // The frame of a top-level application window. Separate from Dialog
    // because a maximisable main window and a small confirm dialog are allowed
    // to differ - but only as two roles a profile decides, never as two
    // numbers two applications picked.
    Window,
};

// Spacing scale. Same reasoning: steps are systemic, values are per profile.
struct SpacingScale
{
    int xs = 4;
    int sm = 8;
    int md = 12;
    int lg = 16;
    int xl = 24;
};

enum class SpacingRole {
    CardPadding,      // inside a card
    CardGap,          // between cards
    SectionGap,       // between a section heading and its content
    ToolbarGap,       // between controls in a toolbar
    FormRowGap,       // between label and control
    TableCellPadding,
    PagePadding,
};

struct TypeScale
{
    int xs = 11;
    int sm = 12;
    int md = 13;
    int lg = 16;
    int xl = 20;
    // One step below xs, for the two places the approved design goes there:
    // the eyebrow above a page title and a version line. Declared last, out
    // of size order, because both profiles initialise this struct
    // positionally — putting it first would silently shift every size.
    int xxs = 10;

    // The family belongs to the profile too. Without it a component has to
    // name a font, and the first product that wants a different one turns
    // every component into a conditional.
    QString family = QStringLiteral("Segoe UI");
    QString monoFamily = QStringLiteral("Consolas");
};

// What a piece of text *is*. A component asks for a role and gets size and
// weight together, so "the card title" is defined once for a product rather
// than re-derived at every call site.
enum class TypeRole {
    PageTitle,
    SectionTitle,
    CardTitle,
    Body,
    Caption,     // muted helper text
    Label,       // form labels
    Metric,      // large numeric value on a StatCard
    Mono,        // ids, hashes, versions
    WindowTitle, // the caption in a window or dialog title bar
};

struct TypeStyle
{
    int size = 13;
    int weight = 400;   // QFont::Weight numeric
    bool monospace = false;
};

// Control geometry that must not stagger within a toolbar.
struct ControlMetrics
{
    int height = 32;
    int heightSmall = 26;
    int rowHeight = 34;
};

enum class ControlSize { Small, Medium, Large };

// Border weight per surface. Kept separate from colour because a product can
// share a palette and still be heavier or lighter in its outlines.
enum class BorderRole { Card, Input, Table, Divider, Focus };

struct BorderScale
{
    int hairline = 1;
    int regular = 1;
    int strong = 2;
};

// Chart colours are part of the theme, not of the charting code. Without this
// every chart re-invents its own series palette.
struct ChartTokens
{
    QColor grid{QStringLiteral("#232830")};
    QColor axis{QStringLiteral("#6E7A8E")};
    QColor bullish{QStringLiteral("#22c55e")};
    QColor bearish{QStringLiteral("#ef4444")};
    QColor volume{QStringLiteral("#3DA9FC")};
    QColor crosshair{QStringLiteral("#A8B2C3")};
    QList<QColor> series;   // categorical palette, filled per profile
};

// Table chrome, so every table in every product agrees on stripes, header
// weight and selection.
struct TableTokens
{
    int headerHeight = 34;
    int rowHeight = 34;
    bool alternatingRows = false;
    QColor headerBg{QStringLiteral("#1A1F25")};
    QColor gridLine{QStringLiteral("#232830")};
    QColor selectionBg{QStringLiteral("#2F353F")};
    QColor hoverBg{QStringLiteral("#2A3038")};
};

// Durations only — easing stays in the components, since a component knows
// what kind of motion it performs. Zero disables animation wholesale, which
// is what a "reduced motion" profile sets.
struct AnimationTokens
{
    int instantMs = 0;
    int fastMs = 120;
    int normalMs = 200;
    int slowMs = 320;
};

// Icon sizing, so no component invents a pixel size for a glyph.
struct IconMetrics
{
    int small = 14;
    int medium = 16;
    int large = 20;
};

// Window chrome: the title bar and its three controls.
//
// This is the token family that stops every application from inventing its own
// frame. Before it existed NovuExplorer drew 46x32 controls, NovuHub drew
// stylesheet buttons with no size at all, and NovuExplorer's own popup drew a
// 22x22 close button - three answers to a question that has exactly one.
//
// Geometry is stated once here and read by TitleBar and WindowControlButton;
// neither an application nor a page may set any of it. The colours live here
// rather than in ThemeColors for the same reason TableTokens holds its own
// header colour: a profile can share a palette and still choose a different
// title-bar treatment, and a component should not have to know which palette
// entry a product decided to use for its chrome.
struct WindowTokens
{
    // The three controls are one bounding box, used for all of them. Close is
    // never wider or taller than Minimize - the size is the contract.
    int titleBarHeight = 32;
    int controlWidth = 46;
    int controlHeight = 32;

    // Leading inset before the window icon, and the gap between icon, title
    // and any trailing widget an application adds. The controls themselves sit
    // flush against the trailing edge - a gap there would leave a dead strip
    // where the pointer expects Close on a maximised window.
    int titleBarPadding = 10;
    int titleBarGap = 8;

    int borderWidth = 1;
    // The grab band along the frame edge, in logical pixels. Hit-testing is
    // done by the shell, so a resizable window and a fixed dialog differ by a
    // flag rather than by two different event filters.
    int resizeMargin = 6;

    int iconSize = 16;
    // The bounding box of a control's glyph, not its stroke length. Minimize
    // draws one line across it, Maximize a square inside it, Close a cross
    // through it, so all three read at the same visual weight.
    int glyphSize = 10;
    int glyphStroke = 1;

    QColor titleBarBg{QStringLiteral("#1A1F25")};
    QColor titleBarBgInactive{QStringLiteral("#161A1F")};
    QColor titleText{QStringLiteral("#E3E8F0")};
    QColor titleTextInactive{QStringLiteral("#6E7A8E")};
    QColor border{QStringLiteral("#232830")};

    // Neutral control states, shared by Minimize and Maximize/Restore.
    QColor controlHoverBg{QStringLiteral("#2A3038")};
    QColor controlPressedBg{QStringLiteral("#2F353F")};
    QColor controlGlyph{QStringLiteral("#A8B2C3")};
    QColor controlGlyphInactive{QStringLiteral("#6E7A8E")};

    // Close is neutral until the pointer is on it. Permanently red is what the
    // applications did before, and it made every window look like it was
    // asking to be closed.
    QColor closeHoverBg{QStringLiteral("#FF5555")};
    QColor closePressedBg{QStringLiteral("#C93F3F")};
    QColor closeHoverGlyph{QStringLiteral("#FFFFFF")};
};

// One product's complete visual language. Everything a component can vary
// lives here — nothing may be hardcoded in a component, and nothing may be
// decided by a page.
struct ThemeProfile
{
    QString id;               // "novuspace", "novutrade", ...
    QString displayName;
    ThemeColors colors;
    RadiusScale radii;
    SpacingScale spacing;
    TypeScale type;
    ControlMetrics controls;
    BorderScale borders;
    ChartTokens chart;
    TableTokens table;
    AnimationTokens animation;
    IconMetrics icons;
    WindowTokens window;
    // Derived from the four status hues in `colors` by applyDerivedTokens().
    // Components read this rather than the hues, so "the plate behind a Danger
    // word" is decided once instead of at every call site.
    SemanticPalette semantic;

    QHash<int, int> roleRadius;    // SurfaceRole  -> RadiusStep
    QHash<int, int> roleSpacing;   // SpacingRole  -> pixels
    QHash<int, TypeStyle> roleType;// TypeRole     -> TypeStyle
    QHash<int, int> roleBorder;    // BorderRole   -> pixels

    int radius(SurfaceRole role) const
    {
        const auto it = roleRadius.constFind(static_cast<int>(role));
        const RadiusStep step = it == roleRadius.constEnd()
            ? RadiusStep::Small
            : static_cast<RadiusStep>(it.value());
        return radii.value(step);
    }

    int space(SpacingRole role) const
    {
        return roleSpacing.value(static_cast<int>(role), spacing.md);
    }

    TypeStyle text(TypeRole role) const
    {
        return roleType.value(static_cast<int>(role), TypeStyle{type.md, 400, false});
    }

    int border(BorderRole role) const
    {
        return roleBorder.value(static_cast<int>(role), borders.regular);
    }

    int controlHeight(ControlSize size) const
    {
        switch (size) {
        case ControlSize::Small:  return controls.heightSmall;
        case ControlSize::Large:  return controls.height + 6;
        case ControlSize::Medium: break;
        }
        return controls.height;
    }

    void setRoleRadius(SurfaceRole role, RadiusStep step)
    {
        roleRadius.insert(static_cast<int>(role), static_cast<int>(step));
    }
    void setRoleSpacing(SpacingRole role, int px)
    {
        roleSpacing.insert(static_cast<int>(role), px);
    }
    void setRoleType(TypeRole role, const TypeStyle& style)
    {
        roleType.insert(static_cast<int>(role), style);
    }
    void setRoleBorder(BorderRole role, int px)
    {
        roleBorder.insert(static_cast<int>(role), px);
    }
};

// Fills in every token family that is a function of the palette rather than a
// choice: table chrome, chart series and window chrome.
//
// Public because an application registering a profile of its own needs exactly
// this. Set `colors`, call this, and the frame, the tables and the charts come
// out in the new palette - without it a custom profile silently keeps whichever
// palette the tokens were written against, which is how a "new theme" ends up
// with the graphite title bar over its own colours.
void applyDerivedTokens(ThemeProfile& profile);

// The profiles shipped with the library. Applications may register their own
// through ThemeManager; these exist so a fresh consumer looks right with no
// configuration at all.
ThemeProfile novuSpaceProfile();   // graphite, near-flat — the canonical theme
ThemeProfile novuTradeProfile();   // slate/emerald, rounded — mirrors the web app

// Application identities. Every one of them is the Novu application palette -
// they carry a name and an id, and not one colour of their own. NovuTrade above
// is the single exception, and a deliberate one.
//
// An identity is worth registering even when it adds no colour: it is what a
// subtree is scoped to, what a window says it belongs to, and what the gallery
// filters components by. An application folded into another's profile cannot be
// asked which components it uses.
ThemeProfile novuAiProfile();
ThemeProfile novuVoiceProfile();
ThemeProfile novuExplorerProfile();
ThemeProfile novuHubProfile();

// Selectable themes, which are a different thing: a user's choice of
// appearance rather than a product's identity, so these are allowed palettes of
// their own. NovuExplorer offers them in its settings.
ThemeProfile midnightBlueProfile();// GitHub dark, blue accent
ThemeProfile warmGrayProfile();    // neutral grey, gold accent
ThemeProfile darkPurpleProfile();  // violet
ThemeProfile novuLightProfile();   // the one light palette in the system

// True for a profile that names a Novusion application rather than a theme a
// user can pick. The distinction matters because an application may not have a
// palette of its own and a theme may.
bool isApplicationProfile(const QString& profileId);

// Every profile the library ships, in the order they are registered.
//
// One list, asked for rather than repeated. ThemeManager registers from it and
// the gallery tells a shipped profile from a loaded overlay with it, so adding
// a profile is one entry here and nothing else anywhere.
QList<ThemeProfile> builtinProfiles();

} // namespace novu::ui
