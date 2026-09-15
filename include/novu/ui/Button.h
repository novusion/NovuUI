#pragma once

#include "novu/ui/Icon.h"
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
class Button : public QPushButton
{
    Q_OBJECT

public:
    enum class Variant {
        Primary,    // the affirmative action of a view or dialog
        Secondary,  // neutral action (Cancel, Refresh)
        Ghost,      // low-emphasis, typically inside table rows
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
    bool m_busy = false;
    bool m_animating = false;
};

// Convenience constructors so call sites read as the design system names them.
Button* primaryButton(const QString& text, QWidget* parent = nullptr);
Button* secondaryButton(const QString& text, QWidget* parent = nullptr);
Button* ghostButton(const QString& text, QWidget* parent = nullptr);
Button* dangerButton(const QString& text, QWidget* parent = nullptr);
Button* successButton(const QString& text, QWidget* parent = nullptr);
Button* warningButton(const QString& text, QWidget* parent = nullptr);

// Detaches a panel into its own window. A named factory rather than a plain
// ghost button so the affordance reads the same in every panel that offers it —
// the gesture is learned once, not per screen.
Button* detachButton(QWidget* parent = nullptr);

} // namespace novu::ui
