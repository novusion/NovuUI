#include "novu/ui/FlowLayout.h"

#include <QWidget>

namespace novu::ui {

FlowLayout::FlowLayout(QWidget* parent, int spacing)
    : QLayout(parent)
    , m_spacing(spacing)
{
    setContentsMargins(0, 0, 0, 0);
}

FlowLayout::~FlowLayout()
{
    while (QLayoutItem* item = takeAt(0)) {
        delete item;
    }
}

void FlowLayout::addItem(QLayoutItem* item)
{
    m_items.append(item);
}

int FlowLayout::count() const
{
    return m_items.size();
}

QLayoutItem* FlowLayout::itemAt(int index) const
{
    return (index >= 0 && index < m_items.size()) ? m_items.at(index) : nullptr;
}

QLayoutItem* FlowLayout::takeAt(int index)
{
    return (index >= 0 && index < m_items.size()) ? m_items.takeAt(index) : nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return {};
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    return arrange(QRect(0, 0, width, 0), true);
}

void FlowLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);
    arrange(rect, false);
}

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (QLayoutItem* item : m_items) {
        size = size.expandedTo(item->minimumSize());
    }
    const QMargins m = contentsMargins();
    return size + QSize(m.left() + m.right(), m.top() + m.bottom());
}

int FlowLayout::gap() const
{
    return m_spacing >= 0 ? m_spacing : spacing();
}

int FlowLayout::arrange(const QRect& rect, bool testOnly) const
{
    const QMargins m = contentsMargins();
    const QRect content = rect.adjusted(m.left(), m.top(), -m.right(), -m.bottom());

    int x = content.x();
    int y = content.y();
    int lineHeight = 0;
    const int space = gap();

    for (QLayoutItem* item : m_items) {
        const QSize hint = item->sizeHint();
        int next = x + hint.width() + space;
        if (next - space > content.right() && lineHeight > 0) {
            x = content.x();
            y += lineHeight + space;
            next = x + hint.width() + space;
            lineHeight = 0;
        }
        if (!testOnly) {
            item->setGeometry(QRect(QPoint(x, y), hint));
        }
        x = next;
        lineHeight = qMax(lineHeight, hint.height());
    }
    return y + lineHeight - rect.y() + m.bottom();
}

} // namespace novu::ui
