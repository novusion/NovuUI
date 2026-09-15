#include "novu/ui/AppMark.h"

#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainter>
#include <QPen>

namespace novu::ui {

namespace {

// `.appmark { 29px }`, `.rail-mark { 30px }`, `.bigmark { 58px }`.
constexpr int kSmallSide = 29;
constexpr int kMediumSide = 30;
constexpr int kLargeSide = 58;

// The design's radii at those sizes - 8, 9 and 16 - are all close to
// `side * 0.28`, so the corner is derived rather than tabulated. A fourth size
// then rounds like the other three instead of needing a new number.
constexpr double kRadiusRatio = 0.28;

// `.appmark svg { 15px }` inside 29, `.bigmark svg { 26px }` inside 58 - half
// the plaque, near enough, in both.
constexpr double kGlyphRatio = 0.5;
constexpr double kInitialsRatio = 0.40;

// `.avatar { width:26px }` — an avatar is a step smaller than the plaque it
// shares a drawing with, because it sits in a message row rather than in a
// header.
constexpr int kAvatarSide = 26;

// The design's violet for an agent. It is not in ThemeColors and must not be:
// it names a kind of participant, not a status and not the product, so no
// profile gets to reinterpret it. Stated once, here.
const QColor kAgentHue(QStringLiteral("#8B6FD4"));
const QColor kAgentInk(QStringLiteral("#C9B6F5"));

int sideFor(AppMark::Size size)
{
    switch (size) {
    case AppMark::Size::Small: return kSmallSide;
    case AppMark::Size::Large: return kLargeSide;
    case AppMark::Size::Medium: break;
    }
    return kMediumSide;
}

} // namespace

AppMark::AppMark(Glyph glyph, QWidget* parent, Size size)
    : QWidget(parent)
    , m_glyph(glyph)
    , m_size(size)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

AppMark::AppMark(const QString& initials, QWidget* parent, Size size)
    : AppMark(Glyph::None, parent, size)
{
    setInitials(initials);
}

AppMark* AppMark::avatar(const QString& initials, QWidget* parent, Tone tone, Size size)
{
    auto* mark = new AppMark(initials, parent, size);
    mark->setShape(Shape::Round);
    mark->setTone(tone);
    // An avatar is never "the product": it is a party. Accented would repaint
    // its glyph in the application accent, which is the opposite of the point.
    mark->setAccented(false);
    return mark;
}

void AppMark::setShape(Shape shape)
{
    if (m_shape == shape) {
        return;
    }
    m_shape = shape;
    updateGeometry();
    update();
}

void AppMark::setTone(Tone tone)
{
    if (m_tone == tone) {
        return;
    }
    m_tone = tone;
    update();
}

void AppMark::setGlyph(Glyph glyph)
{
    if (m_glyph == glyph) {
        return;
    }
    m_glyph = glyph;
    // A glyph replaces letters rather than joining them.
    if (glyph != Glyph::None) {
        m_initials.clear();
    }
    update();
}

void AppMark::setInitials(const QString& initials)
{
    const QString trimmed = initials.trimmed().left(2).toUpper();
    if (m_initials == trimmed) {
        return;
    }
    m_initials = trimmed;
    if (!m_initials.isEmpty()) {
        m_glyph = Glyph::None;
    }
    update();
}

void AppMark::setMarkSize(Size size)
{
    if (m_size == size) {
        return;
    }
    m_size = size;
    updateGeometry();
    update();
}

void AppMark::setSideOverride(int px)
{
    const int clamped = qMax(0, px);
    if (m_sideOverride == clamped) {
        return;
    }
    m_sideOverride = clamped;
    updateGeometry();
    update();
}

void AppMark::setAccented(bool accented)
{
    if (m_accented == accented) {
        return;
    }
    m_accented = accented;
    update();
}

int AppMark::side() const
{
    if (m_sideOverride > 0) {
        return m_sideOverride;
    }
    // A Small round mark is the design's 26px avatar rather than the 29px
    // plaque; everything above it shares the plaque's scale.
    if (m_shape == Shape::Round && m_size == Size::Small) {
        return kAvatarSide;
    }
    return sideFor(m_size);
}

QSize AppMark::sizeHint() const
{
    return QSize(side(), side());
}

StyleSpec AppMark::styleSpec() const
{
    StyleSpec spec;
    // The plate is a gradient between two surfaces, which a single token
    // cannot say. The spec names the top of it, which is what a test asserting
    // "the mark sits on the panel colour" is actually asking about.
    spec.background = token(ColorRole::PanelBg);
    spec.border = token(ColorRole::Border);
    spec.foreground = m_accented ? token(ColorRole::AccentLight)
                                 : token(ColorRole::TextSecondary);

    // `.avatar.acc` — the current user, in the product accent. Agent has no
    // token because its violet is not in any profile's palette; it is resolved
    // in paintEvent() from the constant above, for the reason stated there.
    if (m_tone == Tone::Accent) {
        spec.background = token(ColorRole::Accent, 0.16);
        spec.border = token(ColorRole::Accent, 0.45);
        spec.foreground = token(ColorRole::Accent);
    }

    spec.radius = SurfaceRole::Card;
    spec.borderWidth = BorderRole::Card;
    spec.type = TypeRole::CardTitle;
    return spec;
}

ResolvedStyle AppMark::resolvedStyle() const
{
    return resolve(styleSpec(), this, side());
}

bool AppMark::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void AppMark::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const int edge = side();
    const int borderWidth = qMax(1, profile.border(BorderRole::Card));

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF plate = QRectF(0, 0, edge, edge)
                             .adjusted(borderWidth / 2.0, borderWidth / 2.0,
                                       -borderWidth / 2.0, -borderWidth / 2.0);
    // A circle is the same rounded rectangle with the radius run to half the
    // edge, so Round and Plaque are one drawing rather than two paths that
    // would drift in their border weight and their gradient.
    const double radius = m_shape == Shape::Round ? plate.height() / 2.0
                                                  : edge * kRadiusRatio;

    QColor fill;
    QColor outline = profile.colors.border;
    QColor ink = m_accented ? profile.colors.accentLight : profile.colors.textSecondary;

    switch (m_tone) {
    case Tone::Accent:
        fill = withAlpha(profile.colors.accent, 0.16);
        outline = withAlpha(profile.colors.accent, 0.45);
        ink = profile.colors.accent;
        break;
    case Tone::Agent:
        fill = withAlpha(kAgentHue, 0.22);
        outline = withAlpha(kAgentHue, 0.50);
        ink = kAgentInk;
        break;
    case Tone::Neutral:
        break;
    }

    painter.setPen(QPen(outline, borderWidth));
    if (fill.isValid()) {
        painter.setBrush(fill);
    } else {
        // `linear-gradient(145deg, panel, card)` — diagonal, not vertical. At
        // this size the difference is the whole of why the plaque reads as
        // raised. A toned mark is a flat tint instead, because a gradient under
        // a colour reads as a gradient in that colour.
        QLinearGradient ground(plate.topLeft(), plate.bottomRight());
        ground.setColorAt(0.0, profile.colors.panelBg);
        ground.setColorAt(1.0, profile.colors.panelBgAlt);
        painter.setBrush(ground);
    }
    painter.drawRoundedRect(plate, radius, radius);

    if (m_glyph != Glyph::None) {
        const double glyphSide = edge * kGlyphRatio;
        const double inset = (edge - glyphSide) / 2.0;
        paintGlyph(&painter, m_glyph,
                   QRectF(inset, inset, glyphSide, glyphSide), ink);
        return;
    }

    if (m_initials.isEmpty()) {
        return;
    }
    QFont font = roleFont(this, TypeRole::CardTitle, true);
    font.setPixelSize(qMax(8, static_cast<int>(edge * kInitialsRatio)));
    painter.setFont(font);
    painter.setPen(ink);
    painter.drawText(QRectF(0, 0, edge, edge), Qt::AlignCenter, m_initials);
}

} // namespace novu::ui
