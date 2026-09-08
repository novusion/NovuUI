#include "novu/ui/Fields.h"

#include "novu/ui/Style.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QAbstractItemView>
#include <QCalendarWidget>
#include <QCheckBox>
#include <QCompleter>
#include <QDateEdit>
#include <QLineEdit>
#include <QListView>
#include <QPainter>
#include <QPolygonF>
#include <QTextCharFormat>

namespace novu::ui {

namespace {

// QColor::name() drops alpha; a token defined as a translucent overlay would
// otherwise reach the sheet fully opaque. Same rule the scope stylesheet uses.
QString css(const QColor& color)
{
    return color.alpha() == 255 ? color.name() : rgba(color, color.alphaF());
}

} // namespace

ComboBox::ComboBox(QWidget* parent)
    : QComboBox(parent)
{
    // An explicit QListView, so the popup honours the ::item rules the scope
    // stylesheet defines. QComboBox's built-in view is a private subclass whose
    // delegate paints the row from the palette, which is why the hover on these
    // lists was invisible however the stylesheet was written.
    setView(new QListView(this));

    // And the palette to match, for the same reason: a styled delegate falls
    // back to Highlight for the row under the pointer.
    const ThemeColors& colors = themeFor(this).colors;
    QPalette popup = view()->palette();
    popup.setColor(QPalette::Highlight, colors.surfaceActive.lighter(145));
    popup.setColor(QPalette::HighlightedText, colors.textPrimary);
    popup.setColor(QPalette::Base, colors.panelBgAlt);
    popup.setColor(QPalette::Text, colors.textPrimary);
    view()->setPalette(popup);
}

void ComboBox::setSearchable(bool searchable)
{
    setEditable(searchable);
    if (!searchable) {
        setCompleter(nullptr);
        return;
    }
    // Type to narrow, rather than scrolling a list of several hundred symbols.
    // Contains, not starts-with: nobody remembers whether a pair is PF_XBTUSD or
    // XBTUSD when they are looking for "xbt".
    setInsertPolicy(QComboBox::NoInsert);
    auto* matcher = new QCompleter(model(), this);
    matcher->setCaseSensitivity(Qt::CaseInsensitive);
    matcher->setFilterMode(Qt::MatchContains);
    matcher->setCompletionMode(QCompleter::PopupCompletion);
    setCompleter(matcher);

    const ThemeColors& colors = themeFor(this).colors;
    if (lineEdit()) {
        QPalette field = lineEdit()->palette();
        field.setColor(QPalette::PlaceholderText, colors.textDisabled);
        lineEdit()->setPalette(field);
    }
}

void ComboBox::paintEvent(QPaintEvent* event)
{
    QComboBox::paintEvent(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const qreal cx = width() - 14.0;
    const qreal cy = height() / 2.0;
    p.setPen(Qt::NoPen);
    p.setBrush(themeFor(this).colors.textSecondary);
    p.drawPolygon(QPolygonF({QPointF(cx - 4.5, cy - 2.0),
                             QPointF(cx + 4.5, cy - 2.0),
                             QPointF(cx, cy + 3.5)}));
}

ComboBox* comboBox(const QStringList& items, QWidget* parent, int width)
{
    auto* c = new ComboBox(parent);
    c->addItems(items);
    c->setFont(roleFont(parent, TypeRole::Body));
    c->setFixedHeight(themeFor(parent).controlHeight(ControlSize::Medium));
    if (width > 0) {
        c->setFixedWidth(width);
    }
    return c;
}

DateEdit::DateEdit(QWidget* parent)
    : QDateEdit(parent)
{
}

void DateEdit::paintEvent(QPaintEvent* event)
{
    QDateEdit::paintEvent(event);

    // A calendar mark over the popup button. The stylesheet hides Qt's default
    // arrows to keep the field looking like the inputs beside it, which without
    // this leaves the button invisible - clicking the middle of the field does
    // nothing and only the bare right edge opens anything.
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const ThemeColors& c = themeFor(this).colors;

    const qreal w = 13.0;
    const qreal h = 12.0;
    const qreal x = width() - w - 9.0;
    const qreal y = (height() - h) / 2.0;

    QPen pen(c.textSecondary);
    pen.setWidthF(1.2);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    // The page, its two binder rings and the line under the header - the least
    // that still reads as a calendar at this size.
    p.drawRoundedRect(QRectF(x, y + 1.5, w, h - 1.5), 2.0, 2.0);
    p.drawLine(QPointF(x, y + 5.0), QPointF(x + w, y + 5.0));
    p.drawLine(QPointF(x + 3.5, y), QPointF(x + 3.5, y + 3.0));
    p.drawLine(QPointF(x + w - 3.5, y), QPointF(x + w - 3.5, y + 3.0));
}

DateEdit* dateEdit(const QDate& value, QWidget* parent, int width)
{
    auto* edit = new DateEdit(parent);
    edit->setDate(value);
    edit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    edit->setCalendarPopup(true);
    edit->setFont(roleFont(parent, TypeRole::Body));
    edit->setFixedHeight(themeFor(parent).controlHeight(ControlSize::Medium));
    if (width > 0) {
        edit->setFixedWidth(width);
    }

    const ThemeColors& c = themeFor(parent).colors;
    auto* calendar = new QCalendarWidget(edit);
    calendar->setGridVisible(false);
    calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    // Colours from the profile, not from hex typed into a call site. The popup
    // is a separate top-level window, so the scope stylesheet does not reach it
    // and it carries its own.
    calendar->setStyleSheet(
        QStringLiteral(
            "QCalendarWidget QWidget { background: %1; color: %2; }"
            "QCalendarWidget QToolButton { color: %2; background: transparent;"
            " border: none; padding: 6px; }"
            "QCalendarWidget QToolButton:hover { background: %3; border-radius: 4px; }"
            "QCalendarWidget QMenu { background: %4; color: %2; border: 1px solid %5; }"
            "QCalendarWidget QSpinBox { color: %2; background: %4;"
            " border: 1px solid %5; }"
            "QCalendarWidget QAbstractItemView { background: %1; color: %2;"
            " selection-background-color: %6; selection-color: %7; outline: 0;"
            " padding: 4px; }"
            "QCalendarWidget QAbstractItemView::item { min-width: 34px;"
            " min-height: 26px; }"
            "QCalendarWidget QAbstractItemView:disabled { color: %8; }")
            .arg(css(c.panelBg), css(c.textPrimary), rgba(c.accent, 0.18),
                 css(c.panelBgAlt), css(c.border), css(c.accent),
                 css(c.textOnAccent), css(c.textDisabled)));

    // Weekends in the danger ink, weekdays in the primary one: left alone they
    // inherit the system palette and come out near-black on the dark popup.
    QTextCharFormat weekday;
    weekday.setForeground(c.textPrimary);
    QTextCharFormat weekend;
    weekend.setForeground(c.danger);
    for (Qt::DayOfWeek day : {Qt::Monday, Qt::Tuesday, Qt::Wednesday, Qt::Thursday,
                              Qt::Friday}) {
        calendar->setWeekdayTextFormat(day, weekday);
    }
    calendar->setWeekdayTextFormat(Qt::Saturday, weekend);
    calendar->setWeekdayTextFormat(Qt::Sunday, weekend);
    edit->setCalendarWidget(calendar);
    return edit;
}

QLineEdit* lineEdit(const QString& placeholder, QWidget* parent, int width,
                    const QString& value)
{
    auto* e = new QLineEdit(parent);
    e->setPlaceholderText(placeholder);
    if (!value.isEmpty()) {
        e->setText(value);
    }
    e->setFont(roleFont(parent, TypeRole::Body));
    // Through the palette, because Qt has no ::placeholder pseudo-element —
    // a QSS rule for it is silently ignored, and the hint then renders in the
    // primary ink and reads as a value the user already typed.
    QPalette palette = e->palette();
    palette.setColor(QPalette::PlaceholderText, themeFor(parent).colors.textDisabled);
    e->setPalette(palette);
    e->setFixedHeight(themeFor(parent).controlHeight(ControlSize::Medium));
    if (width > 0) {
        e->setFixedWidth(width);
    }
    return e;
}

CheckBox::CheckBox(QWidget* parent)
    : QCheckBox(parent)
{
    // The style needs an application style to draw through. Installed here as
    // well as from applyStyleSheet(), so a checkbox built before anything has
    // been themed still gets its mark.
    installNovuStyle();
}

CheckBox::CheckBox(const QString& text, QWidget* parent)
    : QCheckBox(text, parent)
{
    installNovuStyle();
}

void CheckBox::setMark(Mark mark)
{
    if (m_mark == mark) {
        return;
    }
    m_mark = mark;
    // Announced as a property rather than painted here. NovuStyle draws every
    // check indicator in the process - including the ones in table cells and
    // tree items, which are not widgets and which this class could never
    // reach - so there is exactly one drawing of a Novu checkbox, and this
    // says which mark it should use.
    setProperty(prop::mark,
                mark == Mark::Cross ? QStringLiteral("cross") : QStringLiteral("check"));
    update();
}

QPlainTextEdit* textArea(const QString& placeholder, QWidget* parent, int rows,
                         const QString& value)
{
    auto* e = new QPlainTextEdit(parent);
    e->setPlaceholderText(placeholder);
    if (!value.isEmpty()) {
        e->setPlainText(value);
    }
    e->setFont(roleFont(parent, TypeRole::Body));
    // Same reason lineEdit() does it: Qt has no ::placeholder pseudo-element,
    // so a QSS rule for it is silently ignored and the hint renders in the
    // primary ink, reading as a value the user already typed.
    QPalette palette = e->palette();
    palette.setColor(QPalette::PlaceholderText, themeFor(parent).colors.textDisabled);
    e->setPalette(palette);

    // Measured in lines rather than pixels, and carrying the same padding the
    // sheet declares. A height guessed in pixels comes out a line short at 125%
    // scaling, which is where a description field starts scrolling while the
    // user is still typing the first sentence.
    const QFontMetrics metrics(e->font());
    e->setFixedHeight(metrics.lineSpacing() * qMax(1, rows) + 12
                      + 2 * themeFor(parent).border(BorderRole::Input));
    e->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    // Tab leaves the field. In a form, a tab that inserts whitespace strands
    // keyboard users in the one control they cannot get out of.
    e->setTabChangesFocus(true);
    e->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    return e;
}

CheckBox* checkBox(const QString& text, QWidget* parent, bool checked)
{
    auto* c = new CheckBox(text, parent);
    c->setFont(roleFont(parent, TypeRole::Body));
    c->setChecked(checked);
    c->setCursor(Qt::PointingHandCursor);
    return c;
}

} // namespace novu::ui
