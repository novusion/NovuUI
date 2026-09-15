#include "novu/ui/Avatar.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
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
    const double radius = m_shape == Shape::Round ? box.height() / 2.0 : style.radius;

    QPainterPath shape;
    shape.addRoundedRect(box, radius, radius);

    QColor plate = plateFor(profile, m_name);
    // Tinted, not solid: a wall of fully saturated avatars in a list is a wall,
    // and the initials have to stay legible on it.
    plate.setAlphaF(m_highlighted ? 0.42 : 0.26);
    painter.fillPath(shape, plate);

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
}

} // namespace novu::ui
