#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/StyleSpec.h"

#include <QAbstractButton>

class QBoxLayout;

class QVariantAnimation;

namespace novu::ui {

// The switch: something is on, or it is off, and changing it takes effect at
// once.
//
// Not a QCheckBox with a different skin. A check box states a *choice* that a
// form will later submit; a switch states a *state* that is already true. The
// approved design draws them differently for that reason, and every settings
// row in all four NovuSpace modules uses this one.
//
// Self-painted: a track, a knob and nothing else. Qt has no switch primitive,
// and the alternatives are a stylesheet that cannot move the knob or two child
// widgets for two shapes.
//
// The knob slides rather than jumps. That is not decoration either - the
// movement is what says the change was accepted, on a control that has no other
// feedback. It runs on QVariantAnimation, whose driver Qt already shares across
// every animation in the process, so this adds no timer of its own.
class NOVUUI_API Toggle : public QAbstractButton
{
    Q_OBJECT

public:
    explicit Toggle(QWidget* parent = nullptr);
    ~Toggle() override;

    // The inspection seam: what the track and knob mean, and what that comes
    // to under the profile this toggle is scoped to.
    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    // Where the knob is, 0 at off and 1 at on. Exposed so a test can assert
    // that the travel happened without watching pixels, and so a screenshot
    // taken mid-slide is explicable rather than mysterious.
    double knobPosition() const { return m_position; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void checkStateSet() override;
    void nextCheckState() override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    bool event(QEvent* event) override;

private:
    void animateTo(double position);

    QVariantAnimation* m_slide = nullptr;
    double m_position = 0.0;
    bool m_hovered = false;
};

// A switch with its label, as a row: caption and hint on the left, the switch
// hard right. The design's `.kv` — every settings screen in every module is a
// column of these, and a page that lays one out by hand gets the alignment
// wrong the moment a hint wraps.
//
// Returns the Toggle, because that is the thing a caller needs to connect to.
Toggle* toggleRow(QBoxLayout* into, QWidget* parent, const QString& caption,
                  const QString& hint = {}, bool on = false);

} // namespace novu::ui
