#include "novu/ui/Field.h"

#include "novu/ui/Icon.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QEvent>
#include <QFontMetrics>
#include <QLineEdit>
#include <QPainter>
#include <QPlainTextEdit>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

// The design's `.field { gap: 6 }` — between the caption and the control, and
// between the control and whatever is under it.
constexpr int kGap = 6;
// `.ferr svg { width: 12 }`, and the gap after it.
constexpr int kErrorGlyph = 12;
constexpr int kErrorGap = 6;

// What the control will actually be given, which is not what it asks for.
//
// lineEdit() and its siblings call setFixedHeight() with the profile's control
// height, and setFixedHeight moves the *minimum*, not sizeHint() - a QLineEdit
// still reports the height its font needs. Measuring the hint alone made a
// field two-thirds the height of the control inside it, and the layout then
// stretched the control past the room the field had reserved.
QSize controlSize(const QWidget* control)
{
    if (!control) {
        return {0, 0};
    }
    return control->sizeHint()
        .expandedTo(control->minimumSizeHint())
        .expandedTo(control->minimumSize());
}

} // namespace

Field::Field(const QString& caption, QWidget* control, QWidget* parent)
    : QWidget(parent)
    , m_caption(caption)
    , m_control(control)
{
    build();
}

Field::Field(const QString& caption, QWidget* control, const QString& hint,
             QWidget* parent)
    : QWidget(parent)
    , m_caption(caption)
    , m_hint(hint)
    , m_control(control)
{
    build();
}

void Field::build()
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto* column = new QVBoxLayout(this);
    column->setSpacing(0);
    // The caption and the footer are painted into the margins rather than laid
    // out, so the layout holds exactly one thing: the control.
    if (m_control) {
        m_control->setParent(this);
        column->addWidget(m_control);
    }
    watchControl();
    relayout();
}

void Field::watchControl()
{
    if (!m_control) {
        return;
    }
    // Connected by concrete type rather than by a generic "something changed"
    // signal, because QWidget has none. These three are what a Novu form is
    // built from; anything else keeps its error until the caller clears it.
    const auto cleared = [this] {
        if (m_clearOnEdit && !m_error.isEmpty()) {
            clearError();
        }
    };
    if (auto* line = qobject_cast<QLineEdit*>(m_control)) {
        connect(line, &QLineEdit::textEdited, this, cleared);
    } else if (auto* text = qobject_cast<QPlainTextEdit*>(m_control)) {
        connect(text, &QPlainTextEdit::textChanged, this, cleared);
    } else if (auto* combo = qobject_cast<QComboBox*>(m_control)) {
        connect(combo, &QComboBox::currentIndexChanged, this, cleared);
    }
}

void Field::setCaption(const QString& caption)
{
    if (m_caption == caption) {
        return;
    }
    m_caption = caption;
    relayout();
    update();
}

void Field::setHint(const QString& hint)
{
    if (m_hint == hint) {
        return;
    }
    m_hint = hint;
    relayout();
    update();
}

void Field::setRequired(bool required)
{
    if (m_required == required) {
        return;
    }
    m_required = required;
    update();
}

void Field::setClearsErrorOnEdit(bool clear)
{
    m_clearOnEdit = clear;
}

void Field::setError(const QString& message)
{
    if (m_error == message) {
        return;
    }
    const bool wasValid = m_error.isEmpty();
    m_error = message;

    // The control says what it is, and the scope sheet decides what that looks
    // like. Setting a border colour here would bake the palette into the widget
    // and stop following a theme change.
    if (m_control) {
        m_control->setProperty(prop::invalid, !m_error.isEmpty());
        repolish(m_control);
    }

    relayout();
    update();

    if (wasValid != m_error.isEmpty()) {
        emit validityChanged(m_error.isEmpty());
    }
}

StyleSpec Field::styleSpec() const
{
    StyleSpec spec;
    spec.type = TypeRole::Eyebrow;
    spec.foreground = token(ColorRole::TextDisabled);          // caption and hint
    spec.border = statusToken(SemanticRole::Danger, ColorRole::StatusBorder);
    // The error sentence. Named as a status so it is the same red as a Danger
    // badge and a Danger button, which is the whole point of the shared palette.
    spec.hoverForeground = statusToken(SemanticRole::Danger, ColorRole::StatusText);
    return spec;
}

ResolvedStyle Field::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

int Field::captionHeight() const
{
    if (m_caption.isEmpty()) {
        return 0;
    }
    return QFontMetrics(roleFont(this, TypeRole::Eyebrow)).height() + kGap;
}

int Field::footerHeight() const
{
    const QString text = m_error.isEmpty() ? m_hint : m_error;
    if (text.isEmpty()) {
        return 0;
    }
    const QFontMetrics fm(roleFont(this, TypeRole::Caption));
    // Wrapped at the field's own width: a hint that explains something is
    // usually longer than the control it explains.
    const int available = qMax(1, width() - (m_error.isEmpty() ? 0 : kErrorGlyph + kErrorGap));
    const QRect bounds = fm.boundingRect(QRect(0, 0, available, 0),
                                         Qt::TextWordWrap, text);
    return kGap + qMax(fm.height(), bounds.height());
}

void Field::relayout()
{
    if (auto* column = layout()) {
        column->setContentsMargins(0, captionHeight(), 0, footerHeight());
    }
    updateGeometry();
}

QSize Field::sizeHint() const
{
    const QSize control = controlSize(m_control);
    return QSize(qMax(control.width(), 180),
                 captionHeight() + control.height() + footerHeight());
}

QSize Field::minimumSizeHint() const
{
    const QSize control = controlSize(m_control);
    return QSize(control.width(), captionHeight() + control.height() + footerHeight());
}

bool Field::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::ParentChange
        || event->type() == QEvent::Resize) {
        relayout();
        update();
    }
    return QWidget::event(event);
}

void Field::paintEvent(QPaintEvent*)
{
    const ResolvedStyle style = resolvedStyle();

    QPainter painter(this);

    if (!m_caption.isEmpty()) {
        QFont font = roleFont(this, TypeRole::Eyebrow);
        // The design's `.1em` tracking. Applied here rather than stored in the
        // role, because it is a treatment of this line and not a property of
        // the font.
        font.setLetterSpacing(QFont::PercentageSpacing, 110);
        painter.setFont(font);
        painter.setPen(style.foreground);
        const QRect line(0, 0, width(), captionHeight() - kGap);
        const QString text = m_caption.toUpper();
        painter.drawText(line, Qt::AlignLeft | Qt::AlignVCenter, text);
        if (m_required) {
            // After the caption, in the danger ink: the mark is about the form
            // refusing to submit, which is the same statement the error is.
            const int after = QFontMetrics(painter.font()).horizontalAdvance(text);
            painter.setPen(style.hoverForeground);
            painter.drawText(line.adjusted(after + 4, 0, 0, 0),
                             Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("*"));
        }
    }

    const QString footer = m_error.isEmpty() ? m_hint : m_error;
    if (footer.isEmpty()) {
        return;
    }

    const bool bad = !m_error.isEmpty();
    const QColor ink = bad ? style.hoverForeground : style.foreground;
    painter.setFont(roleFont(this, TypeRole::Caption));
    painter.setPen(ink);

    QRect box(0, height() - footerHeight() + kGap, width(), footerHeight() - kGap);
    if (bad) {
        // `.ferr svg`: the glyph sits on the first line of the sentence, not
        // centred against a message that wrapped to three.
        const QRectF glyph(0, box.top() + (QFontMetrics(painter.font()).height() - kErrorGlyph) / 2.0,
                           kErrorGlyph, kErrorGlyph);
        paintGlyph(&painter, Glyph::Warning, glyph, ink);
        box.setLeft(kErrorGlyph + kErrorGap);
    }
    painter.drawText(box, Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, footer);
}

Field* field(QBoxLayout* into, QWidget* parent, const QString& caption,
             QWidget* control, const QString& hint)
{
    auto* item = new Field(caption, control, hint, parent);
    if (into) {
        into->addWidget(item);
    }
    return item;
}

} // namespace novu::ui
