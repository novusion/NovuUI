#include "novu/ui/Slider.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QBoxLayout>
#include <QEnterEvent>
#include <QEvent>
#include <QFontMetrics>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

// The design's `.scale`: a 4px groove with a 14px handle riding on it, and the
// readout in the monospace face so the number does not jitter as it changes.
constexpr int kGrooveHeight = 4;
constexpr int kHandleDiameter = 14;
constexpr int kReadoutGap = 12;
// Wide enough for four mono digits plus a unit, so a groove does not change
// length as the value crosses ten or a hundred.
constexpr int kReadoutChars = 5;

} // namespace

Slider::Slider(QWidget* parent)
    : QSlider(Qt::Horizontal, parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    connect(this, &QSlider::valueChanged, this, [this] { update(); });
}

Slider::Slider(int minimum, int maximum, QWidget* parent)
    : Slider(parent)
{
    setRange(minimum, maximum);
}

void Slider::setFormatter(Formatter formatter)
{
    m_formatter = std::move(formatter);
    updateGeometry();
    update();
}

QString Slider::readout() const
{
    return m_formatter ? m_formatter(value()) : QString::number(value());
}

void Slider::setShowsReadout(bool shown)
{
    if (m_readout == shown) {
        return;
    }
    m_readout = shown;
    updateGeometry();
    update();
}

StyleSpec Slider::styleSpec() const
{
    StyleSpec spec;
    spec.radius = SurfaceRole::Badge;      // the groove is a pill at any height
    spec.borderWidth = BorderRole::Input;
    spec.type = TypeRole::Mono;

    // The empty half of the groove, and the filled half.
    spec.background = token(ColorRole::SurfaceHover);
    spec.checkedBackground = token(ColorRole::Accent);
    // The handle, its outline, and the readout.
    spec.checkedForeground = token(ColorRole::AccentLight);
    spec.border = token(ColorRole::BorderHover);
    spec.foreground = token(ColorRole::TextSecondary);
    spec.hoverBorder = token(ColorRole::Accent);
    spec.disabledForeground = token(ColorRole::TextDisabled);
    spec.disabledBorder = token(ColorRole::BorderSubtle);
    return spec;
}

ResolvedStyle Slider::resolvedStyle() const
{
    return resolve(styleSpec(), this, kGrooveHeight);
}

QRect Slider::readoutRect() const
{
    if (!m_readout) {
        return {};
    }
    const QFontMetrics fm(roleFont(this, TypeRole::Mono));
    const int w = fm.horizontalAdvance(QLatin1Char('0')) * kReadoutChars;
    return QRect(width() - w, 0, w, height());
}

QRect Slider::grooveRect() const
{
    const int trailing = m_readout ? readoutRect().width() + kReadoutGap : 0;
    // Inset by half a handle at each end, so the handle stays inside the
    // control at both extremes instead of hanging off it.
    const int inset = kHandleDiameter / 2;
    return QRect(inset, (height() - kGrooveHeight) / 2,
                 qMax(1, width() - trailing - 2 * inset), kGrooveHeight);
}

QRect Slider::handleRect() const
{
    const QRect groove = grooveRect();
    const int span = qMax(1, maximum() - minimum());
    const double t = static_cast<double>(value() - minimum()) / span;
    const int centre = groove.left() + static_cast<int>(std::lround(groove.width() * t));
    return QRect(centre - kHandleDiameter / 2,
                 (height() - kHandleDiameter) / 2,
                 kHandleDiameter, kHandleDiameter);
}

int Slider::valueAt(int x) const
{
    const QRect groove = grooveRect();
    const double t = qBound(0.0, static_cast<double>(x - groove.left()) / groove.width(), 1.0);
    return minimum() + static_cast<int>(std::lround(t * (maximum() - minimum())));
}

QSize Slider::sizeHint() const
{
    const int trailing = m_readout ? readoutRect().width() + kReadoutGap : 0;
    return QSize(160 + trailing, qMax(kHandleDiameter + 4,
                                      themeFor(this).controlHeight(ControlSize::Small)));
}

QSize Slider::minimumSizeHint() const
{
    const int trailing = m_readout ? readoutRect().width() + kReadoutGap : 0;
    return QSize(64 + trailing, sizeHint().height());
}

void Slider::mousePressEvent(QMouseEvent* event)
{
    // Jumps to where the groove was clicked rather than paging towards it.
    // Qt's default is a hold-over from scroll bars; on a setting it means two
    // clicks to reach a value the user already pointed at.
    if (event->button() != Qt::LeftButton || !isEnabled()) {
        QSlider::mousePressEvent(event);
        return;
    }
    m_dragging = true;
    setSliderDown(true);
    setValue(valueAt(event->position().toPoint().x()));
    event->accept();
}

void Slider::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_dragging) {
        QSlider::mouseMoveEvent(event);
        return;
    }
    setValue(valueAt(event->position().toPoint().x()));
    event->accept();
}

void Slider::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_dragging) {
        QSlider::mouseReleaseEvent(event);
        return;
    }
    m_dragging = false;
    setSliderDown(false);
    event->accept();
}

void Slider::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    update();
    QSlider::enterEvent(event);
}

void Slider::leaveEvent(QEvent* event)
{
    m_hovered = false;
    update();
    QSlider::leaveEvent(event);
}

bool Slider::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QSlider::event(event);
}

void Slider::paintEvent(QPaintEvent*)
{
    const ResolvedStyle style = resolvedStyle();
    const ThemeProfile& profile = themeFor(this);
    const bool on = isEnabled();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRect groove = grooveRect();
    const QRect handle = handleRect();
    const double radius = kGrooveHeight / 2.0;

    QColor empty = style.background;
    QColor filled = style.checkedBackground;
    QColor knob = style.checkedForeground;
    QColor outline = m_hovered || hasFocus() ? style.hoverBorder : style.border;
    if (!on) {
        for (QColor* c : {&empty, &filled, &knob, &outline}) {
            c->setAlphaF(c->alphaF() * 0.42);
        }
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(empty);
    painter.drawRoundedRect(groove, radius, radius);

    // The filled half is what says which way the value goes. A groove that is
    // one colour end to end states a position and not a quantity.
    QRect done = groove;
    done.setRight(handle.center().x());
    painter.setBrush(filled);
    painter.drawRoundedRect(done, radius, radius);

    painter.setBrush(knob);
    painter.setPen(QPen(outline, style.borderWidth));
    painter.drawEllipse(handle);

    if (hasFocus()) {
        QColor ring = profile.colors.accent;
        ring.setAlphaF(0.75);
        painter.setPen(QPen(ring, profile.border(BorderRole::Focus)));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(handle.adjusted(-2, -2, 2, 2));
    }

    if (m_readout) {
        painter.setFont(roleFont(this, TypeRole::Mono));
        painter.setPen(on ? style.foreground : style.disabledForeground);
        painter.drawText(readoutRect(), Qt::AlignRight | Qt::AlignVCenter, readout());
    }
}

Slider* sliderRow(QBoxLayout* into, QWidget* parent, const QString& caption,
                  int minimum, int maximum, int value, const QString& hint)
{
    const ThemeProfile& profile = themeFor(parent);

    auto* row = new QHBoxLayout;
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

    auto* slider = new Slider(minimum, maximum, parent);
    slider->setValue(value);
    slider->setFixedWidth(200);
    row->addWidget(slider, 0, Qt::AlignTop);

    if (into) {
        into->addLayout(row);
    }
    return slider;
}

} // namespace novu::ui
