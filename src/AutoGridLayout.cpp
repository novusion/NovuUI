#include "novu/ui/AutoGridLayout.h"

#include <QWidget>

namespace novu::ui {

AutoGridLayout::AutoGridLayout(QWidget* parent, int minimum, int gap)
    : QLayout(parent)
    , m_minimum(qMax(1, minimum))
    , m_gap(qMax(0, gap))
{
    setContentsMargins(0, 0, 0, 0);
}

AutoGridLayout::AutoGridLayout(int minimum, int gap)
    : m_minimum(qMax(1, minimum))
    , m_gap(qMax(0, gap))
{
    setContentsMargins(0, 0, 0, 0);
}

AutoGridLayout::~AutoGridLayout()
{
    while (QLayoutItem* item = takeAt(0)) {
        delete item;
    }
}

void AutoGridLayout::setMinimumColumnWidth(int minimum)
{
    const int wanted = qMax(1, minimum);
    if (m_minimum == wanted) {
        return;
    }
    m_minimum = wanted;
    invalidate();
}

void AutoGridLayout::setGap(int gap)
{
    const int wanted = qMax(0, gap);
    if (m_gap == wanted) {
        return;
    }
    m_gap = wanted;
    invalidate();
}

void AutoGridLayout::addItem(QLayoutItem* item)
{
    m_items.append(item);
}

QLayoutItem* AutoGridLayout::itemAt(int index) const
{
    return m_items.value(index);
}

QLayoutItem* AutoGridLayout::takeAt(int index)
{
    return index >= 0 && index < m_items.size() ? m_items.takeAt(index) : nullptr;
}

int AutoGridLayout::count() const
{
    return m_items.size();
}

Qt::Orientations AutoGridLayout::expandingDirections() const
{
    return Qt::Horizontal;
}

int AutoGridLayout::columnsFor(int width) const
{
    if (m_items.isEmpty()) {
        return 0;
    }
    const QMargins m = contentsMargins();
    const int usable = width - m.left() - m.right();
    // n columns need n*minimum + (n-1)*gap. Solved rather than looped so the
    // answer does not depend on how the loop was written.
    int columns = (usable + m_gap) / (m_minimum + m_gap);
    columns = qBound(1, columns, m_items.size());
    return columns;
}

int AutoGridLayout::layout(const QRect& rect, bool apply) const
{
    if (m_items.isEmpty()) {
        return 0;
    }
    const QMargins m = contentsMargins();
    const QRect inner = rect.adjusted(m.left(), m.top(), -m.right(), -m.bottom());
    const int columns = columnsFor(rect.width());

    // The leftover is spread over the first few columns rather than left at
    // the trailing edge, so the grid reaches the edge exactly -- a rounded
    // share leaves a ragged strip that reads as a misaligned card.
    const int span = inner.width() - (columns - 1) * m_gap;
    const int base = span / columns;
    const int extra = span % columns;

    int y = inner.top();
    int index = 0;
    while (index < m_items.size()) {
        int rowHeight = 0;
        int x = inner.left();
        for (int column = 0; column < columns && index + column < m_items.size();
             ++column) {
            const int width = base + (column < extra ? 1 : 0);
            QLayoutItem* item = m_items.at(index + column);
            const int height = item->hasHeightForWidth()
                ? item->heightForWidth(width)
                : item->sizeHint().height();
            rowHeight = qMax(rowHeight, height);
            if (apply) {
                item->setGeometry(QRect(x, y, width, height));
            }
            x += width + m_gap;
        }
        // A second pass over the row, now that the tallest is known: every
        // cell in a row is the same height, or a tile that wrapped leaves its
        // neighbours floating above a gap.
        if (apply) {
            for (int column = 0; column < columns && index + column < m_items.size();
                 ++column) {
                QRect box = m_items.at(index + column)->geometry();
                box.setHeight(rowHeight);
                m_items.at(index + column)->setGeometry(box);
            }
        }
        y += rowHeight + m_gap;
        index += columns;
    }
    return y - m_gap - rect.top() + m.bottom();
}

int AutoGridLayout::heightForWidth(int width) const
{
    return layout(QRect(0, 0, width, 0), false);
}

void AutoGridLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);
    layout(rect, true);
}

QSize AutoGridLayout::sizeHint() const
{
    return minimumSize();
}

QSize AutoGridLayout::minimumSize() const
{
    const QMargins m = contentsMargins();
    int height = 0;
    for (QLayoutItem* item : m_items) {
        height = qMax(height, item->minimumSize().height());
    }
    return QSize(m_minimum + m.left() + m.right(), height + m.top() + m.bottom());
}

} // namespace novu::ui
