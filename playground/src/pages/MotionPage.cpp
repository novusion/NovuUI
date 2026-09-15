#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Feedback.h"
#include "novu/ui/Form.h"
#include "novu/ui/Motion.h"
#include "novu/ui/Separator.h"
#include "novu/ui/StatusDot.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// Every primitive in Motion.h, drawn side by side on one surface.
//
// This is not a fake component: each panel calls the real motion:: function
// that Button, Spinner and Skeleton call. Drawing them together is the only
// way to see that they are in phase, which is the entire claim the shared
// clock makes and the one thing a per-component page cannot show.
class MotionBoard : public QWidget
{
public:
    explicit MotionBoard(QWidget* parent)
        : QWidget(parent)
    {
        setMinimumHeight(140);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void setAnimated(bool animated)
    {
        m_animated = animated;
        updateMotion();
        update();
    }

protected:
    void showEvent(QShowEvent* event) override
    {
        QWidget::showEvent(event);
        updateMotion();
    }

    void hideEvent(QHideEvent* event) override
    {
        QWidget::hideEvent(event);
        updateMotion();
    }

    void paintEvent(QPaintEvent*) override
    {
        const ThemeProfile& profile = themeFor(this);
        const bool run = m_animated && motion::enabled(this);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const double cell = width() / 4.0;
        const double side = qMin(cell - 28.0, height() - 52.0);
        const double top = 14.0;

        // paintRunningRing — the arc round a primary button.
        motion::paintRunningRing(&painter,
                                 QRectF(cell * 0.5 - side / 2.0, top, side, side),
                                 profile.colors.accent, 2.0, side / 4.0,
                                 profile.animation.ringMs, run);

        // paintSpinner — the busy mark.
        motion::paintSpinner(&painter,
                             QRectF(cell * 1.5 - side / 2.0, top, side, side),
                             profile.colors.border, profile.colors.accentLight,
                             3.0, profile.animation.spinMs, run);

        // paintShimmer — the placeholder sweep.
        motion::paintShimmer(&painter,
                             QRectF(cell * 2.0 + 14.0, top + side / 2.0 - 8.0,
                                    cell - 28.0, 16.0),
                             profile.radii.small,
                             profile.colors.panelBgAlt, profile.colors.surfaceHover,
                             profile.animation.shimmerMs, run);

        // breathe() — the opacity curve a status dot rides.
        const double alpha = run ? motion::breathe(profile.animation.pulseMs) : 1.0;
        QColor dot = profile.semantic.success.text;
        dot.setAlphaF(alpha);
        painter.setPen(Qt::NoPen);
        painter.setBrush(dot);
        const double dotSide = qMin(side, 18.0);
        painter.drawEllipse(QRectF(cell * 3.5 - dotSide / 2.0,
                                   top + side / 2.0 - dotSide / 2.0, dotSide, dotSide));

        painter.setFont(roleMonoFont(this, TypeRole::Mono));
        painter.setPen(profile.colors.textDisabled);
        const QStringList names = {QStringLiteral("paintRunningRing"),
                                   QStringLiteral("paintSpinner"),
                                   QStringLiteral("paintShimmer"),
                                   QStringLiteral("breathe")};
        for (int i = 0; i < names.size(); ++i) {
            painter.drawText(QRectF(cell * i, height() - 26.0, cell, 20.0),
                             Qt::AlignCenter, names.at(i));
        }
    }

private:
    void updateMotion()
    {
        const bool wanted = m_animated && isVisible() && motion::enabled(this);
        if (wanted == m_subscribed) {
            return;
        }
        m_subscribed = wanted;
        if (wanted) {
            Pulse::instance().subscribe(this);
        } else {
            Pulse::instance().release(this);
        }
    }

    bool m_animated = true;
    bool m_subscribed = false;
};

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    // ── the primitives ───────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("The four loops"),
                                 QStringLiteral("Every looping animation in the "
                                                "library, on one clock."));
        QVBoxLayout* body = card->body();

        auto* board = new MotionBoard(card);
        body->addWidget(board);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Clock"));
        auto* stop = new Button(QStringLiteral("Freeze"), Button::Variant::Secondary, card);
        stop->setControlSize(ControlSize::Small);
        auto* start = new Button(QStringLiteral("Run"), Button::Variant::Secondary, card);
        start->setControlSize(ControlSize::Small);
        auto* count = label(QString(), card, TypeRole::Mono, Ink::Muted);

        const auto refresh = [count] {
            count->setText(QStringLiteral("Pulse: %1 subscriber(s), %2")
                               .arg(Pulse::instance().subscriberCount())
                               .arg(Pulse::instance().isRunning()
                                        ? QStringLiteral("running")
                                        : QStringLiteral("stopped")));
        };
        refresh();
        QObject::connect(board, &QObject::destroyed, count, [] {});
        QObject::connect(stop, &Button::clicked, board, [board, refresh] {
            board->setAnimated(false);
            refresh();
        });
        QObject::connect(start, &Button::clicked, board, [board, refresh] {
            board->setAnimated(true);
            refresh();
        });
        row->addWidget(stop);
        row->addWidget(start);
        row->addWidget(count);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Press Freeze. The subscriber count drops and the timer stops: "
                            "nothing is animated when nothing needs to be. That is the whole "
                            "reason there is one clock rather than a QTimer per widget — a "
                            "window with a toolbar, a status bar and a loading table would "
                            "otherwise wake the CPU on four unrelated schedules."));
        column->addWidget(card);
    }

    // ── in phase ─────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("In phase"),
                                 QStringLiteral("Why the clock is shared and not "
                                                "merely reused."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* dots = exampleRow(body, card, QStringLiteral("Eight dots, one clock"));
        for (int i = 0; i < 8; ++i) {
            auto* dot = new StatusDot(StatusDot::Tone::Success, true, card);
            dot->setDiameter(10);
            dots->addWidget(dot);
        }
        dots->addStretch(1);

        QHBoxLayout* spinners = exampleRow(body, card, QStringLiteral("Six spinners, one clock"));
        for (int i = 0; i < 6; ++i) {
            spinners->addWidget(new Spinner(ControlSize::Medium, card));
        }
        spinners->addStretch(1);

        note(body, card,
             QStringLiteral("They breathe and turn together. Phase is read from the monotonic "
                            "clock rather than counted in ticks, so a widget that appeared "
                            "three seconds after its neighbour is still in step with it — and "
                            "a dropped frame does not accumulate drift."));
        column->addWidget(card);
    }

    // ── reduced motion ───────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("When motion is off"),
                                 QStringLiteral("A token, not a flag threaded "
                                                "through every constructor."));
        QVBoxLayout* body = card->body();

        propertyRow(body, card, QStringLiteral("Profile"), ctx.profileId,
                    Ink::Secondary, true);
        propertyRow(body, card, QStringLiteral("motion::enabled()"),
                    motion::enabled(page) ? QStringLiteral("true")
                                          : QStringLiteral("false"),
                    Ink::Secondary, true);
        propertyRow(body, card, QStringLiteral("ringMs / pulseMs / spinMs / shimmerMs"),
                    QStringLiteral("%1 / %2 / %3 / %4")
                        .arg(themeFor(page).animation.ringMs)
                        .arg(themeFor(page).animation.pulseMs)
                        .arg(themeFor(page).animation.spinMs)
                        .arg(themeFor(page).animation.shimmerMs),
                    Ink::Secondary, true);

        note(body, card,
             QStringLiteral("A profile that sets normalMs to zero switches every loop off. "
                            "Components ask motion::enabled() before subscribing rather than "
                            "taking a constructor flag, which is what keeps \"reduced motion\" "
                            "one token instead of an argument on thirty constructors. Note "
                            "what does not happen: the ring and the spinner are still drawn, "
                            "frozen at the start of their sweep, because the mark is part of "
                            "the shape and one that vanishes reads as a layout fault."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec motionComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("motion");
    spec.name = QStringLiteral("Motion");
    spec.category = Category::Foundation;
    spec.description = QStringLiteral(
        "The shared animation clock and the four looping primitives that run on it: "
        "the running ring, the spinner, the shimmer and the breathing curve.");
    spec.headerPath = QStringLiteral("include/novu/ui/Motion.h");
    spec.sourcePath = QStringLiteral("src/Motion.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("paintRunningRing"), QStringLiteral("paintSpinner"),
                     QStringLiteral("paintShimmer"), QStringLiteral("breathe")};
    spec.states = {QStringLiteral("Running"), QStringLiteral("Frozen")};
    spec.keywords = {QStringLiteral("animation"), QStringLiteral("pulse"),
                     QStringLiteral("clock"), QStringLiteral("timer"),
                     QStringLiteral("spin"), QStringLiteral("shimmer"),
                     QStringLiteral("keyframes"), QStringLiteral("reduced motion")};
    spec.properties = {
        {QStringLiteral("periodMs"), QStringLiteral("int"), {}, QStringLiteral("profile"),
         QStringLiteral("How long one loop takes. Read from the profile, never named "
                        "by a component.")},
        {QStringLiteral("animated"), QStringLiteral("bool"), {}, QStringLiteral("true"),
         QStringLiteral("False draws the primitive frozen rather than omitting it.")},
    };
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
