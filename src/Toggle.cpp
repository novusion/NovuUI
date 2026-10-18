#include "novu/ui/Toggle.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QBoxLayout>
#include <QEnterEvent>
#include <QEvent>
#include <QLabel>
#include <QPainter>
#include <QVariantAnimation>

namespace novu::ui {

namespace {

// The design's `.toggle`: a 40x22 track with a 16px knob inset by 2, which
// leaves exactly 18px of travel. Owned here, like every other geometry in this
// library - a page never sizes one.
constexpr int kTrackWidth = 40;
constexpr int kTrackHeight = 22;
constexpr int kKnobInset = 2;
constexpr int kKnobSize = 16;

} // namespace

Toggle::Toggle(QWidget* parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);

    m_slide = new QVariantAnimation(this);
    m_slide->setStartValue(0.0);
    m_slide->setEndValue(1.0);
    m_slide->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_slide, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        m_position = value.toDouble();
        update();
    });
}

Toggle::~Toggle() = default;

StyleSpec Toggle::styleSpec() const
{
    StyleSpec spec;
    spec.radius = SurfaceRole::Badge;      // a pill, at any height
    spec.borderWidth = BorderRole::Input;
    spec.type = TypeRole::Body;

    // Off: the inset well the design uses for anything switched off, with the
    // muted ink as the knob.
    spec.background = token(ColorRole::SurfaceHover);
    spec.border = token(ColorRole::BorderHover);
    spec.foreground = token(ColorRole::TextDisabled);

    // On: the product accent, tinted. Not a status - a switch says "this is
    // turned on", which is the product doing what it was told, and the design
    // is explicit that a status never wears the accent and the accent never
    // states a status.
    spec.checkedBackground = token(ColorRole::Accent, 0.25);
    spec.checkedBorder = token(ColorRole::Accent);
    spec.checkedForeground = token(ColorRole::Accent);

    spec.hoverBorder = token(ColorRole::TextDisabled);
    spec.disabledForeground = token(ColorRole::TextDisabled);
    spec.disabledBorder = token(ColorRole::BorderSubtle);
    return spec;
}

ResolvedStyle Toggle::resolvedStyle() const
{
    return resolve(styleSpec(), this, kTrackHeight);
}

QSize Toggle::sizeHint() const
{
    return QSize(kTrackWidth, kTrackHeight);
}

void Toggle::animateTo(double position)
{
    const int duration = themeFor(this).animation.normalMs;
    if (duration <= 0 || !isVisible()) {
        // Reduced motion, or a toggle whose state was set before it was shown:
        // arriving is what matters, and sliding a control nobody has seen yet
        // is motion without a message.
        m_slide->stop();
        m_position = position;
        update();
        return;
    }
    m_slide->stop();
    m_slide->setStartValue(m_position);
    m_slide->setEndValue(position);
    m_slide->setDuration(duration);
    m_slide->start();
}

void Toggle::checkStateSet()
{
    QAbstractButton::checkStateSet();
    animateTo(isChecked() ? 1.0 : 0.0);
}

void Toggle::nextCheckState()
{
    // Toggling is the whole interaction, and it happens on release inside the
    // control - which QAbstractButton already arranges.
    QAbstractButton::nextCheckState();
    // A click or the space bar reaches setChecked() through here with
    // QAbstractButton's refresh blocked, so checkStateSet() is NOT called for
    // it - Qt calls that hook only for setChecked() from outside
    // nextCheckState(). Without this every click changed the checked state and
    // never moved the knob: the caption beside it said "Shared" while the
    // switch was still drawn off. Only setChecked() from code ever animated.
    animateTo(isChecked() ? 1.0 : 0.0);
}

void Toggle::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    update();
    QAbstractButton::enterEvent(event);
}

void Toggle::leaveEvent(QEvent* event)
{
    m_hovered = false;
    update();
    QAbstractButton::leaveEvent(event);
}

bool Toggle::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QAbstractButton::event(event);
}

void Toggle::paintEvent(QPaintEvent*)
{
    const ResolvedStyle style = resolvedStyle();

    // Interpolated rather than switched, so the track changes colour with the
    // knob instead of snapping under it halfway through the slide.
    const auto blend = [](const QColor& from, const QColor& to, double t) {
        return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * t,
                                from.greenF() + (to.greenF() - from.greenF()) * t,
                                from.blueF() + (to.blueF() - from.blueF()) * t,
                                from.alphaF() + (to.alphaF() - from.alphaF()) * t);
    };

    QColor track = blend(style.background, style.checkedBackground, m_position);
    QColor outline = blend(style.border, style.checkedBorder, m_position);
    QColor knob = blend(style.foreground, style.checkedForeground, m_position);

    if (!isEnabled()) {
        // The design dims a disabled control wholesale rather than recolouring
        // it, so the switch still reads as on or off while being unusable.
        track.setAlphaF(track.alphaF() * 0.42);
        outline.setAlphaF(outline.alphaF() * 0.42);
        knob.setAlphaF(knob.alphaF() * 0.42);
    } else if (m_hovered) {
        outline = style.hoverBorder;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const double border = style.borderWidth;
    const QRectF rect = QRectF(0, 0, kTrackWidth, kTrackHeight)
                            .adjusted(border / 2.0, border / 2.0, -border / 2.0, -border / 2.0);
    const double radius = rect.height() / 2.0;

    painter.setPen(QPen(outline, border));
    painter.setBrush(track);
    painter.drawRoundedRect(rect, radius, radius);

    // No accent ring on focus: on/off is already the track's own colour and
    // the knob's own position, and a second outline around the whole switch
    // was reading as a stray colour rather than as a focus cue.
    const double travel = kTrackWidth - kKnobSize - 2 * kKnobInset;
    const QRectF knobRect(kKnobInset + travel * m_position,
                          (kTrackHeight - kKnobSize) / 2.0,
                          kKnobSize, kKnobSize);
    painter.setPen(Qt::NoPen);
    painter.setBrush(knob);
    painter.drawEllipse(knobRect);
}

Toggle* toggleRow(QBoxLayout* into, QWidget* parent, const QString& caption,
                  const QString& hint, bool on)
{
    auto* row = new QHBoxLayout;
    const ThemeProfile& profile = themeFor(parent);
    row->setSpacing(profile.space(SpacingRole::ToolbarGap));

    auto* text = new QVBoxLayout;
    text->setSpacing(profile.spacing.xs / 2);
    text->addWidget(label(caption, parent, TypeRole::Label));
    if (!hint.isEmpty()) {
        QLabel* note = label(hint, parent, TypeRole::Caption, Ink::Muted);
        note->setWordWrap(true);
        text->addWidget(note);
    }
    row->addLayout(text, 1);

    auto* toggle = new Toggle(parent);
    toggle->setChecked(on);
    // Top-aligned: with a hint that wraps to three lines, a centred switch
    // drifts away from the caption it belongs to.
    row->addWidget(toggle, 0, Qt::AlignTop);

    if (into) {
        into->addLayout(row);
    }
    return toggle;
}

} // namespace novu::ui
