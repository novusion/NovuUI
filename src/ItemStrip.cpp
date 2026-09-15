#include "novu/ui/ItemStrip.h"

#include "novu/ui/Tooltip.h"

#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>

namespace novu::ui {

ItemStrip::ItemStrip(QWidget* parent)
    : QWidget(parent)
{
    setCursor(Qt::PointingHandCursor);
    // One tab stop with arrows inside it, the way a radio group is.
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

void ItemStrip::setCurrentIndex(int index)
{
    if (index < 0 || index >= stripCount() || index == m_current) {
        return;
    }
    m_current = index;
    update();
}

void ItemStrip::choose(int index)
{
    if (index < 0 || index >= stripCount() || index == m_current
        || !stripItemEnabled(index)) {
        return;
    }
    setCurrentIndex(index);
    stripItemChosen(index);
}

void ItemStrip::resetStripState()
{
    m_hovered = -1;
    m_pressed = -1;
    if (m_tipIndex >= 0) {
        Tooltip::remove(this);
        m_tipIndex = -1;
    }
    if (m_current >= stripCount()) {
        m_current = stripCount() > 0 ? 0 : -1;
    }
}

int ItemStrip::stripWidth() const
{
    const int items = stripCount();
    if (items <= 0) {
        return 2 * stripLead();
    }
    int width = 2 * stripLead() + stripGap() * (items - 1);
    for (int i = 0; i < items; ++i) {
        width += stripItemWidth(i);
    }
    return width;
}

QRect ItemStrip::itemRect(int index) const
{
    if (index < 0 || index >= stripCount()) {
        return QRect();
    }
    int x = stripLead();
    for (int i = 0; i < index; ++i) {
        x += stripItemWidth(i) + stripGap();
    }
    return QRect(x, stripTop(), stripItemWidth(index), stripItemHeight());
}

int ItemStrip::indexAt(const QPoint& position) const
{
    for (int i = 0; i < stripCount(); ++i) {
        if (itemRect(i).contains(position)) {
            return i;
        }
    }
    return -1;
}

int ItemStrip::neighbour(int from, int step) const
{
    for (int i = from + step; i >= 0 && i < stripCount(); i += step) {
        if (stripItemEnabled(i)) {
            return i;
        }
    }
    return -1;
}

void ItemStrip::mousePressEvent(QMouseEvent* event)
{
    // Taking the press matters, and not only for correctness: a strip in a
    // title bar sits on a drag surface, and a control that ignores the press
    // hands the gesture to the window, which then moves instead of choosing.
    if (event->button() != Qt::LeftButton || !isEnabled()) {
        QWidget::mousePressEvent(event);
        return;
    }
    const int index = indexAt(event->position().toPoint());
    m_pressed = (index >= 0 && stripItemEnabled(index)) ? index : -1;
    if (m_pressed >= 0) {
        event->accept();
        update();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void ItemStrip::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const int released = indexAt(event->position().toPoint());
    // Press and release in the same item, or nothing happened. Dragging off a
    // control before letting go is how a user cancels.
    if (m_pressed >= 0 && released == m_pressed) {
        choose(released);
    }
    m_pressed = -1;
    update();
    event->accept();
}

void ItemStrip::mouseMoveEvent(QMouseEvent* event)
{
    const int under = indexAt(event->position().toPoint());
    if (under != m_hovered) {
        m_hovered = under;
        update();

        // Tooltips are the reason a glyph-only item is usable at all, and there
        // is no child widget to carry one - so the tip is swapped onto the
        // strip itself as the pointer crosses between items.
        const QString tip = under >= 0 ? stripItemTooltip(under) : QString();
        if (under != m_tipIndex) {
            if (tip.isEmpty()) {
                Tooltip::remove(this);
                m_tipIndex = -1;
            } else {
                Tooltip::install(this, tip);
                m_tipIndex = under;
            }
        }
    }
    QWidget::mouseMoveEvent(event);
}

void ItemStrip::leaveEvent(QEvent* event)
{
    m_hovered = -1;
    m_pressed = -1;
    if (m_tipIndex >= 0) {
        Tooltip::remove(this);
        m_tipIndex = -1;
    }
    update();
    QWidget::leaveEvent(event);
}

void ItemStrip::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Up) {
        const int previous = neighbour(m_current < 0 ? stripCount() : m_current, -1);
        if (previous >= 0) {
            choose(previous);
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Right || event->key() == Qt::Key_Down) {
        const int next = neighbour(m_current, 1);
        if (next >= 0) {
            choose(next);
        }
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace novu::ui
