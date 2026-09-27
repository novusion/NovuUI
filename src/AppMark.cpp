#include "novu/ui/AppMark.h"

#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QGuiApplication>
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

// The family mark, in its own 48x48 space: a left face, a right face and a
// top face. The numbers are the design's own polygons; only the tints are
// resolved, so a NovuHub gem and a NovuAI gem are one drawing.
// Three faces from one hue, or three faces stated outright. The isometric
// polygons are the same either way; only where the colours come from differs.
void paintGem(QPainter* painter, const QRectF& box, const QColor& accent,
              const QColor* left, const QColor* right, const QColor* top)
{
    static const QPointF kLeft[] = {{23.58, 23.76}, {23.58, 10.74},
                                    {12.30, 17.25}, {12.30, 30.27}};
    static const QPointF kRight[] = {{24.42, 23.76}, {24.42, 10.74},
                                     {35.70, 17.25}, {35.70, 30.27}};
    static const QPointF kTop[] = {{24.00, 24.49}, {35.27, 31.00},
                                   {24.00, 37.51}, {12.73, 31.00}};

    const double scale = box.width() / 48.0;
    painter->save();
    painter->translate(box.topLeft());
    painter->scale(scale, scale);
    painter->setPen(Qt::NoPen);

    // One hue, three faces: the light the cube is under is the only thing
    // that separates them, so they are mixes of the accent rather than three
    // colours a profile would have to keep in step.
    const auto mix = [](const QColor& a, const QColor& b, double t) {
        return QColor(int(a.red() + (b.red() - a.red()) * t),
                      int(a.green() + (b.green() - a.green()) * t),
                      int(a.blue() + (b.blue() - a.blue()) * t));
    };
    painter->setBrush(left && left->isValid() ? *left : mix(accent, QColor(0, 0, 0), 0.22));
    painter->drawPolygon(kLeft, 4);
    painter->setBrush(right && right->isValid() ? *right : accent);
    painter->drawPolygon(kRight, 4);
    painter->setBrush(top && top->isValid() ? *top : mix(accent, QColor(255, 255, 255), 0.33));
    painter->drawPolygon(kTop, 4);
    painter->restore();
}

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

void AppMark::setGemTints(const QColor& left, const QColor& right, const QColor& top)
{
    m_gemTinted = left.isValid() && right.isValid() && top.isValid();
    m_gemLeft = left;
    m_gemRight = right;
    m_gemTop = top;
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
    } else if (m_tone == Tone::Danger) {
        spec.background = statusToken(SemanticRole::Danger, ColorRole::StatusText, 0.13);
        spec.border = statusToken(SemanticRole::Danger, ColorRole::StatusText, 0.45);
        spec.foreground = statusToken(SemanticRole::Danger, ColorRole::StatusText);
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

    // The family mark is the drawing, not something drawn on a plate, so it
    // takes none of the plate's fill, outline or ink.
    if (m_shape == Shape::Gem) {
        paintGem(&painter, QRectF(0, 0, edge, edge), profile.colors.accent,
                 m_gemTinted ? &m_gemLeft : nullptr,
                 m_gemTinted ? &m_gemRight : nullptr,
                 m_gemTinted ? &m_gemTop : nullptr);
        return;
    }

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
    case Tone::Danger:
        fill = withAlpha(profile.semantic.danger.text, 0.13);
        outline = withAlpha(profile.semantic.danger.text, 0.45);
        ink = profile.semantic.danger.text;
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
        // `--panel` to `--card`: the two surfaces above the page ground, not
        // the page ground itself - a plaque mixed from panelBg sits a step too
        // dark and stops reading as raised against the card it is on.
        ground.setColorAt(0.0, profile.colors.panelBgAlt);
        ground.setColorAt(1.0, profile.colors.surfaceHover);
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

// ── the product mark ────────────────────────────────────────────────────

namespace {

// The design's own polygon points, in its own 48x48 box - see novuMark() in
// the approved NovuSpace mockup. Copied rather than re-derived: three
// isometric faces of a cube have no formula worth inventing when the design
// already states the coordinates.
constexpr QPointF kFaceLeft[] = {
    {23.58, 23.76}, {23.58, 10.74}, {12.30, 17.25}, {12.30, 30.27},
};
constexpr QPointF kFaceRight[] = {
    {24.42, 23.76}, {24.42, 10.74}, {35.70, 17.25}, {35.70, 30.27},
};
constexpr QPointF kFaceBottom[] = {
    {24.00, 24.49}, {35.27, 31.00}, {24.00, 37.51}, {12.73, 31.00},
};
constexpr double kViewBox = 48.0;

} // namespace

void paintBrandMark(QPainter* painter, const QRectF& box, const QWidget* context)
{
    if (!painter || box.isEmpty()) {
        return;
    }
    const ThemeColors& c = themeFor(context).colors;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->translate(box.topLeft());
    const double scale = qMin(box.width(), box.height()) / kViewBox;
    painter->scale(scale, scale);
    painter->setPen(Qt::NoPen);

    // Left and bottom are the shared neutrals every product draws this in -
    // `#B7C1CE` / `#8793A3` in the design, which are this profile's own
    // textSecondary / textDisabled. Only the right face is the product.
    painter->setBrush(c.textSecondary);
    painter->drawPolygon(kFaceLeft, 4);
    painter->setBrush(c.accent);
    painter->drawPolygon(kFaceRight, 4);
    painter->setBrush(c.textDisabled);
    painter->drawPolygon(kFaceBottom, 4);
    painter->restore();
}

QPixmap brandMarkPixmap(int size, const QWidget* context, qreal devicePixelRatio)
{
    if (size <= 0) {
        return {};
    }
    const qreal ratio = devicePixelRatio > 0.0 ? devicePixelRatio
                                               : (qApp ? qApp->devicePixelRatio() : 1.0);
    QPixmap pixmap(QSize(size, size) * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    paintBrandMark(&painter, QRectF(0, 0, size, size), context);
    return pixmap;
}

QIcon brandMarkIcon(int size, const QWidget* context)
{
    QIcon icon;
    icon.addPixmap(brandMarkPixmap(size, context));
    return icon;
}

} // namespace novu::ui
