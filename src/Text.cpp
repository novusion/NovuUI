#include "novu/ui/Text.h"

#include <QEvent>

#include "novu/ui/ThemeManager.h"

#include <QLabel>
#include <QPainter>
#include <QResizeEvent>
#include <QTextDocument>
#include <QTextLayout>

#include <cmath>
#include <QWidget>

namespace novu::ui {

QColor inkColor(const QWidget* context, Ink ink)
{
    const ThemeProfile& profile = themeFor(context);
    const ThemeColors& c = profile.colors;
    const SemanticPalette& s = profile.semantic;
    switch (ink) {
    case Ink::Secondary: return c.textSecondary;
    case Ink::Tertiary: return c.textTertiary.isValid() ? c.textTertiary : c.textDisabled;
    case Ink::Muted:     return c.textDisabled;
    case Ink::Accent:    return c.accent;
    // Through the semantic palette, not the raw hues: status text and a status
    // badge are the same statement, so they resolve through the same table.
    // Info was accentLight here as well - the application accent standing in
    // for a status.
    case Ink::Success:   return s.success.text;
    case Ink::Warning:   return s.warning.text;
    case Ink::Danger:    return s.danger.text;
    case Ink::Info:      return s.info.text;
    case Ink::Default:
    default:             return c.textPrimary;
    }
}

namespace {

// The roles the display face is for: a title, and nothing that is not a title.
// Kept here rather than as a flag on TypeStyle because it is a property of what
// the role *is*, and a profile that had to say "and this one is a title too"
// for every role would eventually disagree with itself.
bool isDisplayRole(TypeRole role)
{
    switch (role) {
    case TypeRole::PageTitle:
    case TypeRole::SectionTitle:
    case TypeRole::CardTitle:
    case TypeRole::WindowTitle:
    case TypeRole::Metric:
        return true;
    default:
        return false;
    }
}

// The named family first, then the profile's fallbacks. Qt walks the list and
// takes the first installed one, so a machine without the design's faces gets
// the profile's stated second choice instead of whatever Qt would have
// substituted on its own.
QFont familyStack(const QString& primary, const QStringList& fallbacks)
{
    QStringList families;
    if (!primary.isEmpty()) {
        families.append(primary);
    }
    families.append(fallbacks);
    QFont f(families.isEmpty() ? QString() : families.first());
    f.setFamilies(families);
    return f;
}

} // namespace

QFont roleFont(const QWidget* context, TypeRole role, bool bold, int weight)
{
    const ThemeProfile& profile = themeFor(context);
    const TypeStyle style = profile.text(role);
    const TypeScale& scale = profile.type;

    // A role that declares itself monospace gets the mono stack here as well.
    // Before, only roleMonoFont() honoured the flag, so TypeRole::Mono came out
    // in the body face at every call site that asked for it by role.
    const bool mono = style.monospace;
    const QString primary = mono
        ? scale.monoFamily
        : (isDisplayRole(role) && !scale.displayFamily.isEmpty() ? scale.displayFamily
                                                                 : scale.family);

    QFont f = familyStack(primary, mono ? scale.monoFallbackFamilies
                                        : scale.fallbackFamilies);
    if (qFuzzyCompare(style.size, qreal(qRound(style.size)))) f.setPixelSize(qRound(style.size));
    else f.setPointSizeF(style.size * 72.0 / (context ? context->logicalDpiY() : 96.0));
    if(scale.unhintedMetrics)f.setHintingPreference(QFont::PreferNoHinting);
    // `bold` is one step of emphasis, not a jump to 700: every approved design
    // in the system tops out at 600, and a role that already states 600 asked
    // for that weight rather than for "the heaviest available". Bolding a
    // 600 role to 700 is how a page title came out heavier than the mockup
    // draws it while the token said the right thing.
    //
    // `weight`, given nonzero, skips this step altogether and states the exact
    // number instead - `.cellmain{font-weight:500}` is one step from Body's
    // 400 same as `bold` would give, but to Medium, not to the 600 floor
    // `bold` enforces. Called with `bold=true` for that CSS, a whole table of
    // names came out a full step heavier than the design - `.side`, `.pnl` and
    // every `cellmain` in the product share the same 500, so this is not a
    // one-off, it is the common case `bold` was the wrong tool for.
    const int resolvedWeight = weight > 0 ? weight
        : bold ? qMax(style.weight, static_cast<int>(QFont::DemiBold))
              : style.weight;
    f.setWeight(static_cast<QFont::Weight>(resolvedWeight));
    // CSS em tracking is an absolute addition based on the font size.
    if (!qFuzzyIsNull(style.letterSpacing)) {
        f.setLetterSpacing(QFont::AbsoluteSpacing, style.letterSpacing * style.size);
    }
    // `text-transform:uppercase` - a property of the role, so a caller never
    // has to remember toUpper() and two call sites cannot disagree.
    if (style.uppercase) {
        f.setCapitalization(QFont::AllUppercase);
    }
    return f;
}

QFont roleMonoFont(const QWidget* context, TypeRole role, bool bold, int weight)
{
    const ThemeProfile& profile = themeFor(context);
    const TypeStyle style = profile.text(role);

    QFont f = familyStack(profile.type.monoFamily, profile.type.monoFallbackFamilies);
    if (qFuzzyCompare(style.size, qreal(qRound(style.size)))) f.setPixelSize(qRound(style.size));
    else f.setPointSizeF(style.size * 72.0 / (context ? context->logicalDpiY() : 96.0));
    if(profile.type.unhintedMetrics)f.setHintingPreference(QFont::PreferNoHinting);
    // The same three rules roleFont() applies: a role does not lose its
    // tracking or its case because it happens to be set in the mono face -
    // `.stat .lab` and a table's column head are both mono *and* tracked.
    const int resolvedWeight = weight > 0 ? weight
        : bold ? qMax(style.weight, static_cast<int>(QFont::DemiBold))
              : style.weight;
    f.setWeight(static_cast<QFont::Weight>(resolvedWeight));
    if (!qFuzzyIsNull(style.letterSpacing)) {
        f.setLetterSpacing(QFont::AbsoluteSpacing, style.letterSpacing * style.size);
    }
    if (style.uppercase) {
        f.setCapitalization(QFont::AllUppercase);
    }
    return f;
}

QFont iconFont(int pixelSize)
{
    QFont f(QStringLiteral("Segoe Fluent Icons"));
    f.setFamilies({QStringLiteral("Segoe Fluent Icons"),
                   QStringLiteral("Segoe MDL2 Assets")});
    f.setPixelSize(pixelSize);
    return f;
}

namespace {

// The dynamic property an Ink role is remembered under. A string rather than
// the enum so it survives a QVariant round trip and is visible in a debugger.
constexpr const char* kInkRole = "novuInkRole";

// One filter for every label in the process: it holds no state, so a second
// instance would buy nothing. Installed by setInk() when a role - rather than
// a bare colour - is what was asked for.
void renderLineHeight(QLabel* label);

class InkWatcher : public QObject
{
public:
    static InkWatcher* instance()
    {
        static InkWatcher watcher;
        return &watcher;
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::ContentsRectChange || event->type() == QEvent::FontChange) {
            if (auto* label = qobject_cast<QLabel*>(watched); label && label->property("novuLineHeight").isValid())
                renderLineHeight(label);
        }
        if (event->type() != ThemeChangeEvent::type()) {
            return QObject::eventFilter(watched, event);
        }
        auto* lb = qobject_cast<QLabel*>(watched);
        if (!lb) {
            return QObject::eventFilter(watched, event);
        }
        const QVariant role = lb->property(kInkRole);
        if (role.isValid()) {
            // Through the colour overload, which is what actually writes the
            // palette and the one-property sheet.
            setInk(lb, inkColor(lb, static_cast<Ink>(role.toInt())));
        }
        return QObject::eventFilter(watched, event);
    }
};

void rememberInk(QLabel* lb, Ink ink)
{
    if (!lb) {
        return;
    }
    const bool first = !lb->property(kInkRole).isValid();
    lb->setProperty(kInkRole, static_cast<int>(ink));
    if (first) {
        lb->installEventFilter(InkWatcher::instance());
    }
}

} // namespace

namespace {

// Where a label's own text is kept once its line height has been stated. What
// QLabel::text() gives back after that is the markup, not the sentence.
constexpr const char* kPlainText = "novuPlainText";
constexpr const char* kLineHeight = "novuLineHeight";

QString plainTextOf(const QLabel* label)
{
    const QVariant kept = label->property(kPlainText);
    return kept.isValid() ? kept.toString() : label->text();
}

void renderLineHeight(QLabel* label)
{
    const double multiple = label->property(kLineHeight).toDouble();
    const QString body = plainTextOf(label);

    if (qFuzzyIsNull(multiple)) {
        label->setTextFormat(Qt::AutoText);
        label->setMinimumHeight(0);
        label->setMaximumHeight(QWIDGETSIZE_MAX);
        if (label->text() != body) {
            label->setText(body);
        }
        return;
    }

    // CSS measures a line box as a multiple of the font size, so this does
    // too.
    const qreal pixels = label->font().pixelSize() > 0 ? label->font().pixelSize()
        : label->font().pointSizeF() * label->logicalDpiY() / 72.0;
    const int box = qRound(pixels * multiple) + label->contentsMargins().top() + label->contentsMargins().bottom() + 2 * label->margin();
    if (!label->wordWrap()) {
        // One line: the box *is* the label.
        label->setFixedHeight(box);
        if (label->text() != body) {
            label->setText(body);
        }
        return;
    }

    // A wrapped paragraph is the case a minimum height cannot answer. The
    // space *between* the lines is the font's own leading and no widget
    // property reaches it, so a floor on the label's height leaves three lines
    // at 15px pitch inside a 20px-per-line box - which is the design's
    // `line-height:1.6` ignored while appearing to be honoured.
    //
    // Qt's rich text does reach it: a text block carries a CSS line height the
    // same way the design states one. So a wrapped label whose leading is
    // stated is rendered as one styled block, and the sentence it was given is
    // kept alongside so this stays idempotent.
    //
    // One conversion is needed on the way in. CSS measures a line box against
    // the *font size*; Qt's proportional line height measures it against the
    // font's own natural line height, which is a few pixels taller. Passing
    // the design's number straight through overshoots every gap by that
    // difference - 1.6 came out as a 24px pitch where the design draws 20.
    const QFontMetrics metrics(label->font());
    const int natural = qMax(1, metrics.height());
    const int percent = qMax(1, qRound(multiple * pixels
                                       * 100.0 / natural));
    label->setTextFormat(Qt::RichText);
    label->setText(QStringLiteral("<div style=\"line-height:%1%\">%2</div>")
                       .arg(percent)
                       .arg(label->property("novuRichText").toBool()?body:QString(body.toHtmlEscaped()).replace(QLatin1Char('\n'), QStringLiteral("<br>"))));
    // label() may have applied the role line height before the caller enabled
    // word wrapping. That one-line path fixes the maximum height; release it
    // now or the newly wrapped document is clipped to its first line.
    label->setMaximumHeight(QWIDGETSIZE_MAX);
    label->setMinimumHeight(box);
}

} // namespace

void setLineHeight(QLabel* label, double multiple)
{
    if (!label) {
        return;
    }
    if (!label->property(kPlainText).isValid()) {
        label->setProperty(kPlainText, label->text());
    }
    label->installEventFilter(InkWatcher::instance());
    label->setProperty(kLineHeight, multiple);
    renderLineHeight(label);
}

void setLabelText(QLabel* label, const QString& text)
{
    if (!label) {
        return;
    }
    label->setProperty("novuRichText", false);
    label->setProperty(kPlainText, text);
    renderLineHeight(label);
}

void setLabelRichText(QLabel* label,const QString& html)
{
    if(!label)return;
    label->setProperty("novuRichText",true);
    label->setProperty(kPlainText,html);
    renderLineHeight(label);
}

QString labelText(const QLabel* label)
{
    return label ? plainTextOf(label) : QString();
}


ElidedLabel::ElidedLabel(const QString& text, QWidget* parent) : QLabel(text, parent)
{
    setTextFormat(Qt::PlainText);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
}
QSize ElidedLabel::minimumSizeHint() const
{
    return QSize(0, QLabel::minimumSizeHint().height());
}
void ElidedLabel::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setFont(font());
    painter.setPen(palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled, foregroundRole()));
    const QRect bounds = contentsRect().adjusted(margin(), margin(), -margin(), -margin());
    painter.drawText(bounds, alignment(), fontMetrics().elidedText(text(), m_elideMode, bounds.width()));
}
QLabel* elidedLabel(const QString& text, QWidget* parent, TypeRole role, Ink ink, int weight)
{
    auto* result = new ElidedLabel(text, parent);
    applyText(result, role, ink);
    if (weight) result->setFont(roleFont(result, role, false, weight));
    return result;
}

QLabel* label(const QString& text, QWidget* parent, TypeRole role, Ink ink, bool bold,
              int weight)
{
    auto* lb = new QLabel(text, parent);
    lb->setFont(roleFont(parent, role, bold, weight));
    // The role's own line box, the same rule applyText() follows.
    const TypeStyle style = themeFor(parent).text(role);
    if (!qFuzzyIsNull(style.lineHeight)) {
        setLineHeight(lb, style.lineHeight);
    }
    rememberInk(lb, ink);
    setInk(lb, inkColor(parent, ink));
    return lb;
}

namespace {

// The width a paragraph measures itself at before a column has given it one.
// Any figure here is a guess; this one is a narrow column, so a first pass
// over-estimates the height rather than under-estimating it and leaving the
// text clipped until the first resize.
constexpr int kUnplacedWidth = 240;

} // namespace

Paragraph::Paragraph(const QString& text, QWidget* parent, TypeRole role, Ink ink)
    : QLabel(text, parent)
{
    setWordWrap(true);
    // The column decides the width: a paragraph never widens one to fit its
    // longest line.
    setMinimumWidth(1);
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    // Content-first: given more height than its lines need, the text stays
    // at the top of it rather than floating to the middle.
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    applyText(this, role, ink);
}

int Paragraph::naturalWidth() const
{
    // QLabel keeps this number to itself: sizeHint() folds it into a width Qt
    // chooses by its own rule, which is the rule this class exists to get out
    // from under. Measuring the text is the only way to ask the question
    // plainly, and it is the same measurement QLabel makes internally.
    QTextDocument doc;
    doc.setDefaultFont(font());
    if (textFormat() == Qt::RichText
        || (textFormat() == Qt::AutoText && Qt::mightBeRichText(text()))) {
        doc.setHtml(text());
    } else {
        doc.setPlainText(text());
    }
    doc.setTextWidth(-1);
    return int(std::ceil(doc.idealWidth())) + 2 * margin() + 2 * frameWidth();
}

QSize Paragraph::sizeHint() const
{
    // Width: what the text would take unwrapped, never more than the caller
    // allows -- the cap a design puts on a measure (`max-width: 78ch`) is the
    // usual maximum. Height: what it takes at the width it actually has,
    // because that is the number the column adds up.
    const int placed = width() > 1 ? width() : kUnplacedWidth;
    int preferred = naturalWidth();
    const int allowed = maximumWidth();
    if (allowed > 0 && allowed < QWIDGETSIZE_MAX) {
        preferred = qMin(preferred, allowed);
    }
    return QSize(preferred, heightForWidth(placed));
}

QSize Paragraph::minimumSizeHint() const
{
    // The height at the width it has, not the height it would need if it were
    // as narrow as it is allowed to be. The second is what a wrapped QLabel
    // reports, and what a scroll area then sizes its content to.
    const int w = width() > 1 ? width() : kUnplacedWidth;
    return QSize(1, heightForWidth(w));
}

void Paragraph::setMaximumLines(int lines)
{
    lines = qMax(0, lines);
    if (lines == m_maxLines) {
        return;
    }
    m_maxLines = lines;
    if (m_maxLines > 0 && (toolTip().isEmpty() || m_autoToolTip)) {
        setToolTip(plainTextOf(this));
        m_autoToolTip = true;
    }
    updateGeometry();
    update();
}

int Paragraph::textWidthFor(int width) const
{
    const QMargins m = contentsMargins();
    return qMax(1, width - m.left() - m.right() - 2 * margin() - 2 * frameWidth());
}

int Paragraph::verticalChrome() const
{
    const QMargins m = contentsMargins();
    return m.top() + m.bottom() + 2 * margin() + 2 * frameWidth();
}

int Paragraph::lineCountAt(int width) const
{
    QTextLayout layout(plainTextOf(this), font());
    layout.beginLayout();
    int lines = 0;
    const int lane = textWidthFor(width);
    while (true) {
        QTextLine line = layout.createLine();
        if (!line.isValid()) {
            break;
        }
        line.setLineWidth(lane);
        ++lines;
    }
    layout.endLayout();
    return qMax(1, lines);
}

bool Paragraph::isTruncated() const
{
    return m_maxLines > 0 && lineCountAt(width() > 1 ? width() : kUnplacedWidth) > m_maxLines;
}

int Paragraph::heightForWidth(int width) const
{
    if (m_maxLines <= 0) {
        return QLabel::heightForWidth(width);
    }
    // The lines it will show, at the pitch it paints them with.
    const int lines = qMin(m_maxLines, lineCountAt(width));
    return lines * QFontMetrics(font()).lineSpacing() + verticalChrome();
}

void Paragraph::paintEvent(QPaintEvent* event)
{
    if (m_maxLines <= 0) {
        QLabel::paintEvent(event);
        return;
    }
    QPainter painter(this);
    painter.setPen(palette().color(foregroundRole()));
    painter.setFont(font());
    const QMargins m = contentsMargins();
    const QPointF origin(m.left() + margin() + frameWidth(), m.top() + margin() + frameWidth());
    const int lane = textWidthFor(width());
    const QString text = plainTextOf(this);
    const QFontMetrics metrics(font());

    QTextLayout layout(text, font());
    layout.beginLayout();
    QList<QTextLine> lines;
    while (true) {
        QTextLine line = layout.createLine();
        if (!line.isValid()) {
            break;
        }
        line.setLineWidth(lane);
        lines.append(line);
    }
    layout.endLayout();

    const int shown = qMin(m_maxLines, int(lines.size()));
    for (int i = 0; i < shown; ++i) {
        const QTextLine& line = lines.at(i);
        const qreal y = origin.y() + i * metrics.lineSpacing();
        if (i == shown - 1 && lines.size() > shown) {
            // The last line shown carries the rest of the text, shortened.
            const QString rest = text.mid(line.textStart()).simplified();
            painter.drawText(QPointF(origin.x(), y + metrics.ascent()),
                             metrics.elidedText(rest, Qt::ElideRight, lane));
        } else {
            line.draw(&painter, QPointF(origin.x(), y - line.y()));
        }
    }
}

void Paragraph::resizeEvent(QResizeEvent* event)
{
    QLabel::resizeEvent(event);
    // Both hints are answers about the current width, so a new width makes
    // both of them stale.
    if (event->oldSize().width() != event->size().width()) {
        updateGeometry();
    }
}

QLabel* paragraph(const QString& text, QWidget* parent, TypeRole role, Ink ink)
{
    return new Paragraph(text, parent, role, ink);
}

QLabel* caption(const QString& text, QWidget* parent)
{
    // The uppercasing and the tracking are the treatment, not the font, so they
    // are applied here rather than stored in the role. Without the tracking a
    // 10px uppercase mono line reads as a solid bar.
    auto* lb = label(text.toUpper(), parent, TypeRole::Eyebrow, Ink::Muted);
    return lb;
}

void applyText(QLabel* lb, TypeRole role, Ink ink, bool bold)
{
    if (!lb) {
        return;
    }
    lb->setFont(roleFont(lb, role, bold));
    // `line-height` on a single line is the height of the line box, which is
    // what a QLabel would otherwise take from the font's own metrics - taller
    // than the design's own box for a display face. Only for a label that is
    // one line: a wrapped paragraph's leading is not this.
    const TypeStyle style = themeFor(lb).text(role);
    if (!qFuzzyIsNull(style.lineHeight) && !lb->wordWrap()) {
        lb->setFixedHeight(qRound(style.size * style.lineHeight));
    }
    // Through the role overload, so the ink is remembered and a theme change
    // reaches this label the way it reaches a component.
    setInk(lb, ink);
}

void setInk(QLabel* lb, Ink ink)
{
    if (!lb) {
        return;
    }
    rememberInk(lb, ink);
    setInk(lb, inkColor(lb, ink));
}

void setInk(QLabel* lb, const QColor& color)
{
    if (!lb) {
        return;
    }
    QPalette pal = lb->palette();
    pal.setColor(QPalette::WindowText, color);
    lb->setPalette(pal);

    // The palette alone is not enough. The scope sheet carries
    // `QLabel { color: ... }` so that a label with no ink of its own is not
    // left with the light system palette on a dark surface - and a stylesheet
    // colour outranks a palette one. Every ink set inside a themed subtree was
    // therefore being dropped: a loss printed in exactly the same colour as a
    // profit, and an inactive window title stayed at full strength.
    //
    // A widget's own sheet outranks its ancestors', so one declaration on the
    // label restores the role. It is a single property and no selector, so
    // nothing else about the label - background, border, tone rules - changes.
    const QString sheet = QStringLiteral("color: %1;")
        .arg(color.alpha() == 255 ? color.name() : rgba(color, color.alphaF()));
    // Guarded: setInk() is called again on every polish and every theme change,
    // and re-setting an identical sheet re-polishes the widget for nothing.
    if (lb->styleSheet() != sheet) {
        lb->setStyleSheet(sheet);
    }
}

} // namespace novu::ui
