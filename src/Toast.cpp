#include "novu/ui/Toast.h"

#include "novu/ui/Motion.h"
#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QEnterEvent>
#include <QEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QTimer>
#include <QVariantAnimation>

namespace novu::ui {

namespace {

// `.toast { bottom:46px; padding:8px 15px; gap:9px; border-radius:pill }`.
constexpr int kPaddingH = 15;
constexpr int kPaddingV = 8;
constexpr int kGap = 9;
constexpr int kBottomInset = 46;
constexpr int kGlyph = 14;

// The action sits at the end, in the accent, with its own padding.
constexpr int kActionPaddingH = 8;

// How long a toast stays. The floor is what "Copied" needs; the rest is read
// time, at a deliberately generous words-per-minute — a message the user
// glances up at halfway through has to still be there.
constexpr int kMinimumMs = 2200;
constexpr int kPerWordMs = 340;
constexpr int kMaximumMs = 7000;

// How far it rises into place, and over how long.
constexpr int kRise = 10;
constexpr int kEntryMs = 220;

// More than this on screen is a notification centre, which is a different
// thing and not one a toast should turn into by accident.
constexpr int kMaxVisible = 3;
constexpr int kStackGap = 6;
constexpr int kMaxMessageWidth = 560;   // a long message wraps here
constexpr int kHostMargin = 16;         // from the window's edges
constexpr int kWrappedRadius = 16;

Glyph glyphFor(Toast::Tone tone)
{
    switch (tone) {
    case Toast::Tone::Success: return Glyph::Check;
    case Toast::Tone::Warning: return Glyph::Warning;
    case Toast::Tone::Danger:  return Glyph::Warning;
    case Toast::Tone::Info:    return Glyph::Info;
    case Toast::Tone::Accent:  return Glyph::Zap;
    case Toast::Tone::Neutral: break;
    }
    return Glyph::None;
}

SemanticRole roleFor(Toast::Tone tone)
{
    switch (tone) {
    case Toast::Tone::Success: return SemanticRole::Success;
    case Toast::Tone::Warning: return SemanticRole::Warning;
    case Toast::Tone::Danger:  return SemanticRole::Danger;
    case Toast::Tone::Info:    return SemanticRole::Info;
    case Toast::Tone::Accent:
    case Toast::Tone::Neutral: break;
    }
    return SemanticRole::Neutral;
}

QFont messageFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Body);
    font.setPixelSize(profile.type.sm);
    return font;
}

QFont actionFont(const QWidget* context)
{
    QFont font = messageFont(context);
    font.setWeight(QFont::DemiBold);
    return font;
}

} // namespace

// The stacking surface. Private to this file: a caller never needs to name it,
// and exposing it would invite an application to put something other than a
// toast on it.
class ToastLayer : public QWidget
{
    // Q_OBJECT in a .cpp: the class is private to this file and must stay so,
    // but findChild<ToastLayer*>() needs a meta object. The generated moc is
    // included at the bottom, which is Qt's own pattern for exactly this.
    Q_OBJECT

public:
    static ToastLayer* forHost(QWidget* host)
    {
        if (!host) {
            return nullptr;
        }
        if (auto* existing = host->findChild<ToastLayer*>(
                QStringLiteral("novuToastLayer"), Qt::FindDirectChildrenOnly)) {
            return existing;
        }
        return new ToastLayer(host);
    }

    void add(Toast* toast)
    {
        toast->setParent(this);
        m_toasts.append(toast);
        // A fourth replaces the oldest rather than growing the stack.
        while (m_toasts.size() > kMaxVisible) {
            Toast* oldest = m_toasts.takeFirst();
            if (oldest) {
                oldest->dismiss();
            }
        }
        toast->QWidget::show();
        relayout();
    }

    void remove(Toast* toast)
    {
        m_toasts.removeAll(toast);
        relayout();
    }

    QVector<Toast*> toasts() const
    {
        QVector<Toast*> alive;
        for (const QPointer<Toast>& toast : m_toasts) {
            if (toast) {
                alive.append(toast.data());
            }
        }
        return alive;
    }

    void relayout()
    {
        int bottom = height() - kBottomInset;
        for (int i = m_toasts.size() - 1; i >= 0; --i) {
            Toast* toast = m_toasts.at(i);
            if (!toast) {
                continue;
            }
            const QSize hint = toast->sizeHint();
            toast->resize(hint);
            toast->move((width() - hint.width()) / 2, bottom - hint.height());
            bottom -= hint.height() + kStackGap;
        }
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == m_host && event->type() == QEvent::Resize) {
            setGeometry(m_host->rect());
            relayout();
        }
        if (watched == m_host && event->type() == QEvent::ChildAdded) {
            raise();
        }
        return QWidget::eventFilter(watched, event);
    }

private:
    explicit ToastLayer(QWidget* host)
        : QWidget(host)
        , m_host(host)
    {
        setObjectName(QStringLiteral("novuToastLayer"));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
        host->installEventFilter(this);
        setGeometry(host->rect());
        raise();
        show();
    }

    QPointer<QWidget> m_host;
    QVector<QPointer<Toast>> m_toasts;
};

// == Toast ==================================================================

Toast::Toast(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setMouseTracking(true);

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &Toast::dismiss);

    // The design's `slideup`: it arrives rather than appearing. One-shot, so
    // Qt's own animation timer drives it; the shared Pulse is for loops.
    auto* entry = new QVariantAnimation(this);
    entry->setDuration(motion::enabled(this) ? kEntryMs : 0);
    entry->setStartValue(0.0);
    entry->setEndValue(1.0);
    entry->setEasingCurve(QEasingCurve::OutCubic);
    connect(entry, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        m_entry = value.toDouble();
        update();
    });
    entry->start(QAbstractAnimation::DeleteWhenStopped);
}

Toast* Toast::post(QWidget* host, const QString& message, Tone tone, int milliseconds)
{
    return postWithAction(host, message, QString(), tone, milliseconds);
}

Toast* Toast::postWithAction(QWidget* host, const QString& message,
                             const QString& actionText, Tone tone, int milliseconds)
{
    ToastLayer* layer = ToastLayer::forHost(host ? host->window() : nullptr);
    if (!layer) {
        return nullptr;
    }
    // The same words already said are not said again: a button pressed twice
    // gives one toast that stays a while longer, not two of the same.
    for (Toast* shown : layer->toasts()) {
        if (shown && shown->m_message == message && shown->m_tone == tone && shown->m_actionText == actionText) {
            shown->startTimer();
            return shown;
        }
    }
    auto* toast = new Toast(layer);
    toast->m_message = message;
    toast->m_actionText = actionText;
    toast->m_tone = tone;
    toast->m_duration = milliseconds;
    layer->add(toast);
    toast->startTimer();
    return toast;
}

QVector<Toast*> Toast::shownOn(QWidget* host)
{
    ToastLayer* layer = host ? host->window()->findChild<ToastLayer*>(
                            QStringLiteral("novuToastLayer"), Qt::FindDirectChildrenOnly)
                             : nullptr;
    return layer ? layer->toasts() : QVector<Toast*>();
}

void Toast::dismissAll(QWidget* host)
{
    for (Toast* toast : shownOn(host)) {
        toast->dismiss();
    }
}

int Toast::defaultDuration() const
{
    const int words = m_message.split(QLatin1Char(' '), Qt::SkipEmptyParts).size();
    return qBound(kMinimumMs, kMinimumMs + words * kPerWordMs, kMaximumMs);
}

void Toast::startTimer()
{
    m_timer->start(m_duration > 0 ? m_duration : defaultDuration());
}

void Toast::setMessage(const QString& message)
{
    if (m_message == message) {
        return;
    }
    m_message = message;
    updateGeometry();
    if (auto* layer = qobject_cast<ToastLayer*>(parentWidget())) {
        layer->relayout();
    }
    update();
}

void Toast::setTone(Tone tone)
{
    if (m_tone == tone) {
        return;
    }
    m_tone = tone;
    update();
}

void Toast::setActionText(const QString& text)
{
    if (m_actionText == text) {
        return;
    }
    m_actionText = text;
    updateGeometry();
    update();
}

void Toast::setDuration(int milliseconds)
{
    m_duration = milliseconds;
    if (m_timer->isActive()) {
        startTimer();
    }
}

void Toast::hold()
{
    m_timer->stop();
}

void Toast::dismiss()
{
    m_timer->stop();
    if (auto* layer = qobject_cast<ToastLayer*>(parentWidget())) {
        layer->remove(this);
    }
    emit dismissed();
    deleteLater();
}

QRect Toast::actionRect() const
{
    if (m_actionText.isEmpty()) {
        return QRect();
    }
    const QFontMetrics fm(actionFont(this));
    const int actionWidth = fm.horizontalAdvance(m_actionText) + 2 * kActionPaddingH;
    return QRect(width() - kPaddingH - actionWidth + kActionPaddingH / 2, 0,
                 actionWidth, height());
}

// What the message may take across: a long or multi-line message wraps at
// this width (or at the window's, when that is narrower) and the toast grows
// downwards - it used to stay one line high and cut the message off.
int Toast::messageWidthLimit() const
{
    int chrome = 2 * kPaddingH;
    if (glyphFor(m_tone) != Glyph::None) chrome += kGlyph + kGap;
    if (!m_actionText.isEmpty())
        chrome += kGap + QFontMetrics(actionFont(this)).horizontalAdvance(m_actionText) + 2 * kActionPaddingH;
    int limit = kMaxMessageWidth;
    if (parentWidget() && parentWidget()->width() > 0)
        limit = qMin(limit, parentWidget()->width() - 2 * kHostMargin - chrome);
    return qMax(120, limit);
}

QSize Toast::sizeHint() const
{
    const QFontMetrics fm(messageFont(this));
    const QRect text = fm.boundingRect(QRect(0, 0, messageWidthLimit(), 100000), Qt::TextWordWrap, m_message);
    int width = 2 * kPaddingH + text.width();
    if (glyphFor(m_tone) != Glyph::None) {
        width += kGlyph + kGap;
    }
    if (!m_actionText.isEmpty()) {
        width += kGap + QFontMetrics(actionFont(this)).horizontalAdvance(m_actionText)
            + 2 * kActionPaddingH;
    }
    return QSize(width, qMax(kGlyph, text.height()) + 2 * kPaddingV);
}

void Toast::enterEvent(QEnterEvent* event)
{
    // A pointer on a toast is a user reading it. Holding while hovered is the
    // one behaviour that makes a message with an action usable at all.
    hold();
    QWidget::enterEvent(event);
}

void Toast::leaveEvent(QEvent* event)
{
    if (m_actionHovered) {
        m_actionHovered = false;
        update();
    }
    startTimer();
    QWidget::leaveEvent(event);
}

void Toast::mouseMoveEvent(QMouseEvent* event)
{
    const bool inAction = actionRect().contains(event->position().toPoint());
    if (inAction != m_actionHovered) {
        m_actionHovered = inAction;
        setCursor(inAction ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void Toast::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && actionRect().contains(event->position().toPoint())) {
        m_actionPressed = true;
        update();
        return;
    }
    QWidget::mousePressEvent(event);
}

void Toast::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_actionPressed) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    m_actionPressed = false;
    if (actionRect().contains(event->position().toPoint())) {
        emit actionTriggered();
        // A toast whose action has been taken is a toast about something that
        // is no longer true.
        dismiss();
    }
}

bool Toast::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

StyleSpec Toast::styleSpec() const
{
    StyleSpec spec;
    // `.toast { background:var(--card-h) }` — a step above the panel, so it
    // reads as being in front of the window rather than part of it.
    spec.background = token(ColorRole::SurfaceHover);
    spec.border = token(ColorRole::Border);
    spec.foreground = token(ColorRole::TextSecondary);
    spec.radius = SurfaceRole::Badge;   // the pill
    spec.borderWidth = BorderRole::Card;
    spec.type = TypeRole::Body;
    return spec;
}

ResolvedStyle Toast::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

void Toast::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const ResolvedStyle style = resolvedStyle();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // The rise, and the fade that goes with it.
    painter.translate(0.0, (1.0 - m_entry) * kRise);
    painter.setOpacity(qBound(0.0, m_entry, 1.0));

    const QRectF pill = QRectF(rect()).adjusted(style.borderWidth / 2.0,
                                                style.borderWidth / 2.0,
                                                -style.borderWidth / 2.0,
                                                -style.borderWidth / 2.0);
    // a pill while it is one line; a rounded card once it wraps
    const double radius = qMin(pill.height() / 2.0, double(kWrappedRadius));
    painter.setPen(QPen(style.border, style.borderWidth));
    painter.setBrush(style.background);
    painter.drawRoundedRect(pill, radius, radius);

    int x = kPaddingH;
    const Glyph glyph = glyphFor(m_tone);
    if (glyph != Glyph::None) {
        const QColor ink = m_tone == Tone::Accent
            ? profile.colors.accentLight
            : profile.semantic.role(roleFor(m_tone)).text;
        // beside the first line of the message
        const int firstLine = QFontMetrics(messageFont(this)).height();
        paintGlyph(&painter, glyph,
                   QRectF(x, kPaddingV + (firstLine - kGlyph) / 2.0, kGlyph, kGlyph), ink);
        x += kGlyph + kGap;
    }

    const QRect action = actionRect();
    int rightEdge = action.isNull() ? width() - kPaddingH : action.left() - kGap;

    painter.setFont(messageFont(this));
    painter.setPen(profile.colors.textSecondary);
    const QRect text(x, kPaddingV, qMax(0, rightEdge - x), height() - 2 * kPaddingV);
    painter.drawText(text, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, m_message);

    if (action.isNull()) {
        return;
    }
    if (m_actionHovered || m_actionPressed) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(withAlpha(profile.colors.accent, m_actionPressed ? 0.22 : 0.14));
        const QRectF plate = QRectF(action).adjusted(0, kPaddingV / 2.0, 0, -kPaddingV / 2.0);
        painter.drawRoundedRect(plate, plate.height() / 2.0, plate.height() / 2.0);
    }
    painter.setFont(actionFont(this));
    painter.setPen(profile.colors.accentLight);
    painter.drawText(action, Qt::AlignCenter, m_actionText);
}

} // namespace novu::ui

#include "Toast.moc"
