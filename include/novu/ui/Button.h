#pragma once

#include "novu/ui/ThemeProfile.h"

#include <QPushButton>

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
    };

    explicit Button(QWidget* parent = nullptr);
    Button(const QString& text, Variant variant, QWidget* parent = nullptr);

    Variant variant() const { return m_variant; }
    void setVariant(Variant variant);

    // Row actions are the small size; toolbars and forms use the regular one.
    // The pixel heights belong to the profile, not to this class.
    ControlSize controlSize() const { return m_size; }
    void setControlSize(ControlSize size);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    bool event(QEvent* event) override;

private:
    void applyVariant();

    Variant m_variant = Variant::Secondary;
    ControlSize m_size = ControlSize::Medium;
};

// Convenience constructors so call sites read as the design system names them.
Button* primaryButton(const QString& text, QWidget* parent = nullptr);
Button* secondaryButton(const QString& text, QWidget* parent = nullptr);
Button* ghostButton(const QString& text, QWidget* parent = nullptr);
Button* dangerButton(const QString& text, QWidget* parent = nullptr);
Button* successButton(const QString& text, QWidget* parent = nullptr);

// Detaches a panel into its own window. A named factory rather than a plain
// ghost button so the affordance reads the same in every panel that offers it —
// the gesture is learned once, not per screen.
Button* detachButton(QWidget* parent = nullptr);

} // namespace novu::ui
