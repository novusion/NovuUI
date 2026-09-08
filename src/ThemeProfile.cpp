#include "novu/ui/ThemeProfile.h"

namespace novu::ui {

namespace {

// Role defaults shared by both profiles. A profile overrides only what it
// actually differs on — that way adding a role later cannot leave a profile
// silently unset.
void applyCommonRoles(ThemeProfile& p)
{
    p.setRoleSpacing(SpacingRole::CardPadding, p.spacing.lg);
    p.setRoleSpacing(SpacingRole::CardGap, p.spacing.md);
    p.setRoleSpacing(SpacingRole::SectionGap, p.spacing.md);
    p.setRoleSpacing(SpacingRole::ToolbarGap, p.spacing.sm);
    p.setRoleSpacing(SpacingRole::FormRowGap, p.spacing.sm);
    p.setRoleSpacing(SpacingRole::TableCellPadding, p.spacing.sm);
    p.setRoleSpacing(SpacingRole::PagePadding, p.spacing.lg);

    p.setRoleType(TypeRole::PageTitle,    TypeStyle{p.type.xl, 700, false});
    p.setRoleType(TypeRole::SectionTitle, TypeStyle{p.type.lg, 700, false});
    // Bold, not demibold: at 13px the two are nearly indistinguishable on
    // screen, and the approved mockups draw a card title at full weight.
    p.setRoleType(TypeRole::CardTitle,    TypeStyle{p.type.md, 700, false});
    p.setRoleType(TypeRole::Body,         TypeStyle{p.type.md, 400, false});
    p.setRoleType(TypeRole::Caption,      TypeStyle{p.type.sm, 400, false});
    p.setRoleType(TypeRole::Label,        TypeStyle{p.type.sm, 600, false});
    p.setRoleType(TypeRole::Metric,       TypeStyle{p.type.xl, 700, false});
    p.setRoleType(TypeRole::Mono,         TypeStyle{p.type.sm, 400, true});

    // The window title is the caption of a frame, not a heading inside a page:
    // one step under a card title and not bold, so it identifies the window
    // without competing with the first thing in it.
    p.setRoleType(TypeRole::WindowTitle, TypeStyle{p.type.sm, 600, false});

    p.setRoleBorder(BorderRole::Card,    p.borders.hairline);
    p.setRoleBorder(BorderRole::Input,   p.borders.hairline);
    p.setRoleBorder(BorderRole::Table,   p.borders.hairline);
    p.setRoleBorder(BorderRole::Divider, p.borders.hairline);
    p.setRoleBorder(BorderRole::Focus,   p.borders.strong);
}

// Window chrome derived from the profile's own palette.
//
// Called by every profile rather than left to WindowTokens' defaults: the
// defaults are the graphite palette written out as literals, and a profile
// that forgot this call would wear NovuSpace's title bar over its own colours.
// Deriving means a third product gets correct chrome by defining a palette and
// nothing else.
//
// Geometry is deliberately *not* varied here. The task that created these
// tokens exists because three applications each chose their own control size;
// a profile may restyle the title bar, but the bounding box of the three
// controls is the same number in every product.
void applyWindowTokens(ThemeProfile& p)
{
    const ThemeColors& c = p.colors;
    p.window.titleBarBg = c.headerBg;
    p.window.titleBarBgInactive = c.windowBg;
    p.window.titleText = c.textPrimary;
    p.window.titleTextInactive = c.textDisabled;
    p.window.border = c.border;

    p.window.controlHoverBg = c.surfaceHover;
    p.window.controlPressedBg = c.surfaceActive;
    p.window.controlGlyph = c.textSecondary;
    p.window.controlGlyphInactive = c.textDisabled;

    p.window.closeHoverBg = c.danger;
    // Pressed is the same hue one step down rather than a second token a
    // profile has to keep in sync with the first. darker(130) keeps the tint
    // recognisably the danger colour at both ends of the palette.
    p.window.closePressedBg = c.danger.darker(130);
    p.window.closeHoverGlyph = QColor(QStringLiteral("#FFFFFF"));

    p.window.borderWidth = p.borders.hairline;
}

} // namespace

// Every profile must call this, and must call it before any token it means to
// override by hand - a profile that skips it ships a default-constructed
// semantic palette, which paints black. semanticColoursReadAgainstTheirOwnSurface()
// in the tests is the guard that catches it.
void applyDerivedTokens(ThemeProfile& p)
{
    p.semantic.success = semanticColorFrom(p.colors.success);
    p.semantic.warning = semanticColorFrom(p.colors.warning);
    p.semantic.danger = semanticColorFrom(p.colors.danger);
    p.semantic.info = semanticColorFrom(p.colors.info);
    // Neutral is the profile's own secondary ink: it is the absence of a
    // status, not a status of its own, so it has to follow the surface it sits
    // on rather than be shared.
    p.semantic.neutral = semanticColorFrom(p.colors.textSecondary);

    p.table.headerBg = p.colors.headerBg;
    p.table.gridLine = p.colors.border;
    p.table.selectionBg = p.colors.surfaceActive;
    p.table.hoverBg = p.colors.surfaceHover;
    p.table.headerHeight = 34;
    p.table.rowHeight = p.controls.rowHeight;

    p.chart.grid = p.colors.border;
    p.chart.axis = p.colors.textDisabled;
    p.chart.bullish = p.colors.success;
    p.chart.bearish = p.colors.danger;
    p.chart.volume = p.colors.accentLight;
    p.chart.crosshair = p.colors.textSecondary;
    p.chart.series = {p.colors.accent, p.colors.accentLight, p.colors.warning,
                      p.colors.success, p.colors.danger};

    applyWindowTokens(p);
}

ThemeProfile novuSpaceProfile()
{
    ThemeProfile p;
    p.id = QStringLiteral("novuspace");
    p.displayName = QStringLiteral("NovuSpace");
    // Colours are ThemeColors' defaults - the Novu application palette. Every
    // application profile is this one; see AppProfiles.cpp.
    p.radii = RadiusScale{0, 2, 4, 6, 8, 10, 9999};
    p.spacing = SpacingScale{4, 8, 12, 16, 24};
    p.type = TypeScale{11, 12, 13, 14, 16};
    p.controls = ControlMetrics{28, 24, 32};
    // Near-flat: the shell's language. "Flat" is a choice this profile makes,
    // not a property of the system.
    p.setRoleRadius(SurfaceRole::Card, RadiusStep::Small);
    p.setRoleRadius(SurfaceRole::Dialog, RadiusStep::Small);
    p.setRoleRadius(SurfaceRole::Button, RadiusStep::Small);
    p.setRoleRadius(SurfaceRole::Input, RadiusStep::XS);
    p.setRoleRadius(SurfaceRole::Badge, RadiusStep::Pill);
    p.setRoleRadius(SurfaceRole::Toolbar, RadiusStep::None);
    p.setRoleRadius(SurfaceRole::Table, RadiusStep::Small);
    p.setRoleRadius(SurfaceRole::Overlay, RadiusStep::Small);
    // The shell's own frame. Same step as a dialog here — NovuSpace is a
    // near-flat language and does not distinguish them.
    p.setRoleRadius(SurfaceRole::Window, RadiusStep::Small);

    applyCommonRoles(p);
    applyDerivedTokens(p);
    return p;
}

ThemeProfile novuTradeProfile()
{
    ThemeProfile p;
    p.id = QStringLiteral("novutrade");
    p.displayName = QStringLiteral("NovuTrade");

    // Mirrors frontend/tailwind.config.ts of the trading web app, so the
    // desktop reads as the same product rather than a different one.
    p.colors.windowBg = QColor(QStringLiteral("#0f172a"));   // background
    p.colors.panelBg = QColor(QStringLiteral("#111827"));    // surface
    p.colors.panelBgAlt = QColor(QStringLiteral("#0b1220"));
    p.colors.headerBg = QColor(QStringLiteral("#0b1220"));
    p.colors.surfaceHover = QColor(QStringLiteral("#1b2432"));
    p.colors.surfaceActive = QColor(QStringLiteral("#222e40"));

    p.colors.border = QColor(148, 163, 184, 46);
    p.colors.borderSubtle = QColor(148, 163, 184, 30);
    p.colors.borderHover = QColor(16, 185, 129, 90);

    p.colors.textPrimary = QColor(QStringLiteral("#E3E8F0"));
    p.colors.textSecondary = QColor(QStringLiteral("#9ca3af"));
    p.colors.textDisabled = QColor(QStringLiteral("#6b7280"));   // muted
    p.colors.textOnAccent = QColor(QStringLiteral("#062018"));

    p.colors.accent = QColor(QStringLiteral("#10b981"));        // primary
    p.colors.accentLight = QColor(QStringLiteral("#0ea5e9"));   // secondary
    // green-600, not green-500: the lighter one read as a grassy yellow-green
    // against the slate surfaces.
    // Status colours are deliberately NOT overridden here. NovuTrade's identity
    // is its slate surfaces and its emerald accent; "failed" does not mean
    // something different in a trading window. The shared hues were checked
    // against this profile's own panel (#111827) and clear 4.5:1 by more than
    // the shell's do - 5.26, 7.03 and 5.29 - so an override would cost contrast
    // as well as consistency.
    //
    // The candle colours below are a different matter: they are domain
    // colours, not UI status, and they stay the web app's.

    // The Tailwind scale the web actually uses: p-4/px-4 = 16, py-3 = 12,
    // gap-2 = 8, p-6 = 24.
    p.spacing = SpacingScale{4, 8, 12, 16, 24};
    p.type = TypeScale{11, 12, 13, 16, 20};
    p.controls = ControlMetrics{32, 26, 34};
    // rounded-lg = 8, rounded-xl = 12, rounded-2xl = 16.
    p.radii = RadiusScale{0, 4, 6, 8, 12, 16, 9999};

    p.setRoleRadius(SurfaceRole::Card, RadiusStep::Large);      // R_CARD  = 12
    p.setRoleRadius(SurfaceRole::Dialog, RadiusStep::XL);       // R_DIALOG = 16
    p.setRoleRadius(SurfaceRole::Button, RadiusStep::Small);    // R_BTN   = 6
    p.setRoleRadius(SurfaceRole::Input, RadiusStep::XS);        // R_INPUT = 4
    p.setRoleRadius(SurfaceRole::Badge, RadiusStep::Pill);
    p.setRoleRadius(SurfaceRole::Toolbar, RadiusStep::None);
    p.setRoleRadius(SurfaceRole::Table, RadiusStep::Medium);
    p.setRoleRadius(SurfaceRole::Overlay, RadiusStep::Large);
    // R_WINDOW = 12. One step under the dialog, so a dialog reads as the
    // lighter surface floating over the window rather than as a second window.
    p.setRoleRadius(SurfaceRole::Window, RadiusStep::Large);

    // Derive first, then override. Everything below this line is a place where
    // NovuTrade genuinely differs; everything the derivation produced and is not
    // restated is shared, which is what makes the differences readable as a
    // list rather than hidden in a full copy of the token set.
    applyDerivedTokens(p);

    p.table.headerBg = p.colors.headerBg;
    p.table.gridLine = p.colors.borderSubtle;
    p.table.selectionBg = QColor(16, 185, 129, 56);
    p.table.hoverBg = p.colors.surfaceHover;
    // 34, matching the row height: the approved mockup draws the header as
    // one more row of the grid, not as a taller band above it.
    p.table.headerHeight = 34;
    p.table.rowHeight = p.controls.rowHeight;

    p.chart.grid = p.colors.borderSubtle;
    p.chart.axis = p.colors.textDisabled;
    // Pinned rather than taken from the status hues. A candle is not a
    // "success" - it is a price going up - and the trading web app's greens and
    // reds are what users read this chart against.
    p.chart.bullish = QColor(QStringLiteral("#22c55e"));
    p.chart.bearish = QColor(QStringLiteral("#ef4444"));
    p.chart.volume = p.colors.accentLight;
    p.chart.crosshair = p.colors.textSecondary;
    p.chart.series = {p.colors.accent, p.colors.accentLight, p.colors.warning,
                      p.colors.success, p.colors.danger};

    applyCommonRoles(p);
    // The trading UI is denser than the shell: cards sit closer together and
    // the toolbar gap follows the web's gap-2.
    p.setRoleSpacing(SpacingRole::CardPadding, p.spacing.lg);   // p-4
    p.setRoleSpacing(SpacingRole::PagePadding, p.spacing.xl);   // p-6
    p.setRoleSpacing(SpacingRole::CardGap, p.spacing.lg);       // space-y-4
    // A card title is fully bold here, not semibold. Scoped to this profile
    // rather than changed in applyCommonRoles(), because the approved mockup
    // speaks for NovuTrade only and NovuSpace keeps its lighter heading.
    p.setRoleType(TypeRole::CardTitle, TypeStyle{p.type.md, 700, false});
    // 16, not 20. The mockup is a standalone window, but the module renders
    // inside the NovuSpace shell, whose own headings are 16 - a 20px page title
    // there does not read as emphasis, it reads as a different product wedged
    // into the frame.
    p.setRoleType(TypeRole::PageTitle, TypeStyle{p.type.lg, 700, false});
    // The page frame comes down with it: at 24 the heading floated in a band of
    // its own before the first card.
    p.setRoleSpacing(SpacingRole::PagePadding, p.spacing.lg);
    return p;
}

} // namespace novu::ui
