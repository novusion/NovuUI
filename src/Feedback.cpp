#include "novu/ui/Feedback.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

QColor toneColor(const QWidget* context, Badge::Tone tone)
{
    const ThemeColors& c = themeFor(context).colors;
    switch (tone) {
    case Badge::Tone::Success: return c.success;
    case Badge::Tone::Warning: return c.warning;
    case Badge::Tone::Danger:  return c.danger;
    case Badge::Tone::Info:    return c.accentLight;
    case Badge::Tone::Neutral:
    default:                   return c.textSecondary;
    }
}

} // namespace

EmptyState::EmptyState(const QString& glyph, const QString& title,
                       const QString& subtitle, QWidget* parent)
    : QWidget(parent)
{
    const ThemeProfile& profile = themeFor(this);

    auto* column = new QVBoxLayout(this);
    column->setSpacing(profile.space(SpacingRole::FormRowGap) / 2);
    column->setAlignment(Qt::AlignCenter);

    auto* icon = label(glyph, this, TypeRole::PageTitle, Ink::Muted);
    icon->setFont(iconFont(profile.icons.large + 6));
    icon->setAlignment(Qt::AlignCenter);
    column->addWidget(icon);

    m_title = label(title, this, TypeRole::CardTitle, Ink::Secondary, true);
    m_title->setAlignment(Qt::AlignCenter);
    column->addWidget(m_title);

    m_subtitle = label(subtitle, this, TypeRole::Caption, Ink::Muted);
    m_subtitle->setAlignment(Qt::AlignCenter);
    column->addWidget(m_subtitle);
}

void EmptyState::setTitle(const QString& title)
{
    m_title->setText(title);
}

void EmptyState::setSubtitle(const QString& subtitle)
{
    m_subtitle->setText(subtitle);
}

InfoBanner::InfoBanner(const QString& text, Badge::Tone tone, QWidget* parent,
                       QWidget* trailing)
    : QFrame(parent)
    , m_tone(tone)
{
    const ThemeProfile& profile = themeFor(this);

    // Painted, not styled - see the header for why.
    setAttribute(Qt::WA_StyledBackground, false);
    setFixedHeight(profile.controlHeight(ControlSize::Medium) + 6);

    auto* row = new QHBoxLayout(this);
    // The medium step, not the form-row gap: a banner is a strip of prose and
    // the tighter gap left its sentence sitting on the tinted edge.
    row->setContentsMargins(profile.spacing.md, 0, profile.spacing.md, 0);

    m_label = label(text, this, TypeRole::Caption, Ink::Default);
    setInk(m_label, toneColor(this, tone));
    row->addWidget(m_label);
    row->addStretch(1);

    if (trailing) {
        trailing->setParent(this);
        row->addWidget(trailing);
    }
}

void InfoBanner::setText(const QString& text)
{
    m_label->setText(text);
}

void InfoBanner::setTone(Badge::Tone tone)
{
    m_tone = tone;
    setInk(m_label, toneColor(this, tone));
    update();
}

void InfoBanner::paintEvent(QPaintEvent*)
{
    const QColor tone = toneColor(this, m_tone);
    // Button, not Badge.
    //
    // A badge is a pill because it is a word-sized token. A banner is a
    // control-height strip of prose, and asking for the badge radius turned
    // it into a lozenge — most visibly when it carries a trailing action,
    // where a fully rounded strip wrapped a button with 6px corners and the
    // two shapes contradicted each other.
    //
    // Button is the role a banner shares its rhythm with: it stands at
    // control height, next to controls, and often contains one.
    const int radius = resolveRadius(themeFor(this).radius(SurfaceRole::Button), height());

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path;
    path.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);

    QColor fill = tone;
    fill.setAlphaF(0.10f);
    p.fillPath(path, fill);

    QColor outline = tone;
    outline.setAlphaF(0.35f);
    p.setPen(QPen(outline, 1));
    p.drawPath(path);
}

ProgressBar::ProgressBar(int value, QWidget* parent)
    : QWidget(parent)
    , m_value(value)
{
    setFixedHeight(6);
}

void ProgressBar::setValue(int value)
{
    const int clamped = qBound(0, value, 100);
    if (clamped == m_value) {
        return;
    }
    m_value = clamped;
    update();
}

void ProgressBar::paintEvent(QPaintEvent*)
{
    const ThemeColors& c = themeFor(this).colors;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(c.panelBgAlt);
    p.drawRoundedRect(rect(), 3, 3);

    if (m_value > 0) {
        p.setBrush(c.accent);
        p.drawRoundedRect(QRect(0, 0, width() * m_value / 100, height()), 3, 3);
    }
}

} // namespace novu::ui
