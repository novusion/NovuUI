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
