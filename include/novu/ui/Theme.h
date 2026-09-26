#pragma once

#include <QColor>
#include <QPoint>
#include <QList>
#include <QString>

class QWidget;

class QPainter;

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
    // Optional third ink. Invalid preserves the existing muted fallback.
    QColor textTertiary;

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
    QColor success{QStringLiteral("#3CCB7F")};
    QColor warning{QStringLiteral("#FFB14D")};
    QColor danger{QStringLiteral("#C47B85")};
    // Info used to have no colour at all: every component that needed one
    // reached for accentLight, which is the *application* accent. An Info badge
    // was therefore sky blue in NovuTrade, gold in Warm Gray and violet in Dark
    // Purple - the same status wearing three identities.
    QColor info{QStringLiteral("#4DA3FF")};

    // The plate each status is tinted from. It stays a separate token because
    // a light profile may darken its ink for contrast without changing the
    // plate family. See SemanticHue.
    QColor successPlate{QStringLiteral("#3CCB7F")};
    QColor warningPlate{QStringLiteral("#FFB14D")};
    QColor dangerPlate{QStringLiteral("#C47B85")};
    QColor infoPlate{QStringLiteral("#4DA3FF")};
    // ── declared last ────────────────────────────────────────────────────
    //
    // The rail's ground. Two designs, two shapes: NovuSpace runs the rail
    // between two surfaces over its whole height, which is what a zero fade
    // means here; NovuAI lays a tint over `windowBg` that is gone by
    // `railTintFade` pixels -- `.sidebar{background:linear-gradient(180deg,
    // rgba(0,0,0,.06),transparent 120px), var(--window)}`. Run over the whole
    // height instead, the middle of the rail comes out visibly lighter.
    //
    // At the end of the struct on purpose: the profiles initialise this one
    // positionally, so a field added anywhere else silently shifts every
    // colour after it.
    QColor railTint{QColor(0, 0, 0, 0)};
    int railTintFade = 0;
    // Optional input-specific tokens; invalid colours keep the legacy roles.
    QColor inputBorder,inputHoverBorder,inputFocusBg,inputInk;

};

// The status hues every Novusion application shares.
//
// Application accent and semantic status are two different layers. A product
// owns its background, its surfaces, its accent and its selection styling;
// it does not own what "failed" looks like. Success, Warning, Danger and Info
// mean the same thing in every Novu window, so they look the same in every
// Novu window.
//
// The values are the approved NovuSpace design's, adopted as the reference:
//
//     g:\Design\design\NovuSpace\mockups\NovuSpace-Trade-brand-mockup.html
//
// which states this layer outright - "trading semantics are a SEPARATE layer
// and never borrow the accent: green running, red error, amber warning, blue
// informational".
//
// The NovuAI reference is authoritative for the richer shared presentation:
// its `.tag.ok/.warn/.err` uses the saturated family hue as the ink and the
// plate source. Whatever else a product owns, it does not own what "failed"
// looks like, so this remains one shared semantic palette.
//
// They replaced NovuAI's darker set (#2EA043 / #D29922 / #F85149 / #58A6FF),
// which in turn had replaced four versions of the same four meanings - an olive
// #98C379 in the shell, a mint #3CCB7F in NovuVoice, a darker #16A34A in
// NovuTrade. The direction of travel has not changed: one status palette, and
// it is the approved design's.
// A status is two colours in the approved design, never one.
//
//     .tag.ok  { color:#3CCB7F; background:rgba(60,203,127,.10);
//                  border-color:rgba(60,203,127,.40); }
//
// Ink and plate stay independent so light profiles can choose a darker ink
// without changing the semantic tint. On the shared dark palette NovuAI uses
// the saturated family hue for both channels.
//
// The alphas belong here for the same reason: the design states them per role
// and they are not uniform. Danger sits heavier than Success on purpose - a
// pink plate at .10 disappears where a green one reads.
//
// The alphas stay within the small per-role range stated by the NovuAI tags.
// See docs/DESIGN_CONTRACT.md for the cross-product table.
struct SemanticHue
{
    QColor ink;
    QColor plate;
    double fillAlpha;
    double borderAlpha;
};

struct SemanticHues
{
    //                    ink        plate      fill  border
    SemanticHue success{QColor(QStringLiteral("#3CCB7F")),   // --green
                        QColor(QStringLiteral("#3CCB7F")),   // --green
                        0.10, 0.40};
    SemanticHue warning{QColor(QStringLiteral("#FFB14D")),   // --amber / --orange
                        QColor(QStringLiteral("#FFB14D")),
                        0.10, 0.40};
    SemanticHue danger{QColor(QStringLiteral("#C47B85")),    // --pink
                       QColor(QStringLiteral("#C47B85")),    // --pink
                       0.12, 0.45};
    SemanticHue info{QColor(QStringLiteral("#4DA3FF")),      // --blue
                     QColor(QStringLiteral("#4DA3FF")),
                     0.10, 0.38};
};

const SemanticHues& sharedSemanticHues();

// The family's named hues.
//
// Ten colours, stated as a named set by NovuExplorer's folder-colour picker
// and drawn as a ring by its colour wheel. Every product declares the ones it
// uses at exactly these values, which is what makes them shared - but they are
// shared *palette*, not shared *meaning*:
//
//     purple   NovuAI agents | NovuExplorer "Review" | NovuSpace's product mark
//     cyan     NovuAI knowledge | NovuExplorer "Test" | a NovuSpace chart series
//     yellow   declared by NovuAI and NovuExplorer, drawn by neither
//
// Three meanings for one hue is why these are not SemanticRoles. Promoting
// "agents" or "knowledge" into the shared palette would write one product's
// vocabulary into every other. A product assigns the meaning; the library only
// promises that Cyan is the same cyan everywhere.
//
// Rose, Amber, Green, Cyan, Blue and Purple double as the status and accent
// hues - the same set, used for a different purpose.
struct FamilyHue
{
    QString name;
    QColor color;
};

const QList<FamilyHue>& familyHues();

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
SemanticColor semanticColorFrom(const QColor& ink, const QColor& plate,
                                double fillAlpha, double borderAlpha);

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

// ── the active panel ───────────────────────────────────────────────────────

// The property an application sets on a panel to say whether it currently has
// the user's attention. Named once here so a panel, a list and a table cannot
// spell it three ways.
inline constexpr const char* activePanelProperty = "novuActivePanel";

// Whether a selected row in `widget` should be painted at full strength.
//
// Ported unchanged in behaviour from NovuExplorer's `selectionIsActive`, which
// is the answer to a problem every two-panel application has and most get
// wrong: a list that clears its selection when it loses focus forgets where the
// user was, and a list that keeps it at full strength gives a window two rows
// that both look current.
//
// So the selection stays and dims. True while the widget has focus, and also
// while the focus is somewhere else inside the same *panel* — a search box
// above a list is still that list's panel, and typing in it must not dim the
// row the user is about to act on.
//
// A widget with no panel ancestor — a dialog, a popover, a preview — is always
// active. That is deliberate: those have no sibling to be confused with, and
// the alternative is every dialog in the system drawing a dimmed selection.
bool selectionIsActive(const QWidget* widget);

// The property an application sets on an ancestor (its top-level window, or
// any narrower container) to say that nothing under it ever draws a focus
// ring — not the mouse's, which every conforming component already
// suppresses, and not the keyboard's either.
//
// This is an opt-out for the one application that asked, not a change to the
// design system's own default: the keyboard ring stays the library's answer
// everywhere this property is unset. NovuAdmin's navigation chrome decided a
// background-only keyboard indication was enough for it and does not want to
// carry that decision for every other product built on this library, so it
// declares the property on its own window rather than the default moving for
// everyone.
inline constexpr const char* focusRingSuppressedProperty = "novuFocusRingSuppressed";

// Whether `widget` or an ancestor has set focusRingSuppressedProperty. Every
// component that draws a focus ring — through FocusRing::visibleOn() or,
// for the few that predate it, its own `hasFocus()` check — asks this so the
// opt-out reaches every ring from the one place a caller sets it.
bool focusRingSuppressed(const QWidget* widget);

// ── the focus ring ──────────────────────────────────────────────────────
//
// Whether a widget should draw its keyboard focus outline *now*. This is CSS
// `:focus-visible`, not `:focus`, and the difference is the whole point.
//
// The design draws exactly one treatment on an active chrome control - a tint
// and the accent ink, `.modbtn.on{background:rgba(--rl,.17);color:--acc-l}` -
// and no outline anywhere. An outline on top of that is a second highlight
// saying the same thing twice, and on a clicked control it says it to someone
// who already knows where they clicked. A browser does not draw one there
// either: it rings a control reached with Tab and leaves a clicked one alone.
//
// So the ring is for the keyboard, and only the keyboard. Every component that
// draws one asks this rather than `hasFocus()`, so the rule is one rule.
//
// `noteFocusIn` and `noteKeyPress`/`noteMousePress` below are how a component
// keeps the state; a component that forgets to call them simply never shows a
// ring, which is the safe failure.
class FocusRing
{
public:
    // `:focus-visible`, stated precisely rather than by exclusion: only Tab
    // and Backtab are someone driving the control from the keyboard. A
    // window activating, a page being built and shown, a shortcut, a popup
    // closing - excluding only mouse and popup reasons treated every one of
    // those as "keyboard" too, so a control could open already wearing a
    // ring nobody had tabbed to yet.
    void noteFocusIn(Qt::FocusReason reason)
    {
        m_visible = reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason;
    }
    // Driving the control from the keyboard is exactly when the ring is for.
    void noteKeyPress() { m_visible = true; }
    void noteMousePress() { m_visible = false; }

    // Out of line: this header only forward-declares QWidget.
    bool visibleOn(const QWidget* widget) const;

private:
    bool m_visible = false;
};

// How far a selection is dimmed when its panel is not the active one. Used by
// every component that draws a selection, so two lists side by side dim by the
// same amount.
inline constexpr double inactiveSelectionOpacity = 0.55;

// The single theme instance every component reads from.
const ThemeColors& theme();
void setTheme(const ThemeColors& colors);

QString rgba(const QColor& color, double alpha);

// The same tint as a colour rather than as a string, for the components that
// hand it to a painter instead of to a style sheet. Without it every caller
// writes the same three-line copy-and-set-alpha dance.
QColor withAlpha(const QColor& color, double alpha);


// A hairline that lands on a pixel.
//
// A 1px rule stroked through an antialiased painter is centred on the pixel
// *boundary*, so it comes out as two rows at half strength: the design's
// `1px solid var(--hair)` renders as two rows of a colour halfway between the
// rule and the surface under it, and every divider in the product reads a
// shade too faint without any one of them looking obviously wrong.
//
// Filling the rectangle the rule occupies is the whole fix. Every divider in
// this library goes through here so the answer is stated once - and so that a
// component cannot quietly go back to stroking one.
//
// `from` and `to` are the ends of the rule; it must be horizontal or vertical,
// which every divider in the design is.
void paintHairline(QPainter* painter, const QPoint& from, const QPoint& to,
                   const QColor& colour, int weight = 1);

} // namespace novu::ui
