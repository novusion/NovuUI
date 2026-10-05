#include "novu/ui/SettingRow.h"

#include "novu/ui/Icon.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QMouseEvent>
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
constexpr int kSettingsPaddingH = 6;
constexpr int kSettingsPaddingV = 5;
constexpr int kSettingsLabelWidth = 242;

bool settingsRow(const QWidget* row) { return row->property("novuSettingsRow").toBool(); }
int paddingH(const QWidget* row) { return settingsRow(row) ? kSettingsPaddingH : kPaddingH; }
int paddingV(const QWidget* row) { return settingsRow(row) ? kSettingsPaddingV : kPaddingV; }

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
    // `.kv .k .lab` is a *field's* name, which is what TypeRole::Label is.
    // This used to be CardTitle with the size overwritten from the scale,
    // which is two decisions the profile had already made being taken back:
    // a profile that states `.kv .k .lab` at 12.5 got 12 anyway, and every
    // row in the product came out two pixels short of the design.
    return roleFont(context, TypeRole::Label, true);
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

// The height a control will actually be, not the height it asks for.
//
// sizeHint() is a request, and a widget's own minimum and maximum overrule
// it. QPlainTextEdit is the case that matters: it is a scroll area, so its
// hint is Qt's default 256x192 however few lines it is meant to show, and a
// caller that says setFixedHeight(64) does not change that hint - it sets
// min and max on the widget.
//
// A row that measured itself from the hint was therefore 192 pixels tall
// with a 64 pixel field pinned at the top of it, and the remaining 128 read
// as a gap between that field and the next one.
int effectiveControlHeight(const QWidget* control)
{
    if (!control) {
        return 0;
    }
    int height = control->sizeHint().height();
    if (control->maximumHeight() < QWIDGETSIZE_MAX) {
        height = qMin(height, control->maximumHeight());
    }
    return qMax(height, control->minimumHeight());
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
    if (settingsRow(this)) return qMin(kSettingsLabelWidth, qMax(0, width));
    int right = 0;
    if (m_controlColumn > 0) {
        right = m_controlColumn;
    } else if (m_control) {
        right = m_control->sizeHint().width();
    } else if (!m_value.isEmpty()) {
        right = QFontMetrics(valueFont(this, m_mono, m_masked))
                    .horizontalAdvance(displayValue());
    }

    int available = width - 2 * paddingH(this) - (right > 0 ? right + kGap : 0);

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
    const int right = effectiveControlHeight(m_control);
    return qMax(textHeight(width), right) + 2 * paddingV(this);
}

QSize SettingRow::sizeHint() const
{
    const QFontMetrics names(nameFont(this));
    int width = 2 * paddingH(this) + names.horizontalAdvance(m_name);
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
    // The floor is the row with nothing wrapped: one line of name, one line
    // of hint, and whatever control it carries.
    //
    // Not heightForWidth(240): a row's minimum is asked for *while* the layout
    // is deciding its width, so a minimum that depends on the width it has not
    // been given yet is either self-referential or - worse - the height it
    // would need at its narrowest, which every parent then reserves forever.
    const QFontMetrics names(nameFont(this));
    int floor = names.height();
    if (!m_hint.isEmpty()) {
        floor += kHintGap + qRound(QFontMetrics(hintFont(this)).height() * kHintLeading);
    }
    if (m_control) {
        floor = qMax(floor, effectiveControlHeight(m_control));
    }
    return QSize(240, floor + 2 * paddingV(this));
}

void SettingRow::relayout()
{
    if (!m_control) {
        return;
    }
    const QSize hint = m_control->sizeHint();
    int columnWidth = m_controlColumn > 0 ? m_controlColumn : hint.width();
    // Aligned to the name's line rather than to the middle of the row: with a
    // three-line hint a centred control floats beside the sentence instead of
    // beside the thing it changes.
    const int vpad = paddingV(this);
    const int top = vpad
        + (QFontMetrics(nameFont(this)).height() - effectiveControlHeight(m_control)) / 2;

    // The control is placed, not laid out, so the width it is given is the
    // width it gets - and nothing was stopping that running off the end of
    // the row. A size hint is what a widget WANTS; a line edit asks for a
    // couple of hundred pixels and a text area for several, and the section
    // then gives every row the widest of them. In a narrow window the result
    // was a field cut off at the frame.
    //
    // So the column is bounded by what is actually left, and a control that
    // can grow takes all of it rather than its hint: a form's editor column
    // follows the window, which is the whole reason the window resizes.
    int x = 0;
    if (settingsRow(this)) {
        x = paddingH(this) + kSettingsLabelWidth + kGap;
        const int available = qMax(0, width() - paddingH(this) - x);
        const bool expands =
            (m_control->sizePolicy().horizontalPolicy() & QSizePolicy::ExpandFlag) != 0;
        columnWidth = expands ? available : qMin(columnWidth, available);
    } else {
        columnWidth = qMin(columnWidth, qMax(0, width() - 2 * paddingH(this)));
        x = width() - paddingH(this) - columnWidth;
    }
    m_control->setGeometry(x, qMax(vpad, top), columnWidth,
                           effectiveControlHeight(m_control));
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

void SettingRow::setSelectable(bool selectable)
{
    if (m_selectable == selectable) {
        return;
    }
    m_selectable = selectable;
    setCursor(selectable ? Qt::PointingHandCursor : Qt::ArrowCursor);
    setAttribute(Qt::WA_Hover, selectable);
    update();
}

void SettingRow::setSelected(bool selected)
{
    if (m_selected == selected) {
        return;
    }
    m_selected = selected;
    update();
}

void SettingRow::mousePressEvent(QMouseEvent* event)
{
    if (!m_selectable || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    m_pressed = true;
    // Handled here and nowhere else. Passing a press we have acted on to the
    // base class leaves it ignored, and Qt walks an ignored mouse event up
    // the parents until something else answers it.
    event->accept();
}

void SettingRow::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_selectable || event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const bool inside = rect().contains(event->position().toPoint());
    const bool was = m_pressed;
    m_pressed = false;
    event->accept();
    if (was && inside) {
        emit clicked();
    }
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
    // The selection surface, when the row is one that can be picked.
    //
    // The same `.trow.sel { background: rgba(--acc,.14) }` TreeView paints a
    // current row with - one selection colour in the library, not a second
    // one invented for settings. Without these the selected row resolved to
    // an unset colour and painted nothing, so picking a row changed only
    // which detail was below it.
    spec.hoverBackground = token(ColorRole::PanelBg);
    spec.checkedBackground = token(ColorRole::Accent, 0.14);
    spec.checkedForeground = token(ColorRole::TextPrimary);
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

    // The selection, under everything else the row draws.
    if (m_selectable && (m_selected || underMouse())) {
        const ResolvedStyle style = resolvedStyle();
        const QColor fill = m_selected ? style.checkedBackground : style.hoverBackground;
        if (fill.isValid() && fill.alpha() > 0) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(fill);
            painter.drawRoundedRect(QRectF(rect()), profile.radii.small + 1,
                                    profile.radii.small + 1);
        }
    }

    if (m_divider) {
        // `.kv{border-bottom:1px solid var(--hair)}` - the rule between two
        // settings is the same hairline every other list in the design draws,
        // not a fainter one. Filled rather than stroked: an antialiased 1px
        // line lands on a half pixel and comes out as two grey rows instead
        // of the design's one.
        const int borderWidth = qMax(1, profile.border(BorderRole::Divider));
        painter.fillRect(QRect(0, height() - borderWidth, width(), borderWidth),
                         profile.colors.border);
    }

    const int hpad = paddingH(this);
    int y = paddingV(this);

    const QFont names = nameFont(this);
    const QFontMetrics nameMetrics(names);
    painter.setFont(names);
    painter.setPen(profile.colors.textPrimary);
    painter.drawText(QRect(hpad, y, available, nameMetrics.height()),
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
        const int valueX = settingsRow(this) ? hpad + kSettingsLabelWidth + kGap
                                             : width() - hpad - valueWidth;
        painter.drawText(QRect(valueX, y, valueWidth, nameMetrics.height()),
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
        painter.drawText(QRectF(hpad, y, available, hintHeight), m_hint, wrap);
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
               QRectF(hpad, y + (lineHeight - kErrorMark) / 2.0, kErrorMark, kErrorMark),
               ink);
    painter.setFont(font);
    painter.setPen(ink);
    const QRect box(hpad + kErrorMark + kErrorGap, y,
                    qMax(0, width() - 2 * hpad - kErrorMark - kErrorGap), lineHeight);
    painter.drawText(box, Qt::AlignLeft | Qt::AlignVCenter,
                     fm.elidedText(m_error, Qt::ElideRight, box.width()));
}

} // namespace novu::ui
