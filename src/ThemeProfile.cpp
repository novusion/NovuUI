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

    // The group caption: the smallest step, in the monospace face. Uppercasing
    // and tracking are applied by the component that draws it, because they
    // are a treatment rather than a font.
    p.setRoleType(TypeRole::Eyebrow,     TypeStyle{p.type.xxs, 500, true});

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
    // The design runs the bar from the header surface down to the window's own,
    // which is what separates the chrome from the content without a second
    // hairline.
    p.window.titleBarBgBottom = c.windowBg;
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

// The Novusion family baseline.
//
// Every value here is read off the approved shared palette,
//
//     g:\Design\design\Shared\design-system\Novu_Palette.png
//
// which is the design system for the family rather than for any one product.
// It fixes the six surfaces, the three text weights, the four status colours
// and one accent; the NovuSpace, NovuHub, NovuExplorer and NovuAdmin mockups
// are all drawn on top of it.
//
// The accent here is the *family's* cyan. A product with an approved accent of
// its own overrides exactly that one value - see applicationProfile() in
// AppProfiles.cpp. A product without one keeps this, and says so.
//
// Geometry is NovuSpace's, and deliberately: the approved NovuSpace design is
// the only one that states radii, spacing, control heights and a type scale in
// full, and a second product inventing its own would be the drift these tokens
// exist to prevent. A product that genuinely needs different geometry overrides
// the roles it needs, the way novuTradeProfile() does.
ThemeProfile novusionBaseProfile()
{
    ThemeProfile p = novuSpaceProfile();
    p.id = QStringLiteral("novusion");
    p.displayName = QStringLiteral("Novusion");

    // ACCENT & BRAND, from the shared palette.
    p.colors.accent      = QColor(QStringLiteral("#2FC6D8"));
    p.colors.accentLight = QColor(QStringLiteral("#4DD4E6"));
    // Measured rather than chosen: dark ink clears 8.4:1 on the family cyan
    // where white manages 2.1:1. The same reasoning as NovuSpace's amber.
    p.colors.textOnAccent = QColor(QStringLiteral("#10171A"));

    applyDerivedTokens(p);
    return p;
}

// NovuSpace, from the approved design.
//
// Every number and every colour below is read off
//
//     g:\Design\design\NovuSpace\mockups\NovuSpace-Trade-brand-mockup.html
//
// which is the design that was signed off. Where a CSS custom property has an
// obvious token here, the token takes its value; where it does not, the
// mapping is stated in a comment rather than left to be guessed at.
//
// The surfaces, hairlines and type scale are the *family's*, not NovuSpace's —
// the design says so outright, and the NovuHub, NovuExplorer and NovuAdmin
// mockups carry the same ones. The accent is the one thing that identifies the
// product. That is why every other application profile is still built from
// this one (see AppProfiles.cpp): they share what the design says is shared.
ThemeProfile novuSpaceProfile()
{
    ThemeProfile p;
    p.id = QStringLiteral("novuspace");
    p.displayName = QStringLiteral("NovuSpace");

    // ── surfaces ──────────────────────────────────────────────────────────
    // --window / --bg / --panel / --card / --card-h. The design stacks five
    // surfaces, not three: the frame, the card, the raised panel, the inset
    // well and its hover.
    p.colors.windowBg    = QColor(QStringLiteral("#151A20"));   // --window
    p.colors.panelBg     = QColor(QStringLiteral("#1D2228"));   // --bg: a card
    p.colors.panelBgAlt  = QColor(QStringLiteral("#232A33"));   // --panel
    p.colors.headerBg    = QColor(QStringLiteral("#1A1F26"));   // --titlebar-top
    p.colors.surfaceHover  = QColor(QStringLiteral("#29323D")); // --card
    p.colors.surfaceActive = QColor(QStringLiteral("#313B47")); // --card-h

    // --hair is the rule inside a surface, --hair2 the one between surfaces.
    p.colors.border       = QColor(QStringLiteral("#29323D"));  // --hair
    p.colors.borderHover  = QColor(QStringLiteral("#394553"));  // --hair2
    p.colors.borderSubtle = QColor(QStringLiteral("#232A33"));

    // --text / --text2 / --text3. The third is the design's muted ink, which
    // is what Ink::Muted resolves to; a genuinely disabled control drops its
    // opacity rather than taking a fourth grey.
    p.colors.textPrimary   = QColor(QStringLiteral("#E7EDF5"));
    p.colors.textSecondary = QColor(QStringLiteral("#B7C1CE"));
    p.colors.textDisabled  = QColor(QStringLiteral("#8793A3"));

    // ── the product accent ────────────────────────────────────────────────
    // --accent / --acc-l / --ink-on-accent. Amber, chosen in the preset
    // evaluation and frozen. The ink on top is the measured choice the design
    // records: dark ink clears 6.9:1 on the filled accent where white manages
    // 2.6:1.
    p.colors.accent      = QColor(QStringLiteral("#D49335"));
    p.colors.accentLight = QColor(QStringLiteral("#E3B87A"));
    p.colors.textOnAccent = QColor(QStringLiteral("#10171A"));

    // Status is deliberately NOT set here. The design says outright that the
    // semantics are a separate layer from the product — green means running in
    // every Novu window, not only in this one — so the four hues live in
    // sharedSemanticHues() and reach this profile through ThemeColors' own
    // defaults. A line here would be this product forking what it does not own.

    // ── geometry ──────────────────────────────────────────────────────────
    // The radii the design actually draws: 5 on a check box, 7 on a chip and a
    // small button, 10 on a button and an input, 14 on a card, 16 on the window
    // frame. Five steps, five values, nothing interpolated.
    p.radii = RadiusScale{0, 5, 7, 10, 14, 16, 9999};
    p.spacing = SpacingScale{4, 8, 12, 16, 24};
    // Height 30 is the design's `.btn` measured out: 12.5px text at line-height
    // 1.3 inside .52em of vertical padding and a hairline. 26 is `.btn.sm`, 34
    // is a table row.
    p.controls = ControlMetrics{30, 26, 34};
    // `.btn svg` is 15px, `.btn.sm svg` 13px, and a header or empty-state mark
    // is 20px.
    p.icons = IconMetrics{13, 15, 20};

    // 12.5px body rounds to 13; the design's 9.5/10 captions are one scale step
    // below the 11px hint line.
    p.type = TypeScale{11, 12, 13, 14, 16, 10};
    // Three faces, as the design sets them: Instrument Sans for titles, Inter
    // for text, JetBrains Mono for anything that has to line up in a column.
    // None of the three ships with Windows, so each one names what to use until
    // it is installed - the design's intent is recorded here either way.
    p.type.family = QStringLiteral("Inter");
    p.type.displayFamily = QStringLiteral("Instrument Sans");
    p.type.monoFamily = QStringLiteral("JetBrains Mono");

    p.setRoleRadius(SurfaceRole::Card, RadiusStep::Large);       // .card 14
    p.setRoleRadius(SurfaceRole::Dialog, RadiusStep::XL);        // .modal 15
    p.setRoleRadius(SurfaceRole::Button, RadiusStep::Medium);    // .btn 10
    p.setRoleRadius(SurfaceRole::Input, RadiusStep::Medium);     // .input 10
    p.setRoleRadius(SurfaceRole::Badge, RadiusStep::Pill);
    p.setRoleRadius(SurfaceRole::Toolbar, RadiusStep::None);
    p.setRoleRadius(SurfaceRole::Table, RadiusStep::Large);      // inside a card
    p.setRoleRadius(SurfaceRole::Overlay, RadiusStep::Large);    // .popover 12
    p.setRoleRadius(SurfaceRole::Window, RadiusStep::XL);        // .win 16

    applyCommonRoles(p);

    // Spacing the design states directly, rather than the nearest scale step.
    p.setRoleSpacing(SpacingRole::CardPadding, 16);      // .card-body 15px 16px
    p.setRoleSpacing(SpacingRole::CardGap, 14);          // .card + .card
    p.setRoleSpacing(SpacingRole::ToolbarGap, 9);        // .toolbar gap
    p.setRoleSpacing(SpacingRole::FormRowGap, 6);        // .field gap
    p.setRoleSpacing(SpacingRole::TableCellPadding, 9);  // td padding
    p.setRoleSpacing(SpacingRole::PagePadding, 20);      // .page-body
    p.setRoleSpacing(SpacingRole::SectionGap, 18);       // .rail-sec margin-top

    // Type, role by role, as the design sets each one. Weight 600 throughout
    // the titles: the display face is drawn at demibold, not bold.
    p.setRoleType(TypeRole::PageTitle,    TypeStyle{16, 600, false});  // .page-head h1
    p.setRoleType(TypeRole::SectionTitle, TypeStyle{15, 600, false});  // .modal-head h2
    p.setRoleType(TypeRole::CardTitle,    TypeStyle{14, 600, false});  // .card-head h2
    p.setRoleType(TypeRole::Body,         TypeStyle{13, 400, false});  // 12.5px
    p.setRoleType(TypeRole::Caption,      TypeStyle{12, 400, false});  // .sub / .hint
    p.setRoleType(TypeRole::Label,        TypeStyle{13, 600, false});  // .kv .k .lab
    p.setRoleType(TypeRole::Metric,       TypeStyle{25, 600, false});  // .stat .val
    p.setRoleType(TypeRole::Mono,         TypeStyle{11, 400, true});   // .idcell
    p.setRoleType(TypeRole::WindowTitle,  TypeStyle{13, 600, false});  // .app-name
    p.setRoleType(TypeRole::Eyebrow,      TypeStyle{10, 500, true});   // .flabel / th

    applyDerivedTokens(p);

    // Window chrome geometry is deliberately NOT set here. `.titlebar` is
    // 42px, `.app-ico` 18px and `.winbtn` 30x24 - but those are the *system's*
    // numbers, not this product's, so they live in WindowTokens' own defaults
    // where every profile shares them. A profile that restated them would be
    // the first of four applications each picking its own control size again.

    // A table header sits *on* a card and the design gives it the card's own
    // surface, not the title bar's. Restated after the derivation, which has no
    // way to tell the two headers apart.
    p.table.headerBg = p.colors.panelBg;

    // Candles are domain colours, not status: the design draws a profit in
    // --profit and a loss in --loss, which are a shade deeper than the ink a
    // status badge uses.
    p.chart.bullish = QColor(QStringLiteral("#3CCB7F"));
    p.chart.bearish = QColor(QStringLiteral("#E0909B"));

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
