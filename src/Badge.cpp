#include "novu/ui/Badge.h"
#include "novu/ui/Theme.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

namespace novu::ui {

namespace {

// Owned by the component, not by any page.
constexpr int kPaddingH = 10;
constexpr int kPaddingV = 4;

struct ToneColors { QColor fg; QColor bg; };

ToneColors colorsFor(Badge::Tone tone)
{
    const ThemeColors& t = theme();
    auto blend = [](const QColor& c, double alpha) {
        QColor out = c;
        out.setAlphaF(alpha);
        return out;
    };

    switch (tone) {
    case Badge::Tone::Success: return {t.success, blend(t.success, 0.16)};
    case Badge::Tone::Warning: return {t.warning, blend(t.warning, 0.16)};
    case Badge::Tone::Danger:  return {t.danger,  blend(t.danger, 0.16)};
    case Badge::Tone::Info:    return {t.accentLight, blend(t.accentLight, 0.16)};
    case Badge::Tone::Neutral:
    default:                   return {t.textSecondary, blend(t.textSecondary, 0.12)};
    }
}

QFont badgeFont(const QWidget* widget)
{
    QFont font = widget->font();
    font.setPixelSize(metrics::fontSizeSmall);
    font.setWeight(QFont::DemiBold);
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
                 fm.height() + 2 * kPaddingV);
}

void Badge::paintEvent(QPaintEvent*)
{
    const ToneColors colors = colorsFor(m_tone);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF pill = rect().adjusted(0, 0, -1, -1);
    const double radius = pill.height() / 2.0;

    painter.setPen(Qt::NoPen);
    painter.setBrush(colors.bg);
    painter.drawRoundedRect(pill, radius, radius);

    painter.setFont(badgeFont(this));
    painter.setPen(colors.fg);
    painter.drawText(rect(), Qt::AlignCenter, m_text);
}

} // namespace novu::ui
