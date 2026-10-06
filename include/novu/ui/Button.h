#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/Icon.h"
#include "novu/ui/StatusDot.h"
#include "novu/ui/StyleSpec.h"
#include "novu/ui/ThemeProfile.h"

#include <QPushButton>

class QPainter;

namespace novu::ui {

// The single button used across all Novu applications.
//
// Variant selects colour only — every variant has identical geometry rules, so
// a Primary and a Ghost button with the same caption are the same size.
//
// sizeHint() is computed from the button's own font and its own padding rather
// than left to the style sheet, because QSS padding is not reflected in
// QPushButton::sizeHint() until the widget has been polished. Depending on
// that timing means a caption can be painted clipped.
//
// Pages never call setMinimumWidth()/setFixedWidth() on a button.
class NOVUUI_API Button : public QPushButton
{
    Q_OBJECT

public:
    enum class Variant {
        Primary,    // the affirmative action of a view or dialog
        Accent,     // accent-tinted outline (`.btn.accent`), less force than Primary
        Secondary,  // neutral action (Cancel, Refresh)
        // The design's plain `.btn`: a hairline border and a transparent
        // plate at rest, filling to the page's own surface only once the
        // pointer arrives. Neither Secondary (filled at rest, for a control
        // that has to read against a card) nor Ghost (no border until
        // hover, for chrome) draws this - it is the button a toolbar, a
        // table footer or a row reaches for by default, with no emphasis
        // and no context to lift off of.
        Outlined,
        // No border until hover - for chrome (a title bar, a panel header)
        // where a hairline would draw a box around the window's own
        // furniture. Not for a row action: `iact()` in the reference is
        // always `.btn`'s bordered plate, Copy/View/Edit/Detach included, and
        // reaching for Ghost there is the mistake this comment used to invite.
        Ghost,
        Danger,     // destructive action
        Success,    // starts or resumes something (Start, Connect)
        // Attention, not failure: an action that proceeds but has a cost the
        // user should notice — switching to live money, overwriting a file,
        // pausing something that is running. The approved design is explicit
        // that this colour means attention and never branding.
        Warning,
    };

    explicit Button(QWidget* parent = nullptr);
    Button(const QString& text, Variant variant, QWidget* parent = nullptr);

    Variant variant() const { return m_variant; }
    void setVariant(Variant variant);

    // Row actions are the small size; toolbars and forms use the regular one.
    // The pixel heights belong to the profile, not to this class.
    ControlSize controlSize() const { return m_size; }
    void setControlSize(ControlSize size);

    // A leading glyph from the Novu icon set.
    //
    // Named rather than passed as a QIcon: an icon handed in from a page is a
    // drawing the page chose, at a colour and a size the page chose, and it
    // stops following the theme the moment the profile changes. A Glyph is
    // re-rendered in the variant's own ink on every theme change.
    Glyph glyph() const { return m_glyph; }
    void setGlyph(Glyph glyph);

    // A state the action itself carries, as a dot before the caption: a
    // session that is signed in, a service that is running. SelectButton has
    // had this since the designs asked for `.select .dot`, and a button that
    // performs the same kind of action needs the same mark - NovuSpace's
    // account control is "Logout" with the session's own light on it.
    //
    // It is a tone and not a colour, so it keeps its meaning across themes,
    // and it is NOT the variant's ink: a Secondary button saying Logout with
    // a green dot is one neutral action reporting a live session. Tone::None
    // - the default - draws nothing and reserves nothing, so every button
    // that never asks for one is unchanged.
    //
    // It takes the leading slot, so a button cannot carry both a dot and a
    // glyph; the dot wins, because a state is why the mark is there.
    void setStatusDot(StatusDot::Tone tone);
    void clearStatusDot();
    bool hasStatusDot() const { return m_statusDot; }

    // A glyph after the caption, at the trailing edge.
    //
    // Almost always the chevron on a button that opens a menu rather than
    // performing its caption — `svg("plus") + "New item" + svg("chevdown")`
    // in the approved design. It is a different statement from the leading
    // glyph: the leading one illustrates the action, the trailing one says
    // there is a choice behind it. Without this every product drew its own
    // arrow in the label text, where it is set in the body font rather than
    // the icon set and does not follow the ink.
    Glyph trailingGlyph() const { return m_trailingGlyph; }
    void setTrailingGlyph(Glyph glyph);

    // The action is running. The leading glyph becomes a turning spinner, in
    // the design's own `.spin`.
    //
    // A state rather than a separate widget the caller swaps in: a busy button
    // that is replaced by a spinner changes width, and a toolbar that reflows
    // the moment something starts is how a user loses the button they were
    // about to press next. Enabling is left to the caller - some work can be
    // cancelled by pressing again, and this class cannot know which.
    bool isBusy() const { return m_busy; }
    void setBusy(bool busy);

    // The inspection seam. styleSpec() is what this button *means*, in token
    // references; resolvedStyle() is what that comes to under the profile it is
    // currently scoped to. A test asserts on either, and needs no screenshot.
    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    bool event(QEvent* event) override;
    // Primary carries the design's travelling ring, which is painted over
    // whatever the style sheet drew rather than expressed in it: a conic
    // gradient is not something QSS can state, and a child widget holding one
    // would be a decoration that costs a widget.
    void paintEvent(QPaintEvent* event) override;

private:
    void applyVariant();
    void applyGlyph();
    void connectToClock();
    void updateMotion();
    void paintRing(QPainter* painter);

    Variant m_variant = Variant::Secondary;
    ControlSize m_size = ControlSize::Medium;
    Glyph m_glyph = Glyph::None;
    QPixmap statusDotPixmap() const;

    bool m_statusDot = false;
    StatusDot::Tone m_statusDotTone = StatusDot::Tone::Neutral;
    Glyph m_trailingGlyph = Glyph::None;
    bool m_busy = false;
    bool m_animating = false;
};

// Convenience constructors so call sites read as the design system names them.
Button* primaryButton(const QString& text, QWidget* parent = nullptr);
Button* secondaryButton(const QString& text, QWidget* parent = nullptr);
Button* outlinedButton(const QString& text, QWidget* parent = nullptr);
Button* ghostButton(const QString& text, QWidget* parent = nullptr);
Button* dangerButton(const QString& text, QWidget* parent = nullptr);
Button* successButton(const QString& text, QWidget* parent = nullptr);
Button* warningButton(const QString& text, QWidget* parent = nullptr);

// Detaches a panel into its own window. A named factory rather than a plain
// ghost button so the affordance reads the same in every panel that offers it —
// the gesture is learned once, not per screen.
Button* detachButton(QWidget* parent = nullptr);

} // namespace novu::ui
