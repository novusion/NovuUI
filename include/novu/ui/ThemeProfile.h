#pragma once

#include "novu/ui/Theme.h"

#include <QColor>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

// Win32 defines `small` as a macro.
//
// `<rpcndr.h>`, which arrives with `windows.h`, contains `#define small char`.
// RadiusScale and IconMetrics below both have a member called `small`, so any
// Novusion application that reaches a Windows header before this one stops
// compiling with "'int' followed by 'char' is illegal" - which names neither
// the macro nor the member and sends people looking in the wrong file.
//
// Undefining it here is the standard remedy and costs nothing: no Novusion
// code uses the macro, and a translation unit that genuinely wants RPC's
// `small` can include <rpcndr.h> after this header.
#ifdef small
#undef small
#endif

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
    // A button that is only its glyph. The approved designs shape the two
    // differently -- `.btn{border-radius:var(--r-pill)}` beside
    // `.ibtn{border-radius:var(--r-sm)}` -- because a pill the size of an
    // icon is a circle, and a row of circles in a title bar reads as a
    // traffic light. A profile that says nothing here gets Button, so this
    // role changes no existing one.
    //
    // This is `.nb` - the chrome ghost icon, no plate until hover - and it is
    // the IconButton radius a profile states unless it also states
    // ActionIcon below. Kept as the general-purpose role for exactly that
    // reason: adding ActionIcon must not move every icon button that had
    // nothing to do with it.
    IconButton,
    // `.btn.ico` - an icon-only *action* button, `IconButton::Variant::Outlined`
    // with `setActionPlate(true)`: the hairline plate a table row, a card
    // header or a toolbar reaches for. Separate from IconButton above because
    // the two approved radii are one pixel apart and genuinely different
    // (`.nb{border-radius:7px}` against `.btn.ico{border-radius:8px}`) - not
    // a rounding choice, the two classes in the same sheet disagree. Falls
    // back to IconButton when a profile says nothing, so adding this role
    // changed no existing shape until a profile states it.
    ActionIcon,
    Input,
    // A closed select. Shaped apart from Input for the same reason: the
    // designs draw a select as a pill and a text field as a rounded box, and
    // one role cannot be both. Falls back to Input when unstated.
    Select,
    Badge,
    Toolbar,
    Table,
    Overlay,
    // The frame of a top-level application window. Separate from Dialog
    // because a maximisable main window and a small confirm dialog are allowed
    // to differ - but only as two roles a profile decides, never as two
    // numbers two applications picked.
    Window,
    // A window the application draws inside its own: the design's `.floatwin`.
    // It is neither the frame of the application nor a modal - it is smaller
    // than one and less final than the other, and the approved design shapes
    // it accordingly (13 against the window's 16 and the modal's 15). Falls
    // back to Dialog when a profile says nothing, so adding it changed no
    // existing shape.
    FloatingWindow,
    // One chip inside a `Segmented` tray - `.chip{border-radius:7px}` sitting
    // inside `.chipset{border-radius:10px}` (Select). A tray and the plate
    // lifted out of it are drawn at two different radii in the approved
    // sheet; a segment that reused the tray's own Select radius came out a
    // near-pill on a ~26-30px-tall control, not the crisp small chip the
    // design draws. Falls back to Select when a profile says nothing.
    Segment,
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
    // A table cell's own left and right padding, when the design states it
    // apart from the vertical one - `td{padding:9px 13px}`. Added last and
    // read with the vertical value as its fallback, so a profile that never
    // states it keeps the single padding it always had.
    TableCellPaddingH,
    // The same split for a card body: `.card-body{padding:15px 16px}` sets
    // the two apart, and CardPadding alone made every card a pixel taller
    // than the design draws it. Falls back to CardPadding when a profile
    // does not state it.
    CardPaddingV,
    // The gap between two blocks *inside* one card body.
    //
    // It used to borrow SectionGap, which NovuSpace states as 18 for
    // `.rail-sec{margin-top:18px}` - a rail measurement. Every card in the
    // application was then laid out at the sidebar's rhythm and read loose.
    // Falls back to SectionGap, so a profile that never states it is
    // unchanged.
    CardBodyGap,
    // `.page-body{padding:16px 20px 22px}` - the page's own inset is three
    // different numbers, not one. The top is a step tighter because the page
    // header is already above it, and the bottom is a step looser so the last
    // card does not sit on the status bar. Both fall back to PagePadding.
    PagePaddingTop,
    PagePaddingBottom,
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

    // The third family: the one titles are set in.
    //
    // The approved designs use two sans faces, not one — a display face for
    // window, page and card titles and a text face for everything else. With
    // one family a title is the body font made bigger, which is the single
    // most visible difference between the mockups and the current windows.
    //
    // Empty means "the same as `family`", so a profile that has only one face
    // says nothing and nothing changes for it.
    QString displayFamily;

    // What to fall back to when the named family is not installed.
    //
    // Qt's own substitution picks by script coverage, and on a machine without
    // Inter it lands on whatever the system's default sans is — which on
    // Windows is not Segoe UI. Naming the fallback keeps an un-provisioned
    // developer machine and a deployed one the same shape rather than two.
    // Every family above is tried first, then these, in order.
    QStringList fallbackFamilies = {QStringLiteral("Segoe UI"),
                                    QStringLiteral("Noto Sans"),
                                    QStringLiteral("Arial")};
    QStringList monoFallbackFamilies = {QStringLiteral("Cascadia Mono"),
                                        QStringLiteral("Consolas"),
                                        QStringLiteral("Courier New")};
    // Use natural outlines and advances instead of platform grid fitting.
    bool unhintedMetrics = false;
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
    // The uppercase, letter-spaced line that names a group: the caption over a
    // row of controls, a table's column head, a menu section, the breadcrumb
    // in front of a page title.
    //
    // Separate from Label because the two look nothing alike and mean
    // different things — Label is the name of one field, sentence case beside
    // its control; Eyebrow is the name of a *group*, several steps smaller and
    // set in the monospace face. They were one role, and the result was every
    // caption in the library being a small bold body font while the approved
    // design drew a tracked mono line. Declared last so the existing
    // enumerators keep their values.
    Eyebrow,
    // The window's own bottom strip. Not Body: a status strip is read at a
    // glance and never read closely, and every approved design sets it a step
    // or two under the body size -- NovuAI's `.statusbar{font-size:var(--fs-sm)}`
    // against a 13px body. A profile that does not state it falls back to Body,
    // so the role changes nothing until it is asked for. Declared last so the
    // existing enumerators keep their values.
    StatusBar,
};

struct TypeStyle
{
    TypeStyle(qreal pixels = 13, int fontWeight = 400, bool mono = false,
              double leading = 0.0, double tracking = 0.0, bool caps = false)
        : size(pixels), weight(fontWeight), monospace(mono),
          lineHeight(leading), letterSpacing(tracking), uppercase(caps) {}
    TypeStyle(int pixels, int fontWeight = 400, bool mono = false,
              double leading = 0.0, double tracking = 0.0, bool caps = false)
        : TypeStyle(static_cast<qreal>(pixels), fontWeight, mono, leading, tracking, caps) {}
    qreal size = 13;
    int weight = 400;   // QFont::Weight numeric
    bool monospace = false;

    // The three properties the approved CSS states per role and a font alone
    // cannot carry. Declared after the first three so every existing
    // positional initialiser keeps meaning what it meant.
    //
    // lineHeight is the CSS multiple (`line-height:1.05`), 0 meaning "the
    // font's own" - which is what a QLabel does by default and is taller than
    // a display face laid out to a stated line box. letterSpacing is in em,
    // as the CSS states it (`letter-spacing:.06em`, `-.02em`). uppercase is
    // `text-transform:uppercase`, which belongs to the role rather than to
    // every call site that has to remember to call toUpper().
    double lineHeight = 0.0;
    double letterSpacing = 0.0;
    bool uppercase = false;
};

// Control geometry that must not stagger within a toolbar.
// The inks a navigation rail uses, named rather than numbered. The approved
// designs disagree about both of them and both are right for their own rail:
// NovuSpace sets `.navbtn{color:var(--text3)}` with `.on{color:var(--acc-l)}`,
// NovuAI sets `.sb-item{color:var(--text2)}` with `.on{color:var(--text)}` --
// a rail that has already spent its accent on the bar beside the row does not
// spend it again on the label. A component cannot choose; a profile can.
enum class NavInk { Muted, Secondary, Primary, Accent };

// `.cnt` is an outline over the window in one design and `.sb-count` a filled
// chip in the other.
enum class NavCountStyle { Outlined, Filled };

// How a nested row shows that it is nested. `.navbtn.child .dot` puts a mark
// beside each child; `.tkids{border-left:1px solid var(--hair)}` runs one rule
// down the whole level instead. A rule reads as a level and a dot reads as a
// row, which is why the two designs chose differently.
enum class NavChildMark { Dot, Guide };

struct ControlMetrics
{
    int height = 32;
    int heightSmall = 26;
    // A row in a table or a list.
    int rowHeight = 34;
    // A row in a navigation rail, which is a different measurement: the
    // approved NovuSpace design draws `.navbtn` at 30 and its table rows at
    // 34, and NovuAI draws its module rows at 27. NavList reads this.
    int navRowHeight = 30;

    // The ink a current navigation row takes, and the gap under a caption.
    //
    // The approved designs disagree, and both are right for their own rail:
    // NovuSpace's `.navbtn.on{color:var(--acc-l)}` names the accent, because
    // its rail is the only accented thing on the page; NovuAI's
    // `.sb-item.on{color:var(--text)}` names the primary ink, because its
    // accent is already spent on the active bar beside the row. A component
    // cannot choose between them -- a profile can.
    NavInk navRestInk = NavInk::Muted;
    NavInk navCurrentInk = NavInk::Accent;
    NavCountStyle navCountStyle = NavCountStyle::Outlined;

    // The ink on a count pill that is only a count, on a row that is not the
    // current one. The same disagreement as navRestInk, one level down: the
    // approved designs draw `.cnt` as a muted chip, which is right where the
    // rail's counts are incidental, and NovuAdmin's rail is the other case --
    // the counts ARE the rail's content, one per administered collection, and
    // they carry which of the two servers is being administered. So the same
    // answer: a component cannot choose, a profile can.
    //
    // Only the ink. The pill's outline and fill stay neutral, which is what
    // keeps a current row's pill (accent outline, no fill) still reading as
    // the current one. And only CountTone::Normal -- an Alert or Bad count
    // means something, and a product accent does not get to overrule it.
    NavInk navCountInk = NavInk::Muted;
    NavChildMark navChildMark = NavChildMark::Dot;
    // `.sb-head{padding-bottom:5px}` against NovuSpace's own step. The gap
    // above a caption is SpacingRole::SectionGap; this is the one below it.
    int navSectionBottom = 8;

    // One nesting step, for every tree in the application.
    //
    // A token because it had already drifted: NovuAI's own sheet records that
    // its workspace tree, Coding's files and UI Designer's structure had each
    // grown a step of their own - 17, 20 and 13 - before `--tree-step` was
    // introduced to collapse them. TreeView had a fourth, a hard-coded 16.
    //
    // 17 is the only value any authority states.
    int treeStep = 17;
    // Compact buttons may keep the product pill or use a smaller corner.
    RadiusStep smallButtonRadius = RadiusStep::Small;
    int segmentedPadding=3;
    int segmentedHeight=0; // zero keeps content-derived height
    int buttonPadding=13;
    int buttonPaddingSmall=8;
    int buttonPaddingLarge=13;
    bool buttonPaddingIncludesBorder=true;
    // Negative retains the muted disabled palette; otherwise composite the
    // entire ordinary button at this opacity, including its surface.
    qreal buttonDisabledOpacity=-1;
    bool scrollThumbOnAreaHover=false;
    int scrollThumbInset=0;
    // `.select` and `.select.xs`. SelectButton falls back to the button
    // heights when a profile leaves these at zero, which made `.select.xs`
    // come out at the small BUTTON height - taller than the 23px the design
    // gives it, and taller than StatusBar already documents it as being.
    // Stated here so the component is the height its own header says it is
    // rather than borrowing a button's.
    int selectHeight=0,selectSmallHeight=23;
    int iconHeight=0,iconSmallHeight=0;

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

    // The loops. A transition has a duration because something changed; these
    // have a period because nothing did — they run for as long as the thing
    // they belong to is on screen, so they are a different kind of number and
    // are named separately.
    //
    // Values are the approved design's CSS keyframes, in order: rlspin, the
    // ring travelling round a primary button; softglow, a live status dot
    // breathing; spin, a busy indicator; shimmer, a skeleton's sweep.
    int ringMs = 7000;
    int pulseMs = 2200;
    int spinMs = 900;
    int shimmerMs = 1300;
};

// Icon sizing, so no component invents a pixel size for a glyph.
struct IconMetrics
{
    int small = 14;
    int medium = 16;
    int large = 20;
    QString family; // empty keeps the shared default outlines
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
    // The approved NovuSpace design's `.titlebar` and `.winbtn`. Systemic
    // rather than per profile: controlGeometryIsIdenticalAcrossProfiles() in
    // the tests is what stops a fourth application inventing a fourth size.
    // 44, which is what three of the four authoritative mockups state -
    // NovuAI, NovuAdmin and NovuExplorer. It was 42 here, which is NovuSpace's,
    // and NovuSpace is the outlier: see docs/DESIGN_CONTRACT.md, where the 2px
    // is recorded as unreconciled rather than quietly kept or quietly dropped.
    // Its own token header says it lifts chrome from the other three.
    int titleBarHeight = 44;
    int controlWidth = 30;
    // The status bar, which - unlike the title bar - the design does vary per
    // product and states as such: NovuSpace 32, NovuAI 26, NovuExplorer 24.
    // StatusBar had 32 compiled in, so the other two could not have it.
    int statusBarHeight = 32;
    int controlHeight = 24;

    // Leading inset before the window icon, and the gap between icon, title
    // and any trailing widget an application adds.
    int titleBarPadding = 12;
    int titleBarGap = 8;
    // The gap between the window mark and the application's own leading
    // widget, when that pair should read tighter than the rest of the bar.
    //
    // A logo with the product's name beside it is one block; the bar's other
    // neighbours - a caption, a toolbar button, a window control - are
    // separate things and want the wider gap. QHBoxLayout has one spacing for
    // the whole row, so the pair gets a row of its own.
    //
    // -1 means "no opinion": the pair is spaced like everything else and the
    // bar lays out exactly as it did before this existed. A profile opts in
    // by setting a value.
    int brandGap = -1;
    // The trailing edge past the last control, before the window's own edge -
    // the same clearance titleBarPadding gives the icon on the other side, so
    // Close is never flush against the window's own outer edge. Was zero for
    // every profile but NovuAI's, on the Fitts's-law argument that a
    // maximised window wants Close reachable by slamming the pointer into the
    // screen corner; in practice every WindowShell here draws its own frame
    // rather than deferring to real OS chrome, and a Close sitting one
    // hairline off the edge simply read as cramped rather than as an
    // optimised target. One authoritative inset for every WindowShell kind
    // and state now, set once here rather than by any one profile or sample.
    int titleBarTrailingInset = 8;

    int borderWidth = 1;
    // The grab band along the frame edge, in logical pixels. Hit-testing is
    // done by the shell, so a resizable window and a fixed dialog differ by a
    // flag rather than by two different event filters.
    int resizeMargin = 6;

    int iconSize = 18;
    // The bounding box of a control's glyph, not its stroke length. Minimize
    // draws one line across it, Maximize a square inside it, Close a cross
    // through it, so all three read at the same visual weight.
    int glyphSize = 11;
    int glyphStroke = 1;

    // The bar is a vertical gradient in the design, not a flat field:
    // --titlebar-top to --titlebar-bot. Two tokens rather than one, because a
    // profile that wants a flat bar simply sets them equal.
    QColor titleBarBg{QStringLiteral("#1A1F25")};
    QColor titleBarBgBottom{QStringLiteral("#151A20")};
    // The rail runs its own gradient, and the design states it separately
    // from the title bar's: `--rail-top:#1A1F26` / `--rail-bot:#161B21`,
    // where the bar ends on `--titlebar-bot:#151A20`. One pixel-value apart
    // and deliberately so - the rail does not fade quite as far as the frame.
    QColor railBg{QStringLiteral("#1A1F26")};
    QColor railBgBottom{QStringLiteral("#161B21")};
    QColor titleBarBgInactive{QStringLiteral("#161A1F")};
    QColor titleText{QStringLiteral("#E3E8F0")};
    QColor titleTextInactive{QStringLiteral("#6E7A8E")};
    QColor border{QStringLiteral("#232830")};

    // `.titlebar{border-bottom:1px solid var(--hair2)}` - the rule between two
    // surfaces, which is the heavier hairline. Separate from `border` above,
    // which is the window's own outer edge; derived from the palette, so a
    // profile that says nothing still gets it right.
    QColor titleBarBorder;

    // Neutral control states, shared by Minimize and Maximize/Restore.
    QColor controlHoverBg{QStringLiteral("#2A3038")};
    QColor controlPressedBg{QStringLiteral("#2F353F")};
    QColor controlGlyph{QStringLiteral("#A8B2C3")};
    QColor controlGlyphInactive{QStringLiteral("#6E7A8E")};

    // `.winbtn{border-radius:6px}` - the painted hover box of a title bar
    // control is a small rounded square around the glyph, not the full height
    // of the bar and not a hard-edged block.
    int controlRadius = 6;

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
    QHash<int, int> roleRadiusPx;  // SurfaceRole  -> pixels, exactly

    int radius(SurfaceRole role) const
    {
        // A role may state its radius outright, in pixels.
        //
        // The step scale is the right shape for a *system*: five steps, and a
        // product decides what each one is worth. It is the wrong shape for
        // this particular design, which draws eight distinct radii - 8 on a
        // small button, 10 on a button, 11 on a note, 12 on a popover, 13 on a
        // floating window, 14 on a card, 15 on a modal, 16 on the frame - and
        // deliberately: those surfaces sit inside one another, and the
        // difference between 13 and 16 is what keeps a window inside a window
        // from reading as the same box twice. Forced onto five steps, four of
        // the eight were one to three pixels wrong.
        //
        // So a profile may pin a role exactly. The steps stay, and stay the
        // default: this is the exception, stated per role, not a replacement.
        const auto exact = roleRadiusPx.constFind(static_cast<int>(role));
        if (exact != roleRadiusPx.constEnd()) {
            return exact.value();
        }
        const auto it = roleRadius.constFind(static_cast<int>(role));
        if (it != roleRadius.constEnd()) {
            return radii.value(static_cast<RadiusStep>(it.value()));
        }
        // A role a profile has not spoken about falls back to the one it
        // refines, never to a bare default. That is what lets a role be added
        // without every existing profile changing shape the day it appears.
        switch (role) {
        case SurfaceRole::IconButton:    return radius(SurfaceRole::Button);
        case SurfaceRole::ActionIcon:    return radius(SurfaceRole::IconButton);
        case SurfaceRole::Select:        return radius(SurfaceRole::Input);
        case SurfaceRole::FloatingWindow: return radius(SurfaceRole::Dialog);
        case SurfaceRole::Segment:       return radius(SurfaceRole::Select);
        default:                         return radii.value(RadiusStep::Small);
        }
    }

    int space(SpacingRole role) const
    {
        return roleSpacing.value(static_cast<int>(role), spacing.md);
    }

    TypeStyle text(TypeRole role) const
    {
        const auto it = roleType.constFind(static_cast<int>(role));
        if (it != roleType.constEnd()) {
            return it.value();
        }
        // A role a profile has not stated falls back to the one it refines,
        // so a role can be added without every profile changing the day it
        // appears.
        if (role == TypeRole::StatusBar) {
            return text(TypeRole::Body);
        }
        return TypeStyle{type.md, 400, false};
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
    // The exception to the step scale: this role is exactly this many pixels.
    // See radius() for when that is the right thing to say.
    void setRoleRadiusPx(SurfaceRole role, int px)
    {
        roleRadiusPx.insert(static_cast<int>(role), px);
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
// The family baseline: the surfaces, text, status and accent of
// `design/Shared/design-system/Novu_Palette.png`, which is the approved
// palette for the Novusion family as a whole.
//
// Every application profile is built from this. What an application may
// override is its **accent**, and only its accent - the surfaces a product
// sits on and the colour of the word "failed" are the family's, not the
// product's.
ThemeProfile novusionBaseProfile();

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
ThemeProfile novuAiLightProfile();   // the same table, light
ThemeProfile novuVoiceProfile();
ThemeProfile novuExplorerProfile();
ThemeProfile novuHubProfile();
// NovuAdmin consumes this library today and is the reference implementation
// for the dialog and form patterns, so it needs an identity like any other
// product - a subtree has to be scopeable to it, and the gallery has to be
// able to answer "which components does NovuAdmin use".
ThemeProfile novuAdminProfile();

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
