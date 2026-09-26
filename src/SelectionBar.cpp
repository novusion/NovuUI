#include "novu/ui/SelectionBar.h"

#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>

namespace novu::ui {

namespace {

// `.sel-bar { padding:7px 8px; gap:7px; background:rgba(--acc,.07) }`.
constexpr int kPaddingH = 8;
constexpr int kPaddingV = 7;
constexpr int kGap = 7;
constexpr double kTint = 0.07;

QFont countFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    // `.sel-bar .sel-n` is monospace: the number changes as the user drags a
    // selection, and a proportional figure makes the sentence twitch.
    QFont font = roleMonoFont(context, TypeRole::Mono);
    font.setPixelSize(qMax(8, profile.type.xs));
    return font;
}

} // namespace

SelectionBar::SelectionBar(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_actions = new QWidget(this);
    auto* layout = new QHBoxLayout(m_actions);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(kGap);

    // Nothing is selected at construction, so the bar is not there.
    QWidget::setVisible(false);
}

void SelectionBar::setCount(int count)
{
    const int clamped = qMax(0, count);
    if (m_count == clamped) {
        return;
    }
    const bool was = isVisible();
    m_count = clamped;

    // Hiding at zero is the component's job. A bar reading "0 selected" with a
    // Delete button in it invites exactly the wrong click.
    const bool now = m_count > 0;
    QWidget::setVisible(now);
    updateGeometry();
    relayout();
    update();

    if (was != now) {
        emit shownChanged(now);
    }
}

void SelectionBar::setNoun(const QString& singular, const QString& plural)
{
    m_noun = singular;
    // English's regular plural, which covers file, row, workspace, peer and
    // every other noun these lists hold. Anything else passes both.
    m_plural = plural.isEmpty() ? singular + QLatin1Char('s') : plural;
    updateGeometry();
    update();
}

void SelectionBar::setText(const QString& text)
{
    if (m_override == text) {
        return;
    }
    m_override = text;
    updateGeometry();
    update();
}

QString SelectionBar::displayText() const
{
    if (!m_override.isEmpty()) {
        return m_override;
    }
    if (m_noun.isEmpty()) {
        return QObject::tr("%n selected", nullptr, m_count);
    }
    return QStringLiteral("%1 %2 selected")
        .arg(m_count)
        .arg(m_count == 1 ? m_noun : m_plural);
}

QString SelectionBar::text() const
{
    return displayText();
}

QHBoxLayout* SelectionBar::actions() const
{
    return qobject_cast<QHBoxLayout*>(m_actions->layout());
}

void SelectionBar::relayout()
{
    const bool any = actions()->count() > 0;
    m_actions->setVisible(any);
    if (!any) {
        return;
    }
    const QSize hint = m_actions->sizeHint();
    m_actions->setGeometry(width() - kPaddingH - hint.width(),
                           (height() - hint.height()) / 2,
                           hint.width(), hint.height());
}

QSize SelectionBar::sizeHint() const
{
    const QFontMetrics fm(countFont(this));
    int width = 2 * kPaddingH + fm.horizontalAdvance(displayText());
    int height = fm.height();
    if (actions()->count() > 0) {
        const QSize hint = m_actions->sizeHint();
        width += kGap + hint.width();
        height = qMax(height, hint.height());
    }
    return QSize(width, height + 2 * kPaddingV);
}

QSize SelectionBar::minimumSizeHint() const
{
    return QSize(120, sizeHint().height());
}

void SelectionBar::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    relayout();
}

bool SelectionBar::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange
        || event->type() == QEvent::LayoutRequest) {
        updateGeometry();
        relayout();
        update();
    }
    return QWidget::event(event);
}

StyleSpec SelectionBar::styleSpec() const
{
    StyleSpec spec;
    spec.background = token(ColorRole::Accent, kTint);
    spec.border = token(ColorRole::BorderSubtle);
    spec.foreground = token(ColorRole::AccentLight);
    spec.radius = SurfaceRole::Toolbar;
    spec.borderWidth = BorderRole::Divider;
    spec.type = TypeRole::Mono;
    return spec;
}

ResolvedStyle SelectionBar::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

void SelectionBar::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const ResolvedStyle style = resolvedStyle();

    QPainter painter(this);
    painter.fillRect(rect(), style.background);

    painter.setPen(QPen(style.border, qMax(1, profile.border(BorderRole::Divider))));
    paintHairline(&painter, QPoint(0, 0), QPoint(width() - 1, 0),
                  painter.pen().color(), qMax(1, painter.pen().width()));

    const QFont font = countFont(this);
    const QFontMetrics fm(font);
    painter.setFont(font);
    painter.setPen(style.foreground);

    int available = width() - 2 * kPaddingH;
    if (actions()->count() > 0) {
        available -= m_actions->sizeHint().width() + kGap;
    }
    const QRect box(kPaddingH, 0, qMax(0, available), height());
    painter.drawText(box, Qt::AlignLeft | Qt::AlignVCenter,
                     fm.elidedText(displayText(), Qt::ElideRight, box.width()));
}

} // namespace novu::ui
