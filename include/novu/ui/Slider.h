#pragma once

#include "novu/ui/StyleSpec.h"

#include <QSlider>
#include <QString>

#include <functional>

class QBoxLayout;

namespace novu::ui {

// A value chosen along a range, and the value shown while it is chosen.
//
// The approved NovuHub design's `.scale` with its `.scale-val`: the readout is
// part of the control, not something next to it. Without it a slider states a
// proportion and nothing else, and "somewhere past the middle" is not a setting
// anyone can repeat or report.
//
// Self-painted rather than a styled QSlider. Qt's groove and handle are drawn
// by the style, and a stylesheet can recolour them but cannot put the filled
// part on the left of the handle or a number to the right of the groove — both
// of which the design asks for.
//
// Still a QSlider underneath, so value(), setRange(), valueChanged(), the arrow
// keys, Page Up/Down and the wheel all behave exactly as every caller expects.
class Slider : public QSlider
{
    Q_OBJECT

public:
    explicit Slider(QWidget* parent = nullptr);
    Slider(int minimum, int maximum, QWidget* parent = nullptr);

    // How the number beside the groove is written. The default is the value
    // itself; a caller that means percent, seconds or pixels says so here
    // rather than putting a second label next to the control.
    //
    //     slider->setFormatter([](int v) { return QStringLiteral("%1 %").arg(v); });
    using Formatter = std::function<QString(int)>;
    void setFormatter(Formatter formatter);
    QString readout() const;

    // Hides the readout for the one case that does not want it: a slider whose
    // value is shown somewhere else in the same row.
    bool showsReadout() const { return m_readout; }
    void setShowsReadout(bool shown);

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    // Where the parts are, so a test can drive the control without guessing.
    QRect grooveRect() const;
    QRect handleRect() const;
    QRect readoutRect() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    bool event(QEvent* event) override;

private:
    int valueAt(int x) const;

    Formatter m_formatter;
    bool m_readout = true;
    bool m_hovered = false;
    bool m_dragging = false;
};

// A slider in a settings column: caption and hint on the left, the control and
// its readout on the right. The same shape toggleRow() gives a switch, so a
// settings page of switches and sliders lines up.
Slider* sliderRow(QBoxLayout* into, QWidget* parent, const QString& caption,
                  int minimum, int maximum, int value, const QString& hint = {});

} // namespace novu::ui
