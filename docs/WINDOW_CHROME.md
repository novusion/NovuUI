# Novu window chrome — the canonical specification

Every Novu application window and every Novu popup, dialog and picker wears the
same frame. This file is the source of truth for what that frame is: its
components, its tokens, its behaviour, the contract a non-C++ port has to
mirror, and the list of applications still to be migrated onto it.

The system it belongs to is described in `DESIGN_SYSTEM.md`; this file is the
window layer of it.

---

## 1. Why this exists

Before it, three applications each answered the same question differently:

| | title bar | controls | Close |
|---|---|---|---|
| NovuExplorer `TitleBarWidget` | 32px, custom | 46×32 flat `QPushButton`, font glyphs | neutral, themed QSS |
| NovuExplorer `NovuPopupWindow` | 8px padding, custom | one button, 22×22 | lowercase "x" |
| NovuHub `HubWindow` | inline QSS, hardcoded `#161a1f` | no size at all — whatever the caption measured | hover `#d98a8a` |
| everything else | native Windows | native | native |

Four frames, four Close buttons, three of them a different size. A user moving
between two Novu applications was looking at two products.

The rule that replaces all of it:

> An application declares what a window **is**. It never draws chrome.

---

## 2. Components

```
WindowShell            installs the frame onto a top-level widget
  TitleBar             icon, caption, leading/trailing slots, the three controls
    WindowControlButton  Minimize | Maximize | Restore | Close — one class

AppWindow    : QWidget   a main application window   (WindowKind::Main)
ToolWindow   : AppWindow a detached panel            (WindowKind::Tool)
Dialog       : QDialog   every dialog and popup      (WindowKind::Dialog)
```

`WindowShell` is composed rather than inherited. A dialog has to be a `QDialog`
— `exec()`, `accept()`, modality, default-button handling — and a main window
has to be a `QWidget`. C++ gives one base class, so a shared base would force
one of the two into the wrong hierarchy. That is exactly how the applications
ended up with three separate title bars, and composing the shell is what stops
it happening again.

### 2.1 The API an application uses

```cpp
#include "novu/ui/WindowShell.h"
using namespace novu::ui;

AppWindow window;
window.setTitle(tr("NovuExplorer"));
window.setIcon(QIcon(":/icons/NovuExplorer.png"));
window.setContentWidget(new ExplorerWorkspace);
window.shell()->setContentSize({1200, 700});
window.show();
```

That is the whole integration. Semantics only:

| call | meaning |
|---|---|
| `setTitle()` | caption **and** the OS window title, so taskbar and Alt-Tab agree |
| `setIcon()` | application mark, drawn at the profile's icon size and the display's pixel ratio |
| `shell()->setResizable(bool)` | also clears the OS maximize box, so Aero Snap agrees with the bar |
| `shell()->setMinimizable(bool)` | hides Minimize; moves nothing else |
| `shell()->setMaximizable(bool)` | hides Maximize and disables double-click on the bar |
| `shell()->setContentSize(QSize)` | sizes the **content**; the frame is added on top |
| `setModal(true)` on a `Dialog` | ordinary `QDialog` modality, unchanged |
| `titleBar()->setLeading(menuBar)` | a menu bar inside the bar, not under it |
| `titleBar()->setTrailing({chip, indicator})` | status widgets before the controls |

There is deliberately **no** way to set a control's size, colour or position,
and no accessor that hands out a mutable control. "The application picked its
own" is the defect being removed.

### 2.2 Dialogs

```cpp
Dialog dialog(tr("Settings"), parent);
dialog.setModal(true);
dialog.contentLayout()->addWidget(page);

auto* cancel = secondaryButton(tr("Cancel"), dialog.content());
auto* save   = primaryButton(tr("Save"), dialog.content());
dialog.buttonRow()->addWidget(cancel);
dialog.buttonRow()->addWidget(save);
connect(cancel, &QAbstractButton::clicked, &dialog, &QDialog::reject);
connect(save,   &QAbstractButton::clicked, &dialog, &QDialog::accept);

dialog.shell()->setContentSize({460, 320});
dialog.exec();
```

`buttonRow()` is created on first use and is always in the same place —
right-aligned, below the content, with the profile's toolbar gap between
buttons. Without it every dialog invents its own footer and they stop lining
up.

Three ready-made replacements exist for the `QMessageBox` calls that are the
last native windows in most applications:

```cpp
Dialog::confirm(parent, tr("Delete workspace"),
                tr("The workspace and its 12 saved layouts are removed."),
                tr("Delete"), tr("Keep"), /*destructive=*/true);
Dialog::inform(parent, tr("Import finished"), tr("48 files imported."));
Dialog::reportError(parent, tr("Upload failed"), message);
```

### 2.3 What a page may not do

A page placed in `content()`:

- must not build a title bar of its own
- must not call `setWindowFlags()` on its window
- must not style, resize or reposition anything in the bar
- must not create a `WindowControlButton`

`window.content()->findChild<TitleBar*>()` returns nothing, and a test asserts
it: a page cannot walk its own children to find the chrome above it.

---

## 3. Tokens

All of it lives in `ThemeProfile::window` (`WindowTokens`) and in two radius
roles. Nothing is hardcoded in a component, and no application sets any of it.

### 3.1 Geometry — identical in every profile

| token | value | what it is |
|---|---:|---|
| `titleBarHeight` | 32 | the bar |
| `controlWidth` | 46 | **all four** controls |
| `controlHeight` | 32 | **all four** controls |
| `titleBarPadding` | 10 | leading inset before the icon |
| `titleBarGap` | 8 | icon → caption, caption → trailing |
| `borderWidth` | 1 | window frame hairline |
| `resizeMargin` | 6 | grab band along the frame edge |
| `iconSize` | 16 | the application mark |
| `glyphSize` | 10 | bounding box of a control's glyph |
| `glyphStroke` | 1 | line weight inside that box |

A profile may restyle the bar. It may **not** resize it: geometry is asserted
equal across every registered profile by `controlGeometryIsIdenticalAcrossProfiles()`,
because otherwise the products drift apart through the theme instead of through
the code.

The controls sit **flush** against the trailing edge — no gap. A gap leaves a
dead strip on a maximised window exactly where the pointer is thrown to hit
Close.

### 3.2 Colours — derived from each profile's palette

`applyWindowTokens()` in `ThemeProfile.cpp` derives every one of them, so a
third product gets correct chrome by defining a palette and nothing else:

| token | derived from | NovuSpace value |
|---|---|---|
| `titleBarBg` | `colors.headerBg` | `#1A1F25` |
| `titleBarBgInactive` | `colors.windowBg` | `#161A1F` |
| `titleText` | `colors.textPrimary` | `#E3E8F0` |
| `titleTextInactive` | `colors.textDisabled` | `#6E7A8E` |
| `border` | `colors.border` | `#232830` |
| `controlHoverBg` | `colors.surfaceHover` | `#2A3038` |
| `controlPressedBg` | `colors.surfaceActive` | `#2F353F` |
| `controlGlyph` | `colors.textSecondary` | `#A8B2C3` |
| `controlGlyphInactive` | `colors.textDisabled` | `#6E7A8E` |
| `closeHoverBg` | `colors.danger` | `#FF5555` |
| `closePressedBg` | `colors.danger.darker(130)` | `#C44141` |
| `closeHoverGlyph` | white | `#FFFFFF` |

### 3.3 Type and radius roles

| role | NovuSpace | NovuTrade |
|---|---|---|
| `TypeRole::WindowTitle` | 12px / 600 | 12px / 600 |
| `SurfaceRole::Window` | `Small` → 4 | `Large` → 12 |
| `SurfaceRole::Dialog` | `Small` → 4 | `XL` → 16 |

A main window and a dialog are allowed to differ — but as two roles a profile
decides, never as two numbers two applications picked.

---

## 4. Behaviour

### 4.1 Close

Neutral by default: nothing is painted, so the title-bar surface shows through.

| state | plate | glyph |
|---|---|---|
| normal | *(transparent)* | `controlGlyph` |
| hover | `closeHoverBg` (danger) | `closeHoverGlyph` (white) |
| pressed | `closePressedBg` (danger, darkened) | `closeHoverGlyph` |

Close is **never** permanently red. `closeIsNeutralUntilHovered()` renders the
button and reads a pixel to assert it.

### 4.2 Minimize / Maximize / Restore

Same geometry, neutral states: `controlHoverBg` on hover, `controlPressedBg`
pressed, and the glyph brightens to `titleText` — without that it sits at the
same weight on a lighter plate and reads as disabled.

Maximize becomes Restore from the real window state, never from the last click:
the shell watches `QEvent::WindowStateChange`.

### 4.3 Glyphs

Drawn, not typed. A font glyph (`✕`, `□`) is at the mercy of whatever font the
system substitutes; these have to stay crisp at 125% and 150%.

Inside the 10×10 box, stroke 1:

| control | drawing |
|---|---|
| Minimize | one 10×1 bar on the box's own centre line |
| Maximize | 10×10 outline, drawn as four filled bars |
| Restore | front 8×8 outline offset down-left by 2, plus the back sheet's top and right edges |
| Close | two diagonals corner to corner, antialiased |

Outlines are four filled bars rather than `drawRect()` with a pen: a one-pixel
pen straddles the path and antialiases into two rows of grey.

### 4.4 Window gestures

| gesture | result |
|---|---|
| drag the bar | native window move (`startSystemMove`) — a maximised window restores first |
| double-click the bar | maximize/restore, only when maximizable |
| drag an edge or corner | native resize, from the `resizeMargin` band |
| window deactivated | caption and glyphs drop to the inactive tokens |
| maximised | radius 0, border not drawn — its corners are the screen's |

Dragging and resizing both go through the compositor. Moving the frame by hand
with `setGeometry()` loses snapping on every platform and is not permitted at
all on Wayland.

### 4.5 Captions

The caption elides; it never widens the window and never pushes a control off
the frame. `title()` still returns what the caller set — the eliding is
presentation — and a tooltip appears only when something was actually cut.

---

## 5. Platform behaviour

### Windows

The shell keeps the native frame alive **under** its own chrome:

- `WS_THICKFRAME` is restored after Qt's `FramelessWindowHint` removes it, so
  resize borders, Aero Snap and the taskbar keep working
- `WM_NCCALCSIZE` returns a zero-height caption, so nothing native is drawn;
  a maximised window still gets the frame compensation Windows adds, or its
  edges are cropped off-screen
- `WM_NCHITTEST` answers the resize edges from `resizeMargin`, never narrower
  than `SM_CXSIZEFRAME + SM_CXPADDEDBORDERWIDTH`
- `WM_GETMINMAXINFO` publishes the window's own layout minimum
- `DwmExtendFrameIntoClientArea` with a 1px margin gives the drop shadow
- `DWMWA_WINDOW_CORNER_PREFERENCE` carries the radius token: `0 → DONOTROUND`,
  `1..5 → ROUNDSMALL`, `>5 → ROUND`
- `DWMWA_BORDER_COLOR` carries the border token, flattened against the window
  background because DWM takes an opaque `COLORREF`
- `DWMWA_USE_IMMERSIVE_DARK_MODE` follows the palette's luminance

Corner rounding is handed to DWM rather than painted. Painting it ourselves
would need a translucent backing store, and that costs sub-pixel text
antialiasing on **every label in the window** — a bad trade for four pixels of
curve. The consequence, stated plainly: on Windows 10 and on compositors
without an equivalent, Novu windows are square. The radius role still governs
what is asked for, so the day the platform can honour it, it does.

### Elsewhere

`QWindow::startSystemResize()` from a grab band inside the frame, and whatever
corners the compositor draws. The appearance is otherwise identical. Novu is
Windows-first; this path is best-effort and is where a Linux port starts.

---

## 6. Reference

The gallery ships the reference window, the reference tool window and the
reference dialog:

```
cmake --build build-gallery --config Release --target NovuUIPlayground
build-gallery/playground/Release/NovuUIPlayground.exe
```

Foundation → **WindowShell**. The page shows the control matrix, the
capability variants, the elision behaviour and the tokens in force, and opens
real windows in whichever profile is selected — a bar in a page cannot
demonstrate resizing, snapping or the corner radius.

Source: `playground/src/pages/WindowShellPage.cpp`.

---

## 7. The contract for a non-C++ port

NovuCrawler is PySide6, and other tools will be. NovuUI stays the source of
truth: a port **mirrors** this section, it does not redesign it, and it does not
get to pick its own numbers.

### 7.1 Values to mirror

Geometry from §3.1 and colours from §3.2, verbatim. A port that hardcodes the
NovuSpace hex values is correct today; a port that derives them from a palette
the way `applyWindowTokens()` does is correct after the next profile.

### 7.2 Layout algorithm

```
bar: fixed height 32, horizontal layout, margins (10, 0, 0, 0), spacing 0
  icon    16×32 label, hidden when there is no icon
  caption stretch 1, left/vcentre, 8px left margin when an icon precedes it,
          8px right margin, elided right, tooltip only when cut
  trailing application widgets, max height 32
  controls Minimize, Maximize/Restore, Close — 46×32 each, flush right, no gap
```

Font: `Segoe UI`, 12px, weight 600.

### 7.3 Behaviour to mirror

- Close: transparent → `#FF5555` on hover → `#C44141` pressed, glyph white on
  both
- Minimize / Maximize: transparent → `#2A3038` hover → `#2F353F` pressed, glyph
  `#A8B2C3` brightening to `#E3E8F0`
- inactive window: bar `#161A1F`, caption `#6E7A8E`, glyphs `#6E7A8E`
- Maximize swaps to Restore from the real window state
- controls are square, edge to edge — no rounded corners on the plate
- a dialog hides Minimize and Maximize and changes nothing else
- glyphs are drawn, not font characters, to the recipe in §4.3

### 7.4 PySide6 skeleton

```python
CONTROL_W, CONTROL_H, BAR_H = 46, 32, 32
GLYPH, STROKE, PAD, GAP = 10, 1, 10, 8

TITLEBAR_BG        = "#1A1F25"
TITLEBAR_BG_INACT  = "#161A1F"
TITLE_TEXT         = "#E3E8F0"
TITLE_TEXT_INACT   = "#6E7A8E"
BORDER             = "#232830"
CONTROL_HOVER      = "#2A3038"
CONTROL_PRESSED    = "#2F353F"
CONTROL_GLYPH      = "#A8B2C3"
CLOSE_HOVER        = "#FF5555"
CLOSE_PRESSED      = "#C44141"
CLOSE_GLYPH        = "#FFFFFF"

TITLEBAR_QSS = f"""
QWidget#NovuTitleBar {{ background: {TITLEBAR_BG};
                        border-bottom: 1px solid {BORDER}; }}
QWidget#NovuTitleBar QLabel#NovuTitle {{ color: {TITLE_TEXT};
                                         font-family: 'Segoe UI';
                                         font-size: 12px; font-weight: 600; }}
QWidget#NovuTitleBar QToolButton {{ background: transparent; border: none;
                                    width: {CONTROL_W}px; height: {CONTROL_H}px; }}
QWidget#NovuTitleBar QToolButton:hover  {{ background: {CONTROL_HOVER}; }}
QWidget#NovuTitleBar QToolButton:pressed {{ background: {CONTROL_PRESSED}; }}
QWidget#NovuTitleBar QToolButton#NovuClose:hover   {{ background: {CLOSE_HOVER}; }}
QWidget#NovuTitleBar QToolButton#NovuClose:pressed {{ background: {CLOSE_PRESSED}; }}
"""
```

Paint the glyphs in `paintEvent` to the §4.3 recipe — the stylesheet above sizes
and tints the plate, it does not draw the marks. A port that uses text glyphs
will not match at 125% scaling and should not be accepted as done.

Do **not** redesign NovuCrawler while porting. The visual specification is this
file.

---

## 8. Migration

### 8.1 Order

1. Main window — the whole application moves at once, one file.
2. Its dialogs — one at a time, each a mechanical change.
3. `QMessageBox` → `Dialog::confirm` / `inform` / `reportError`.
4. `QFileDialog` → the Novu picker once it exists (§8.4).
5. Delete the application's own title bar, its native-event handling, its DWM
   calls and its dialog QSS. Until they are gone there are two implementations
   and the second one drifts.

### 8.2 A main window

```diff
-class MainWindow : public QMainWindow {
-    TitleBarWidget* titleBar_;
-    bool nativeEvent(...) override;   // WM_NCCALCSIZE, WM_NCHITTEST, ...
-};
+class MainWindow : public novu::ui::AppWindow {
+};

-setWindowFlag(Qt::FramelessWindowHint);
-restoreNativeResizeFrame(this);
-DwmExtendFrameIntoClientArea(hwnd, &margins);
-DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, ...);
-titleBar_ = new TitleBarWidget(this);
-connect(titleBar_, &TitleBarWidget::minimizeClicked, ...);
-vbox->addWidget(titleBar_);
-vbox->addWidget(workspace);
+setTitle(tr("NovuExplorer — v%1").arg(version));
+setIcon(QIcon(":/icons/NovuExplorer.png"));
+titleBar()->setLeading(menuBar);
+setContentWidget(workspace);
+shell()->setContentSize({1200, 700});
```

An application built on `QMainWindow` for docks and toolbars keeps it: put the
`QMainWindow` inside `AppWindow::content()` and give it `Qt::Widget` flags.

### 8.3 A dialog

```diff
-class SettingsDialog : public QDialog {
-    SettingsDialog(QWidget* p) : QDialog(p) {
-        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
-        ThemeManager::applyDwmDarkFrame(this);
-        setStyleSheet(ThemeManager::instance().dialogQss());
-        auto* bar = new QWidget(this);            // custom title bar
-        auto* close = new QPushButton("x", bar);
-        close->setFixedSize(22, 22);
+class SettingsDialog : public novu::ui::Dialog {
+    SettingsDialog(QWidget* p) : Dialog(tr("Settings"), p) {
```

Everything the old constructor did to the frame is deleted; the content moves
into `contentLayout()` and the buttons into `buttonRow()`.

### 8.4 File and folder pickers

`QFileDialog` is not the final Novu UX. The reusable picker comes from
NovuExplorer, but its **window** is this shell:

```cpp
Dialog picker(tr("Choose a folder"), parent);
picker.setContentWidget(new NovuFileBrowser(NovuFileBrowser::Folders));
picker.shell()->setResizable(true);
picker.shell()->setContentSize({860, 560});
```

Until the picker lands, a native `QFileDialog` is the documented interim — it
is the one place where replacing the window before the content exists would
lose function.

### 8.5 Name collision to expect

NovuSpace already has `novuspace::ui::AppWindow`. Different namespace, so no
hard conflict, but a translation unit with both `using namespace novu::ui;` and
`using namespace novuspace::ui;` makes an unqualified `AppWindow` ambiguous.
Qualify during the transition; the collision disappears when the NovuSpace
class is deleted.

---

## 9. Applications currently using custom or native chrome

Surveyed 2026-08-29.

### Custom chrome to replace

| application | file | what it does now |
|---|---|---|
| NovuExplorer | `src/ui/title_bar_widget.{hpp,cpp}` | own 32px bar, 46×32 controls, font glyphs, own QSS |
| NovuExplorer | `src/ui/main_window.cpp` | `FramelessWindowHint`, own `nativeEvent` (`WM_NCCALCSIZE`, `WM_NCHITTEST`, `WM_GETMINMAXINFO`), own DWM calls, own `ExplorerNativeResizeFilter` |
| NovuExplorer | `src/ui/novu_popup_window.{hpp,cpp}` | own popup bar, single 22×22 Close with a lowercase "x" |
| NovuExplorer | `src/preview/ui/PreviewWindow.cpp` | a second copy of the `nativeEvent` + DWM recipe |
| NovuExplorer | `src/ui/process_watcher_window.cpp` | a third copy of the DWM corner/border calls |
| NovuExplorer | `src/ui/theme/theme_manager.cpp` | `applyDwmDarkFrame()`, `dialogQss()`, `QMessageBox` restyling |
| NovuHub | `src/ui/HubWindow.cpp` | `FramelessWindowHint`, inline QSS bar with hardcoded `#161a1f`, no control size at all, Close hover `#d98a8a`, own `nativeEvent` |
| NovuHub | `src/main.cpp` | DWM dark mode, corner preference, caption and border colour, applied per process |
| NovuSpace | `src/ui/AppWindow.{h,cpp}` | own frameless shell, own `WS_THICKFRAME` restore, own resize filter |
| NovuSpace | `src/ui/FirstRunSetup.cpp` | `FramelessWindowHint` + `WindowStaysOnTopHint`, own frame |
| NovuSpace | `src/modules/novutrade/NovuTradeWorkspace.cpp:1756` | a dialog made frameless by hand |
| NovuSpace | `simple-web/main.cpp` | a fourth copy of the DWM + `WM_NCCALCSIZE` recipe |
| NovuCrawler | `novucrawler/ui/window.py`, `ui/dialogs.py` | PySide6, native chrome — port per §7 |

### Native dialogs to replace

`QMessageBox::` / `QFileDialog::` call sites, by application:

| application | sites |
|---:|---:|
| NovuAI | 268 |
| NovuSpace | 203 |
| NovuExplorer | 132 |
| NovuAdmin | 28 |
| NovuHub | 27 |

Named dialogs already identified as needing the shell: NovuExplorer's
`NovuFileDialog`, `MultiRenameDialog`, `ShareDestinationDialog`; NovuSpace's
`EmailAccountSettingsDialog`, `VaultFirstRunWizard`, `PlatformSetupPanel`
dialogs; NovuAdmin's `UploadBuildDialog`; NovuAI's settings pages and
`WhatsNewManager`.

### Suggested sequence

| step | scope | risk |
|---|---|---|
| 1 | NovuExplorer main window + `NovuPopupWindow` | highest payoff — deletes three copies of the native recipe |
| 2 | NovuHub `HubWindow` | removes the last hardcoded chrome palette |
| 3 | NovuSpace `AppWindow` | touches the shell that hosts the modules; do it alone |
| 4 | `QMessageBox` sweeps, one application at a time | mechanical, high volume |
| 5 | NovuCrawler PySide6 port | §7 is the whole brief |
| 6 | the Novu file/folder picker | unblocks the `QFileDialog` sweep |

Step 3 is the point to run every application and confirm the look before the
remaining sweeps.
