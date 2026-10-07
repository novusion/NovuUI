#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/Icon.h"
#include "novu/ui/StyleSpec.h"
#include "novu/ui/Theme.h"
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
class NOVUUI_API IconButton : public QAbstractButton
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
        // The design's `.winbtn`: a title bar control. The same 30x24 box and
        // 6px radius the three window controls use, no border in any state,
        // and the panel surface under the pointer - which is what makes the
        // theme toggle beside Minimize read as one row of controls rather
        // than as a toolbar dropped onto the bar. Distinct from Ghost, which
        // grows an outline on hover and takes the control height of the
        // product rather than the window chrome's own box.
        Chrome,
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

    // A favourite can use a filled semantic glyph without a selected plate.
    void setCheckedGlyphStyle(ColorToken ink,bool filled=true,bool plate=false){m_checkedInk=ink;m_checkedFilled=filled;m_checkedPlate=plate;m_ownChecked=true;update();}
    void setGlyphSize(int pixels){m_glyphSize=pixels;update();}
    // An icon-only ordinary action uses the button family's box.
    void setActionPlate(bool enabled){m_actionPlate=enabled;updateGeometry();update();}
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    bool event(QEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    Glyph m_glyph = Glyph::None;
    Variant m_variant = Variant::Outlined;
    ControlSize m_size = ControlSize::Medium;
    bool m_destructive = false;
    bool m_hovered = false;
    bool m_ownChecked=false,m_checkedFilled=false,m_checkedPlate=true;
    ColorToken m_checkedInk;
    int m_glyphSize=0;
    bool m_actionPlate=false;
    // The ring is the keyboard's, not the mouse's - the same FocusRing
    // ModuleBar, TabBar and NavList share. This control used to paint on bare
    // hasFocus(), so any route that left focus on it after a click - a theme
    // toggle rebuilding the chrome, a popup closing, the window activating -
    // left an accent ring around a title-bar icon that nobody had tabbed to.
    FocusRing m_ring;
};

// The row action every table offers: small, ghost, one glyph. A named factory
// so twenty panels do not each decide what size an inline action is.
IconButton* rowAction(Glyph glyph, QWidget* parent, const QString& tooltip = {});

// A chrome icon with a count over it: "there are three of these waiting".
//
// The second of its kind in the library, which is why it is here rather than
// in the module that needed it. FeedbackButton is exactly this widget with the
// chat glyph and feedback's wording baked in; the next surface that wants an
// unread count - invitations, in NovuAI - would otherwise have copied the
// badge positioning, the 9+ cap and the transparent-to-mouse attribute into a
// third place. FeedbackButton is left alone: it carries its own progressive
// tooltip and is in use.
//
// The badge is drawn over the button and takes no mouse events, so clicking
// the number is still clicking the button rather than a dead spot.
class NOVUUI_API CountIconButton : public QWidget
{
    Q_OBJECT

public:
    CountIconButton(Glyph glyph, QWidget* parent = nullptr);

    // Zero hides the badge rather than drawing a "0", which reads as a fault
    // rather than as nothing waiting.
    void setCount(int count);
    int count() const { return m_count; }

    // The button underneath, for a tooltip the host wants to word itself.
    IconButton* button() const { return m_button; }

signals:
    void clicked();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void positionBadge();

    IconButton* m_button = nullptr;
    class Badge* m_badge = nullptr;
    int m_count = 0;
};

} // namespace novu::ui
