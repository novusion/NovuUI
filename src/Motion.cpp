#include "novu/ui/Motion.h"
#include <QLinearGradient>
#include <QColor>
#include <QRectF>
#include <QPen>
#include <QPainter>
#include <QConicalGradient>

#include "novu/ui/ThemeManager.h"

#include <QElapsedTimer>
#include <functional>
#include <QEvent>
#include <QRegion>
#include <QVariant>
#include <QGuiApplication>
#include <QSet>
#include <QTimer>
#include <QWidget>
#include <QtMath>

namespace novu::ui {

namespace {

// 30fps. The slowest loop in the design turns once every seven seconds and the
// fastest once every 0.7, so nothing here needs more, and asking for 60 doubles
// the wake-ups for motion no one can see.
constexpr int kIntervalMs = 33;

// One monotonic origin for the whole process, so every loop of the same period
// is in phase no matter when its widget appeared.
QElapsedTimer& clock()
{
    static QElapsedTimer timer = [] {
        QElapsedTimer t;
        t.start();
        return t;
    }();
    return timer;
}

// Tells the clock when a subscriber appears or disappears - itself or with a
// parent (a page of a stack, a hidden module, a closed window), which Qt
// reports to the child as its own Show/Hide.
class VisibilityWatch final : public QObject
{
public:
    explicit VisibilityWatch(std::function<void()> changed) : m_changed(std::move(changed)) {}

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::Show || event->type() == QEvent::Hide) {
            m_changed();
        }
        return QObject::eventFilter(watched, event);
    }

private:
    std::function<void()> m_changed;
};

} // namespace

struct Pulse::Private
{
    QTimer timer;
    QSet<QWidget*> subscribers;
    VisibilityWatch* watch = nullptr;
    bool appActive = true;

    // Runs only while someone can see it: at least one subscriber on screen,
    // in the application the person is using. A subscriber in a hidden page or
    // module, or the whole application in the background or minimised, used to
    // keep a 30 Hz timer waking the process for nothing.
    void retime()
    {
        bool anyVisible = false;
        if (appActive) {
            for (QWidget* widget : std::as_const(subscribers)) {
                if (widget->isVisible()) {
                    anyVisible = true;
                    break;
                }
            }
        }
        if (anyVisible && !timer.isActive()) {
            timer.start();
        } else if (!anyVisible && timer.isActive()) {
            timer.stop();
        }
    }
};

Pulse::Pulse()
    : d(new Private)
{
    clock();
    d->watch = new VisibilityWatch([this] { d->retime(); });
    d->watch->setParent(this);
    if (auto* app = qobject_cast<QGuiApplication*>(QCoreApplication::instance())) {
        d->appActive = app->applicationState() == Qt::ApplicationActive;
        connect(app, &QGuiApplication::applicationStateChanged, this,
                [this](Qt::ApplicationState state) {
                    d->appActive = state == Qt::ApplicationActive;
                    d->retime();
                });
    }
    d->timer.setInterval(kIntervalMs);
    d->timer.setTimerType(Qt::CoarseTimer);
    connect(&d->timer, &QTimer::timeout, this, [this] {
        emit tick();
        for (QWidget* widget : std::as_const(d->subscribers)) {
            // A subscriber scrolled out of view, on a background tab or in a
            // hidden window is not repainted. The timer keeps running for the
            // others; the work does not.
            if (widget->isVisible()) {
                // A widget that animates only a part of itself (a tab bar's
                // loading ring) says which part; repainting all of it 30
                // times a second for one small ring cost a core.
                const QVariant region = widget->property("novuMotionRegion");
                if (region.canConvert<QRegion>() && !region.value<QRegion>().isEmpty()) {
                    widget->update(region.value<QRegion>());
                } else {
                    widget->update();
                }
            }
        }
    });
}

Pulse& Pulse::instance()
{
    static Pulse pulse;
    return pulse;
}

void Pulse::subscribe(QWidget* widget)
{
    if (!widget || d->subscribers.contains(widget)) {
        return;
    }
    d->subscribers.insert(widget);
    widget->installEventFilter(d->watch);
    // Released by the widget itself rather than by whoever subscribed it: a
    // component that has to remember to unsubscribe in its destructor is a
    // component that will one day forget and leave a dangling pointer in here.
    connect(widget, &QObject::destroyed, this, [this](QObject* object) {
        d->subscribers.remove(static_cast<QWidget*>(object));
        d->retime();
    });
    d->retime();
}

void Pulse::release(QWidget* widget)
{
    if (!widget || !d->subscribers.remove(widget)) {
        return;
    }
    disconnect(widget, &QObject::destroyed, this, nullptr);
    widget->removeEventFilter(d->watch);
    d->retime();
}

bool Pulse::isRunning() const
{
    return d->timer.isActive();
}

int Pulse::subscriberCount() const
{
    return d->subscribers.size();
}

namespace motion {

double phase(int periodMs)
{
    if (periodMs <= 0) {
        return 0.0;
    }
    return static_cast<double>(clock().elapsed() % periodMs) / periodMs;
}

double breathe(int periodMs, double low, double high)
{
    // (1 - cos) / 2 is the ease-in-out the CSS keyframe describes: slowest at
    // both ends, so the dot reads as breathing rather than flashing.
    const double eased = (1.0 - std::cos(phase(periodMs) * 2.0 * M_PI)) / 2.0;
    return low + (high - low) * eased;
}

bool enabled(const QWidget* context)
{
    // Zero disables animation wholesale — that is what AnimationTokens says a
    // reduced-motion profile sets, and this is the one place that reads it.
    return themeFor(context).animation.normalMs > 0;
}

// The stops the approved prototype settled on. Named rather than inline
// because the gap between 0.717 and 0.894 *is* the arc: move either and the
// light changes length, not just position.
namespace {
constexpr double kRingFaintEnd = 0.717;
constexpr double kRingBrightStart = 0.894;
constexpr double kRingPeak = 0.955;
constexpr double kRingFaintAlpha = 0.12;
constexpr double kRingBrightAlpha = 0.90;
}

void paintRunningRing(QPainter* painter,
                      const QRectF& outline,
                      const QColor& accent,
                      double strokeWidth,
                      double radius,
                      int periodMs,
                      bool animated)
{
    if (!painter || outline.isEmpty()) {
        return;
    }
    painter->setRenderHint(QPainter::Antialiasing, true);

    QColor faint = accent;
    QColor bright = accent;
    faint.setAlphaF(kRingFaintAlpha);
    bright.setAlphaF(kRingBrightAlpha);

    // The design measures the sweep clockwise from twelve; QConicalGradient
    // measures counter-clockwise from three. Negating is the whole of the
    // conversion, and it is done here so no caller has to remember it.
    const double angle = animated ? phase(periodMs) * 360.0 : 0.0;

    QConicalGradient gradient(outline.center(), -angle);
    gradient.setColorAt(0.0, faint);
    gradient.setColorAt(kRingFaintEnd, faint);
    gradient.setColorAt(kRingBrightStart, bright);
    gradient.setColorAt(kRingPeak, QColor(Qt::white));
    gradient.setColorAt(1.0, bright);

    painter->setPen(QPen(QBrush(gradient), strokeWidth));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(outline, radius, radius);
}

void paintSpinner(QPainter* painter,
                  const QRectF& box,
                  const QColor& track,
                  const QColor& head,
                  double strokeWidth,
                  int periodMs,
                  bool animated)
{
    if (!painter || box.isEmpty()) {
        return;
    }
    painter->setRenderHint(QPainter::Antialiasing, true);

    // Inset by half the stroke, or the ring is drawn straddling the box and
    // clipped on all four sides.
    const double inset = strokeWidth / 2.0;
    const QRectF ring = box.adjusted(inset, inset, -inset, -inset);
    if (ring.isEmpty()) {
        return;
    }

    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(track, strokeWidth, Qt::SolidLine, Qt::FlatCap));
    painter->drawEllipse(ring);

    // CSS colours one border edge, which is a quarter of the circle. Qt counts
    // sixteenths of a degree, anticlockwise from three o'clock; the design
    // turns clockwise from twelve. Ninety degrees of arc starting at twelve is
    // therefore 90*16 as a span and -angle as an offset.
    const double angle = animated ? phase(periodMs) * 360.0 : 0.0;
    painter->setPen(QPen(head, strokeWidth, Qt::SolidLine, Qt::RoundCap));
    painter->drawArc(ring, static_cast<int>((90.0 - angle) * 16.0), 90 * 16);
}

void paintShimmer(QPainter* painter,
                  const QRectF& box,
                  double radius,
                  const QColor& base,
                  const QColor& highlight,
                  int periodMs,
                  bool animated)
{
    if (!painter || box.isEmpty()) {
        return;
    }
    painter->setRenderHint(QPainter::Antialiasing, true);

    // The gradient is twice the plate wide and slides one plate width per
    // period, so the band crosses once and the seam between repeats never
    // shows. Offset runs 0 -> -1 because the design animates the background
    // position to -200% of a 200% image, which is leftwards.
    const double offset = animated ? -phase(periodMs) : 0.0;
    const double span = box.width() * 2.0;
    QLinearGradient sweep(box.left() + offset * box.width(), 0.0,
                          box.left() + offset * box.width() + span, 0.0);
    sweep.setColorAt(0.00, base);
    sweep.setColorAt(0.25, base);
    sweep.setColorAt(0.37, highlight);
    sweep.setColorAt(0.63, base);
    sweep.setColorAt(1.00, base);

    painter->setPen(Qt::NoPen);
    painter->setBrush(sweep);
    painter->drawRoundedRect(box, radius, radius);
}

} // namespace motion

} // namespace novu::ui
