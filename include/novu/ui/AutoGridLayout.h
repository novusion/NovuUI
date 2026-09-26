#pragma once

#include <QLayout>
#include <QList>

namespace novu::ui {

// `repeat(auto-fit, minmax(<min>, 1fr))`, as a layout.
//
// The approved dashboards are built on it -- Summary's module tiles, the
// Models catalogue, the Create galleries -- and it is the one grid rule a box
// layout cannot express: as many equal columns as fit at a stated minimum,
// with the leftover shared out so the last column reaches the edge. A
// FlowLayout comes close and then stops short, because its items keep their
// own widths and the row ends ragged; a QGridLayout can do the stretch but
// only for a column count decided in advance, which is the part that has to
// change with the window.
//
// Items are laid out in order, left to right, and every cell is the same
// width. Row height is the tallest item in that row, so a tile that wraps to
// two lines does not leave its neighbours short.
class AutoGridLayout : public QLayout
{
    Q_OBJECT

public:
    // `minimum` is the minmax() floor: the narrowest a column may be before
    // the grid drops one. `gap` is the `gap` property, used on both axes.
    explicit AutoGridLayout(QWidget* parent, int minimum, int gap);
    AutoGridLayout(int minimum, int gap);
    ~AutoGridLayout() override;

    int minimumColumnWidth() const { return m_minimum; }
    void setMinimumColumnWidth(int minimum);

    int gap() const { return m_gap; }
    void setGap(int gap);

    // How many columns the grid would use at `width`. Public because it is
    // the whole behaviour, and a test that wants to prove the grid drops a
    // column should not have to measure pixels to see it.
    int columnsFor(int width) const;

    void addItem(QLayoutItem* item) override;
    QLayoutItem* itemAt(int index) const override;
    QLayoutItem* takeAt(int index) override;
    int count() const override;

    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect& rect) override;

private:
    int layout(const QRect& rect, bool apply) const;

    QList<QLayoutItem*> m_items;
    int m_minimum = 160;
    int m_gap = 9;
};

} // namespace novu::ui
