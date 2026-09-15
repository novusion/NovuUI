#pragma once

#include "novu/ui/Icon.h"
#include "novu/ui/ThemeProfile.h"

#include <QAbstractButton>

namespace novu::ui {

// A button that is a glyph and nothing else.
//
// Not Button with an empty caption: the two have different geometry rules. A
// Button is as wide as its text plus padding; an IconButton is square, and it
// stays square at every control size so a row of them reads as one strip
// rather than as a ragged edge.
//
// Self-painted for the same reason Badge is. A stylesheet cannot put a stroke
// drawing inside a control, so a styled icon button ends up with the glyph in
// a child QLabel — a second widget, a second palette and a second place for
// the alignment to go wrong. Here the plate, the border and the outline are
// three calls in one paintEvent().
//
// Checkable is the design's `.on` state: a control that toggles something
// which stays on, such as a detached panel or a visible side rail. It goes to
// the product accent, the same signal an active module or a selected row uses,
// because it is the same statement.
class IconButton : public QAbstractButton
{
    Q_OBJECT

public:
    enum class Variant {
        // A hairline plate, the same outline a Secondary button wears. For an
        // icon action sitting among other controls in a toolbar or a row.
        Outlined,
        // No plate until the pointer arrives. For chrome: a title bar, a panel
        // header, anywhere a border would draw a box around the window's own
        // furniture.
        Ghost,
    };

    explicit IconButton(Glyph glyph, QWidget* parent = nullptr);
    IconButton(Glyph glyph, Variant variant, QWidget* parent = nullptr);

    Glyph glyph() const { return m_glyph; }
    void setGlyph(Glyph glyph);

    Variant variant() const { return m_variant; }
    void setVariant(Variant variant);

    ControlSize controlSize() const { return m_size; }
    void setControlSize(ControlSize size);

    // Destructive actions colour on hover rather than at rest, so a row of
    // actions does not read as a row of warnings.
    bool isDestructive() const { return m_destructive; }
    void setDestructive(bool destructive);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    bool event(QEvent* event) override;

private:
    Glyph m_glyph = Glyph::None;
    Variant m_variant = Variant::Outlined;
    ControlSize m_size = ControlSize::Medium;
    bool m_destructive = false;
    bool m_hovered = false;
};

// The row action every table offers: small, ghost, one glyph. A named factory
// so twenty panels do not each decide what size an inline action is.
IconButton* rowAction(Glyph glyph, QWidget* parent, const QString& tooltip = {});

} // namespace novu::ui
