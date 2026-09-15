#include "novu/ui/Sidebar.h"

#include "novu/ui/Motion.h"
#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Tooltip.h"

#include <QEvent>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QRadialGradient>
#include <QResizeEvent>
#include <QScrollArea>
#include <QShowEvent>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

// `.rail-conn { margin:0 12px 8px; padding:7px 10px; border-radius:10px; gap:8px }`.
constexpr int kConnPaddingH = 10;
constexpr int kConnPaddingV = 7;
constexpr int kConnGap = 8;
constexpr int kConnDot = 7;
constexpr double kConnGlowScale = 2.6;

// `.rail-conn .lockb { 20px }` with a 13px glyph.
constexpr int kConnActionSide = 20;
constexpr int kConnActionGlyph = 13;

// `.rail-head { padding:13px 14px 10px; gap:10px }`.
constexpr int kHeadPaddingH = 14;
constexpr int kHeadPaddingTop = 13;
constexpr int kHeadPaddingBottom = 10;
constexpr int kHeadGap = 10;
// `.rail-head .sub { margin-top:3px }`.
constexpr int kHeadLineGap = 3;

// `.rail-conn` sits inside a 12px gutter, and the nav inside a 10px one.
constexpr int kConnMargin = 12;
constexpr int kNavMargin = 10;
constexpr int kNavBottom = 12;
constexpr int kActionsGap = 7;

// The collapsed rail is the mark plus its gutters, which is what keeps the
// glyph column on the same axis at both widths.
constexpr int kCollapsedPaddingH = 10;

SemanticRole roleFor(ConnectionStrip::State state)
{
    switch (state) {
    case ConnectionStrip::State::Degraded: return SemanticRole::Warning;
    case ConnectionStrip::State::Offline:  return SemanticRole::Danger;
    case ConnectionStrip::State::Online:   break;
    }
    return SemanticRole::Success;
}

QFont hostFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Mono);
    font.setPixelSize(profile.type.xs);
    return font;
}

QFont detailFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Mono);
    font.setPixelSize(qMax(8, profile.type.xxs));
    return font;
}

QFont nameFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::CardTitle, true);
    font.setPixelSize(profile.type.lg - 2);
    return font;
}

QFont subtitleFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Eyebrow);
    font.setPixelSize(qMax(8, profile.type.xxs - 1));
    font.setLetterSpacing(QFont::PercentageSpacing, 105);
    return font;
}

} // namespace

// == ConnectionStrip ========================================================

ConnectionStrip::ConnectionStrip(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMouseTracking(true);
}

void ConnectionStrip::setHost(const QString& host)
{
    if (m_host == host) {
        return;
    }
    m_host = host;
    updateGeometry();
    update();
}

void ConnectionStrip::setDetail(const QString& detail)
{
    if (m_detail == detail) {
        return;
    }
    m_detail = detail;
    updateGeometry();
    update();
}

void ConnectionStrip::setState(State state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    // Offline stops the breathing. The design's `.rail-conn.off .dot
    // { animation:none }` - a light that pulses while nothing is connected is
    // the one that stops being read.
    updateMotion();
    update();
}

void ConnectionStrip::setAction(Glyph glyph, const QString& tooltip)
{
    m_action = glyph;
    m_actionTip = tooltip;
    updateGeometry();
    update();
}

void ConnectionStrip::clearAction()
{
    m_action = Glyph::None;
    m_actionTip.clear();
    updateGeometry();
    update();
}

void ConnectionStrip::setCompact(bool compact)
{
    if (m_compact == compact) {
        return;
    }
    m_compact = compact;
    updateGeometry();
    update();
}

QRect ConnectionStrip::actionRect() const
{
    if (m_action == Glyph::None || m_compact) {
        return QRect();
    }
    return QRect(width() - kConnPaddingH - kConnActionSide,
                 (height() - kConnActionSide) / 2, kConnActionSide, kConnActionSide);
}

void ConnectionStrip::updateMotion()
{
    const bool wanted = m_state == State::Online && isVisible() && motion::enabled(this);
    if (wanted == m_subscribed) {
        return;
    }
    m_subscribed = wanted;
    if (wanted) {
        Pulse::instance().subscribe(this);
    } else {
        Pulse::instance().release(this);
    }
}

void ConnectionStrip::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    updateMotion();
}

void ConnectionStrip::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    updateMotion();
}

bool ConnectionStrip::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateMotion();
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void ConnectionStrip::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && actionRect().contains(event->pos())) {
        m_actionPressed = true;
        update();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ConnectionStrip::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_actionPressed) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    m_actionPressed = false;
    if (actionRect().contains(event->pos())) {
        emit actionTriggered();
    }
    update();
}

void ConnectionStrip::mouseMoveEvent(QMouseEvent* event)
{
    const bool inAction = actionRect().contains(event->pos());
    if (inAction != m_actionHovered) {
        m_actionHovered = inAction;
        setCursor(inAction ? Qt::PointingHandCursor : Qt::ArrowCursor);
        if (!m_actionTip.isEmpty()) {
            if (inAction) {
                Tooltip::install(this, m_actionTip);
            } else {
                Tooltip::remove(this);
            }
        }
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void ConnectionStrip::leaveEvent(QEvent* event)
{
    if (m_actionHovered) {
        m_actionHovered = false;
        setCursor(Qt::ArrowCursor);
        update();
    }
    QWidget::leaveEvent(event);
}

StyleSpec ConnectionStrip::styleSpec() const
{
    StyleSpec spec;
    // `background:var(--bg)` — the *window* colour, which is darker than the
    // rail it sits on. That inversion is what makes the strip read as a well.
    spec.background = token(ColorRole::WindowBg);
    spec.border = token(ColorRole::BorderSubtle);
    spec.foreground = token(ColorRole::TextSecondary);
    spec.radius = SurfaceRole::Card;
    spec.borderWidth = BorderRole::Card;
    spec.type = TypeRole::Mono;
    return spec;
}

ResolvedStyle ConnectionStrip::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

QSize ConnectionStrip::sizeHint() const
{
    const QFontMetrics hosts(hostFont(this));
    const int height = qMax(hosts.height(), kConnActionSide) + 2 * kConnPaddingV;
    if (m_compact) {
        return QSize(kConnDot + 2 * kConnPaddingH, height);
    }
    int width = 2 * kConnPaddingH + kConnDot + kConnGap + hosts.horizontalAdvance(m_host);
    if (!m_detail.isEmpty()) {
        width += kConnGap + QFontMetrics(detailFont(this)).horizontalAdvance(m_detail);
    }
    if (m_action != Glyph::None) {
        width += kConnGap + kConnActionSide;
    }
    return QSize(width, height);
}

QSize ConnectionStrip::minimumSizeHint() const
{
    return QSize(m_compact ? sizeHint().width() : 90, sizeHint().height());
}

void ConnectionStrip::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const ResolvedStyle style = resolvedStyle();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF well = QRectF(rect()).adjusted(style.borderWidth / 2.0, style.borderWidth / 2.0,
                                                -style.borderWidth / 2.0,
                                                -style.borderWidth / 2.0);
    painter.setPen(QPen(style.border, style.borderWidth));
    painter.setBrush(style.background);
    painter.drawRoundedRect(well, style.radius, style.radius);

    const QColor hue = profile.semantic.role(roleFor(m_state)).text;

    // The design's `softglow` on the dot, on the shared clock. Only when
    // online: see setState().
    double glowAlpha = 0.55;
    if (m_state == State::Online && motion::enabled(this)) {
        glowAlpha = motion::breathe(profile.animation.pulseMs, 0.35, 0.75);
    }

    const double dotLeft = m_compact ? (width() - kConnDot) / 2.0 : kConnPaddingH;
    const QRectF dot(dotLeft, (height() - kConnDot) / 2.0, kConnDot, kConnDot);
    const QPointF centre = dot.center();
    const double glow = kConnDot * kConnGlowScale / 2.0;

    QRadialGradient halo(centre, glow);
    QColor inner = hue;
    inner.setAlphaF(glowAlpha);
    QColor outer = hue;
    outer.setAlphaF(0.0);
    halo.setColorAt(0.0, inner);
    halo.setColorAt(0.42, inner);
    halo.setColorAt(1.0, outer);
    painter.setPen(Qt::NoPen);
    painter.setBrush(halo);
    painter.drawEllipse(centre, glow, glow);

    painter.setBrush(hue);
    painter.drawEllipse(dot);

    if (m_compact) {
        return;
    }

    int rightEdge = width() - kConnPaddingH;

    if (m_action != Glyph::None) {
        const QRect box = actionRect();
        if (m_actionHovered || m_actionPressed) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(m_actionPressed ? profile.colors.surfaceActive
                                             : profile.colors.surfaceHover);
            painter.drawRoundedRect(QRectF(box), profile.radii.small + 1,
                                    profile.radii.small + 1);
        }
        const double inset = (kConnActionSide - kConnActionGlyph) / 2.0;
        paintGlyph(&painter, m_action, QRectF(box).adjusted(inset, inset, -inset, -inset),
                   m_actionHovered ? profile.colors.textPrimary : profile.colors.textDisabled);
        rightEdge = box.left() - kConnGap;
    }

    if (!m_detail.isEmpty()) {
        const QFont font = detailFont(this);
        const QFontMetrics fm(font);
        const int detailWidth = fm.horizontalAdvance(m_detail);
        painter.setFont(font);
        painter.setPen(profile.colors.textDisabled);
        painter.drawText(QRect(rightEdge - detailWidth, 0, detailWidth, height()),
                         Qt::AlignVCenter | Qt::AlignRight, m_detail);
        rightEdge -= detailWidth + kConnGap;
    }

    const QFont font = hostFont(this);
    const QFontMetrics fm(font);
    const int left = static_cast<int>(dot.right()) + kConnGap;
    painter.setFont(font);
    painter.setPen(style.foreground);
    const QRect textBox(left, 0, qMax(0, rightEdge - left), height());
    painter.drawText(textBox, Qt::AlignVCenter | Qt::AlignLeft,
                     fm.elidedText(m_host, Qt::ElideMiddle, textBox.width()));
}

// == Sidebar ================================================================

Sidebar::Sidebar(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    m_mark = new AppMark(Glyph::None, this, AppMark::Size::Medium);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // The rail is painted; a scroll area with its own opaque viewport would
    // draw a flat panel over the gradient.
    m_scroll->setAttribute(Qt::WA_TranslucentBackground);
    m_scroll->viewport()->setAutoFillBackground(false);

    m_navContent = new QWidget(m_scroll);
    m_navContent->setAutoFillBackground(false);
    auto* navLayout = new QVBoxLayout(m_navContent);
    navLayout->setContentsMargins(kNavMargin, 0, kNavMargin, kNavBottom);
    navLayout->setSpacing(2);
    navLayout->addStretch(1);
    m_scroll->setWidget(m_navContent);

    m_actions = new QWidget(this);
    m_actions->setAutoFillBackground(false);
    auto* actionsLayout = new QVBoxLayout(m_actions);
    actionsLayout->setContentsMargins(kNavMargin + 2, 0, kNavMargin + 2, kNavBottom);
    actionsLayout->setSpacing(kActionsGap);

    setFixedWidth(m_expandedWidth);
}

void Sidebar::setIdentity(Glyph glyph, const QString& name, const QString& subtitle)
{
    m_mark->setGlyph(glyph);
    setName(name);
    setSubtitle(subtitle);
}

void Sidebar::setName(const QString& name)
{
    if (m_name == name) {
        return;
    }
    m_name = name;
    update();
}

void Sidebar::setSubtitle(const QString& subtitle)
{
    if (m_subtitle == subtitle) {
        return;
    }
    m_subtitle = subtitle;
    updateGeometry();
    relayout();
    update();
}

ConnectionStrip* Sidebar::connection()
{
    if (!m_connection) {
        m_connection = new ConnectionStrip(this);
        m_connection->setCompact(m_collapsed);
        relayout();
    }
    return m_connection;
}

QVBoxLayout* Sidebar::navigation() const
{
    return qobject_cast<QVBoxLayout*>(m_navContent->layout());
}

NavList* Sidebar::nav()
{
    if (!m_nav) {
        m_nav = new NavList(m_navContent);
        m_nav->setCollapsed(m_collapsed);
        // Ahead of the trailing stretch, so a caller adding a Disclosure block
        // afterwards lands under the list rather than under the spring.
        navigation()->insertWidget(navigation()->count() - 1, m_nav);
    }
    return m_nav;
}

QVBoxLayout* Sidebar::actions() const
{
    return qobject_cast<QVBoxLayout*>(m_actions->layout());
}

void Sidebar::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed) {
        return;
    }
    m_collapsed = collapsed;
    applyCollapsed();
    emit collapsedChanged(collapsed);
}

void Sidebar::toggleCollapsed()
{
    setCollapsed(!m_collapsed);
}

void Sidebar::applyCollapsed()
{
    setFixedWidth(m_collapsed ? collapsedWidth() : m_expandedWidth);
    if (m_connection) {
        m_connection->setCompact(m_collapsed);
    }
    // Every NavList in the navigation region, not only the convenience one: a
    // rail with three lists inside Disclosure blocks must collapse whole.
    for (NavList* list : m_navContent->findChildren<NavList*>()) {
        list->setCollapsed(m_collapsed);
    }
    m_actions->setVisible(!m_collapsed || actions()->count() > 0);
    updateGeometry();
    relayout();
    update();
}

void Sidebar::setExpandedWidth(int px)
{
    const int clamped = qMax(120, px);
    if (m_expandedWidth == clamped) {
        return;
    }
    m_expandedWidth = clamped;
    if (!m_collapsed) {
        setFixedWidth(clamped);
    }
    updateGeometry();
}

int Sidebar::collapsedWidth() const
{
    return m_mark->side() + 2 * kCollapsedPaddingH;
}

void Sidebar::setHeadVisible(bool visible)
{
    if (m_headVisible == visible) {
        return;
    }
    m_headVisible = visible;
    m_mark->setVisible(visible);
    relayout();
    updateGeometry();
    update();
}

int Sidebar::headHeight() const
{
    if (!m_headVisible) {
        return 0;
    }
    const QFontMetrics names(nameFont(this));
    int text = names.height();
    if (!m_subtitle.isEmpty()) {
        text += kHeadLineGap + QFontMetrics(subtitleFont(this)).height();
    }
    return qMax(text, m_mark->side()) + kHeadPaddingTop + kHeadPaddingBottom;
}

void Sidebar::relayout()
{
    const int head = headHeight();

    if (!m_headVisible) {
        // nothing to place
    } else if (m_collapsed) {
        m_mark->move((width() - m_mark->side()) / 2, kHeadPaddingTop);
    } else {
        m_mark->move(kHeadPaddingH, kHeadPaddingTop + (head - kHeadPaddingTop
                                                       - kHeadPaddingBottom
                                                       - m_mark->side()) / 2);
    }

    int top = head;
    if (m_connection) {
        const int margin = m_collapsed ? kCollapsedPaddingH : kConnMargin;
        const int connHeight = m_connection->sizeHint().height();
        m_connection->setGeometry(margin, top, qMax(0, width() - 2 * margin), connHeight);
        top += connHeight + 8;
    }

    int bottom = height();
    const bool hasActions = actions()->count() > 0;
    if (hasActions) {
        const int actionsHeight = m_actions->sizeHint().height();
        m_actions->setGeometry(0, qMax(top, height() - actionsHeight), width(), actionsHeight);
        bottom = m_actions->geometry().top();
    }
    m_actions->setVisible(hasActions);

    m_scroll->setGeometry(0, top, width(), qMax(0, bottom - top));
}

StyleSpec Sidebar::styleSpec() const
{
    StyleSpec spec;
    spec.background = token(ColorRole::PanelBg);
    spec.border = token(ColorRole::Border);
    spec.foreground = token(ColorRole::TextSecondary);
    spec.radius = SurfaceRole::Toolbar;
    spec.borderWidth = BorderRole::Divider;
    spec.type = TypeRole::Body;
    return spec;
}

ResolvedStyle Sidebar::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

QSize Sidebar::sizeHint() const
{
    return QSize(m_collapsed ? collapsedWidth() : m_expandedWidth, 480);
}

QSize Sidebar::minimumSizeHint() const
{
    return QSize(m_collapsed ? collapsedWidth() : m_expandedWidth, headHeight());
}

void Sidebar::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    relayout();
}

bool Sidebar::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        relayout();
        update();
    }
    if (event->type() == QEvent::LayoutRequest) {
        relayout();
    }
    return QWidget::event(event);
}

void Sidebar::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // `linear-gradient(180deg, --rail-top, --rail-bot)`. The design's rail is
    // one step lighter at the top than at the bottom; flat, it reads as a
    // second window rather than as the edge of this one.
    QLinearGradient ground(0, 0, 0, height());
    ground.setColorAt(0.0, profile.colors.panelBg);
    ground.setColorAt(1.0, profile.colors.windowBg);
    painter.fillRect(rect(), ground);

    const int borderWidth = qMax(1, profile.border(BorderRole::Divider));
    painter.setPen(QPen(profile.colors.border, borderWidth));
    painter.drawLine(width() - borderWidth, 0, width() - borderWidth, height());

    if (m_collapsed || !m_headVisible) {
        return;   // the design hides `.rail-head .t` entirely
    }

    const int left = kHeadPaddingH + m_mark->side() + kHeadGap;
    const int available = qMax(0, width() - left - kHeadPaddingH);
    if (available <= 0) {
        return;
    }

    const QFont names = nameFont(this);
    const QFontMetrics nameMetrics(names);
    const QFont subs = subtitleFont(this);
    const QFontMetrics subMetrics(subs);

    int textHeight = nameMetrics.height();
    if (!m_subtitle.isEmpty()) {
        textHeight += kHeadLineGap + subMetrics.height();
    }
    int y = kHeadPaddingTop
        + (headHeight() - kHeadPaddingTop - kHeadPaddingBottom - textHeight) / 2;

    painter.setFont(names);
    painter.setPen(profile.colors.textPrimary);
    painter.drawText(QRect(left, y, available, nameMetrics.height()),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     nameMetrics.elidedText(m_name, Qt::ElideRight, available));

    if (m_subtitle.isEmpty()) {
        return;
    }
    y += nameMetrics.height() + kHeadLineGap;
    painter.setFont(subs);
    painter.setPen(profile.colors.textDisabled);
    painter.drawText(QRect(left, y, available, subMetrics.height()),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     subMetrics.elidedText(m_subtitle, Qt::ElideRight, available));
}

} // namespace novu::ui
