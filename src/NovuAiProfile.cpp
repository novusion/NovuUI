// The NovuAI theme profile.
//
// Every value below is the approved mockup's own token table, moved here from
// the frozen reference implementation it was first stated in
// (NovuAI/design/novusion_qt6_baseline/src/NovuAiTheme.cpp). It lives in the
// library rather than in the application for the reason every other profile
// does: an application states which theme it wants, and the library knows
// what that theme is. A production build that compiled its palette out of a
// frozen prototype would have a prototype as a dependency.
//
// This replaces the eight-line placeholder that used to stand in AppProfiles.cpp
// under the same name - accent colours and two type roles, enough to be
// recognisable and not enough to be the design.
//
// loadFonts() did not come with it. Reading typefaces out of a design
// repository is something an application may do on a developer's machine; it
// is not something a shared library may assume about anybody's disk.

#include "novu/ui/ThemeProfile.h"
#include "novu/ui/Icon.h"

namespace novu::ui {

namespace {
constexpr const char* kProfileId = "novuai";
constexpr const char* kLightProfileId = "novuai-light";
} // namespace

namespace {

// ── the mockup's token table ─────────────────────────────────────────────
//
// Names are the CSS custom properties, unchanged, so a value here can be
// checked against spec.py by searching for the same string.

constexpr const char* kWindow      = "#151A20";   // --window
constexpr const char* kPage        = "#0E1216";   // --page
constexpr const char* kBg          = "#1D2228";   // --bg
constexpr const char* kPanel       = "#232A33";   // --panel
constexpr const char* kCard        = "#29323D";   // --card
constexpr const char* kCardHover   = "#313B47";   // --card-h

constexpr const char* kHair        = "#252D36";   // --hair
constexpr const char* kHair3       = "#2C3540";   // --hair3
constexpr const char* kHair2       = "#354150";   // --hair2

constexpr const char* kText        = "#E7EDF5";   // --text
constexpr const char* kText2       = "#B7C1CE";   // --text2
constexpr const char* kText3       = "#8793A3";   // --text3
constexpr const char* kText4       = "#68737F";   // --text4
constexpr const char* kInkOnAccent = "#0C151F";   // --ink-on-accent

constexpr const char* kAccent      = "#4DA3FF";   // --acc
constexpr const char* kAccentLight = "#7DBEFF";   // --acc-l

constexpr const char* kGreen       = "#3CCB7F";   // --green
constexpr const char* kOrange      = "#FFB14D";   // --orange
constexpr const char* kPink        = "#C47B85";   // --pink

constexpr const char* kTitlebarTop = "#1F252E";   // --titlebar-top
constexpr const char* kTitlebarBot = "#151A20";   // --titlebar-bot

// CSS sizes remain fractional in TypeStyle and are resolved at the widget DPI.
constexpr int kFsXs = 10;    // --fs-xs  10
constexpr int kFsSm = 12;    // --fs-sm  11.5
constexpr int kFsMd = 13;    // --fs-md  12.5   the body size
constexpr int kFsLg = 14;    // --fs-lg  14
constexpr int kFsXl = 16;    // --fs-xl  16
constexpr int kFsH2 = 19;    // --fs-h2  19
constexpr int kFsH1 = 24;    // --fs-h1  24

QColor c(const char* hex)
{
    return QColor(QString::fromLatin1(hex));
}

} // namespace

ThemeProfile novuAiProfile()
{
    static const bool glyphsRegistered=[](){registerGlyphSet("novuai",{
#include "NovuAiGlyphs.inc"
    },16.0,1.7);return true;}();
    Q_UNUSED(glyphsRegistered);
    ThemeProfile p;
    p.icons.family=QStringLiteral("novuai");
    p.id = QString::fromLatin1(kProfileId);
    p.displayName = QStringLiteral("NovuAI");

    // ── surfaces ─────────────────────────────────────────────────────────
    // The mockup's ladder, in order: window → bg → panel → card → card
    // hover. NovuUI's six surface slots take it in that order.
    p.colors.windowBg      = c(kWindow);   // sidebar, rail, status bar
    p.colors.panelBg       = c(kBg);       // the centre column
    p.colors.panelBgAlt    = c(kPanel);
    p.colors.headerBg      = c(kTitlebarTop);
    p.colors.surfaceHover  = c(kCard);
    p.colors.surfaceActive = c(kCardHover);

    // `.sidebar{background:linear-gradient(180deg,rgba(0,0,0,.06),transparent 120px),
    //  var(--window)}` -- the window colour with a 6% black tint over the
    // first 120px, not a gradient run over the whole rail.
    p.colors.railTint = QColor(0, 0, 0, 15);   // rgba(0,0,0,.06)
    p.colors.railTintFade = 120;

    p.colors.borderSubtle = c(kHair);      // the common hairline
    // `.card`, `.code`, `.tool` and `.rail` all state `1px solid var(--hair)`.
    // --hair3 is the faintest list separator, not the common border, and
    // using it here made every card outline a step too light.
    p.colors.border       = c(kHair);
    p.colors.borderHover  = c(kHair2);     // and the window edge

    p.colors.textPrimary   = c(kText);
    p.colors.textSecondary = c(kText2);
    p.colors.textTertiary  = c(kText3);
    p.colors.textDisabled  = c(kText4);
    p.colors.textOnAccent  = c(kInkOnAccent);

    // The port mapping in NovuUI guessed a cyan of the #2FC6D8 family and
    // said so. The mockup states the accent outright, and it is blue.
    p.colors.accent      = c(kAccent);
    p.colors.accentLight = c(kAccentLight);

    // Status: the mockup's own hues. NovuUI asks a profile not to restate
    // these, but the mockup is the design authority for this application
    // and it states them, so they are taken from it.
    p.colors.success = c(kGreen);
    p.colors.warning = c(kOrange);
    p.colors.danger  = c(kPink);
    p.colors.info    = c(kAccent);

    // ── radii ────────────────────────────────────────────────────────────
    p.radii.none   = 0;
    p.radii.xs     = 2;
    p.radii.small  = 6;    // --r-sm
    p.radii.medium = 8;    // --r-md
    p.radii.large  = 11;   // --r-lg
    p.radii.xl     = 14;   // --r-xl
    p.radii.pill   = 9999; // --r-pill

    p.setRoleRadius(SurfaceRole::Card,    RadiusStep::Large);
    p.setRoleRadius(SurfaceRole::Dialog,  RadiusStep::XL);
    // `.btn{border-radius:var(--r-pill)}` -- every button in this design is a
    // pill. `.ibtn{border-radius:var(--r-sm)}` is not: a pill the size of an
    // icon is a circle, and a row of circles in a title bar reads as a traffic
    // light. `.select{border-radius:var(--r-pill)}` while `.omni` and a text
    // field keep a box, so the select has a shape of its own.
    p.setRoleRadius(SurfaceRole::Button,     RadiusStep::Pill);
    p.setRoleRadius(SurfaceRole::IconButton, RadiusStep::Small);
    p.setRoleRadius(SurfaceRole::Input,      RadiusStep::Small);
    p.setRoleRadius(SurfaceRole::Select,     RadiusStep::Pill);
    p.setRoleRadius(SurfaceRole::Badge,   RadiusStep::Pill);
    p.setRoleRadius(SurfaceRole::Toolbar, RadiusStep::Medium);
    p.setRoleRadius(SurfaceRole::Table,   RadiusStep::Medium);
    p.setRoleRadius(SurfaceRole::Overlay, RadiusStep::Large);
    p.setRoleRadius(SurfaceRole::Window,  RadiusStep::Medium);

    // ── type ─────────────────────────────────────────────────────────────
    p.type.xxs = 10;   // --fs-xs  10
    p.type.xs  = 11;   // --fs-sm  11.5
    p.type.sm  = 12;   // --fs-md  12.5
    p.type.md  = 12;   // --fs-md  12.5
    p.type.lg  = 14;   // --fs-lg  14
    p.type.xl  = 16;   // --fs-xl  16
    // The whole stack, not just the first name.
    //
    //   --f-head:'Instrument Sans','Segoe UI',system-ui,sans-serif
    //   --f-body:'Inter','Segoe UI',system-ui,sans-serif
    //   --f-mono:'JetBrains Mono','Cascadia Mono',Consolas,monospace
    //
    // None of the three first choices is installed on a stock Windows, so
    // the fallback is what actually renders -- the browser follows the list
    // and lands on Segoe UI, while Qt, told only "Inter", picks a substitute
    // by its own rule and lands somewhere else entirely. That single missing
    // list is most of the difference between the two screenshots.
    p.type.family        = QStringLiteral("Inter");
    p.type.unhintedMetrics = true;
    p.type.displayFamily = QStringLiteral("Instrument Sans");
    p.type.monoFamily    = QStringLiteral("JetBrains Mono");
    p.type.fallbackFamilies = {QStringLiteral("Segoe UI"),
                               QStringLiteral("system-ui"),
                               QStringLiteral("sans-serif")};
    p.type.monoFallbackFamilies = {QStringLiteral("Cascadia Mono"),
                                   QStringLiteral("Consolas"),
                                   QStringLiteral("monospace")};

    // The three properties the CSS states per role that a font alone cannot
    // carry: `.nt{letter-spacing:-0.02em; line-height:1.25}`,
    // `.now-t{letter-spacing:-0.01em; line-height:1.3}`,
    // `.md{line-height:1.62}`, `.sb-head{letter-spacing:.09em; uppercase}`.
    // Left unstated a display face lays itself out to its own metrics, which
    // is several pixels taller per line than the design's box.
    p.setRoleType(TypeRole::PageTitle,    {kFsH2, 600, false, 1.25, -0.02, false});
    p.setRoleType(TypeRole::SectionTitle, {kFsXl, 600, false, 1.30, -0.01, false});
    p.setRoleType(TypeRole::CardTitle,    {kFsLg, 600, false, 1.45, -0.01, false});
    p.setRoleType(TypeRole::Body,         {12.5, 400, false, 1.45, 0.0, false});
    p.setRoleType(TypeRole::Caption,      {11.5, 400, false,1.45});
    p.setRoleType(TypeRole::Label,        {11.5, 500, false,1.45});
    p.setRoleType(TypeRole::Metric,       {kFsH1, 600, false});
    p.setRoleType(TypeRole::Mono,         {11.5, 400, true,1.45});
    p.setRoleType(TypeRole::WindowTitle,  {12.5, 600, false});
    // `.sb-head`, `.code-head`, `.pill` -- mono, uppercase, tracked, and all
    // three at the normal weight. A 9.5px uppercase mono line set in semibold
    // reads as a solid bar.
    // `.sb-head`/`.code-head`/`.pill`: mono, uppercase, tracked .09em.
    p.setRoleType(TypeRole::Eyebrow,      {kFsXs, 400, true, 0.0, 0.09, true});
    // `.statusbar{font-size:calc(var(--fs-sm))}` -- the strip is read at a
    // glance, a step under the body size.
    p.setRoleType(TypeRole::StatusBar,    {11.5, 400, false});

    // ── metrics ──────────────────────────────────────────────────────────
    p.controls.height       = 28;
    p.controls.heightSmall  = 24;
    p.controls.buttonPadding = 14;
    p.controls.buttonPaddingSmall = 11;
    p.controls.buttonPaddingLarge = 18;
    p.controls.buttonPaddingIncludesBorder = false;
    p.controls.buttonDisabledOpacity = .42;
    p.controls.scrollThumbOnAreaHover = true;
    p.controls.scrollThumbInset = 3;
    p.controls.selectHeight = 26;
    p.controls.selectSmallHeight = 23;
    p.controls.iconHeight = 26;
    p.controls.iconSmallHeight = 22;
    p.controls.segmentedPadding = 2;
    p.controls.segmentedHeight = 26;
    p.controls.rowHeight    = 26;   // --row-h, a list or table row
    // A module row in the rail. Measured off the reference: 27px between
    // consecutive labels.
    p.controls.navRowHeight = 27;

    p.borders.hairline = 1;
    p.borders.regular  = 1;
    p.borders.strong   = 2;

    p.icons.small  = 14;
    p.icons.medium = 16;
    p.icons.large  = 20;

    p.spacing.xs = 4;    // `.queue-list{gap:5px}`, `.checks{gap:6px}`
    p.spacing.sm = 7;    // `.specs{gap:7px}`, `.rail-foot{gap:7px}`
    p.spacing.md = 10;   // `.rail-body{gap:10px}`
    p.spacing.lg = 14;   // `.card.pad{padding:14px ...}`
    p.spacing.xl = 20;

    // The gap above a section heading in the rail. The reference leaves 39px
    // between the last row of one section and the heading of the next, and
    // 20px between a heading and its first row.
    // `.sb-sec{margin-bottom:11px}` above a caption, `.sb-head{padding-bottom:5px}`
    // below it.
    // `.sec-head{margin-bottom:11px}` -- the gap under a card heading.
    p.setRoleSpacing(SpacingRole::SectionGap, 11);
    // `.card.pad{padding:14px 15px}`
    p.setRoleSpacing(SpacingRole::CardPadding, 15);
    p.setRoleSpacing(SpacingRole::CardPaddingV, 14);

    // ── motion ───────────────────────────────────────────────────────────
    // --t-fast 110ms, --t-med 160ms, --t-slow 220ms.
    p.animation.fastMs   = 110;
    p.animation.normalMs = 160;
    p.animation.slowMs   = 220;

    // ── window chrome ────────────────────────────────────────────────────
    // --titlebar-h 44px, and the bar is a gradient from --titlebar-top to
    // --titlebar-bot rather than a flat fill.
    // `.sb-item.on{color:var(--text)}` -- this rail spends its accent on the
    // bar beside the row, so the label itself stays the primary ink.
    // `.sb-item{color:var(--text2)}` with `.on{color:var(--text)}`, and
    // `.sb-count{background:var(--card)}` with no outline of its own.
    p.controls.navRestInk = NavInk::Secondary;
    p.controls.navCurrentInk = NavInk::Primary;
    p.controls.navCountStyle = NavCountStyle::Filled;
    // `.tkids{border-left:1px solid var(--hair)}` -- this tree shows a level
    // with a rule down it, not with a mark beside every row.
    p.controls.navChildMark = NavChildMark::Guide;
    p.controls.navSectionBottom = 4;
    p.controls.smallButtonRadius = RadiusStep::Pill;

    // `.titlebar{padding:0 8px 0 13px}` -- the mockup's `.win` is a rounded
    // card inside a browser tab, not a real frame, so the trailing controls
    // need 8px clear of its simulated corner radius.
    p.window.titleBarTrailingInset = 8;
    p.window.titleBarPadding = 13;
    p.window.titleBarGap = 12;
    p.window.titleBarHeight   = 44;   // --titlebar-h
    p.window.statusBarHeight  = 26;   // --statusbar-h
    p.window.titleBarBg       = c(kTitlebarTop);
    p.window.titleBarBgBottom = c(kTitlebarBot);
    p.window.titleBarBgInactive = c(kWindow);
    p.window.titleText        = c(kText);
    p.window.titleTextInactive = c(kText4);
    p.window.border           = c(kHair);
    p.window.controlGlyph     = c(kText3);
    p.window.controlGlyphInactive = c(kText4);
    p.window.controlHoverBg   = c(kCard);
    p.window.controlPressedBg = c(kCardHover);

    // `.drow{height:32px; padding:0 10px}` -- the row states its height and
    // takes no vertical padding, so a cell's own content (the title cell
    // carries a badge) cannot push the row past it. TableView sizes a row as
    // max(rowHeight, content + 2 * TableCellPadding), and this profile had
    // never stated the padding, so it fell back to spacing.sm.
    p.setRoleSpacing(SpacingRole::TableCellPadding, 4);
    p.setRoleSpacing(SpacingRole::TableCellPaddingH, 10);
    p.table.rowHeight    = 32;   // `.drow{height:32px}`
    p.table.headerHeight = 28;
    p.table.headerBg     = c(kPanel);
    p.table.gridLine     = c(kHair);
    // `.drow:hover{background:var(--panel)}`
    p.table.hoverBg      = c(kPanel);
    // `.drow.on{background:rgba(var(--acc-rgb),.10)}` -- a selected row is
    // tinted with the accent, not filled with the hover grey. Flattened
    // against `--bg` because a table row paints an opaque fill.
    {
        const QColor accent = c(kAccent), ground = c(kBg);
        p.table.selectionBg = QColor(
            qRound(ground.red()   * 0.90 + accent.red()   * 0.10),
            qRound(ground.green() * 0.90 + accent.green() * 0.10),
            qRound(ground.blue()  * 0.90 + accent.blue()  * 0.10));
    }

    p.chart.grid = c(kHair);
    p.chart.axis = c(kText4);

    // Fills in whatever is derived rather than stated — the semantic
    // palette above all, which comes from the four hues.
    applyDerivedTokens(p);
    p.colors.inputBorder=p.colors.borderHover;
    p.colors.inputHoverBorder=p.colors.textDisabled;
    p.colors.inputFocusBg=p.colors.panelBgAlt;
    p.colors.inputInk=p.colors.textSecondary;
    return p;
}

// ── the light palette ────────────────────────────────────────────────────
//
// `spec.TOKENS_LIGHT` in the reference: the same token names, the second set
// of values. Only the colours differ - every metric, radius, font and
// duration is shared, which is what makes this a theme rather than a second
// design.
ThemeProfile novuAiLightProfile()
{
    ThemeProfile p = novuAiProfile();
    p.id = QString::fromLatin1(kLightProfileId);
    p.displayName = QStringLiteral("NovuAI Light");

    p.colors.windowBg      = c("#E6E9ED");   // --window
    p.colors.panelBg       = c("#EEF0F3");   // --bg
    p.colors.panelBgAlt    = c("#F3F5F7");   // --panel
    p.colors.headerBg      = c("#F3F5F7");   // --titlebar-top
    p.colors.surfaceHover  = c("#E4E8EC");   // --card
    p.colors.surfaceActive = c("#DCE1E6");   // --card-h

    p.colors.borderSubtle = c("#D9DEE4");    // --hair
    p.colors.border       = c("#D9DEE4");    // --hair
    p.colors.borderHover  = c("#C3CBD3");    // --hair2

    p.colors.textPrimary   = c("#1A1F26");   // --text
    p.colors.textSecondary = c("#3A4450");   // --text2
    p.colors.textTertiary  = c("#68727E");   // --text3
    p.colors.textDisabled  = c("#8A94A0");   // --text4
    p.colors.textOnAccent  = c("#F5F8FA");   // --ink-on-accent

    p.colors.accent      = c("#2F7FD6");     // --acc
    p.colors.accentLight = c("#1F66B4");     // --acc-l

    p.colors.success = c("#2FA866");         // --green
    p.colors.warning = c("#C87F22");         // --orange
    p.colors.danger  = c("#A85F6A");         // --pink
    p.colors.info    = c("#2F7FD6");

    p.window.titleBarBg         = c("#F3F5F7");
    p.window.titleBarBgBottom   = c("#E6E9ED");
    p.window.titleBarBgInactive = c("#E6E9ED");
    p.window.titleText          = c("#1A1F26");
    p.window.titleTextInactive  = c("#8A94A0");
    p.window.border             = c("#D9DEE4");
    p.window.controlGlyph       = c("#68727E");
    p.window.controlGlyphInactive = c("#8A94A0");
    p.window.controlHoverBg     = c("#E4E8EC");
    p.window.controlPressedBg   = c("#DCE1E6");

    p.table.headerBg    = c("#F3F5F7");
    p.table.gridLine    = c("#D9DEE4");
    p.table.hoverBg     = c("#E4E8EC");
    p.table.selectionBg = c("#DCE1E6");

    p.chart.grid = c("#D9DEE4");
    p.chart.axis = c("#8A94A0");

    applyDerivedTokens(p);
    p.colors.inputBorder=p.colors.borderHover;
    p.colors.inputHoverBorder=p.colors.textDisabled;
    p.colors.inputFocusBg=p.colors.panelBgAlt;
    p.colors.inputInk=p.colors.textSecondary;
    return p;
}

} // namespace novu::ui
