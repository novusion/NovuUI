#pragma once

#include <QLayout>
#include <QList>

namespace novu::ui {

// A row that wraps onto the next line when it runs out of width.
//
// Qt ships no such layout. The alternatives both fail for a strip of chips: a
// QHBoxLayout overflows off the edge, and a QGridLayout stretches every cell to
// the widest one, so a "Side" chip ends up as wide as "Current Price". Neither
// is acceptable for a control where the label length is the content.
class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget* parent = nullptr, int spacing = -1);
    ~FlowLayout() override;

    void addItem(QLayoutItem* item) override;
    int count() const override;
    QLayoutItem* itemAt(int index) const override;
    QLayoutItem* takeAt(int index) override;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    void setGeometry(const QRect& rect) override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;

private:
    int arrange(const QRect& rect, bool testOnly) const;
    int gap() const;

    QList<QLayoutItem*> m_items;
    int m_spacing = -1;
};

} // namespace novu::ui
