#include "novu/ui/IconButton.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/ThemeManager.h"

#include <QEnterEvent>
#include <QEvent>
#include <QPainter>

namespace novu::ui {

namespace {

// The design's `.btn.ico` is a square plate around a glyph: the control height
// in both directions, so it lines up with the text buttons beside it.
int plateSide(const ThemeProfile& profile, ControlSize size)
{
    return profile.controlHeight(size);
}

} // namespace

IconButton::IconButton(Glyph glyph, QWidget* parent)
    : QAbstractButton(parent)
    , m_glyph(glyph)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::TabFocus);
}

IconButton::IconButton(Glyph glyph, Variant variant, QWidget* parent)
    : IconButton(glyph, parent)
{
    m_variant = variant;
}

void IconButton::setGlyph(Glyph glyph)
{
    if (m_glyph == glyph) {
        return;
    }
    m_glyph = glyph;
    update();
}

void IconButton::setVariant(Variant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    update();
}

void IconButton::setControlSize(ControlSize size)
{
    if (m_size == size) {
        return;
    }
    m_size = size;
    updateGeometry();
    update();
}

void IconButton::setDestructive(bool destructive)
{
    if (m_destructive == destructive) {
        return;
    }
    m_destructive = destructive;
    update();
}

QSize IconButton::sizeHint() const
{
    const int side = plateSide(themeFor(this), m_size);
    return QSize(side, side);
}

bool IconButton::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QAbstractButton::event(event);
}

void IconButton::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    update();
    QAbstractButton::enterEvent(event);
}

void IconButton::leaveEvent(QEvent* event)
{
    m_hovered = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void IconButton::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const ThemeColors& t = profile.colors;

    const bool on = isCheckable() && isChecked();
    const bool active = isEnabled() && (m_hovered || isDown());

    QColor plate = Qt::transparent;
    QColor outline = Qt::transparent;
    QColor ink = t.textSecondary;

    if (m_variant == Variant::Outlined) {
        // The control outline, not the surface one. The design draws a button
        // on --hair2 and a card on --hair; at --hair a button on the card it
        // sits in has no visible edge at all.
        outline = t.borderHover;
    }

    if (on) {
        // The `.on` state: the product accent, the same signal an active
        // module and a selected row already use.
        plate = withAlpha(t.accent, 0.12);
        outline = withAlpha(t.accent, 0.45);
        ink = t.accentLight;
    }

    if (active) {
        if (m_destructive) {
            const SemanticColor& danger = profile.semantic.danger;
            plate = danger.fill;
            outline = danger.border;
            ink = danger.text;
        } else if (on) {
            plate = withAlpha(t.accent, 0.20);
            outline = t.accent;
        } else {
            plate = t.surfaceHover;
            // A ghost control grows its outline on hover rather than having
            // one at rest — which is how a title bar keeps from looking like a
            // row of boxes.
            outline = t.borderHover;
            ink = t.textPrimary;
        }
    }

    if (!isEnabled()) {
        ink = t.textDisabled;
        if (outline.alpha() > 0) {
            outline = t.borderSubtle;
        }
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int borderWidth = profile.border(BorderRole::Input);
    const QRectF plateRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const double radius = resolveRadius(profile.radius(SurfaceRole::Button), height());

    painter.setPen(outline.alpha() > 0 ? QPen(outline, borderWidth) : QPen(Qt::NoPen));
    painter.setBrush(plate.alpha() > 0 ? QBrush(plate) : Qt::NoBrush);
    if (plate.alpha() > 0 || outline.alpha() > 0) {
        painter.drawRoundedRect(plateRect, radius, radius);
    }

    // Focus is drawn, not styled: a focus ring on a self-painted control has
    // nowhere else to come from, and a keyboard user must be able to see which
    // action is about to fire.
    if (hasFocus()) {
        QPen ring(withAlpha(t.accent, 0.75), profile.border(BorderRole::Focus));
        painter.setPen(ring);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1), radius, radius);
    }

    const int glyphSize = glyphSizeFor(this, m_size);
    const QRectF box((width() - glyphSize) / 2.0, (height() - glyphSize) / 2.0,
                     glyphSize, glyphSize);
    paintGlyph(&painter, m_glyph, box, ink);
}

IconButton* rowAction(Glyph glyph, QWidget* parent, const QString& tooltip)
{
    auto* button = new IconButton(glyph, IconButton::Variant::Ghost, parent);
    button->setControlSize(ControlSize::Small);
    if (!tooltip.isEmpty()) {
        button->setToolTip(tooltip);
    }
    return button;
}

} // namespace novu::ui
