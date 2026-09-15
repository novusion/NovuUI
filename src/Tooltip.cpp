#include "novu/ui/Tooltip.h"

#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QPainter>
#include <QPen>
#include <QScreen>
#include <QTextOption>
#include <QTimer>

#include <algorithm>

namespace novu::ui {

namespace {

// The design's timings. The first is the hover delay every tooltip has; the
// second is how long the pointer must stay before the long form is offered.
constexpr int kShowDelayMs = 140;
constexpr int kDetailDelayMs = 620;

// `.tip { max-width:274px; padding:9px 11px; border-radius:9px }`.
constexpr int kMaxWidth = 274;
constexpr int kCompactMaxWidth = 210;
constexpr int kPaddingH = 11;
constexpr int kPaddingV = 9;

// Clear of the target, and clear of the screen edge.
constexpr int kOffset = 8;
constexpr int kScreenMargin = 8;

// The vertical rhythm inside the card, straight off the design's margins:
// `.tip-s { margin-top:3px }`, `.tip-st { margin-top:5px }`,
// `.tip-d { margin-top:7px; padding-top:7px; border-top }`,
// `.tip-more { margin-top:6px }`.
constexpr int kBriefGap = 3;
constexpr int kStatusGap = 5;
constexpr int kDetailGap = 7;
constexpr int kDetailPad = 7;
constexpr int kMoreGap = 6;

// `line-height:1.45` on the brief, `1.5` on the detail.
constexpr double kBriefLeading = 1.45;
constexpr double kDetailLeading = 1.5;

// The shortcut chip: `.tip-t .sc { border-radius:4px; padding:1px 5px }`.
constexpr int kChipPaddingH = 5;
constexpr int kChipPaddingV = 1;
constexpr int kTitleGap = 8;

// Compact tips are the same card one step down the type scale. A ratio rather
// than a second set of sizes, so a profile that changes its scale moves both.
double scaleFor(const TipContent& content)
{
    return content.compact ? 0.88 : 1.0;
}

QFont sized(QFont font, int pixels)
{
    font.setPixelSize(qMax(7, pixels));
    return font;
}

QFont titleFont(const QWidget* context, const TipContent& content)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::CardTitle, true);
    return sized(font, static_cast<int>(profile.type.sm * scaleFor(content)) + 1);
}

QFont chipFont(const QWidget* context, const TipContent& content)
{
    const ThemeProfile& profile = themeFor(context);
    return sized(roleMonoFont(context, TypeRole::Mono),
                 static_cast<int>(profile.type.xxs * scaleFor(content)));
}

QFont briefFont(const QWidget* context, const TipContent& content)
{
    const ThemeProfile& profile = themeFor(context);
    return sized(roleFont(context, TypeRole::Body),
                 static_cast<int>(profile.type.xs * scaleFor(content)));
}

QFont statusFont(const QWidget* context, const TipContent& content)
{
    const ThemeProfile& profile = themeFor(context);
    return sized(roleMonoFont(context, TypeRole::Mono),
                 static_cast<int>(profile.type.xxs * scaleFor(content)));
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

Tooltip& Tooltip::instance()
{
    static Tooltip tip;
    return tip;
}

Tooltip::Tooltip()
    : QWidget(nullptr, Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);

    m_showTimer = new QTimer(this);
    m_showTimer->setSingleShot(true);
    m_showTimer->setInterval(kShowDelayMs);
    connect(m_showTimer, &QTimer::timeout, this, [this] {
        if (m_pending) {
            present(m_pending);
        }
    });

    m_detailTimer = new QTimer(this);
    m_detailTimer->setSingleShot(true);
    m_detailTimer->setInterval(kDetailDelayMs);
    connect(m_detailTimer, &QTimer::timeout, this, [this] {
        if (!m_shown || m_content.detail.isEmpty() || m_detail) {
            return;
        }
        m_detail = true;
        relayout();
    });
}

void Tooltip::install(QWidget* target, const QString& brief)
{
    TipContent content;
    content.brief = brief;
    install(target, content);
}

void Tooltip::install(QWidget* target, const TipContent& content)
{
    if (!target) {
        return;
    }
    Tooltip& tip = instance();
    if (content.isEmpty()) {
        remove(target);
        return;
    }
    if (!tip.m_tips.contains(target)) {
        target->installEventFilter(&tip);
        // Removing on destruction rather than sweeping the table later: a
        // QWidget* key that outlives its widget is a dangling compare on the
        // next hover, not merely a leak.
        connect(target, &QObject::destroyed, &tip, [&tip, target] {
            tip.m_tips.remove(static_cast<QWidget*>(target));
            if (tip.m_shown == target) {
                tip.dismiss();
            }
        });
    }
    tip.m_tips.insert(target, content);
    // A branded tip and the OS one must never both appear.
    target->setToolTip(QString());
}

void Tooltip::remove(QWidget* target)
{
    Tooltip& tip = instance();
    tip.m_tips.remove(target);
    if (tip.m_shown == target) {
        tip.dismiss();
    }
}

TipContent Tooltip::contentFor(const QWidget* target)
{
    return instance().m_tips.value(const_cast<QWidget*>(target));
}

QList<TipContent> Tooltip::all()
{
    QList<TipContent> result = instance().m_tips.values();
    std::sort(result.begin(), result.end(), [](const TipContent& a, const TipContent& b) {
        return a.title.compare(b.title, Qt::CaseInsensitive) < 0;
    });
    return result;
}

void Tooltip::showFor(QWidget* target)
{
    instance().present(target);
}

void Tooltip::hideTip()
{
    instance().dismiss();
}

void Tooltip::setHelpMode(bool on)
{
    Tooltip& tip = instance();
    if (tip.m_help == on) {
        return;
    }
    tip.m_help = on;
    // A tip already on screen is redrawn in the new mode rather than left
    // showing the short form until the pointer moves.
    if (tip.m_shown) {
        tip.present(tip.m_shown);
    }
}

bool Tooltip::helpMode()
{
    return instance().m_help;
}

QWidget* Tooltip::shownFor()
{
    return instance().m_shown;
}

bool Tooltip::eventFilter(QObject* watched, QEvent* event)
{
    auto* widget = qobject_cast<QWidget*>(watched);
    if (!widget || !m_tips.contains(widget)) {
        return QWidget::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::Enter:
        scheduleFor(widget);
        break;
    case QEvent::FocusIn: {
        // Only a deliberate keyboard focus shows a tip. Qt hands the first
        // focusable widget focus at startup, and presenting on that pops a
        // tooltip over an application nobody has touched yet.
        const auto reason = static_cast<QFocusEvent*>(event)->reason();
        if (reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason
            || reason == Qt::ShortcutFocusReason) {
            present(widget);
        }
        break;
    }
    case QEvent::Leave:
    case QEvent::FocusOut:
    case QEvent::MouseButtonPress:
    case QEvent::Wheel:
    case QEvent::Hide:
        dismiss();
        break;
    default:
        break;
    }
    return QWidget::eventFilter(watched, event);
}

void Tooltip::scheduleFor(QWidget* target)
{
    m_pending = target;
    m_showTimer->start();
}

QSize Tooltip::measure(const TipContent& content, bool withDetail) const
{
    const int maxWidth = content.compact ? kCompactMaxWidth : kMaxWidth;
    const int textWidth = maxWidth - 2 * kPaddingH;

    const QFontMetrics titleMetrics(titleFont(this, content));
    const QFontMetrics chipMetrics(chipFont(this, content));
    const QFontMetrics briefMetrics(briefFont(this, content));
    const QFontMetrics statusMetrics(statusFont(this, content));

    int width = 0;
    int height = 0;

    if (!content.title.isEmpty()) {
        int line = titleMetrics.horizontalAdvance(content.title);
        if (!content.shortcut.isEmpty()) {
            line += kTitleGap + chipMetrics.horizontalAdvance(content.shortcut)
                + 2 * kChipPaddingH + 2;
        }
        width = qMax(width, line);
        height += qMax(titleMetrics.height(), chipMetrics.height() + 2 * kChipPaddingV + 2);
    }

    if (!content.brief.isEmpty()) {
        width = qMax(width, qMin(briefMetrics.horizontalAdvance(content.brief), textWidth));
        height += (height > 0 ? kBriefGap : 0)
            + wrappedHeight(briefMetrics, content.brief, textWidth, kBriefLeading);
    }

    if (!content.status.isEmpty()) {
        width = qMax(width, statusMetrics.horizontalAdvance(content.status));
        height += (height > 0 ? kStatusGap : 0) + statusMetrics.height();
    }

    if (withDetail && !content.detail.isEmpty()) {
        width = textWidth;   // the long form always uses the full card
        height += kDetailGap + kDetailPad
            + wrappedHeight(briefMetrics, content.detail, textWidth, kDetailLeading);
    } else if (m_help && !content.detail.isEmpty()) {
        height += kMoreGap + statusMetrics.height();
    }

    width = qBound(60, width, textWidth);
    return QSize(width + 2 * kPaddingH, height + 2 * kPaddingV);
}

void Tooltip::relayout()
{
    if (!m_shown) {
        return;
    }
    const QSize size = measure(m_content, m_detail);
    setFixedSize(size);

    const QRect targetRect(m_shown->mapToGlobal(QPoint(0, 0)), m_shown->size());
    const QRect screen = m_shown->screen()
        ? m_shown->screen()->availableGeometry()
        : QGuiApplication::primaryScreen()->availableGeometry();

    int x = targetRect.center().x() - size.width() / 2;
    x = qBound(screen.left() + kScreenMargin, x,
               qMax(screen.left() + kScreenMargin, screen.right() - size.width() - kScreenMargin));

    int y = targetRect.top() - size.height() - kOffset;
    if (y < screen.top() + kScreenMargin) {
        y = targetRect.bottom() + kOffset;   // no room above: sit below
    }

    move(x, y);
    update();
}

void Tooltip::present(QWidget* target)
{
    m_showTimer->stop();
    if (!target || !m_tips.contains(target) || !target->isVisible()) {
        dismiss();
        return;
    }

    m_content = m_tips.value(target);
    m_shown = target;
    m_detail = false;

    // Off, a tip is the name and the key. The one-line brief is shown even then
    // when there is no title, because a plain install() has nothing else to
    // say and an empty card is worse than the OS tooltip it replaced.
    if (!m_help && !m_content.title.isEmpty()) {
        m_content.status.clear();
        if (!m_content.brief.isEmpty() && !m_content.detail.isEmpty()) {
            m_content.brief.clear();
        }
    }

    relayout();
    show();
    raise();

    if (m_help && !m_content.detail.isEmpty()) {
        m_detailTimer->start();
    }
}

void Tooltip::dismiss()
{
    m_showTimer->stop();
    m_detailTimer->stop();
    m_pending = nullptr;
    m_shown = nullptr;
    m_detail = false;
    if (isVisible()) {
        hide();
    }
}

QSize Tooltip::sizeHint() const
{
    return measure(m_content, m_detail);
}

void Tooltip::paintEvent(QPaintEvent*)
{
    // Resolved against the *target*, not against this popup. A tooltip is a
    // top-level window and therefore inside no ThemeScope of its own, so asking
    // for its own profile would give every tip the default product's colours -
    // which is exactly the bug the token layer exists to prevent.
    const QWidget* context = m_shown ? m_shown.data() : static_cast<const QWidget*>(this);
    const ThemeProfile& profile = themeFor(context);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int borderWidth = qMax(1, profile.border(BorderRole::Card));
    const QRectF card = QRectF(rect()).adjusted(borderWidth / 2.0, borderWidth / 2.0,
                                                -borderWidth / 2.0, -borderWidth / 2.0);
    painter.setPen(QPen(profile.colors.border, borderWidth));
    painter.setBrush(profile.colors.panelBg);
    painter.drawRoundedRect(card, profile.radius(SurfaceRole::Overlay),
                            profile.radius(SurfaceRole::Overlay));

    const int left = kPaddingH;
    const int textWidth = width() - 2 * kPaddingH;
    int y = kPaddingV;

    QTextOption wrap(Qt::AlignLeft | Qt::AlignTop);
    wrap.setWrapMode(QTextOption::WordWrap);

    if (!m_content.title.isEmpty()) {
        const QFont font = titleFont(context, m_content);
        const QFontMetrics fm(font);
        const QFont chip = chipFont(context, m_content);
        const QFontMetrics chipMetrics(chip);
        const int rowHeight =
            qMax(fm.height(), chipMetrics.height() + 2 * kChipPaddingV + 2);

        painter.setFont(font);
        painter.setPen(profile.colors.textPrimary);
        painter.drawText(QRect(left, y, textWidth, rowHeight),
                         Qt::AlignLeft | Qt::AlignVCenter, m_content.title);

        if (!m_content.shortcut.isEmpty()) {
            const int chipWidth =
                chipMetrics.horizontalAdvance(m_content.shortcut) + 2 * kChipPaddingH;
            const int chipHeight = chipMetrics.height() + 2 * kChipPaddingV;
            const QRectF box(left + textWidth - chipWidth - 1.0,
                             y + (rowHeight - chipHeight) / 2.0,
                             chipWidth, chipHeight);
            painter.setPen(QPen(profile.colors.border, 1));
            painter.setBrush(profile.colors.windowBg);
            painter.drawRoundedRect(box, profile.radii.xs + 2, profile.radii.xs + 2);
            painter.setFont(chip);
            painter.setPen(profile.colors.textSecondary);
            painter.drawText(box, Qt::AlignCenter, m_content.shortcut);
        }
        y += rowHeight;
    }

    if (!m_content.brief.isEmpty()) {
        const QFont font = briefFont(context, m_content);
        const QFontMetrics fm(font);
        const int height = wrappedHeight(fm, m_content.brief, textWidth, kBriefLeading);
        if (y > kPaddingV) {
            y += kBriefGap;
        }
        painter.setFont(font);
        painter.setPen(profile.colors.textSecondary);
        painter.drawText(QRectF(left, y, textWidth, height), m_content.brief, wrap);
        y += height;
    }

    if (!m_content.status.isEmpty()) {
        const QFont font = statusFont(context, m_content);
        const QFontMetrics fm(font);
        if (y > kPaddingV) {
            y += kStatusGap;
        }
        painter.setFont(font);
        painter.setPen(profile.colors.accentLight);
        painter.drawText(QRect(left, y, textWidth, fm.height()),
                         Qt::AlignLeft | Qt::AlignVCenter, m_content.status);
        y += fm.height();
    }

    if (m_detail && !m_content.detail.isEmpty()) {
        const QFont font = briefFont(context, m_content);
        const QFontMetrics fm(font);
        y += kDetailGap;
        painter.setPen(QPen(profile.colors.borderSubtle, 1));
        painter.drawLine(left, y, left + textWidth, y);
        y += kDetailPad;
        painter.setFont(font);
        painter.setPen(profile.colors.textDisabled);
        painter.drawText(
            QRectF(left, y, textWidth,
                   wrappedHeight(fm, m_content.detail, textWidth, kDetailLeading)),
            m_content.detail, wrap);
    } else if (m_help && !m_content.detail.isEmpty()) {
        // `.tip-more` — the offer, not the answer.
        const QFont font = statusFont(context, m_content);
        const QFontMetrics fm(font);
        y += kMoreGap;
        painter.setFont(font);
        QColor faded = profile.colors.textDisabled;
        faded.setAlphaF(0.75);
        painter.setPen(faded);
        painter.drawText(QRect(left, y, textWidth, fm.height()),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QObject::tr("Keep hovering for detail"));
    }
}

} // namespace novu::ui
