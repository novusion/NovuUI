#include "novu/ui/Badge.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

namespace novu::ui {

namespace {

// Owned by the component, not by any page.
constexpr int kPaddingH = 9;
constexpr int kPaddingV = 3;
// A badge in a card header would otherwise stretch to the height of the whole
// header row and stop reading as a pill.
constexpr int kMinHeight = 22;

struct ToneColors { QColor fg; QColor bg; };

ToneColors colorsFor(Badge::Tone tone, const ThemeProfile& profile)
{
    const ThemeColors& t = profile.colors;

    switch (tone) {
    case Badge::Tone::Success: return {t.success, withAlpha(t.success, 0.16)};
    case Badge::Tone::Warning: return {t.warning, withAlpha(t.warning, 0.16)};
    case Badge::Tone::Danger:  return {t.danger,  withAlpha(t.danger, 0.16)};
    case Badge::Tone::Info:    return {t.accentLight, withAlpha(t.accentLight, 0.16)};
    case Badge::Tone::Neutral:
    // 0.16 like every other tone. Neutral was tinted at 0.12 and read as a
    // bare word next to a row of filled pills.
    default:                   return {t.textSecondary, withAlpha(t.textSecondary, 0.16)};
    }
}

QFont badgeFont(const QWidget* widget)
{
    QFont font = widget->font();
    font.setPixelSize(themeFor(widget).type.xs);
    font.setWeight(QFont::Bold);
    return font;
}

} // namespace

Badge::Badge(QWidget* parent)
    : QWidget(parent)
{
    // Fixed in both directions: the badge is exactly its sizeHint(), so no
    // layout can squeeze it below its text.
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAttribute(Qt::WA_TranslucentBackground, false);
}

Badge::Badge(const QString& text, Tone tone, QWidget* parent)
    : Badge(parent)
{
    m_text = text;
    m_tone = tone;
}

void Badge::setText(const QString& text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    updateGeometry(); // width depends on the text
    update();
}

void Badge::setTone(Tone tone)
{
    if (m_tone == tone) {
        return;
    }
    m_tone = tone;
    update();
}

QSize Badge::sizeHint() const
{
    const QFontMetrics fm(badgeFont(this));
    return QSize(fm.horizontalAdvance(m_text) + 2 * kPaddingH,
                 qMax(kMinHeight, fm.height() + 2 * kPaddingV));
}

bool Badge::event(QEvent* event)
{
    // The scope that decides the palette may be installed after the badge is
    // constructed, so the tokens are re-read rather than captured once.
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void Badge::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const ToneColors colors = colorsFor(m_tone, profile);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF pill = rect().adjusted(0, 0, -1, -1);
    const double radius = resolveRadius(profile.radius(SurfaceRole::Badge),
                                        static_cast<int>(pill.height()));

    painter.setPen(Qt::NoPen);
    painter.setBrush(colors.bg);
    painter.drawRoundedRect(pill, radius, radius);

    painter.setFont(badgeFont(this));
    painter.setPen(colors.fg);
    painter.drawText(rect(), Qt::AlignCenter, m_text);
}

} // namespace novu::ui
