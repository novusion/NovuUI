#pragma once

#include "novu/ui/StyleSpec.h"

#include <QWidget>

namespace novu::ui {

// A filled dot that says something is live.
//
// The smallest thing in the design and one of the most used: the connection
// light in the status bar, the "recording" mark in a sidebar, the one beside a
// price that is still ticking. `.livedot` and `.sdot` in the approved design.
//
// It breathes rather than blinks — the design's `softglow`, on the shared clock
// — and only when it is actually reporting something live. A dot that pulses
// while nothing is happening is a dot nobody reads any more.
//
// Ported from NovusionUI's `Dot` (novusion/ui/BrandWidgets.h), with two
// changes. It takes a *tone* rather than a QColor, so the meaning survives a
// theme switch; and the colour cross-fade on a state change is kept, because
// that behaviour is the reason the original is better than a repaint: a
// connection dropping should be something the eye catches, and a dot that
// snaps from green to red between two frames is something the eye misses.
class StatusDot : public QWidget
{
    Q_OBJECT

public:
    enum class Tone {
        Neutral,   // idle, stopped, not configured
        Success,   // connected, running, live
        Warning,   // degraded, paused, reconnecting
        Danger,    // disconnected, failed
        Info,      // informational
        Accent,    // the product pointing at something; never a status
    };

    explicit StatusDot(Tone tone = Tone::Neutral, QWidget* parent = nullptr);
    StatusDot(Tone tone, bool pulsing, QWidget* parent = nullptr);

    Tone tone() const { return m_tone; }
    // Cross-fades rather than snapping. See the class note.
    void setTone(Tone tone);

    bool isPulsing() const { return m_pulsing; }
    void setPulsing(bool pulsing);

    // The dot itself, not the widget: the widget carries a halo around it, so
    // the glow has somewhere to go without the layout moving.
    int diameter() const { return m_diameter; }
    void setDiameter(int px);

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

private:
    void updateMotion();

    Tone m_tone = Tone::Neutral;
    // Where the cross-fade is, and what it is coming from. Held as a tone, not
    // a colour, so a theme switch mid-fade lands in the new palette.
    Tone m_previousTone = Tone::Neutral;
    double m_blend = 1.0;
    int m_diameter = 7;
    bool m_pulsing = false;
    bool m_animating = false;
};

} // namespace novu::ui
