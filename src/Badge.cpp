#include "novu/ui/Badge.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

namespace novu::ui {

namespace {

// Owned by the component, not by any page. The two sizes are the approved
// design's `.badge` and `.badge.lg`.
constexpr int kPaddingH = 9;
constexpr int kPaddingV = 3;
constexpr int kPaddingHLarge = 11;
constexpr int kPaddingVLarge = 5;
// A badge in a card header would otherwise stretch to the height of the whole
// header row and stop reading as a pill.
constexpr int kMinHeight = 22;
constexpr int kMinHeightLarge = 27;

// The dot, and the gap between it and the word. Both are the design's.
constexpr int kDotDiameter = 5;
constexpr int kDotGap = 6;

int paddingH(Badge::Size size)
{
    return size == Badge::Size::Large ? kPaddingHLarge : kPaddingH;
}

int paddingV(Badge::Size size)
{
    return size == Badge::Size::Large ? kPaddingVLarge : kPaddingV;
}

int minHeight(Badge::Size size)
{
    return size == Badge::Size::Large ? kMinHeightLarge : kMinHeight;
}

// A badge tone is a semantic role. The mapping is the whole of it: the ink and
// the plate come from the shared palette, so a Success badge is the same green
// in every Novusion application and a theme switch does not move it.
//
// Info used to resolve to accentLight - the application accent - which made it
// sky blue in NovuTrade, gold in Warm Gray and violet in Dark Purple. It is a
// status now, like the other three.
//
// Neutral stays the profile's own secondary ink, tinted the same 0.16 as every
// other tone: it is the absence of a status, and at 0.12 it read as a bare word
// next to a row of filled pills.
//
// Accent is the one tone that is *not* in the semantic palette, and deliberately
// so - it is the product, not a meaning. It is resolved from the profile's
// accent in paintEvent() rather than given a SemanticRole here, because adding
// it to that enum would let a status resolve to the product accent again, which
// is the defect the shared palette was introduced to remove.
SemanticRole roleFor(Badge::Tone tone)
{
    switch (tone) {
    case Badge::Tone::Success: return SemanticRole::Success;
    case Badge::Tone::Warning: return SemanticRole::Warning;
    case Badge::Tone::Danger:  return SemanticRole::Danger;
    case Badge::Tone::Info:    return SemanticRole::Info;
    case Badge::Tone::Accent:
    case Badge::Tone::Neutral: break;
    }
    return SemanticRole::Neutral;
}

// Monospace, at the smallest step on the scale.
//
// This is the approved design's choice and not a stylistic one: badges sit in
// columns, and a column of proportional pills has a ragged left edge wherever
// the words differ in width. The mono family also makes "12/40" and "3 of 8"
// line up with the numbers in the cells above and below them.
QFont badgeFont(const QWidget* widget, Badge::Size size)
{
    const ThemeProfile& profile = themeFor(widget);
    QFont font = roleMonoFont(widget, TypeRole::Mono);
    font.setPixelSize(size == Badge::Size::Large ? profile.type.xs : profile.type.xxs);
    font.setWeight(QFont::Medium);
    // The design's .04em tracking. A pill is a handful of characters inside a
    // tinted plate; without it they read as one blob at 10px.
    font.setLetterSpacing(QFont::PercentageSpacing, 104);
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

void Badge::setSize(Size size)
{
    if (m_size == size) {
        return;
    }
    m_size = size;
    updateGeometry();
    update();
}

void Badge::setDot(bool dot)
{
    if (m_dot == dot) {
        return;
    }
    m_dot = dot;
    updateGeometry(); // the dot and its gap are part of the width
    update();
}

void Badge::setUppercase(bool uppercase)
{
    if (m_uppercase == uppercase) {
        return;
    }
    m_uppercase = uppercase;
    updateGeometry();
    update();
}

QString Badge::displayText() const
{
    return m_uppercase ? m_text.toUpper() : m_text;
}

QSize Badge::sizeHint() const
{
    const QFontMetrics fm(badgeFont(this, m_size));
    const int lead = m_dot ? kDotDiameter + kDotGap : 0;
    return QSize(fm.horizontalAdvance(displayText()) + lead + 2 * paddingH(m_size),
                 qMax(minHeight(m_size), fm.height() + 2 * paddingV(m_size)));
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

    SemanticColor colors = profile.semantic.role(roleFor(m_tone));
    if (m_tone == Tone::Accent) {
        colors.text = profile.colors.accentLight;
        colors.fill = withAlpha(profile.colors.accent, 0.12);
        colors.border = withAlpha(profile.colors.accent, 0.42);
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int borderWidth = qMax(1, profile.border(BorderRole::Card));
    const QRectF pill = QRectF(rect()).adjusted(borderWidth / 2.0, borderWidth / 2.0,
                                                -borderWidth / 2.0, -borderWidth / 2.0);
    const double radius = resolveRadius(profile.radius(SurfaceRole::Badge),
                                        static_cast<int>(pill.height()));

    // The outline is what separates a badge from a coloured word. Without it a
    // Neutral badge on the panel it sits in has no edge at all, which is how
    // the tinted plate stopped reading as a pill.
    painter.setPen(QPen(colors.border, borderWidth));
    painter.setBrush(colors.fill);
    painter.drawRoundedRect(pill, radius, radius);

    const QFont font = badgeFont(this, m_size);
    painter.setFont(font);
    painter.setPen(colors.text);

    QRect textBox = rect().adjusted(paddingH(m_size), 0, -paddingH(m_size), 0);
    if (m_dot) {
        const QRectF dot(textBox.left(),
                         (height() - kDotDiameter) / 2.0,
                         kDotDiameter, kDotDiameter);
        painter.setPen(Qt::NoPen);
        painter.setBrush(colors.text);
        painter.drawEllipse(dot);
        painter.setPen(colors.text);
        textBox.setLeft(textBox.left() + kDotDiameter + kDotGap);
    }
    painter.drawText(textBox, Qt::AlignVCenter | Qt::AlignLeft, displayText());
}

Badge* pill(const QString& text, QWidget* parent)
{
    auto* badge = new Badge(text, Badge::Tone::Neutral, parent);
    badge->setUppercase(true);
    return badge;
}

Badge* statusBadge(const QString& text, Badge::Tone tone, QWidget* parent)
{
    auto* badge = new Badge(text, tone, parent);
    badge->setDot(true);
    return badge;
}

} // namespace novu::ui
