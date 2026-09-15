#include "novu/ui/StatusDot.h"

#include "novu/ui/Motion.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QPainter>
#include <QRadialGradient>
#include <QVariantAnimation>

namespace novu::ui {

namespace {

// The halo the design draws as a box-shadow. It is outside the dot, so the
// widget is bigger than the dot by this much on every side and the layout does
// not move when the glow comes and goes.
constexpr int kHalo = 3;

// How long a status takes to become a different status. Long enough to be seen
// as a change rather than a repaint, short enough not to lie about when it
// happened.
constexpr int kFadeMs = 280;

SemanticRole statusFor(StatusDot::Tone tone)
{
    switch (tone) {
    case StatusDot::Tone::Success: return SemanticRole::Success;
    case StatusDot::Tone::Warning: return SemanticRole::Warning;
    case StatusDot::Tone::Danger:  return SemanticRole::Danger;
    case StatusDot::Tone::Info:    return SemanticRole::Info;
    case StatusDot::Tone::Accent:
    case StatusDot::Tone::Neutral: break;
    }
    return SemanticRole::Neutral;
}

ColorToken tokenFor(StatusDot::Tone tone)
{
    // Accent is the one tone outside the semantic palette, deliberately: the
    // design says a status never wears the product accent, so it cannot be a
    // SemanticRole without opening that door.
    if (tone == StatusDot::Tone::Accent) {
        return token(ColorRole::AccentLight);
    }
    return statusToken(statusFor(tone), ColorRole::StatusText);
}

} // namespace

StatusDot::StatusDot(Tone tone, QWidget* parent)
    : QWidget(parent)
    , m_tone(tone)
    , m_previousTone(tone)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

StatusDot::StatusDot(Tone tone, bool pulsing, QWidget* parent)
    : StatusDot(tone, parent)
{
    setPulsing(pulsing);
}

StyleSpec StatusDot::styleSpec() const
{
    StyleSpec spec;
    spec.radius = SurfaceRole::Badge;
    spec.foreground = tokenFor(m_tone);
    spec.background = tokenFor(m_previousTone);
    spec.disabledForeground = token(ColorRole::TextDisabled);
    return spec;
}

ResolvedStyle StatusDot::resolvedStyle() const
{
    return resolve(styleSpec(), this, m_diameter);
}

void StatusDot::setTone(Tone tone)
{
    if (m_tone == tone) {
        return;
    }
    // Whatever is on screen right now is what the fade comes *from*: changing
    // tone twice inside 280ms must not snap back to the first colour.
    m_previousTone = m_blend >= 1.0 ? m_tone : m_previousTone;
    m_tone = tone;

    const int duration = themeFor(this).animation.normalMs > 0 ? kFadeMs : 0;
    if (duration == 0 || !isVisible()) {
        m_blend = 1.0;
        m_previousTone = tone;
        update();
        return;
    }

    m_blend = 0.0;
    auto* fade = new QVariantAnimation(this);
    fade->setDuration(duration);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->setEasingCurve(QEasingCurve::OutCubic);
    connect(fade, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        m_blend = value.toDouble();
        update();
    });
    // Qt drives every QAbstractAnimation in the process from one timer, so a
    // one-shot transition adds no clock of its own. The shared Pulse is for
    // loops, which this is not.
    fade->start(QAbstractAnimation::DeleteWhenStopped);
}

void StatusDot::setPulsing(bool pulsing)
{
    if (m_pulsing == pulsing) {
        return;
    }
    m_pulsing = pulsing;
    updateMotion();
    update();
}

void StatusDot::setDiameter(int px)
{
    px = qMax(1, px);
    if (m_diameter == px) {
        return;
    }
    m_diameter = px;
    updateGeometry();
    update();
}

void StatusDot::updateMotion()
{
    const bool wants = m_pulsing && motion::enabled(this);
    if (wants == m_animating) {
        return;
    }
    m_animating = wants;
    if (wants) {
        Pulse::instance().subscribe(this);
    } else {
        Pulse::instance().release(this);
    }
}

QSize StatusDot::sizeHint() const
{
    return QSize(m_diameter + 2 * kHalo, m_diameter + 2 * kHalo);
}

bool StatusDot::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::ParentChange) {
        updateMotion();
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void StatusDot::paintEvent(QPaintEvent*)
{
    const ResolvedStyle style = resolvedStyle();

    // background carries the tone being faded *from*, foreground the one being
    // faded to; at rest the two are the same and the blend is a no-op.
    const QColor from = style.background;
    const QColor to = style.foreground;
    QColor colour = QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * m_blend,
                                     from.greenF() + (to.greenF() - from.greenF()) * m_blend,
                                     from.blueF() + (to.blueF() - from.blueF()) * m_blend,
                                     from.alphaF() + (to.alphaF() - from.alphaF()) * m_blend);

    double opacity = 1.0;
    if (m_pulsing && motion::enabled(this)) {
        // softglow: slowest at both ends, so it reads as breathing.
        opacity = motion::breathe(themeFor(this).animation.pulseMs);
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPointF centre(width() / 2.0, height() / 2.0);
    const double radius = m_diameter / 2.0;

    // The halo first, under the dot. It is what the design's box-shadow does,
    // and it is what makes a 6px dot readable against a busy row.
    QColor haloColour = colour;
    haloColour.setAlphaF(colour.alphaF() * 0.55 * opacity);
    QRadialGradient halo(centre, radius + kHalo);
    halo.setColorAt(0.0, haloColour);
    haloColour.setAlphaF(0.0);
    halo.setColorAt(1.0, haloColour);
    painter.setPen(Qt::NoPen);
    painter.setBrush(halo);
    painter.drawEllipse(centre, radius + kHalo, radius + kHalo);

    colour.setAlphaF(colour.alphaF() * opacity);
    painter.setBrush(colour);
    painter.drawEllipse(centre, radius, radius);
}

} // namespace novu::ui
