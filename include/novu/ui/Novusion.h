#pragma once

// =============================================================================
// Novusion presets - the approved look, as configuration rather than classes
// =============================================================================
// The Novusion design is built almost entirely out of components this library
// already has. Its rail is Cards with particular metrics; its rows are Cards
// with the header off and a fixed height; its key/value lines are two labels in
// a box. Almost nothing in it is a new widget.
//
// So almost nothing here is one. These are presets and small compositions over
// existing components, and that is deliberate: a subclass per visual variant is
// how a component library ends up with forty classes that differ by a margin.
//
// WHY THEY ARE HERE RATHER THAN IN THE APPLICATION
//
// Because they were already in two applications' worth of places, disagreeing.
// The frozen NovuAI baseline has two railCard() definitions - one in its module
// view, one in its help window - with different header metrics, and every card
// built by the second is a few pixels off the design. One canonical preset is
// the whole point.
//
// EVERY VALUE COMES FROM THE PROFILE OR FROM THE DESIGN
//
// Metrics that the approved token sheet states are named constants below, with
// the CSS rule they came from. Everything else - colour, ink, type - is read
// from the active ThemeProfile through the usual helpers, so a preset applied
// under a different profile is that profile's, not NovuAI's.
//
// WHAT IS NOT HERE
//
// Helpers the baseline uses once or twice, helpers that exist to shape one
// screen's sample data, and the painted classes (a task state mark, a tinted
// check row, a block header surface). Those are not vocabulary yet. They can
// arrive with the first module that needs them, which is also when it will be
// clear whether they are one thing or three.
// =============================================================================

#include "novu/ui/Card.h"
#include "novu/ui/StatusDot.h"

#include <QMargins>
#include <QString>

class QLabel;
class QVBoxLayout;
class QWidget;

namespace novu::ui::novusion {

// ── rail metrics ─────────────────────────────────────────────────────────────
// The approved sheet's own numbers. Named so a caller that needs to line
// something up beside a rail card does not measure it by eye.
inline constexpr int kRailPad       = 12;   // left/right inside a rail card
inline constexpr int kRailPadV      = 10;   // top/bottom inside a rail card
inline constexpr int kRailTitleGap  = 9;    // heading to first block
inline constexpr int kRailBlockGap  = 8;    // block to block inside a card
inline constexpr int kRailRowHeight = 26;   // `.erow{height:26px}`
inline constexpr int kRailRowPad    = 8;
inline constexpr int kRailRowGap    = 8;

// ── surfaces ─────────────────────────────────────────────────────────────────

// A rail card: the block a right-hand panel is made of.
//
// `rows = true` removes the gap between children, for a card whose body is a
// list of railRow()s that should touch.
//
// No header divider. NovuUI draws one by default because the designs it was
// first written against have `.card-head{border-bottom:1px solid var(--hair)}`;
// Novusion's rail does not, and every rail card in the port was wearing a rule
// the reference has not got.
Card* railCard(QWidget* parent, const QString& title = QString(), bool rows = false);

// A card on the window ground rather than the panel one, with the roomier
// padding the approved `.card.pad{padding:14px 15px}` states. What a module's
// main column is built from.
Card* workspaceCard(QWidget* parent, const QString& title = QString());

// The step below a rail card and above its hover: a bordered box for a group
// inside a card. Medium radius, not large - a nested box wearing a card's own
// radius reads as a second card rather than as an item.
Card* nestedBox(QWidget* parent, const QMargins& padding = QMargins(9, 7, 9, 7),
                int spacing = 6);

// One line in a rail card's list: an optional mark, an eliding name, and an
// optional trailing note in the small uppercase mono face. Interactive and
// hover-lit, because in the design these are things you click.
Card* railRow(QWidget* parent, QWidget* mark, const QString& text,
              const QString& trailing = QString(), bool monoText = false);

// ── lines inside a card ──────────────────────────────────────────────────────
// These add to a layout rather than returning a widget: every one of them is
// used as a statement in a sequence, and handing back something the caller has
// to remember to add is how a row goes missing.

// Caption above, value below, the value wrapping into whatever width it has.
// The rail's workhorse - the most-used piece of this vocabulary by a wide
// margin.
void propertyRow(QVBoxLayout* into, const QString& caption, const QString& value);

// A fixed-width uppercase mono key beside its value, for metadata that should
// line up down the column. `grid-template-columns:78px 1fr` in the reference.
void infoRow(QVBoxLayout* into, const QString& caption, const QString& value,
             bool monoValue = false);

// The small uppercase caption that separates sections down a sidebar.
QLabel* sideSection(QVBoxLayout* into, const QString& caption);

// ── small marks ──────────────────────────────────────────────────────────────

// A count beside a toolbar control, in the profile's smallest mono face.
QLabel* toolbarCount(const QString& text, QWidget* parent);

// A status dot narrowed for a dense row, where the default one's width is a
// gap rather than a mark.
StatusDot* denseDot(StatusDot::Tone tone, QWidget* parent);

} // namespace novu::ui::novusion
