#pragma once

#include "novu/ui/Export.h"

#include <QObject>

class QColor;
class QPainter;
class QRectF;

class QWidget;

namespace novu::ui {

// The one clock every looping animation in the library runs on.
//
// The approved designs animate nine things, and four of them loop forever: the
// ring travelling round a primary button, a status dot breathing, a spinner,
// and a skeleton's shimmer. Given a QTimer each, a window with a toolbar, a
// status bar and a loading table wakes the CPU on four unrelated schedules and
// repaints four times as often as it needs to.
//
// So there is one timer. Widgets subscribe, it starts; the last one goes away,
// it stops. Nothing is animated when nothing is on screen.
//
// Ported from the NovuHub prototype's `Pulse`
// (g:\Design\prototypes\qt6\NovuHub\novuhub_brand_ui.py), which is the
// already-approved Qt translation of the same CSS keyframes — same interval,
// same phase arithmetic. It is here rather than there because the motion
// belongs to the components, and a second copy of it is a second thing to keep
// in step.
class NOVUUI_API Pulse : public QObject
{
    Q_OBJECT

public:
    static Pulse& instance();

    // Repaints `widget` on every tick for as long as it lives and is visible.
    // Idempotent: subscribing twice costs nothing and releases once.
    void subscribe(QWidget* widget);
    void release(QWidget* widget);

    bool isRunning() const;
    int subscriberCount() const;

signals:
    // For a component that has to do more on a tick than repaint — a spinner
    // that re-renders its glyph, say. Repainting alone needs only subscribe().
    void tick();

private:
    Pulse();

    struct Private;
    Private* d;
};

namespace motion {

// Where a loop of `periodMs` stands right now, as 0..1. Read from the monotonic
// clock rather than counted in ticks, so two widgets that subscribed seconds
// apart are in phase and a dropped frame does not accumulate drift.
double phase(int periodMs);

// The design's `softglow`: a value easing between `low` and `high` and back
// once per period. A dot that breathes, not one that blinks.
double breathe(int periodMs, double low = 0.55, double high = 1.0);

// False when the widget's profile has switched motion off. Every animated
// component asks before subscribing, so "reduced motion" is one token rather
// than a flag threaded through every constructor.
bool enabled(const QWidget* context);

// The running light: a mostly invisible ring with one bright arc travelling
// round it.
//
// It lived inside Button, which is where the design first asked for it — but
// the motion is not a property of buttons. A card that is connecting, a status
// tile that is syncing and a tool window that is working all want the same arc
// on their own outline, and each one writing the gradient again is four
// chances for the stops to drift apart.
//
// So the arithmetic is here, once: the same stop positions and the same
// counter-rotation the approved NovuHub prototype settled on, over the shared
// Pulse clock so every ring on screen travels in phase.
//
// `accent` is resolved by the caller from its own profile — the primitive
// takes a colour, never a token, so it cannot pin a theme. `periodMs` is the
// profile's ring period. When motion is off the arc is drawn frozen at the
// start of its sweep rather than omitted, because the ring is part of the
// shape and an outline that vanishes reads as a missing border.
void paintRunningRing(QPainter* painter,
                      const QRectF& outline,
                      const QColor& accent,
                      double strokeWidth,
                      double radius,
                      int periodMs,
                      bool animated);

// The busy ring: a faint circle with one bright quarter turning round it. The
// design's `.sp` — `border:1.7px solid hair2; border-top-color:acc-l;
// animation:spin`.
//
// Here rather than in a Spinner widget because the same mark appears inside a
// button that is submitting, on a status line that is working, and in a cell of
// a table that is still loading. Two of those three have no business owning a
// widget, so the drawing is a function and Spinner is the one caller that also
// wants a box in a layout.
//
// Frozen at twelve o'clock when motion is off, for the reason the running ring
// is: the mark is the shape, and a missing one reads as a layout fault.
void paintSpinner(QPainter* painter,
                  const QRectF& box,
                  const QColor& track,
                  const QColor& head,
                  double strokeWidth,
                  int periodMs,
                  bool animated);

// The design's `shimmer`: a light band sweeping left across a placeholder
// plate. `background-size:200%` and a position animated to -200% is a gradient
// twice the width of the box, moved by one box width per period — which is what
// this draws.
//
// Takes the two plate colours rather than tokens, like every other primitive
// here, so it cannot pin a theme. `radius` is applied to the plate; pass the
// profile's own.
void paintShimmer(QPainter* painter,
                  const QRectF& box,
                  double radius,
                  const QColor& base,
                  const QColor& highlight,
                  int periodMs,
                  bool animated);

} // namespace motion

} // namespace novu::ui
