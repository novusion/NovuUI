#pragma once

#include "novu/ui/ThemeProfile.h"

#include <QString>

class QBoxLayout;
class QHBoxLayout;
class QVBoxLayout;
class QScrollArea;
class QWidget;

namespace novu::ui {

// Layout constructs, not widgets.
//
// Every function here returns a *layout* wherever a layout is enough. A row of
// buttons does not need a QWidget to live in — the parent layout already
// groups it, and the wrapper would only add a paint pass, a size hint and a
// place for a stray stylesheet to land.

// Where a form's actions sit along its width.
//
// A layout behaviour rather than a position one screen chose. A sign-in card, a
// dialog footer and a settings page all put their primary action somewhere
// along the bottom edge, and each one picking for itself is how three forms in
// one product end up with the button in three places.
enum class ActionAlignment {
    Left,     // under the first field: a secondary or inline action
    Center,   // a single action on a narrow card
    Right,    // the default for a form: the primary action ends where the
              // fields end, which is the edge the eye is already following
};

// Places `row`'s contents according to `alignment`, by managing the stretches
// around them.
//
// Idempotent, and safe to call after widgets have been added or removed - it
// removes the stretches it added before putting new ones in, so a row does not
// accumulate them. Callers normally do not call this: a component that owns an
// action row re-applies it when its layout changes.
void applyActionAlignment(QHBoxLayout* row, ActionAlignment alignment);

// A horizontal strip of controls with the profile's toolbar gap. Add it to the
// layout you already have; nothing is created but the layout itself.
QHBoxLayout* toolbar(QBoxLayout* into);

// The page's outermost layout: page padding and card gap, applied to a widget
// that already exists (the page itself).
QVBoxLayout* pageLayout(QWidget* page);

// A scrollable page. This one *does* create a widget, and says why: a
// QScrollArea needs a viewport widget to scroll. Returns the layout to fill.
//
// The scroll area is transparent and frameless, so it adds no visible surface
// of its own — it is a mechanism, not a container in the design sense.
QVBoxLayout* scrollPage(QWidget* page, QScrollArea** areaOut = nullptr);

// Two-part header: a title (and optional subtitle) on the left, actions on the
// right. Returns the right-hand layout so the caller can drop buttons in.
//
// `eyebrow` is the uppercase accent line above the title — the product or
// module a page belongs to. It exists because a page inside a host shell has
// to say which product it is: NovuSpace loads NovuTrade, NovuMail and
// NovuVault as modules in one window, and a bare "Trade History" does not
// tell the user whose history it is. Omitted when empty, so a standalone
// application's pages are unchanged.
QHBoxLayout* splitHeader(QBoxLayout* into,
                         QWidget* parent,
                         const QString& title,
                         const QString& subtitle = {},
                         const QString& eyebrow = {});

} // namespace novu::ui
