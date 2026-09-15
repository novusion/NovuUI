#include "novu/ui/TabBar.h"

#include "novu/ui/Motion.h"
#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Tooltip.h"

#include <QEvent>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QShowEvent>

#include <utility>

namespace novu::ui {

namespace {

// `.btab { height:32px; padding:0 10px; gap:8px }`.
constexpr int kHeight = 32;
constexpr int kPaddingH = 10;
constexpr int kGap = 8;
constexpr int kRadius = 8;

// The mark at the leading edge, and the close box at the trailing one.
constexpr int kMark = 14;
constexpr int kCloseSide = 16;
constexpr int kCloseGlyph = 9;

// What a tab may shrink to before the strip starts scrolling instead. Below
// this the label is gone and the tab is only its mark and its close box, which
// is the point at which shrinking stops helping.
constexpr int kMaxWidth = 220;
constexpr int kMinWidth = 92;
constexpr int kTabGap = 2;

// The trailing "+".
constexpr int kNewTabWidth = 28;

// `.btab.on::after` — the accent underline on the active tab, and the per-tab
// colour when one is set.
constexpr double kUnderlineHeight = 2.0;

// `.btab.sleeping` — a tab whose content has been discarded.
constexpr double kAsleepOpacity = 0.5;

// How far a press has to travel before it becomes a reorder rather than a
// click. Without it every click on a tab is a one-pixel drag.
constexpr int kDragThreshold = 6;

QFont tabFont(const QWidget* context, bool current)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Body);
    font.setPixelSize(profile.type.sm);
    font.setWeight(current ? QFont::Medium : QFont::Normal);
    return font;
}

} // namespace

TabBar::TabBar(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(kHeight);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
}

// == shape ==================================================================

int TabBar::addTab(const QString& key, const QString& text, Glyph glyph)
{
    return insertTab(m_tabs.size(), key, text, glyph);
}

int TabBar::insertTab(int index, const QString& key, const QString& text, Glyph glyph)
{
    if (key.isEmpty() || indexOfKey(key) >= 0) {
        // A duplicate key would give two tabs one identity, and every signal
        // about either would name the same document.
        return -1;
    }
    Tab tab;
    tab.key = key;
    tab.label = text;
    tab.glyph = glyph;

    const int at = qBound(0, index, m_tabs.size());
    m_tabs.insert(at, tab);
    if (m_current < 0) {
        m_current = at;
    } else if (at <= m_current) {
        ++m_current;
    }
    updateGeometry();
    update();
    return at;
}

void TabBar::removeTab(const QString& key)
{
    const int index = indexOfKey(key);
    if (index < 0) {
        return;
    }
    m_tabs.remove(index);

    if (m_tabs.isEmpty()) {
        m_current = -1;
    } else if (index < m_current) {
        --m_current;
    } else if (index == m_current) {
        // Closing the current tab moves to the one that took its place, or to
        // the last if it was the last - which is what every editor does.
        m_current = qMin(index, m_tabs.size() - 1);
        emit currentChanged(currentKey());
    }

    m_hovered = -1;
    m_pressed = -1;
    m_dragIndex = -1;
    updateMotion();
    updateGeometry();
    update();
}

void TabBar::clear()
{
    m_tabs.clear();
    m_current = -1;
    m_hovered = -1;
    m_pressed = -1;
    m_dragIndex = -1;
    updateMotion();
    updateGeometry();
    update();
}

int TabBar::indexOfKey(const QString& key) const
{
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs.at(i).key == key) {
            return i;
        }
    }
    return -1;
}

QString TabBar::keyAt(int index) const
{
    return index >= 0 && index < m_tabs.size() ? m_tabs.at(index).key : QString();
}

QStringList TabBar::keys() const
{
    QStringList result;
    result.reserve(m_tabs.size());
    for (const Tab& tab : m_tabs) {
        result.append(tab.key);
    }
    return result;
}

QString TabBar::currentKey() const
{
    return keyAt(m_current);
}

const TabBar::Tab* TabBar::find(const QString& key) const
{
    const int index = indexOfKey(key);
    return index < 0 ? nullptr : &m_tabs.at(index);
}

TabBar::Tab* TabBar::find(const QString& key)
{
    return const_cast<Tab*>(std::as_const(*this).find(key));
}

void TabBar::setCurrentIndex(int index)
{
    if (index < -1 || index >= m_tabs.size() || index == m_current) {
        return;
    }
    m_current = index;
    update();
    emit currentChanged(currentKey());
}

void TabBar::setCurrentKey(const QString& key)
{
    setCurrentIndex(indexOfKey(key));
}

// == content ================================================================

void TabBar::setLabel(const QString& key, const QString& text)
{
    Tab* tab = find(key);
    if (!tab || tab->label == text) {
        return;
    }
    tab->label = text;
    updateGeometry();
    update();
}

QString TabBar::label(const QString& key) const
{
    const Tab* tab = find(key);
    return tab ? tab->label : QString();
}

void TabBar::setGlyph(const QString& key, Glyph glyph)
{
    Tab* tab = find(key);
    if (!tab || tab->glyph == glyph) {
        return;
    }
    tab->glyph = glyph;
    update();
}

void TabBar::setState(const QString& key, State state)
{
    Tab* tab = find(key);
    if (!tab || tab->state == state) {
        return;
    }
    tab->state = state;
    // The clock is held open only while something is actually turning.
    updateMotion();
    update();
}

TabBar::State TabBar::state(const QString& key) const
{
    const Tab* tab = find(key);
    return tab ? tab->state : State::Idle;
}

void TabBar::setColor(const QString& key, const QColor& colour)
{
    Tab* tab = find(key);
    if (!tab || tab->color == colour) {
        return;
    }
    // Kept on the tab rather than in a parallel list beside it. Every
    // implementation that kept colours alongside got them out of step the first
    // time a tab was dragged.
    tab->color = colour;
    update();
}

QColor TabBar::color(const QString& key) const
{
    const Tab* tab = find(key);
    return tab ? tab->color : QColor();
}

void TabBar::setClosable(const QString& key, bool closable)
{
    Tab* tab = find(key);
    if (!tab || tab->closable == closable) {
        return;
    }
    tab->closable = closable;
    updateGeometry();
    update();
}

void TabBar::setReorderable(bool reorderable)
{
    if (m_reorderable == reorderable) {
        return;
    }
    m_reorderable = reorderable;
    m_dragIndex = -1;
}

void TabBar::setNewTabButton(bool enabled)
{
    if (m_newTab == enabled) {
        return;
    }
    m_newTab = enabled;
    updateGeometry();
    update();
}

// == geometry ===============================================================

int TabBar::tabWidth(const Tab& tab) const
{
    const QFontMetrics fm(tabFont(this, false));
    int width = 2 * kPaddingH + fm.horizontalAdvance(tab.label);
    if (tab.glyph != Glyph::None || tab.state != State::Idle) {
        width += kMark + kGap;
    }
    if (tab.closable) {
        width += kGap + kCloseSide;
    }
    return qBound(kMinWidth, width, kMaxWidth);
}

int TabBar::laidOutWidth() const
{
    if (m_tabs.isEmpty()) {
        return 0;
    }
    int widest = 0;
    for (const Tab& tab : m_tabs) {
        widest = qMax(widest, tabWidth(tab));
    }

    const int available = width() - (m_newTab ? kNewTabWidth + kTabGap : 0)
        - kTabGap * (m_tabs.size() - 1);
    const int even = available / m_tabs.size();

    // Every tab is the same width, and they shrink together - which is what a
    // browser does. Tabs that each sized to their own title make the strip a
    // ragged row whose close boxes are never in the same place twice.
    return qBound(kMinWidth, qMin(widest, even), kMaxWidth);
}

QRect TabBar::tabRect(int index) const
{
    if (index < 0 || index >= m_tabs.size()) {
        return QRect();
    }
    const int each = laidOutWidth();
    return QRect(index * (each + kTabGap), 0, each, height());
}

QRect TabBar::closeRect(int index) const
{
    if (index < 0 || index >= m_tabs.size() || !m_tabs.at(index).closable) {
        return QRect();
    }
    const QRect box = tabRect(index);
    if (box.width() < kMinWidth) {
        return QRect();
    }
    return QRect(box.right() - kPaddingH - kCloseSide + 1,
                 box.top() + (box.height() - kCloseSide) / 2, kCloseSide, kCloseSide);
}

QRect TabBar::newTabRect() const
{
    if (!m_newTab) {
        return QRect();
    }
    const int each = laidOutWidth();
    const int left = m_tabs.isEmpty() ? 0 : m_tabs.size() * (each + kTabGap);
    return QRect(left, (height() - kNewTabWidth) / 2 + 2, kNewTabWidth, kNewTabWidth - 4);
}

int TabBar::indexAt(const QPoint& position) const
{
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (tabRect(i).contains(position)) {
            return i;
        }
    }
    return -1;
}

QSize TabBar::sizeHint() const
{
    int width = 0;
    for (const Tab& tab : m_tabs) {
        width += tabWidth(tab) + kTabGap;
    }
    if (m_newTab) {
        width += kNewTabWidth;
    }
    return QSize(qMax(width, kMinWidth), kHeight);
}

QSize TabBar::minimumSizeHint() const
{
    return QSize(kMinWidth + (m_newTab ? kNewTabWidth : 0), kHeight);
}

// == motion =================================================================

void TabBar::updateMotion()
{
    bool anyLoading = false;
    for (const Tab& tab : m_tabs) {
        if (tab.state == State::Loading) {
            anyLoading = true;
            break;
        }
    }
    const bool wanted = anyLoading && isVisible() && motion::enabled(this);
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

void TabBar::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    updateMotion();
}

void TabBar::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    updateMotion();
}

bool TabBar::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateMotion();
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

// == interaction ============================================================

void TabBar::mousePressEvent(QMouseEvent* event)
{
    const QPoint position = event->position().toPoint();

    // Middle-click closes, everywhere it exists. It is the one gesture users
    // bring with them from every browser and every editor.
    if (event->button() == Qt::MiddleButton) {
        const int index = indexAt(position);
        if (index >= 0 && m_tabs.at(index).closable) {
            emit closeRequested(m_tabs.at(index).key);
        }
        return;
    }
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (newTabRect().contains(position)) {
        m_pressedNew = true;
        update();
        return;
    }

    const int index = indexAt(position);
    m_pressed = index;
    m_pressedClose = index >= 0 && closeRect(index).contains(position);
    m_dragOrigin = position;
    m_dragIndex = -1;
    update();
}

void TabBar::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint position = event->position().toPoint();

    if (m_pressed >= 0 && !m_pressedClose && m_reorderable) {
        // A press only becomes a reorder once it has travelled. Without the
        // threshold every click on a tab is a one-pixel drag.
        if (m_dragIndex < 0
            && (position - m_dragOrigin).manhattanLength() > kDragThreshold) {
            m_dragIndex = m_pressed;
        }
        if (m_dragIndex >= 0) {
            const int over = indexAt(position);
            if (over >= 0 && over != m_dragIndex) {
                const QString key = m_tabs.at(m_dragIndex).key;
                const int from = m_dragIndex;
                m_tabs.move(m_dragIndex, over);
                // The colour and the state travel with the tab, because they
                // live on it.
                if (m_current == from) {
                    m_current = over;
                } else if (from < m_current && over >= m_current) {
                    --m_current;
                } else if (from > m_current && over <= m_current) {
                    ++m_current;
                }
                m_dragIndex = over;
                m_pressed = over;
                update();
                emit tabMoved(key, from, over);
            }
            return;
        }
    }

    const int index = indexAt(position);
    const bool onClose = index >= 0 && closeRect(index).contains(position);
    const bool onNew = newTabRect().contains(position);
    if (index != m_hovered || onClose != m_hoveredClose || onNew != m_hoveredNew) {
        // A shrunken tab hides its label, so the tip is the only way to read
        // it. Installed only when the label is actually elided.
        if (index != m_hovered) {
            if (index >= 0 && tabRect(index).width() <= kMinWidth + 8) {
                Tooltip::install(this, m_tabs.at(index).label);
            } else {
                Tooltip::remove(this);
            }
        }
        m_hovered = index;
        m_hoveredClose = onClose;
        m_hoveredNew = onNew;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void TabBar::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const QPoint position = event->position().toPoint();

    if (m_pressedNew) {
        m_pressedNew = false;
        if (newTabRect().contains(position)) {
            emit newTabRequested();
        }
        update();
        return;
    }

    const int pressed = m_pressed;
    const bool pressedClose = m_pressedClose;
    const bool dragged = m_dragIndex >= 0;
    m_pressed = -1;
    m_pressedClose = false;
    m_dragIndex = -1;

    if (pressed < 0 || dragged) {
        update();
        return;   // a reorder is not also a selection
    }

    const int released = indexAt(position);
    const bool releasedClose = released >= 0 && closeRect(released).contains(position);
    if (released != pressed || releasedClose != pressedClose) {
        update();
        return;   // cancelled
    }

    if (pressedClose) {
        // The bar does not remove the tab: a document with unsaved changes has
        // to be able to ask first, and a bar that had already closed it would
        // be asking about something gone.
        emit closeRequested(m_tabs.at(pressed).key);
    } else {
        setCurrentIndex(pressed);
    }
    update();
}

void TabBar::leaveEvent(QEvent* event)
{
    if (m_hovered != -1 || m_hoveredClose || m_hoveredNew) {
        m_hovered = -1;
        m_hoveredClose = false;
        m_hoveredNew = false;
        Tooltip::remove(this);
        update();
    }
    QWidget::leaveEvent(event);
}

void TabBar::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Right:
        if (m_current + 1 < m_tabs.size()) {
            setCurrentIndex(m_current + 1);
        }
        return;
    case Qt::Key_Left:
        if (m_current > 0) {
            setCurrentIndex(m_current - 1);
        }
        return;
    case Qt::Key_W:
        if (event->modifiers().testFlag(Qt::ControlModifier) && m_current >= 0
            && m_tabs.at(m_current).closable) {
            emit closeRequested(m_tabs.at(m_current).key);
            return;
        }
        break;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

// == appearance =============================================================

StyleSpec TabBar::styleSpec() const
{
    StyleSpec spec;
    spec.background = token(ColorRole::HeaderBg);
    spec.border = token(ColorRole::Border);
    spec.foreground = token(ColorRole::TextDisabled);
    spec.radius = SurfaceRole::Toolbar;
    spec.borderWidth = BorderRole::Divider;
    spec.type = TypeRole::Body;
    return spec;
}

StyleSpec TabBar::tabStyleSpec(int index) const
{
    StyleSpec spec = styleSpec();
    spec.radius = SurfaceRole::Button;
    spec.background = token(ColorRole::Transparent);
    spec.border = token(ColorRole::Transparent);
    spec.hoverBackground = token(ColorRole::SurfaceHover, 0.55);
    spec.hoverForeground = token(ColorRole::TextSecondary);
    spec.checkedBackground = token(ColorRole::PanelBg);
    spec.checkedForeground = token(ColorRole::TextPrimary);

    if (index >= 0 && index < m_tabs.size()) {
        switch (m_tabs.at(index).state) {
        case State::Failed:
            spec.foreground = statusToken(SemanticRole::Danger, ColorRole::StatusText);
            break;
        case State::Asleep:
            spec.foreground = token(ColorRole::TextDisabled, kAsleepOpacity);
            spec.checkedForeground = token(ColorRole::TextSecondary, kAsleepOpacity);
            break;
        case State::Idle:
        case State::Loading:
        case State::Modified:
            break;
        }
    }
    return spec;
}

ResolvedStyle TabBar::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

void TabBar::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(rect(), profile.window.titleBarBg);
    painter.setPen(QPen(profile.colors.border, qMax(1, profile.border(BorderRole::Divider))));
    painter.drawLine(0, height() - 1, width(), height() - 1);

    for (int index = 0; index < m_tabs.size(); ++index) {
        const Tab& tab = m_tabs.at(index);
        const QRect box = tabRect(index);
        if (!box.intersects(rect())) {
            continue;
        }
        const ResolvedStyle style = resolve(tabStyleSpec(index), this, box.height());
        const bool current = index == m_current;
        const bool hot = index == m_hovered && !current;

        // The plate. Only the top corners are round: the tab meets the page
        // below it, and a fully rounded tab reads as a chip that happens to be
        // near the edge.
        if (current || hot) {
            QPainterPath plate;
            plate.moveTo(box.left(), box.bottom() + 1);
            plate.lineTo(box.left(), box.top() + kRadius);
            plate.quadTo(box.left(), box.top(), box.left() + kRadius, box.top());
            plate.lineTo(box.right() - kRadius, box.top());
            plate.quadTo(box.right(), box.top(), box.right(), box.top() + kRadius);
            plate.lineTo(box.right(), box.bottom() + 1);
            plate.closeSubpath();
            painter.setPen(Qt::NoPen);
            painter.setBrush(current ? style.checkedBackground : style.hoverBackground);
            painter.drawPath(plate);
        }

        if (current) {
            // The underline: the tab's own colour when it has one, the accent
            // otherwise. NovuExplorer's contribution, and the reason the colour
            // lives on the tab.
            const QColor line = tab.color.isValid() ? tab.color : profile.colors.accent;
            painter.setPen(Qt::NoPen);
            painter.setBrush(line);
            painter.drawRect(QRectF(box.left(), box.bottom() - kUnderlineHeight + 1,
                                    box.width(), kUnderlineHeight));
        } else if (tab.color.isValid()) {
            // Inactive tabs keep a faint trace of their colour, or a coloured
            // set is only ever one tab's worth of information.
            painter.setPen(Qt::NoPen);
            painter.setBrush(withAlpha(tab.color, 0.45));
            painter.drawRect(QRectF(box.left(), box.bottom() - 1.0, box.width(), 1.0));
        }

        QColor ink = current ? style.checkedForeground
                             : (hot ? style.hoverForeground : style.foreground);

        int left = box.left() + kPaddingH;
        const QRect close = closeRect(index);
        int rightEdge = close.isNull() ? box.right() - kPaddingH : close.left() - kGap;

        // ── the mark ─────────────────────────────────────────────────────
        const QRectF markBox(left, box.center().y() - kMark / 2.0, kMark, kMark);
        switch (tab.state) {
        case State::Loading:
            motion::paintSpinner(&painter, markBox.adjusted(1, 1, -1, -1),
                                 profile.colors.border, profile.colors.accentLight, 1.6,
                                 profile.animation.spinMs, motion::enabled(this));
            left += kMark + kGap;
            break;
        case State::Failed:
            painter.setPen(Qt::NoPen);
            painter.setBrush(profile.semantic.danger.text);
            painter.drawEllipse(markBox.adjusted(4, 4, -4, -4));
            left += kMark + kGap;
            break;
        case State::Modified:
            painter.setPen(Qt::NoPen);
            painter.setBrush(profile.colors.accentLight);
            painter.drawEllipse(markBox.adjusted(4, 4, -4, -4));
            left += kMark + kGap;
            break;
        case State::Idle:
        case State::Asleep:
            if (tab.glyph != Glyph::None) {
                paintGlyph(&painter, tab.glyph, markBox,
                           tab.state == State::Asleep ? withAlpha(ink, kAsleepOpacity) : ink);
                left += kMark + kGap;
            }
            break;
        }

        // ── the label ────────────────────────────────────────────────────
        const QFont font = tabFont(this, current);
        const QFontMetrics fm(font);
        painter.setFont(font);
        painter.setPen(ink);
        const QRect textBox(left, box.top(), qMax(0, rightEdge - left), box.height());
        if (textBox.width() > 8) {
            painter.drawText(textBox, Qt::AlignVCenter | Qt::AlignLeft,
                             fm.elidedText(tab.label, Qt::ElideRight, textBox.width()));
        }

        // ── the close box ────────────────────────────────────────────────
        //
        // Drawn on the current tab and on the hovered one, and nowhere else. A
        // strip of thirty close boxes is thirty invitations to lose work.
        if (!close.isNull() && (current || index == m_hovered)) {
            const bool hotClose = m_hoveredClose && index == m_hovered;
            if (hotClose) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(m_pressedClose ? profile.window.closePressedBg
                                                : profile.window.closeHoverBg);
                painter.drawRoundedRect(QRectF(close), profile.radii.small,
                                        profile.radii.small);
            }
            const double inset = (kCloseSide - kCloseGlyph) / 2.0;
            paintGlyph(&painter, Glyph::Close,
                       QRectF(close).adjusted(inset, inset, -inset, -inset),
                       hotClose ? profile.window.closeHoverGlyph
                                : profile.colors.textDisabled);
        }
    }

    // ── the new-tab button ───────────────────────────────────────────────
    const QRect plus = newTabRect();
    if (!plus.isNull()) {
        if (m_hoveredNew) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(m_pressedNew ? profile.colors.surfaceActive
                                          : profile.colors.surfaceHover);
            painter.drawRoundedRect(QRectF(plus), profile.radii.small, profile.radii.small);
        }
        paintGlyph(&painter, Glyph::Plus, QRectF(plus).adjusted(7, 7, -7, -7),
                   m_hoveredNew ? profile.colors.textPrimary : profile.colors.textDisabled);
    }

    if (hasFocus() && m_current >= 0) {
        painter.setPen(QPen(profile.colors.accent, profile.border(BorderRole::Focus)));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(QRectF(tabRect(m_current)).adjusted(1, 1, -1, -2),
                                kRadius, kRadius);
    }
}

} // namespace novu::ui
