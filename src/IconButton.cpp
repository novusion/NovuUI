#include "novu/ui/IconButton.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/ThemeManager.h"

#include <QEnterEvent>
#include <QEvent>
#include <QPainter>
#include <QPainterPath>

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
    const ThemeProfile& profile = themeFor(this);
    if (m_variant == Variant::Chrome) {
        // The window control's own box, so the three actions and the three
        // controls are one row of identical plates.
        return QSize(profile.window.controlWidth, profile.window.controlHeight);
    }
    const int iconSide=m_size==ControlSize::Small?profile.controls.iconSmallHeight:m_size==ControlSize::Medium?profile.controls.iconHeight:0;
    const int side = !m_actionPlate&&iconSide>0?iconSide:plateSide(profile, m_size);
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
    } else if (m_variant == Variant::Chrome) {
        // `.winbtn{color:var(--text3);background:transparent;border:none}`.
        ink = t.textDisabled;
    }

    if (on) {
        // The `.on` state: the product accent, the same signal an active
        // module and a selected row already use.
        plate = withAlpha(t.accent, 0.14);
        outline = withAlpha(t.accent, 0.30);
        ink = t.accent;
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
        } else if (m_variant == Variant::Chrome) {
            // `.winbtn:hover{background:var(--panel);color:var(--text)}` - and
            // no outline in any state: the window controls beside it have
            // none either.
            const WindowTokens& w = profile.window;
            plate = isDown() ? w.controlPressedBg : w.controlHoverBg;
            outline = Qt::transparent;
            ink = w.titleText;
        } else {
            plate = t.surfaceHover;
            // A ghost control grows its outline on hover rather than having
            // one at rest — which is how a title bar keeps from looking like a
            // row of boxes.
            outline = t.borderHover;
            ink = t.textPrimary;
        }
    }

    if(on&&m_ownChecked){
        ink=m_checkedInk.resolve(profile);
        if(!m_checkedPlate){plate=active?t.surfaceHover:QColor(Qt::transparent);outline=Qt::transparent;}
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
    // `.btn.ico{border-radius:8px}` against `.nb{border-radius:7px}` - an
    // Outlined action plate (rowAction(), setActionPlate(true)) and a Ghost
    // chrome icon are one pixel apart in the approved sheet, not the same
    // shape at two names.
    const double radius = m_variant == Variant::Chrome
        ? profile.window.controlRadius
        : m_variant == Variant::Outlined && m_actionPlate
            ? resolveRadius(profile.radius(SurfaceRole::ActionIcon), height())
            : resolveRadius(profile.radius(SurfaceRole::IconButton), height());

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

    const int glyphSize = m_glyphSize>0?m_glyphSize:glyphSizeFor(this, m_size);
    const QRectF box((width() - glyphSize) / 2.0, (height() - glyphSize) / 2.0,
                     glyphSize, glyphSize);
    if(on&&m_ownChecked&&m_checkedFilled){
        painter.save();painter.translate(box.topLeft());painter.scale(box.width()/24.0,box.height()/24.0);
        painter.fillPath(glyphPath(m_glyph,this),ink);painter.restore();
    }
    paintGlyph(&painter, m_glyph, box, ink);
}

IconButton* rowAction(Glyph glyph, QWidget* parent, const QString& tooltip)
{
    // `iact(icon, title, cls, attrs)` in the reference: `class="btn sm ico"`,
    // which is `.btn` with icon-only padding - a hairline border at rest,
    // same as every other button. Ghost ("no border until hover") is for
    // chrome - a title bar, a panel header - and every row action (Copy,
    // View, Edit, Inspect, the per-row Detach) is `iact()`, never chrome.
    auto* button = new IconButton(glyph, IconButton::Variant::Outlined, parent);
    button->setControlSize(ControlSize::Small);
    button->setActionPlate(true);
    if (!tooltip.isEmpty()) {
        button->setToolTip(tooltip);
    }
    return button;
}

} // namespace novu::ui
