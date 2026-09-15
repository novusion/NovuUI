#include "novu/ui/Icon.h"

#include "novu/ui/ThemeManager.h"

#include <QGuiApplication>
#include <QHash>
#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <QStringView>
#include <QtMath>

#include <array>

namespace novu::ui {

namespace {

// The design's own outlines.
//
// Copied out of the approved NovuSpace mockup rather than retyped: the strings
// below are the exact contents of its icon table, in the same 24x24 viewBox.
// Keeping them in the design's notation - `<path d>`, `<rect>`, `<circle>` -
// is deliberate. It means a glyph can be diffed against the HTML by eye, and a
// new icon is copied across in one step instead of being hand-converted into
// QPainterPath calls that nobody can check against anything.
struct GlyphDef
{
    Glyph glyph;
    const char* name;
    const char* source;
};

const GlyphDef kGlyphs[] = {
#include "GlyphTable.inc"
};

// ── the SVG subset the icon set uses ────────────────────────────────────────
//
// Exactly three elements and the path commands those outlines contain, and no
// more. This is not a general SVG reader and must not grow into one: an icon
// that needs anything else is an icon that has stopped being a stroke drawing.

constexpr double kViewBox = 24.0;
// The design's stroke-width, in viewBox units. Scaled with the box, so a glyph
// at 13px and the same glyph at 20px are one drawing rather than one weight.
constexpr double kStrokeWidth = 1.7;

// Reads one numeric attribute out of an element. Absent attributes take the
// fallback, which is how `<rect>` without `rx` comes out square.
double attribute(QStringView element, QLatin1String name, double fallback = 0.0)
{
    const QString needle = name + QLatin1String("=\"");
    const qsizetype at = element.indexOf(needle);
    if (at < 0) {
        return fallback;
    }
    const qsizetype from = at + needle.size();
    const qsizetype to = element.indexOf(QLatin1Char('"'), from);
    if (to < 0) {
        return fallback;
    }
    bool ok = false;
    const double value = element.mid(from, to - from).toDouble(&ok);
    return ok ? value : fallback;
}

QStringView attributeText(QStringView element, QLatin1String name)
{
    const QString needle = name + QLatin1String("=\"");
    const qsizetype at = element.indexOf(needle);
    if (at < 0) {
        return {};
    }
    const qsizetype from = at + needle.size();
    const qsizetype to = element.indexOf(QLatin1Char('"'), from);
    return to < 0 ? QStringView() : element.mid(from, to - from);
}

// A reader over path data. SVG's number syntax is its own: "-" separates as
// well as signs, ".5.5" is two numbers, and a command letter may be omitted for
// a repeated command. All three are handled here rather than at the call sites.
class PathReader
{
public:
    explicit PathReader(QStringView data) : m_data(data) {}

    void skipSeparators()
    {
        while (m_at < m_data.size()) {
            const QChar c = m_data.at(m_at);
            if (c == QLatin1Char(',') || c.isSpace()) {
                ++m_at;
            } else {
                break;
            }
        }
    }

    bool atEnd()
    {
        skipSeparators();
        return m_at >= m_data.size();
    }

    // A letter here is a new command; anything else continues the previous one.
    bool nextIsCommand()
    {
        skipSeparators();
        return m_at < m_data.size() && m_data.at(m_at).isLetter();
    }

    QChar takeCommand()
    {
        skipSeparators();
        return m_at < m_data.size() ? m_data.at(m_at++) : QChar();
    }

    double takeNumber()
    {
        skipSeparators();
        const qsizetype start = m_at;
        if (m_at < m_data.size()
            && (m_data.at(m_at) == QLatin1Char('-') || m_data.at(m_at) == QLatin1Char('+'))) {
            ++m_at;
        }
        bool seenDot = false;
        while (m_at < m_data.size()) {
            const QChar c = m_data.at(m_at);
            if (c.isDigit()) {
                ++m_at;
            } else if (c == QLatin1Char('.') && !seenDot) {
                seenDot = true;
                ++m_at;
            } else if ((c == QLatin1Char('e') || c == QLatin1Char('E'))
                       && m_at + 1 < m_data.size()) {
                ++m_at;
                if (m_at < m_data.size()
                    && (m_data.at(m_at) == QLatin1Char('-') || m_data.at(m_at) == QLatin1Char('+'))) {
                    ++m_at;
                }
            } else {
                break;
            }
        }
        return m_data.mid(start, m_at - start).toDouble();
    }

    // The arc flags are single digits and are *not* separated from what follows
    // them - "a5.5 5.5 0 1 0-11 0" carries "1", "0" and "-11" in six
    // characters. Reading them as numbers would swallow the next value.
    double takeFlag()
    {
        skipSeparators();
        if (m_at < m_data.size()
            && (m_data.at(m_at) == QLatin1Char('0') || m_data.at(m_at) == QLatin1Char('1'))) {
            return m_data.at(m_at++) == QLatin1Char('1') ? 1.0 : 0.0;
        }
        return takeNumber();
    }

private:
    QStringView m_data;
    qsizetype m_at = 0;
};

// An elliptical arc as cubic segments.
//
// QPainterPath::arcTo() draws only axis-aligned ellipses and measures its
// angles in the opposite direction from SVG, so converting to cubics is both
// shorter and the one version that stays correct if a rotated arc is ever
// added to the set.
void appendArc(QPainterPath& path, QPointF from, double rx, double ry,
               double rotationDeg, bool largeArc, bool sweep, QPointF to)
{
    if (qFuzzyIsNull(rx) || qFuzzyIsNull(ry)) {
        path.lineTo(to);
        return;
    }
    rx = qAbs(rx);
    ry = qAbs(ry);

    const double phi = qDegreesToRadians(rotationDeg);
    const double cosPhi = std::cos(phi);
    const double sinPhi = std::sin(phi);

    const double dx = (from.x() - to.x()) / 2.0;
    const double dy = (from.y() - to.y()) / 2.0;
    const double x1 = cosPhi * dx + sinPhi * dy;
    const double y1 = -sinPhi * dx + cosPhi * dy;

    // An arc whose radii cannot span the endpoints is scaled up until it can,
    // which is what the specification asks for and what keeps a hand-tuned
    // outline from collapsing.
    const double lambda = (x1 * x1) / (rx * rx) + (y1 * y1) / (ry * ry);
    if (lambda > 1.0) {
        const double scale = std::sqrt(lambda);
        rx *= scale;
        ry *= scale;
    }

    const double rxSq = rx * rx;
    const double rySq = ry * ry;
    const double numerator = qMax(0.0, rxSq * rySq - rxSq * y1 * y1 - rySq * x1 * x1);
    const double denominator = rxSq * y1 * y1 + rySq * x1 * x1;
    const double coefficient = (largeArc == sweep ? -1.0 : 1.0)
        * (qFuzzyIsNull(denominator) ? 0.0 : std::sqrt(numerator / denominator));

    const double cx1 = coefficient * rx * y1 / ry;
    const double cy1 = -coefficient * ry * x1 / rx;
    const double cx = cosPhi * cx1 - sinPhi * cy1 + (from.x() + to.x()) / 2.0;
    const double cy = sinPhi * cx1 + cosPhi * cy1 + (from.y() + to.y()) / 2.0;

    const double startAngle = std::atan2((y1 - cy1) / ry, (x1 - cx1) / rx);
    const double endAngle = std::atan2((-y1 - cy1) / ry, (-x1 - cx1) / rx);
    double delta = endAngle - startAngle;
    if (!sweep && delta > 0.0) {
        delta -= 2.0 * M_PI;
    } else if (sweep && delta < 0.0) {
        delta += 2.0 * M_PI;
    }

    const auto pointAt = [&](double angle) {
        return QPointF(cx + rx * std::cos(angle) * cosPhi - ry * std::sin(angle) * sinPhi,
                       cy + rx * std::cos(angle) * sinPhi + ry * std::sin(angle) * cosPhi);
    };
    const auto tangentAt = [&](double angle) {
        return QPointF(-rx * std::sin(angle) * cosPhi - ry * std::cos(angle) * sinPhi,
                       -rx * std::sin(angle) * sinPhi + ry * std::cos(angle) * cosPhi);
    };

    // A cubic tracks a circular arc to within a fraction of a pixel up to a
    // quarter turn; beyond that it visibly flattens, so the sweep is split.
    const int segments = qMax(1, static_cast<int>(std::ceil(qAbs(delta) / (M_PI / 2.0))));
    const double step = delta / segments;
    const double alpha = std::sin(step)
        * (std::sqrt(4.0 + 3.0 * std::pow(std::tan(step / 2.0), 2.0)) - 1.0) / 3.0;

    double angle = startAngle;
    for (int i = 0; i < segments; ++i) {
        const double next = angle + step;
        const QPointF p1 = pointAt(angle);
        const QPointF p2 = pointAt(next);
        const QPointF c1 = p1 + alpha * tangentAt(angle);
        const QPointF c2 = p2 - alpha * tangentAt(next);
        path.cubicTo(c1, c2, p2);
        angle = next;
    }
}

void appendPathData(QPainterPath& path, QStringView data)
{
    PathReader reader(data);
    QChar command;
    QPointF current;
    QPointF subpathStart;
    QPointF lastControl;
    bool lastWasCubic = false;

    while (!reader.atEnd()) {
        if (reader.nextIsCommand()) {
            command = reader.takeCommand();
        } else if (command.isNull()) {
            break;
        } else if (command == QLatin1Char('M')) {
            // A repeated moveto is a lineto, per the specification.
            command = QLatin1Char('L');
        } else if (command == QLatin1Char('m')) {
            command = QLatin1Char('l');
        }

        const bool relative = command.isLower();
        const QPointF origin = relative ? current : QPointF();
        const char op = command.toUpper().toLatin1();
        bool cubic = false;

        switch (op) {
        case 'M': {
            const double x = reader.takeNumber();
            const double y = reader.takeNumber();
            current = origin + QPointF(x, y);
            subpathStart = current;
            path.moveTo(current);
            break;
        }
        case 'L': {
            const double x = reader.takeNumber();
            const double y = reader.takeNumber();
            current = origin + QPointF(x, y);
            path.lineTo(current);
            break;
        }
        case 'H': {
            const double x = reader.takeNumber();
            current.setX(relative ? current.x() + x : x);
            path.lineTo(current);
            break;
        }
        case 'V': {
            const double y = reader.takeNumber();
            current.setY(relative ? current.y() + y : y);
            path.lineTo(current);
            break;
        }
        case 'C': {
            const QPointF c1 = origin + QPointF(reader.takeNumber(), reader.takeNumber());
            const QPointF c2 = origin + QPointF(reader.takeNumber(), reader.takeNumber());
            const QPointF end = origin + QPointF(reader.takeNumber(), reader.takeNumber());
            path.cubicTo(c1, c2, end);
            lastControl = c2;
            current = end;
            cubic = true;
            break;
        }
        case 'S': {
            // The first control point is the reflection of the previous one,
            // or the current point when the previous command was not a cubic.
            const QPointF c1 = lastWasCubic ? 2.0 * current - lastControl : current;
            const QPointF c2 = origin + QPointF(reader.takeNumber(), reader.takeNumber());
            const QPointF end = origin + QPointF(reader.takeNumber(), reader.takeNumber());
            path.cubicTo(c1, c2, end);
            lastControl = c2;
            current = end;
            cubic = true;
            break;
        }
        case 'A': {
            const double rx = reader.takeNumber();
            const double ry = reader.takeNumber();
            const double rotation = reader.takeNumber();
            const bool largeArc = reader.takeFlag() != 0.0;
            const bool sweep = reader.takeFlag() != 0.0;
            const QPointF end = origin + QPointF(reader.takeNumber(), reader.takeNumber());
            appendArc(path, current, rx, ry, rotation, largeArc, sweep, end);
            current = end;
            break;
        }
        case 'Z':
            path.closeSubpath();
            current = subpathStart;
            break;
        default:
            // An unsupported command would otherwise spin the reader forever.
            return;
        }
        lastWasCubic = cubic;
    }
}

QPainterPath parseGlyph(QStringView source)
{
    QPainterPath path;
    qsizetype at = 0;
    while (at < source.size()) {
        const qsizetype open = source.indexOf(QLatin1Char('<'), at);
        if (open < 0) {
            break;
        }
        const qsizetype close = source.indexOf(QLatin1Char('>'), open);
        if (close < 0) {
            break;
        }
        const QStringView element = source.mid(open, close - open + 1);
        at = close + 1;

        if (element.startsWith(QLatin1String("<path"))) {
            appendPathData(path, attributeText(element, QLatin1String("d")));
        } else if (element.startsWith(QLatin1String("<rect"))) {
            const double rx = attribute(element, QLatin1String("rx"));
            const QRectF box(attribute(element, QLatin1String("x")),
                             attribute(element, QLatin1String("y")),
                             attribute(element, QLatin1String("width")),
                             attribute(element, QLatin1String("height")));
            if (rx > 0.0) {
                path.addRoundedRect(box, rx, attribute(element, QLatin1String("ry"), rx));
            } else {
                path.addRect(box);
            }
        } else if (element.startsWith(QLatin1String("<circle"))) {
            const double r = attribute(element, QLatin1String("r"));
            path.addEllipse(QPointF(attribute(element, QLatin1String("cx")),
                                    attribute(element, QLatin1String("cy"))),
                            r, r);
        }
    }
    return path;
}

QHash<int, QPainterPath>& pathCache()
{
    static QHash<int, QPainterPath> cache;
    return cache;
}

const GlyphDef* definitionFor(Glyph glyph)
{
    for (const GlyphDef& def : kGlyphs) {
        if (def.glyph == glyph) {
            return &def;
        }
    }
    return nullptr;
}

// Pixmaps are keyed on everything that changes the pixels. Colour is part of
// the key because the same glyph is drawn in the muted ink, the accent and a
// status colour within one toolbar.
// A spinner turns continuously, so the angle is quantised before it is cached:
// 24 steps of 15 degrees is finer than the eye resolves at 0.9s a turn and
// bounds the cache at 24 entries per glyph, size and colour instead of one per
// frame forever.
constexpr int kRotationSteps = 24;

int rotationStep(double rotationDeg)
{
    const double normalised = std::fmod(std::fmod(rotationDeg, 360.0) + 360.0, 360.0);
    return static_cast<int>(std::lround(normalised / 360.0 * kRotationSteps)) % kRotationSteps;
}

QString pixmapKey(Glyph glyph, int size, const QColor& color, qreal ratio, int step)
{
    return QStringLiteral("%1|%2|%3|%4|%5")
        .arg(static_cast<int>(glyph))
        .arg(size)
        .arg(color.rgba())
        .arg(ratio, 0, 'f', 2)
        .arg(step);
}

QHash<QString, QPixmap>& pixmapCache()
{
    static QHash<QString, QPixmap> cache;
    return cache;
}

} // namespace

QString glyphName(Glyph glyph)
{
    const GlyphDef* def = definitionFor(glyph);
    return def ? QString::fromLatin1(def->name) : QString();
}

Glyph glyphFromName(const QString& name)
{
    for (const GlyphDef& def : kGlyphs) {
        if (name == QLatin1String(def.name)) {
            return def.glyph;
        }
    }
    return Glyph::None;
}

QVector<Glyph> allGlyphs()
{
    QVector<Glyph> glyphs;
    glyphs.reserve(static_cast<int>(std::size(kGlyphs)));
    for (const GlyphDef& def : kGlyphs) {
        glyphs.append(def.glyph);
    }
    return glyphs;
}

const QPainterPath& glyphPath(Glyph glyph)
{
    QHash<int, QPainterPath>& cache = pathCache();
    const int key = static_cast<int>(glyph);
    const auto cached = cache.constFind(key);
    if (cached != cache.constEnd()) {
        return cached.value();
    }
    const GlyphDef* def = definitionFor(glyph);
    return *cache.insert(key, def ? parseGlyph(QString::fromLatin1(def->source))
                                  : QPainterPath());
}

void paintGlyph(QPainter* painter, Glyph glyph, const QRectF& box, const QColor& color,
                double rotationDeg)
{
    if (!painter || glyph == Glyph::None || box.isEmpty()) {
        return;
    }
    const QPainterPath& path = glyphPath(glyph);
    if (path.isEmpty()) {
        return;
    }

    // Square, centred: the outlines are drawn in a square viewBox and a
    // non-uniform scale would thin the strokes along one axis.
    const double side = qMin(box.width(), box.height());
    const double scale = side / kViewBox;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->translate(box.center());
    if (!qFuzzyIsNull(rotationDeg)) {
        painter->rotate(rotationDeg);
    }
    painter->scale(scale, scale);
    // The outlines are drawn in a 24x24 box whose origin is its top-left, so
    // the centre has to come back out after the rotation is applied about it.
    painter->translate(-kViewBox / 2.0, -kViewBox / 2.0);

    QPen pen(color);
    pen.setWidthF(kStrokeWidth);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path);
    painter->restore();
}

QPixmap glyphPixmap(Glyph glyph, int size, const QColor& color, qreal devicePixelRatio,
                    double rotationDeg)
{
    if (size <= 0) {
        return {};
    }
    const qreal ratio = devicePixelRatio > 0.0 ? devicePixelRatio : qApp->devicePixelRatio();
    const int step = rotationStep(rotationDeg);
    const QString key = pixmapKey(glyph, size, color, ratio, step);

    QHash<QString, QPixmap>& cache = pixmapCache();
    const auto cached = cache.constFind(key);
    if (cached != cache.constEnd()) {
        return cached.value();
    }

    QPixmap pixmap(QSize(size, size) * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);
    {
        QPainter painter(&pixmap);
        paintGlyph(&painter, glyph, QRectF(0, 0, size, size), color,
                   step * 360.0 / kRotationSteps);
    }
    cache.insert(key, pixmap);
    return pixmap;
}

QIcon glyphIcon(Glyph glyph, int size, const QColor& color)
{
    QIcon icon;
    icon.addPixmap(glyphPixmap(glyph, size, color), QIcon::Normal);
    // Qt's own disabled rendering desaturates towards grey, which on a dark
    // surface produces a glyph *lighter* than the enabled one. Supplying the
    // faded version keeps a disabled control reading as disabled.
    QColor faded = color;
    faded.setAlphaF(color.alphaF() * 0.42);
    icon.addPixmap(glyphPixmap(glyph, size, faded), QIcon::Disabled);
    return icon;
}

int glyphSizeFor(const QWidget* context, ControlSize size)
{
    const IconMetrics& icons = themeFor(context).icons;
    switch (size) {
    case ControlSize::Small: return icons.small;
    case ControlSize::Large: return icons.large;
    case ControlSize::Medium: break;
    }
    return icons.medium;
}

} // namespace novu::ui
