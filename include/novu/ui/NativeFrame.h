#pragma once

#include "novu/ui/Export.h"

#include <QColor>

class QWidget;

namespace novu::ui {

// One frame for every Novu window - the main window, its menus, popups and
// tooltips, and the panels floating inside it.
//
// On Windows 11 the compositor rounds a top-level window and draws its
// hairline (WindowShell, as NovuHub and NovuAI do). A popup asks it for the
// same corner and the same border colour instead of painting its own curve,
// so a menu's edge is the window's edge. Where the compositor cannot (Windows
// 10, other systems) - and for panels drawn inside a window, which are not
// windows - the same radius and colour are painted.

//! True where the compositor rounds and outlines top-level windows.
NOVUUI_API bool compositorDrawsWindowFrame();

//! The corner radius a Novu window shows here, in logical pixels: the
//! compositor's own (8 or 4) where it draws it, else the window radius token.
NOVUUI_API int windowCornerRadius(const QWidget* context);

//! The outline of a Novu window, from the window tokens.
NOVUUI_API QColor windowBorderColor(const QWidget* context);
NOVUUI_API int windowBorderWidth(const QWidget* context);

//! For a frameless top-level popup: the compositor rounds and outlines it like
//! the main window each time it is shown. Returns whether it does - when it
//! does, the popup paints a plain opaque background (no curve, no border) and
//! must not be translucent. Call from the popup's constructor.
NOVUUI_API bool useCompositorFrame(QWidget* popup);

} // namespace novu::ui
