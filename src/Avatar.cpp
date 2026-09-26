#include "novu/ui/Avatar.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>

namespace novu::ui {

namespace {

// The plate a name gets when there is no portrait.
//
// Derived from the name rather than stored, so the same person is the same
// colour in every window and nothing has to keep a colour per user. Taken from
// the profile's chart series, which is the one categorical palette in the
// system that is already checked against the surfaces - inventing a second set
// of "nice colours for avatars" is how a product ends up with two palettes.
QColor plateFor(const ThemeProfile& profile, const QString& name)
{
    const QList<QColor>& series = profile.chart.series;
    if (series.isEmpty()) {
        return profile.colors.surfaceHover;
    }
    // A sum of the code points rather than qHash: qHash is salted per process
    // since Qt 5, so the same name would be a different colour on every run.
    uint sum = 0;
    for (const QChar c : name) {
        sum += c.unicode();
    }
    return series.at(static_cast<int>(sum % static_cast<uint>(series.size())));
}

} // namespace

Avatar::Avatar(const QString& name, QWidget* parent)
    : QWidget(parent)
    , m_name(name)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

Avatar::Avatar(const QString& name, int size, QWidget* parent)
    : Avatar(name, parent)
{
    setDiameter(size);
}

void Avatar::setName(const QString& name)
{
    if (m_name == name) {
        return;
    }
    m_name = name;
    update();
}

QString Avatar::initials() const
{
    // Words first; failing that, the parts of an address; failing that, the
    // first letter of whatever there is. An avatar with nothing in it is worse
    // than one with a single character.
    QString source = m_name.trimmed();
    if (source.isEmpty()) {
        return {};
    }
    const int at = source.indexOf(QLatin1Char('@'));
    if (at > 0 && !source.contains(QLatin1Char(' '))) {
        source = source.left(at);
        source.replace(QLatin1Char('.'), QLatin1Char(' '));
        source.replace(QLatin1Char('_'), QLatin1Char(' '));
        source.replace(QLatin1Char('-'), QLatin1Char(' '));
    }

    QString out;
    const QStringList words = source.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const QString& word : words) {
        if (out.size() >= 2) {
            break;
        }
        out.append(word.at(0));
    }
    if (out.isEmpty()) {
        out = source.left(1);
    }
    return out.toUpper();
}

void Avatar::setPortrait(const QPixmap& portrait)
{
    m_portrait = portrait;
    update();
}

void Avatar::setDiameter(int px)
{
    px = qMax(8, px);
    if (m_diameter == px) {
        return;
    }
    m_diameter = px;
    updateGeometry();
    update();
}

void Avatar::setShape(Shape shape)
{
    if (m_shape == shape) {
        return;
    }
    m_shape = shape;
    update();
}

void Avatar::setHighlighted(bool highlighted)
{
    if (m_highlighted == highlighted) {
        return;
    }
    m_highlighted = highlighted;
    update();
}

void Avatar::setCornerRadius(int px)
{
    if (m_cornerRadius == px) {
        return;
    }
    m_cornerRadius = px;
    update();
}

void Avatar::setPlate(const QColor& plate)
{
    if (m_plate == plate && !m_plateTo.isValid()) {
        return;
    }
    m_plate = plate;
    m_plateTo = QColor();
    update();
}

void Avatar::setPlateGradient(const QColor& from, const QColor& to)
{
    m_plate = from;
    m_plateTo = to;
    update();
}

void Avatar::setGlyph(Glyph glyph)
{
    if (m_glyph == glyph) {
        return;
    }
    m_glyph = glyph;
    update();
}

void Avatar::setPresence(Presence presence)
{
    if (m_presence == presence) {
        return;
    }
    m_presence = presence;
    update();
}

StyleSpec Avatar::styleSpec() const
{
    StyleSpec spec;
    spec.radius = m_shape == Shape::Round ? SurfaceRole::Badge : SurfaceRole::Button;
    spec.borderWidth = BorderRole::Card;
    spec.type = TypeRole::Label;
    // The plate is derived from the name, so it is not a token - what *is* a
    // token is the ink on it and the outline round it.
    spec.foreground = token(ColorRole::TextPrimary);
    spec.border = token(ColorRole::BorderHover);
    spec.checkedBorder = token(ColorRole::Accent, 0.65);
    return spec;
}

ResolvedStyle Avatar::resolvedStyle() const
{
    return resolve(styleSpec(), this, m_diameter);
}

QSize Avatar::sizeHint() const
{
    return QSize(m_diameter, m_diameter);
}

bool Avatar::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void Avatar::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const ResolvedStyle style = resolvedStyle();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const double border = style.borderWidth;
    const QRectF box = QRectF(0, 0, m_diameter, m_diameter)
                           .adjusted(border / 2.0, border / 2.0, -border / 2.0, -border / 2.0);
    const double radius = m_shape == Shape::Round
        ? box.height() / 2.0
        : (m_cornerRadius >= 0 ? double(m_cornerRadius) : style.radius);

    QPainterPath shape;
    shape.addRoundedRect(box, radius, radius);

    if (m_plate.isValid() && m_plateTo.isValid()) {
        // The design's assistant mark: `linear-gradient(145deg, ...)`, which on
        // a square box is the diagonal.
        QLinearGradient wash(box.topLeft(), box.bottomRight());
        wash.setColorAt(0.0, m_plate);
        wash.setColorAt(1.0, m_plateTo);
        painter.fillPath(shape, wash);
    } else if (m_plate.isValid()) {
        // Stated by the caller, so painted as stated - see setPlate().
        painter.fillPath(shape, m_plate);
    } else {
        QColor plate = plateFor(profile, m_name);
        // Tinted, not solid: a wall of fully saturated avatars in a list is a
        // wall, and the initials have to stay legible on it.
        plate.setAlphaF(m_highlighted ? 0.42 : 0.26);
        painter.fillPath(shape, plate);
    }

    if (!m_portrait.isNull()) {
        painter.save();
        painter.setClipPath(shape);
        const QPixmap scaled = m_portrait.scaled(size() * devicePixelRatioF(),
                                                 Qt::KeepAspectRatioByExpanding,
                                                 Qt::SmoothTransformation);
        const QRectF target(0, 0, m_diameter, m_diameter);
        painter.drawPixmap(target, scaled,
                           QRectF((scaled.width() - target.width() * devicePixelRatioF()) / 2.0,
                                  (scaled.height() - target.height() * devicePixelRatioF()) / 2.0,
                                  target.width() * devicePixelRatioF(),
                                  target.height() * devicePixelRatioF()));
        painter.restore();
    } else if (m_glyph != Glyph::None) {
        // Proportioned to the avatar the same way the initials are, so a group
        // mark and a set of initials sit on one optical size.
        const double side = m_diameter * 0.45;
        const QRectF mark((m_diameter - side) / 2.0, (m_diameter - side) / 2.0, side, side);
        paintGlyph(&painter, m_glyph, mark, style.foreground, 0.0, 0.0, this);
    } else {
        QFont font = roleFont(this, TypeRole::Label, true);
        // Proportional to the avatar, so one component covers a 20px row mark
        // and a 64px account portrait without a size table.
        font.setPixelSize(qMax(8, static_cast<int>(m_diameter * 0.42)));
        painter.setFont(font);
        painter.setPen(style.foreground);
        painter.drawText(QRectF(0, 0, m_diameter, m_diameter), Qt::AlignCenter, initials());
    }

    const QColor outline = m_highlighted ? style.checkedBorder : style.border;
    painter.setPen(QPen(outline, border));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(shape);

    paintPresence(&painter, profile);
}

// The corner dot. Cut out of the avatar rather than laid over it: the design
// rings it in the window surface (`border:2px solid var(--window)`), which is
// what keeps it readable over a portrait as well as over a plate.
void Avatar::paintPresence(QPainter* painter, const ThemeProfile& profile) const
{
    if (m_presence == Presence::None) {
        return;
    }
    const SemanticPalette& semantic = profile.semantic;
    QColor ink = profile.colors.textTertiary.isValid() ? profile.colors.textTertiary
                                                       : profile.colors.textDisabled;
    switch (m_presence) {
    case Presence::Online:
    case Presence::InCall: ink = semantic.success.text; break;
    case Presence::Busy:   ink = semantic.danger.text;  break;
    case Presence::Away:   ink = semantic.warning.text; break;
    case Presence::Offline:
    case Presence::None:   break;
    }

    // `.av .pres{width:10px}` on a 32px avatar, `.av.s .pres{width:8px}` on 24
    // and `.av.l .pres{13px}` on 44: one third of the avatar, to the pixel.
    const double dot = qMax(6.0, m_diameter * 0.31);
    const double ring = qMax(1.5, dot * 0.2);
    const QRectF box(m_diameter - dot - 1.0, m_diameter - dot - 1.0, dot, dot);

    painter->setPen(Qt::NoPen);
    painter->setBrush(profile.colors.windowBg);
    painter->drawEllipse(box.adjusted(-ring, -ring, ring, ring));
    painter->setBrush(ink);
    painter->drawEllipse(box);

    if (m_presence == Presence::InCall) {
        // Online, and already talking: the same green, ringed rather than
        // recoloured, so "reachable" does not turn into a fifth state.
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(withAlpha(ink, 0.28), ring));
        painter->drawEllipse(box.adjusted(-ring, -ring, ring, ring));
    }
}

} // namespace novu::ui
