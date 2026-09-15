#include "novu/ui/Drawer.h"

#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QScrollArea>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

// `.drawer-head { padding:16px 18px; gap:13px }`.
constexpr int kHeadPaddingH = 18;
constexpr int kHeadPaddingV = 16;
constexpr int kHeadGap = 13;
constexpr int kHeadLineGap = 5;

// `.drawer-head .appmark { width:40px; border-radius:11px }`.
constexpr int kMarkSide = 40;

// `.drawer-acts { padding:11px 18px; gap:7px }`.
constexpr int kActionsPaddingH = 18;
constexpr int kActionsPaddingV = 11;
constexpr int kActionsGap = 7;

// `.drawer-body { padding:16px 18px 22px }`.
constexpr int kBodyPaddingH = 18;
constexpr int kBodyPaddingTop = 16;
constexpr int kBodyPaddingBottom = 22;

// `.scrim { background:rgba(8,11,14,.58) }`.
constexpr double kScrimAlpha = 0.58;

QFont titleFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::SectionTitle, true);
    font.setPixelSize(profile.type.lg + 1);
    return font;
}

QFont subtitleFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Mono);
    font.setPixelSize(profile.type.xs);
    return font;
}

// The scrim. Its own class so it can paint a dim wash and take the click that
// closes the drawer, which a bare QWidget with a stylesheet cannot do.
class DrawerScrim : public QWidget
{
public:
    DrawerScrim(QWidget* host, Drawer* drawer)
        : QWidget(host)
        , m_drawer(drawer)
    {
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
        hide();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        const ThemeProfile& profile = themeFor(m_drawer);
        QColor wash = profile.colors.windowBg;
        wash.setAlphaF(kScrimAlpha);
        QPainter(this).fillRect(rect(), wash);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        // Swallowed whether or not it closes: the whole point of a scrim is
        // that the list behind it cannot be clicked by accident.
        event->accept();
        if (m_drawer && m_drawer->closesOnScrimClick()) {
            m_drawer->close();
        }
    }

private:
    QPointer<Drawer> m_drawer;
};

} // namespace

Drawer::Drawer(QWidget* host)
    : QWidget(host)
    , m_host(host)
{
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(false);

    m_scrim = new DrawerScrim(host, this);
    m_mark = new AppMark(Glyph::None, this, AppMark::Size::Medium);
    m_mark->setSideOverride(kMarkSide);

    // The body scrolls; the head and the actions are pinned. A drawer whose
    // head scrolled away would lose the name of the thing it is about halfway
    // down a long detail.
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->viewport()->setAutoFillBackground(false);
    m_bodyHost = scroll;

    m_body = new QWidget(scroll);
    m_body->setAutoFillBackground(false);
    auto* layout = new QVBoxLayout(m_body);
    layout->setContentsMargins(kBodyPaddingH, kBodyPaddingTop, kBodyPaddingH,
                               kBodyPaddingBottom);
    layout->setSpacing(themeFor(this).space(SpacingRole::SectionGap));
    scroll->setWidget(m_body);

    if (host) {
        host->installEventFilter(this);
    }
    hide();
}

void Drawer::setTitle(const QString& title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    update();
}

void Drawer::setSubtitle(const QString& subtitle)
{
    if (m_subtitle == subtitle) {
        return;
    }
    m_subtitle = subtitle;
    relayout();
    update();
}

void Drawer::setGlyph(Glyph glyph)
{
    m_mark->setGlyph(glyph);
}

QVBoxLayout* Drawer::body() const
{
    return qobject_cast<QVBoxLayout*>(m_body->layout());
}

QWidget* Drawer::actionsWidget()
{
    if (!m_actions) {
        m_actions = new QWidget(this);
        m_actions->setAutoFillBackground(false);
        auto* layout = new QHBoxLayout(m_actions);
        layout->setContentsMargins(kActionsPaddingH, kActionsPaddingV, kActionsPaddingH,
                                   kActionsPaddingV);
        layout->setSpacing(kActionsGap);
        relayout();
    }
    return m_actions;
}

QHBoxLayout* Drawer::actions()
{
    return qobject_cast<QHBoxLayout*>(actionsWidget()->layout());
}

void Drawer::setEdge(Edge edge)
{
    if (m_edge == edge) {
        return;
    }
    m_edge = edge;
    placeOnHost();
    update();
}

void Drawer::setMaximumWidth(int px)
{
    const int clamped = qMax(200, px);
    if (m_maximumWidth == clamped) {
        return;
    }
    m_maximumWidth = clamped;
    placeOnHost();
}

void Drawer::setWidthFraction(double fraction)
{
    const double clamped = qBound(0.2, fraction, 1.0);
    if (qFuzzyCompare(m_fraction + 1.0, clamped + 1.0)) {
        return;
    }
    m_fraction = clamped;
    placeOnHost();
}

void Drawer::setClosesOnScrimClick(bool closes)
{
    m_scrimCloses = closes;
}

void Drawer::open()
{
    if (m_open) {
        return;
    }
    m_open = true;
    placeOnHost();

    m_scrim->setGeometry(m_host ? m_host->rect() : QRect());
    m_scrim->show();
    m_scrim->raise();
    show();
    raise();
    setFocus(Qt::OtherFocusReason);
    emit opened();
}

void Drawer::close()
{
    if (!m_open) {
        return;
    }
    m_open = false;
    m_scrim->hide();
    hide();
    emit closed();
}

int Drawer::headHeight() const
{
    const QFontMetrics titles(titleFont(this));
    int text = titles.height();
    if (!m_subtitle.isEmpty()) {
        text += kHeadLineGap + QFontMetrics(subtitleFont(this)).height();
    }
    return qMax(text, kMarkSide) + 2 * kHeadPaddingV;
}

void Drawer::placeOnHost()
{
    if (!m_host) {
        return;
    }
    const QRect host = m_host->rect();
    const int drawerWidth = qMin(m_maximumWidth, int(host.width() * m_fraction));
    const int left = m_edge == Edge::Trailing ? host.width() - drawerWidth : 0;
    setGeometry(left, 0, drawerWidth, host.height());
    m_scrim->setGeometry(host);
    relayout();
}

void Drawer::relayout()
{
    int top = headHeight();
    if (m_actions) {
        const int actionsHeight = m_actions->sizeHint().height();
        m_actions->setGeometry(0, top, width(), actionsHeight);
        m_actions->setVisible(actions()->count() > 0);
        if (actions()->count() > 0) {
            top += actionsHeight;
        }
    }
    m_bodyHost->setGeometry(0, top, width(), qMax(0, height() - top));
}

void Drawer::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void Drawer::keyPressEvent(QKeyEvent* event)
{
    // Escape closes. A drawer is not asking a question, so leaving it is always
    // allowed - which is the difference between this and a Dialog.
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    QWidget::keyPressEvent(event);
}

bool Drawer::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_host && event->type() == QEvent::Resize && m_open) {
        placeOnHost();
    }
    if (watched == m_host && event->type() == QEvent::ChildAdded && m_open) {
        // Anything added to the host after the drawer would paint over it.
        m_scrim->raise();
        raise();
    }
    return QWidget::eventFilter(watched, event);
}

bool Drawer::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange
        || event->type() == QEvent::LayoutRequest) {
        relayout();
        update();
    }
    return QWidget::event(event);
}

StyleSpec Drawer::styleSpec() const
{
    StyleSpec spec;
    spec.background = token(ColorRole::WindowBg);
    spec.border = token(ColorRole::Border);
    spec.foreground = token(ColorRole::TextPrimary);
    spec.radius = SurfaceRole::Overlay;
    spec.borderWidth = BorderRole::Card;
    spec.type = TypeRole::SectionTitle;
    return spec;
}

ResolvedStyle Drawer::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

void Drawer::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const ResolvedStyle style = resolvedStyle();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Square: the drawer runs the full height of the host and meets its edges,
    // so a rounded corner would leave a notch of the list showing through.
    painter.fillRect(rect(), style.background);

    painter.setPen(QPen(style.border, style.borderWidth));
    if (m_edge == Edge::Trailing) {
        painter.drawLine(0, 0, 0, height());
    } else {
        painter.drawLine(width() - 1, 0, width() - 1, height());
    }

    // ── the head ─────────────────────────────────────────────────────────
    const int head = headHeight();
    painter.setPen(QPen(profile.colors.borderSubtle, 1));
    painter.drawLine(0, head - 1, width(), head - 1);

    m_mark->move(kHeadPaddingH, (head - kMarkSide) / 2);

    const int left = kHeadPaddingH + kMarkSide + kHeadGap;
    const int available = qMax(0, width() - left - kHeadPaddingH);

    const QFont titles = titleFont(this);
    const QFontMetrics titleMetrics(titles);
    const QFont subs = subtitleFont(this);
    const QFontMetrics subMetrics(subs);

    int text = titleMetrics.height();
    if (!m_subtitle.isEmpty()) {
        text += kHeadLineGap + subMetrics.height();
    }
    int y = (head - text) / 2;

    painter.setFont(titles);
    painter.setPen(profile.colors.textPrimary);
    painter.drawText(QRect(left, y, available, titleMetrics.height()),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     titleMetrics.elidedText(m_title, Qt::ElideRight, available));

    if (m_subtitle.isEmpty()) {
        return;
    }
    y += titleMetrics.height() + kHeadLineGap;
    painter.setFont(subs);
    painter.setPen(profile.colors.textDisabled);
    painter.drawText(QRect(left, y, available, subMetrics.height()),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     subMetrics.elidedText(m_subtitle, Qt::ElideMiddle, available));

    // The action strip's own ground, so it reads as part of the head rather
    // than as the first thing in the body.
    if (m_actions && m_actions->isVisible()) {
        const QRect strip = m_actions->geometry();
        painter.fillRect(strip, profile.colors.panelBg);
        painter.setPen(QPen(profile.colors.borderSubtle, 1));
        painter.drawLine(strip.left(), strip.bottom(), strip.right(), strip.bottom());
    }
}

} // namespace novu::ui
