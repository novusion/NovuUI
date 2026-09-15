#include "novu/ui/SettingRow.h"

#include "novu/ui/Icon.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QTextOption>

namespace novu::ui {

namespace {

// `.kv { gap:16px; padding:13px 16px }`.
constexpr int kPaddingH = 16;
constexpr int kPaddingV = 13;
constexpr int kGap = 16;

// `.kv .k .hint { margin-top:3px; line-height:1.45 }`.
constexpr int kHintGap = 3;
constexpr double kHintLeading = 1.45;

// `.ferr { gap:6px }` and its 12px mark.
constexpr int kErrorGap = 6;
constexpr int kErrorMark = 12;
constexpr int kErrorTop = 6;

// The design caps the hint at 58ch so a sentence does not run the width of a
// wide window. Approximated in pixels from the hint's own average character
// width, which is what `ch` means.
constexpr int kHintMaxChars = 58;

// Masked values are drawn as bullets with the design's .18em tracking.
const QChar kMaskGlyph = QChar(0x2022);   // BULLET
constexpr int kMaskLength = 12;

QFont nameFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::CardTitle, true);
    font.setPixelSize(profile.type.sm);
    return font;
}

QFont hintFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Caption);
    font.setPixelSize(profile.type.xs);
    return font;
}

QFont valueFont(const QWidget* context, bool mono, bool masked)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = mono ? roleMonoFont(context, TypeRole::Mono)
                      : roleFont(context, TypeRole::Body);
    font.setPixelSize(profile.type.sm);
    if (masked) {
        font.setLetterSpacing(QFont::PercentageSpacing, 118);
    }
    return font;
}

QFont errorFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Caption);
    font.setPixelSize(profile.type.xs);
    return font;
}

int wrappedHeight(const QFontMetrics& fm, const QString& text, int width, double leading)
{
    if (text.isEmpty() || width <= 0) {
        return 0;
    }
    const QRect bounds = fm.boundingRect(QRect(0, 0, width, 10000),
                                         Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                                         text);
    const int lines = qMax(1, qRound(double(bounds.height()) / qMax(1, fm.height())));
    return qRound(fm.height() * leading) * lines;
}

} // namespace

SettingRow::SettingRow(const QString& name, const QString& hint, QWidget* parent)
    : QWidget(parent)
    , m_name(name)
    , m_hint(hint)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}

void SettingRow::setName(const QString& name)
{
    if (m_name == name) {
        return;
    }
    m_name = name;
    update();
}

void SettingRow::setHint(const QString& hint)
{
    if (m_hint == hint) {
        return;
    }
    m_hint = hint;
    updateGeometry();
    update();
}

void SettingRow::setControl(QWidget* control)
{
    if (m_control == control) {
        return;
    }
    if (m_control) {
        m_control->setParent(nullptr);
        m_control->deleteLater();
    }
    m_control = control;
    if (m_control) {
        m_control->setParent(this);
        m_control->show();
        // A control and a value are two answers to the same question.
        m_value.clear();
    }
    updateGeometry();
    relayout();
    update();
}

void SettingRow::setValue(const QString& value, bool mono)
{
    if (m_value == value && m_mono == mono) {
        return;
    }
    m_value = value;
    m_mono = mono;
    if (!m_value.isEmpty() && m_control) {
        m_control->setParent(nullptr);
        m_control->deleteLater();
        m_control = nullptr;
    }
    updateGeometry();
    update();
}

void SettingRow::setMasked(bool masked)
{
    if (m_masked == masked) {
        return;
    }
    m_masked = masked;
    updateGeometry();
    update();
}

void SettingRow::setError(const QString& message)
{
    if (m_error == message) {
        return;
    }
    m_error = message;
    updateGeometry();
    relayout();
    update();
}

void SettingRow::setDivider(bool divider)
{
    if (m_divider == divider) {
        return;
    }
    m_divider = divider;
    update();
}

void SettingRow::setControlColumn(int px)
{
    const int clamped = qMax(0, px);
    if (m_controlColumn == clamped) {
        return;
    }
    m_controlColumn = clamped;
    updateGeometry();
    relayout();
    update();
}

QString SettingRow::displayValue() const
{
    if (!m_masked) {
        return m_value;
    }
    // A fixed run of bullets, not one per character. Masking that preserves the
    // length leaks the length, which for a passphrase is most of what an
    // attacker wanted to know.
    return QString(kMaskLength, kMaskGlyph);
}

int SettingRow::textWidth(int width) const
{
    int right = 0;
    if (m_controlColumn > 0) {
        right = m_controlColumn;
    } else if (m_control) {
        right = m_control->sizeHint().width();
    } else if (!m_value.isEmpty()) {
        right = QFontMetrics(valueFont(this, m_mono, m_masked))
                    .horizontalAdvance(displayValue());
    }

    int available = width - 2 * kPaddingH - (right > 0 ? right + kGap : 0);

    // The design's 58ch cap on the hint. Applied to the text column as a whole
    // so the name and its sentence share one measure.
    const QFontMetrics hints(hintFont(this));
    const int cap = hints.averageCharWidth() * kHintMaxChars;
    if (cap > 0) {
        available = qMin(available, cap);
    }
    return qMax(0, available);
}

int SettingRow::textHeight(int width) const
{
    int height = QFontMetrics(nameFont(this)).height();
    if (!m_hint.isEmpty()) {
        height += kHintGap
            + wrappedHeight(QFontMetrics(hintFont(this)), m_hint, textWidth(width),
                            kHintLeading);
    }
    if (!m_error.isEmpty()) {
        height += kErrorTop
            + qMax(kErrorMark, QFontMetrics(errorFont(this)).height());
    }
    return height;
}

int SettingRow::heightForWidth(int width) const
{
    const int right = m_control ? m_control->sizeHint().height() : 0;
    return qMax(textHeight(width), right) + 2 * kPaddingV;
}

QSize SettingRow::sizeHint() const
{
    const QFontMetrics names(nameFont(this));
    int width = 2 * kPaddingH + names.horizontalAdvance(m_name);
    if (m_control) {
        width += kGap + m_control->sizeHint().width();
    } else if (!m_value.isEmpty()) {
        width += kGap
            + QFontMetrics(valueFont(this, m_mono, m_masked)).horizontalAdvance(displayValue());
    }
    width = qMax(width, 280);
    return QSize(width, heightForWidth(width));
}

QSize SettingRow::minimumSizeHint() const
{
    return QSize(240, heightForWidth(240));
}

void SettingRow::relayout()
{
    if (!m_control) {
        return;
    }
    const QSize hint = m_control->sizeHint();
    const int columnWidth = m_controlColumn > 0 ? m_controlColumn : hint.width();
    // Aligned to the name's line rather than to the middle of the row: with a
    // three-line hint a centred control floats beside the sentence instead of
    // beside the thing it changes.
    const int top = kPaddingV
        + (QFontMetrics(nameFont(this)).height() - hint.height()) / 2;
    m_control->setGeometry(width() - kPaddingH - columnWidth, qMax(kPaddingV, top),
                           columnWidth, hint.height());
}

void SettingRow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    relayout();
}

bool SettingRow::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange
        || event->type() == QEvent::LayoutRequest) {
        updateGeometry();
        relayout();
        update();
    }
    return QWidget::event(event);
}

StyleSpec SettingRow::styleSpec() const
{
    StyleSpec spec;
    spec.background = token(ColorRole::Transparent);
    spec.border = token(ColorRole::BorderSubtle);
    spec.foreground = token(ColorRole::TextPrimary);
    spec.radius = SurfaceRole::Card;
    spec.borderWidth = BorderRole::Divider;
    spec.type = TypeRole::Body;
    if (hasError()) {
        // The row's own contract changes when it is in error, which is what a
        // test asserting "an invalid setting is marked" should be able to read
        // without measuring a pixel.
        spec.foreground = statusToken(SemanticRole::Danger, ColorRole::StatusText);
        spec.border = statusToken(SemanticRole::Danger, ColorRole::StatusBorder);
    }
    return spec;
}

ResolvedStyle SettingRow::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

void SettingRow::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const int available = textWidth(width());

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (m_divider) {
        const int borderWidth = qMax(1, profile.border(BorderRole::Divider));
        painter.setPen(QPen(profile.colors.borderSubtle, borderWidth));
        painter.drawLine(0, height() - borderWidth, width(), height() - borderWidth);
    }

    int y = kPaddingV;

    const QFont names = nameFont(this);
    const QFontMetrics nameMetrics(names);
    painter.setFont(names);
    painter.setPen(profile.colors.textPrimary);
    painter.drawText(QRect(kPaddingH, y, available, nameMetrics.height()),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     nameMetrics.elidedText(m_name, Qt::ElideRight, available));

    // The read-only value, on the name's own line.
    if (!m_value.isEmpty()) {
        const QFont font = valueFont(this, m_mono, m_masked);
        const QFontMetrics fm(font);
        const QString text = displayValue();
        const int valueWidth = fm.horizontalAdvance(text);
        painter.setFont(font);
        painter.setPen(profile.colors.textSecondary);
        painter.drawText(QRect(width() - kPaddingH - valueWidth, y, valueWidth,
                               nameMetrics.height()),
                         Qt::AlignRight | Qt::AlignVCenter, text);
    }
    y += nameMetrics.height();

    if (!m_hint.isEmpty()) {
        y += kHintGap;
        const QFont font = hintFont(this);
        const QFontMetrics fm(font);
        QTextOption wrap(Qt::AlignLeft | Qt::AlignTop);
        wrap.setWrapMode(QTextOption::WordWrap);
        painter.setFont(font);
        painter.setPen(profile.colors.textDisabled);
        const int hintHeight = wrappedHeight(fm, m_hint, available, kHintLeading);
        painter.drawText(QRectF(kPaddingH, y, available, hintHeight), m_hint, wrap);
        y += hintHeight;
    }

    if (m_error.isEmpty()) {
        return;
    }

    // `.ferr` — a small warning mark and the message, in the danger ink. Under
    // the row rather than beside the control, so a long message does not push
    // the control off the right edge.
    y += kErrorTop;
    const QFont font = errorFont(this);
    const QFontMetrics fm(font);
    const int lineHeight = qMax(kErrorMark, fm.height());
    const QColor ink = profile.semantic.danger.text;

    paintGlyph(&painter, Glyph::Warning,
               QRectF(kPaddingH, y + (lineHeight - kErrorMark) / 2.0, kErrorMark, kErrorMark),
               ink);
    painter.setFont(font);
    painter.setPen(ink);
    const QRect box(kPaddingH + kErrorMark + kErrorGap, y,
                    qMax(0, width() - 2 * kPaddingH - kErrorMark - kErrorGap), lineHeight);
    painter.drawText(box, Qt::AlignLeft | Qt::AlignVCenter,
                     fm.elidedText(m_error, Qt::ElideRight, box.width()));
}

} // namespace novu::ui
