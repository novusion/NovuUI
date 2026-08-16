#include "novu/ui/Button.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>

namespace novu::ui {

namespace {

// Horizontal padding is owned by the component; the vertical size comes from
// the profile's control height, so a button never staggers against the combo
// box next to it.
constexpr int kPaddingH = 14;
constexpr int kPaddingHSmall = 9;

int paddingFor(ControlSize size)
{
    return size == ControlSize::Small ? kPaddingHSmall : kPaddingH;
}

} // namespace

Button::Button(QWidget* parent)
    : QPushButton(parent)
{
    // The button is exactly its sizeHint(); nothing may compress it.
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
    applyVariant();
}

Button::Button(const QString& text, Variant variant, QWidget* parent)
    : QPushButton(text, parent)
    , m_variant(variant)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
    applyVariant();
}

void Button::setVariant(Variant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    applyVariant();
}

void Button::setControlSize(ControlSize size)
{
    if (m_size == size) {
        return;
    }
    m_size = size;
    applyVariant();
}

QSize Button::sizeHint() const
{
    // Derived from this button's own font and its own padding rather than from
    // the style sheet, because QSS padding is not reflected in
    // QPushButton::sizeHint() until the widget has been polished. Depending on
    // that timing means a caption can be painted clipped.
    const ThemeProfile& profile = themeFor(this);
    const QFontMetrics fm(font());
    const int padding = paddingFor(m_size);
    return QSize(fm.horizontalAdvance(text()) + 2 * padding,
                 qMax(profile.controlHeight(m_size), fm.height() + 8));
}

bool Button::event(QEvent* event)
{
    // The profile is resolved from the widget tree, and at construction the
    // tree may not carry a scope yet — a button built before its page is
    // parented would otherwise keep the default product's palette forever.
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::PolishRequest
        || event->type() == QEvent::ParentChange) {
        applyVariant();
    }
    return QPushButton::event(event);
}

void Button::applyVariant()
{
    const ThemeProfile& profile = themeFor(this);
    const ThemeColors& t = profile.colors;

    QColor background;
    QColor foreground;
    QColor borderColor;

    switch (m_variant) {
    case Variant::Primary:
        // Solid accent. The border matches the fill so a Primary and a Ghost
        // button with the same caption stay the same size.
        background = t.accent;
        foreground = t.textOnAccent;
        borderColor = t.accent;
        break;
    case Variant::Danger:
        background = withAlpha(t.danger, 0.14);
        foreground = t.danger;
        borderColor = withAlpha(t.danger, 0.40);
        break;
    case Variant::Success:
        background = withAlpha(t.success, 0.14);
        foreground = t.success;
        borderColor = withAlpha(t.success, 0.40);
        break;
    case Variant::Ghost:
        background = Qt::transparent;
        foreground = t.textSecondary;
        borderColor = t.border;
        break;
    case Variant::Secondary:
    default:
        // A lift off the card, not a well: a secondary button sits on top of
        // the surface it belongs to.
        background = t.surfaceHover;
        foreground = t.textPrimary;
        borderColor = t.border;
        break;
    }

    // Every variant hovers into the same accent tint, so the whole toolbar
    // answers the pointer the same way.
    const QColor hover = withAlpha(t.accent, 0.16);

    const bool small = m_size == ControlSize::Small;

    QFont buttonFont = font();
    // A row action drops one step on the type scale. It sits inside a 34px
    // table row beside 12px cell text, and at the same size it competed with
    // the data instead of sitting under it.
    buttonFont.setPixelSize(small ? profile.type.xs : profile.type.sm);
    buttonFont.setWeight(m_variant == Variant::Primary ? QFont::Bold : QFont::Normal);
    setFont(buttonFont);

    // Padding is intentionally NOT declared here — it lives in sizeHint()
    // above so that geometry has exactly one source of truth.
    setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; color: %2; border: %3px solid %4;"
        " border-radius: %5px; }"
        "QPushButton:hover { background: %6; }"
        "QPushButton:disabled { color: %7; border-color: %8; background: transparent; }")
        .arg(background.alpha() == 0 ? QStringLiteral("transparent")
                                     : rgba(background, background.alphaF()),
             foreground.name())
        .arg(profile.border(BorderRole::Input))
        .arg(rgba(borderColor, borderColor.alphaF()))
        // A small button takes the input corner, not the button one — the
        // same distinction the scope sheet makes for [novuSize="small"].
        .arg(resolveRadius(small ? profile.radii.xs
                                 : profile.radius(SurfaceRole::Button),
                           profile.controlHeight(m_size)))
        .arg(rgba(hover, hover.alphaF()),
             t.textDisabled.name(),
             rgba(t.borderSubtle, t.borderSubtle.alphaF())));

    updateGeometry();
}

Button* primaryButton(const QString& text, QWidget* parent)
{
    return new Button(text, Button::Variant::Primary, parent);
}

Button* secondaryButton(const QString& text, QWidget* parent)
{
    return new Button(text, Button::Variant::Secondary, parent);
}

Button* ghostButton(const QString& text, QWidget* parent)
{
    return new Button(text, Button::Variant::Ghost, parent);
}

Button* dangerButton(const QString& text, QWidget* parent)
{
    return new Button(text, Button::Variant::Danger, parent);
}

Button* successButton(const QString& text, QWidget* parent)
{
    return new Button(text, Button::Variant::Success, parent);
}

Button* detachButton(QWidget* parent)
{
    // U+29C9 "two joined squares" reads as a window pulled out of another,
    // and the word stays so the gesture is not left to a glyph alone.
    auto* button = new Button(QStringLiteral("⧉  Detach"),
                              Button::Variant::Ghost, parent);
    button->setToolTip(QObject::tr("Detach this panel into its own window"));
    return button;
}

} // namespace novu::ui
