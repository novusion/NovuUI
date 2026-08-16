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

// The profiles shipped with the library. Applications may register their own
// through ThemeManager; these exist so a fresh consumer looks right with no
// configuration at all.
ThemeProfile novuSpaceProfile();   // graphite, near-flat — the canonical theme
ThemeProfile novuTradeProfile();   // slate/emerald, rounded — mirrors the web app

} // namespace novu::ui
