#include "novu/ui/Button.h"
#include "novu/ui/Theme.h"

#include <QFontMetrics>

namespace novu::ui {

namespace {

// Owned by the component.
constexpr int kPaddingH = 14;
constexpr int kPaddingV = 7;
constexpr int kMinHeight = 28;

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

QSize Button::sizeHint() const
{
    // Derived from this button's own font and padding, so it is correct
    // immediately and independent of when the style sheet gets applied.
    const QFontMetrics fm(font());
    return QSize(fm.horizontalAdvance(text()) + 2 * kPaddingH,
                 qMax(kMinHeight, fm.height() + 2 * kPaddingV));
}

void Button::applyVariant()
{
    const ThemeColors& t = theme();

    QColor background;
    QColor foreground;
    QColor borderColor;
    QColor hover;

    switch (m_variant) {
    case Variant::Primary:
        background = t.accent;
        foreground = t.textOnAccent;
        borderColor = t.accent;
        hover = t.accent.lighter(115);
        break;
    case Variant::Danger:
        background = QColor(t.danger.red(), t.danger.green(), t.danger.blue(), 36);
        foreground = t.danger;
        borderColor = QColor(t.danger.red(), t.danger.green(), t.danger.blue(), 100);
        hover = QColor(t.danger.red(), t.danger.green(), t.danger.blue(), 60);
        break;
    case Variant::Ghost:
        background = Qt::transparent;
        foreground = t.textSecondary;
        borderColor = t.border;
        hover = t.surfaceHover;
        break;
    case Variant::Secondary:
    default:
        background = t.headerBg;
        foreground = t.textPrimary;
        borderColor = t.border;
        hover = t.surfaceHover;
        break;
    }

    // Padding is intentionally NOT declared here — it lives in sizeHint()
    // above so that geometry has exactly one source of truth.
    setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %3;"
        " border-radius: %4px; font-weight: 600; }"
        "QPushButton:hover { background: %5; }"
        "QPushButton:disabled { color: %6; border-color: %7; background: transparent; }")
        .arg(background.alpha() == 0 ? QStringLiteral("transparent") : rgba(background, background.alphaF()),
             foreground.name(),
             rgba(borderColor, borderColor.alphaF()))
        .arg(metrics::radiusFlat)
        .arg(rgba(hover, hover.alphaF()),
             t.textDisabled.name(),
             t.borderSubtle.name()));

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

} // namespace novu::ui
