#include "novu/ui/GateCard.h"

#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QTextOption>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

// `.gate-card { width:min(420px, 92%); padding:28px 30px }`.
constexpr int kWidth = 420;
constexpr int kPaddingH = 30;
constexpr int kPaddingTop = 28;
constexpr int kPaddingBottom = 24;

// `.gate-mark` / `.login-mark` — the big one, centred, with the title under it.
constexpr int kMarkSide = 58;
constexpr int kMarkGap = 16;
constexpr int kTitleGap = 6;
constexpr int kFormGap = 22;

// `.gate-foot { border-top:1px solid hair; padding-top:14px }`.
constexpr int kFootGap = 18;
constexpr int kFootPadding = 14;
constexpr int kFootLineGap = 4;

// The error strip above the form.
constexpr int kErrorGap = 14;
constexpr int kErrorPadding = 9;
constexpr int kErrorMark = 13;
constexpr double kErrorTint = 0.10;

constexpr double kSubtitleLeading = 1.5;

QFont titleFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::PageTitle, true);
    font.setPixelSize(profile.type.xl - 2);
    return font;
}

QFont subtitleFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Caption);
    font.setPixelSize(profile.type.sm);
    return font;
}

QFont footFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Mono);
    font.setPixelSize(profile.type.xs);
    return font;
}

int wrappedHeight(const QFontMetrics& fm, const QString& text, int width, double leading)
{
    if (text.isEmpty() || width <= 0) {
        return 0;
    }
    const QRect bounds = fm.boundingRect(QRect(0, 0, width, 10000),
                                         Qt::TextWordWrap | Qt::AlignHCenter | Qt::AlignTop,
                                         text);
    const int lines = qMax(1, qRound(double(bounds.height()) / qMax(1, fm.height())));
    return qRound(fm.height() * leading) * lines;
}

} // namespace

GateCard::GateCard(const QString& title, QWidget* parent)
    : QWidget(parent)
    , m_title(title)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    setFixedWidth(kWidth);

    m_mark = new AppMark(Glyph::None, this, AppMark::Size::Large);
    m_mark->setSideOverride(kMarkSide);

    m_form = new QWidget(this);
    auto* layout = new QVBoxLayout(m_form);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(themeFor(this).space(SpacingRole::FormRowGap));
}

void GateCard::setTitle(const QString& title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    update();
}

void GateCard::setSubtitle(const QString& subtitle)
{
    if (m_subtitle == subtitle) {
        return;
    }
    m_subtitle = subtitle;
    updateGeometry();
    relayout();
    update();
}

void GateCard::setGlyph(Glyph glyph)
{
    m_mark->setGlyph(glyph);
}

QHBoxLayout* GateCard::actions()
{
    if (!m_actions) {
        m_actions = new QHBoxLayout;
        m_actions->setSpacing(themeFor(this).space(SpacingRole::ToolbarGap));
        form()->addLayout(m_actions);
        applyActionAlignment(m_actions, m_actionAlignment);
    }
    return m_actions;
}

void GateCard::setActionAlignment(ActionAlignment alignment)
{
    m_actionAlignment = alignment;
    if (m_actions) {
        applyActionAlignment(m_actions, alignment);
    }
}

QVBoxLayout* GateCard::form() const
{
    return qobject_cast<QVBoxLayout*>(m_form->layout());
}

void GateCard::setHost(const QString& host)
{
    if (m_host == host) {
        return;
    }
    m_host = host;
    updateGeometry();
    relayout();
    update();
}

void GateCard::setFootnote(const QString& note)
{
    if (m_footnote == note) {
        return;
    }
    m_footnote = note;
    updateGeometry();
    relayout();
    update();
}

void GateCard::setError(const QString& message)
{
    if (m_error == message) {
        return;
    }
    m_error = message;
    // An error is the answer to the attempt, so the attempt is over.
    if (!m_error.isEmpty() && m_busy) {
        setBusy(false);
    }
    updateGeometry();
    relayout();
    update();
}

void GateCard::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    // The whole form, not only the button: a second press while the first
    // request is out creates a second session.
    m_form->setEnabled(!busy);
    update();
}

int GateCard::headHeight() const
{
    const QFontMetrics titles(titleFont(this));
    int height = kMarkSide + kMarkGap + titles.height();
    if (!m_subtitle.isEmpty()) {
        height += kTitleGap
            + wrappedHeight(QFontMetrics(subtitleFont(this)), m_subtitle,
                            kWidth - 2 * kPaddingH, kSubtitleLeading);
    }
    return height;
}

int GateCard::errorHeight() const
{
    if (m_error.isEmpty()) {
        return 0;
    }
    const QFontMetrics fm(subtitleFont(this));
    const int text = wrappedHeight(fm, m_error, kWidth - 2 * kPaddingH - kErrorMark - 8,
                                   kSubtitleLeading);
    return kErrorGap + qMax(text, kErrorMark) + 2 * kErrorPadding;
}

int GateCard::footHeight() const
{
    if (m_host.isEmpty() && m_footnote.isEmpty()) {
        return 0;
    }
    const QFontMetrics fm(footFont(this));
    int height = kFootGap + kFootPadding + fm.height();
    if (!m_host.isEmpty() && !m_footnote.isEmpty()) {
        height += kFootLineGap + fm.height();
    }
    return height;
}

QSize GateCard::sizeHint() const
{
    const int height = kPaddingTop + headHeight() + errorHeight() + kFormGap
        + m_form->sizeHint().height() + footHeight() + kPaddingBottom;
    return QSize(kWidth, height);
}

QSize GateCard::minimumSizeHint() const
{
    return sizeHint();
}

void GateCard::relayout()
{
    m_mark->move((width() - kMarkSide) / 2, kPaddingTop);

    const int top = kPaddingTop + headHeight() + errorHeight() + kFormGap;
    const int formHeight = m_form->sizeHint().height();
    m_form->setGeometry(kPaddingH, top, qMax(0, width() - 2 * kPaddingH), formHeight);
}

void GateCard::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    relayout();
}

bool GateCard::event(QEvent* event)
{
    // The alignment has to survive the row being filled. A caller adds its
    // button after actions() handed the layout over, and appending lands the
    // widget *after* the trailing stretch unless the stretches are put back.
    if (event->type() == QEvent::LayoutRequest && m_actions) {
        applyActionAlignment(m_actions, m_actionAlignment);
    }

    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange
        || event->type() == QEvent::LayoutRequest) {
        updateGeometry();
        relayout();
        update();
    }
    return QWidget::event(event);
}

StyleSpec GateCard::styleSpec() const
{
    StyleSpec spec;
    spec.background = token(ColorRole::PanelBg);
    spec.border = token(ColorRole::Border);
    spec.foreground = token(ColorRole::TextPrimary);
    spec.radius = SurfaceRole::Dialog;
    spec.borderWidth = BorderRole::Card;
    spec.type = TypeRole::PageTitle;
    if (hasError()) {
        spec.border = statusToken(SemanticRole::Danger, ColorRole::StatusBorder);
    }
    return spec;
}

ResolvedStyle GateCard::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

void GateCard::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const ResolvedStyle style = resolvedStyle();
    const int available = width() - 2 * kPaddingH;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF card = QRectF(rect()).adjusted(style.borderWidth / 2.0,
                                                style.borderWidth / 2.0,
                                                -style.borderWidth / 2.0,
                                                -style.borderWidth / 2.0);
    painter.setPen(QPen(style.border, style.borderWidth));
    painter.setBrush(style.background);
    painter.drawRoundedRect(card, style.radius, style.radius);

    int y = kPaddingTop + kMarkSide + kMarkGap;

    const QFont titles = titleFont(this);
    const QFontMetrics titleMetrics(titles);
    painter.setFont(titles);
    painter.setPen(profile.colors.textPrimary);
    painter.drawText(QRect(kPaddingH, y, available, titleMetrics.height()),
                     Qt::AlignHCenter | Qt::AlignVCenter,
                     titleMetrics.elidedText(m_title, Qt::ElideRight, available));
    y += titleMetrics.height();

    if (!m_subtitle.isEmpty()) {
        y += kTitleGap;
        const QFont subs = subtitleFont(this);
        const QFontMetrics fm(subs);
        QTextOption wrap(Qt::AlignHCenter | Qt::AlignTop);
        wrap.setWrapMode(QTextOption::WordWrap);
        painter.setFont(subs);
        painter.setPen(profile.colors.textDisabled);
        const int height = wrappedHeight(fm, m_subtitle, available, kSubtitleLeading);
        painter.drawText(QRectF(kPaddingH, y, available, height), m_subtitle, wrap);
        y += height;
    }

    // ── the error ────────────────────────────────────────────────────────
    if (!m_error.isEmpty()) {
        y += kErrorGap;
        const QFont font = subtitleFont(this);
        const QFontMetrics fm(font);
        const int textWidth = available - kErrorMark - 8;
        const int textHeight = wrappedHeight(fm, m_error, textWidth, kSubtitleLeading);
        const int stripHeight = qMax(textHeight, kErrorMark) + 2 * kErrorPadding;

        const SemanticColor danger = profile.semantic.danger;
        const QRectF strip(kPaddingH, y, available, stripHeight);
        painter.setPen(QPen(danger.border, 1));
        painter.setBrush(withAlpha(danger.text, kErrorTint));
        painter.drawRoundedRect(strip, profile.radii.medium, profile.radii.medium);

        paintGlyph(&painter, Glyph::Warning,
                   QRectF(strip.left() + kErrorPadding, strip.top() + kErrorPadding,
                          kErrorMark, kErrorMark),
                   danger.text);

        QTextOption wrap(Qt::AlignLeft | Qt::AlignTop);
        wrap.setWrapMode(QTextOption::WordWrap);
        painter.setFont(font);
        painter.setPen(danger.text);
        painter.drawText(QRectF(strip.left() + kErrorPadding + kErrorMark + 8,
                                strip.top() + kErrorPadding, textWidth, textHeight),
                         m_error, wrap);
    }

    // ── the footer ───────────────────────────────────────────────────────
    if (m_host.isEmpty() && m_footnote.isEmpty()) {
        return;
    }

    const QFont font = footFont(this);
    const QFontMetrics fm(font);
    int footTop = height() - kPaddingBottom - fm.height();
    if (!m_host.isEmpty() && !m_footnote.isEmpty()) {
        footTop -= kFootLineGap + fm.height();
    }

    painter.setPen(QPen(profile.colors.borderSubtle, 1));
    painter.drawLine(kPaddingH, footTop - kFootPadding, width() - kPaddingH,
                     footTop - kFootPadding);

    painter.setFont(font);
    if (!m_host.isEmpty()) {
        // The one thing a person must be able to check before typing a
        // credential is where it is going, so it is set in the monospace face
        // and given the brighter ink - a hostname in a proportional font is one
        // nobody reads character by character.
        painter.setPen(profile.colors.textSecondary);
        painter.drawText(QRect(kPaddingH, footTop, available, fm.height()),
                         Qt::AlignHCenter | Qt::AlignVCenter,
                         fm.elidedText(m_host, Qt::ElideMiddle, available));
        footTop += fm.height() + kFootLineGap;
    }
    if (!m_footnote.isEmpty()) {
        painter.setPen(profile.colors.textDisabled);
        painter.drawText(QRect(kPaddingH, footTop, available, fm.height()),
                         Qt::AlignHCenter | Qt::AlignVCenter,
                         fm.elidedText(m_footnote, Qt::ElideRight, available));
    }
}

} // namespace novu::ui
