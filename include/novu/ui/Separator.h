#pragma once

#include <QWidget>

class QBoxLayout;

namespace novu::ui {

// A hairline. The one that divides a menu, the one that stands between two
// groups in a title bar, the one that closes a card header.
//
// It exists because a rule is the single most-repeated thing in the approved
// design and the most-reinvented thing in the code: a QFrame with
// HLine|Sunken here, a one-pixel QWidget with a stylesheet there, a painted
// line inside a third component. Those three do not match — Qt's sunken frame
// is two lines in the system palette, and the stylesheet version carries the
// palette with it.
//
// Painted, with no children and no stylesheet. A separator is decoration, and
// decoration is exactly what must not cost a styled widget: a card header with
// a rule under it should be one more paint call, not one more QFrame with its
// own polish pass and its own place for a stray sheet to land.
class Separator : public QWidget
{
    Q_OBJECT

public:
    // The design draws rules at two weights: the hairline that separates rows
    // inside a surface, and the stronger one that separates surfaces from each
    // other. Named rather than numbered, so a profile can move either.
    enum class Weight {
        Hairline,   // between rows, under a card header — the common one
        Strong,     // between surfaces: title bar, side rail, status bar
    };

    explicit Separator(Qt::Orientation orientation = Qt::Horizontal,
                       QWidget* parent = nullptr);
    Separator(Qt::Orientation orientation, Weight weight, QWidget* parent = nullptr);

    Qt::Orientation orientation() const { return m_orientation; }
    void setOrientation(Qt::Orientation orientation);

    Weight weight() const { return m_weight; }
    void setWeight(Weight weight);

    // How far the rule stops short of each end. The design insets a menu
    // separator and runs a card's rule to the full width, and that is a
    // property of the rule rather than of the layout around it — a margin on
    // the layout would move the next item too.
    int inset() const { return m_inset; }
    void setInset(int px);

    // A fixed run along the rule's own axis; 0 lets it take the space it is
    // given. The vertical divider between two groups of title-bar controls is
    // shorter than the bar, which is a length, not a margin.
    int length() const { return m_length; }
    void setLength(int px);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void applyPolicy();

    Qt::Orientation m_orientation = Qt::Horizontal;
    Weight m_weight = Weight::Hairline;
    int m_inset = 0;
    int m_length = 0;
};

// Adds a rule to a layout that already exists. The counterpart of section()
// and toolbar(): the caller wanted a line, not an object to hold on to.
Separator* separator(QBoxLayout* into, QWidget* parent,
                     Qt::Orientation orientation = Qt::Horizontal);

// The short upright rule between two groups of controls in a toolbar or title
// bar. Its length is the design's, not the caller's.
Separator* toolbarDivider(QBoxLayout* into, QWidget* parent);

} // namespace novu::ui
