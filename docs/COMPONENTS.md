# NovuUI component catalogue

**Generated — do not edit.** `python tools/generate_catalogue.py` rebuilds it
from the gallery's own component specs, which are what the Playground renders.
Editing this file by hand makes it disagree with the library.

Every component here is real and previewable: open the Playground and pick it
from the navigation, or launch straight into it with `run.bat <id>`.

- **Currently used by** is measured from each application's own sources.
  Empty means built and not yet adopted, which during extraction is most of them.
- **Designed for** is stated applicability — what the component was extracted for.

See [README](../README.md) for the library's rules, [THEMING](THEMING.md) for how
styling resolves, and [EXTRACTION_STATUS](EXTRACTION_STATUS.md) for what is
deliberately missing.

| | |
|---|---|
| components | 66 |
| Foundation | 13 |
| Input | 13 |
| Navigation | 16 |
| Data display | 12 |
| Feedback | 12 |


## Foundation

### Active panel

The rule every component that draws a selection follows: a panel that is not the active one keeps its selection, dimmed, rather than clearing it or competing with the one that is.

```cpp
#include "novu/ui/Theme.h"
```

| | |
|---|---|
| States | `Active`, `Inactive` |
| API | `novuActivePanel` |
| Currently used by | novuadmin |
| Designed for | novuexplorer |
| Playground | `run.bat active-panel` |
| Source | `src/Theme.cpp` |

| property | type | notes |
|---|---|---|
| `novuActivePanel` | `bool` | Set on the panel. Unset means always active, which is what a dialog wants. |

### FloatingPanel

A panel that floats inside the application window: a grab bar, a body, a task strip, a resize grip and a roll-up. Clipped to its host, which is the point.

```cpp
#include "novu/ui/FloatingPanel.h"
```

| | |
|---|---|
| Variants | `With a task strip`, `Rolled` |
| States | `Front`, `Behind`, `Dragging`, `Resizing` |
| API | `rolled`, `front`, `resizable` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuai |
| Playground | `run.bat floating-panel` |
| Source | `src/FloatingPanel.cpp` |

| property | type | notes |
|---|---|---|
| `rolled` | `bool` | Only the bar. Keeps its contents and its height. |
| `front` | `bool` | Set by the layer, never by the panel. |
| `resizable` | `bool` | Hides the grip as well as refusing the drag. |

### FlowLayout

A row that wraps onto the next line when it runs out of width, with every item keeping its own size. Qt ships no equivalent.

```cpp
#include "novu/ui/FlowLayout.h"
```

| | |
|---|---|
| Variants | `Uniform items`, `Mixed heights` |
| States | `One line`, `Wrapped`, `Empty` |
| API | `spacing` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace, novutrade |
| Playground | `run.bat flow-layout` |
| Source | `src/FlowLayout.cpp` |

| property | type | notes |
|---|---|---|
| `spacing` | `int` | -1 takes the style's default layout spacing. |

### GateCard

The card a person signs in on: a mark, a title, the form the caller fills, and a footer naming the host the credential is about to go to.

```cpp
#include "novu/ui/GateCard.h"
```

| | |
|---|---|
| Variants | `With a footnote`, `Host only` |
| States | `Idle`, `Busy`, `Failed` |
| API | `host`, `error`, `busy` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat gate-card` |
| Source | `src/GateCard.cpp` |

| property | type | notes |
|---|---|---|
| `host` | `QString` | Where the credential is going. A security property. |
| `error` | `QString` | Above the form. Setting one clears busy. |
| `busy` | `bool` | Disables the whole form, not only the button. |

### Icon

The Novu icon set: stroke outlines taken from the approved design, painted rather than instantiated. No icon font, no QtSvg, no widget per glyph.

```cpp
#include "novu/ui/Icon.h"
```

| | |
|---|---|
| Variants | `Small`, `Medium`, `Large` |
| States | `Normal`, `Disabled` |
| API | `glyph`, `size`, `color` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat icon` |
| Source | `src/Icon.cpp` |

| property | type | notes |
|---|---|---|
| `glyph` | `enum` | Which outline. Named, so it follows the theme. |
| `size` | `int` | From IconMetrics — a component never invents one. |
| `color` | `QColor` | Resolved from an Ink role by the caller. |

### Layout

toolbar(), pageLayout(), splitHeader() and scrollPage(). Layout constructs, not widgets — each returns a layout wherever a layout is enough.

```cpp
#include "novu/ui/Layout.h"
```

| | |
|---|---|
| Variants | `toolbar()`, `pageLayout()`, `splitHeader()`, `scrollPage()` |
| States | `Normal`, `Empty`, `Scrolling` |
| API | `title`, `subtitle` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace, novuai, novuexplorer, novuhub, novuvoice, novutrade |
| Playground | `run.bat layout` |
| Source | `src/Layout.cpp` |

| property | type | notes |
|---|---|---|
| `title` | `QString` | splitHeader() title. |
| `subtitle` | `QString` | splitHeader() subtitle; omitted when empty. |

### Motion

The shared animation clock and the four looping primitives that run on it: the running ring, the spinner, the shimmer and the breathing curve.

```cpp
#include "novu/ui/Motion.h"
```

| | |
|---|---|
| Variants | `paintRunningRing`, `paintSpinner`, `paintShimmer`, `breathe` |
| States | `Running`, `Frozen` |
| API | `periodMs`, `animated` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat motion` |
| Source | `src/Motion.cpp` |

| property | type | notes |
|---|---|---|
| `periodMs` | `int` | How long one loop takes. Read from the profile, never named by a component. |
| `animated` | `bool` | False draws the primitive frozen rather than omitting it. |

### Separator

The hairline. Painted, with no children and no stylesheet, at the two weights the design draws: inside a surface, and between surfaces.

```cpp
#include "novu/ui/Separator.h"
```

| | |
|---|---|
| Variants | `Hairline`, `Strong`, `Horizontal`, `Vertical` |
| States | `Normal` |
| API | `orientation`, `weight`, `inset`, `length` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat separator` |
| Source | `src/Separator.cpp` |

| property | type | notes |
|---|---|---|
| `orientation` | `enum` | Which way the rule runs. |
| `weight` | `enum` | Inside a surface, or between two of them. |
| `inset` | `int` | How far it stops short of each end. |
| `length` | `int` | A fixed run; 0 takes the space it is given. |

### Splitter

The handle between two panels, with a collapse arrow at each end — and CollapseDivider, for a fixed strip that folds but does not resize.

```cpp
#include "novu/ui/Splitter.h"
```

| | |
|---|---|
| Variants | `Horizontal`, `Vertical`, `No arrows`, `CollapseDivider` |
| States | `Idle`, `Hover`, `Dragging`, `Collapsed` |
| API | `collapseArrows`, `panelCollapsed` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuexplorer, novuspace |
| Playground | `run.bat splitter` |
| Source | `src/Splitter.cpp` |

| property | type | notes |
|---|---|---|
| `collapseArrows` | `bool` | Off leaves a plain drag handle. |
| `panelCollapsed` | `bool` | Sets a size to zero; restoring returns the size it had. |

### Theme & Styling

The live reference for the styling architecture. Real components and the tokens they resolve through — change the Theme selector and watch the same widgets move.

```cpp
#include "novu/ui/StyleSpec.h"
```

| | |
|---|---|
| Variants | `Every profile` |
| States | `Normal`, `Invalid`, `Selected` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace, novuai, novuexplorer, novuadmin, novuhub, novuvoice, novutrade |
| Playground | `run.bat theme-styling` |
| Source | `src/StyleSpec.cpp` |

### Theme roles

Every token the active ThemeProfile resolves: colours, surface radii, spacing, borders, control heights, chart and table tokens. Switch the profile above to diff two products.

```cpp
#include "novu/ui/ThemeProfile.h"
```

| | |
|---|---|
| Variants | `Colours`, `SurfaceRole`, `SpacingRole`, `BorderRole`, `ControlSize`, `Chart`, `Table` |
| States | `Per profile` |
| API | `id`, `colors`, `roleRadius` |
| Currently used by | novuspace |
| Designed for | novuspace, novuai, novuexplorer, novuhub, novuvoice, novutrade |
| Playground | `run.bat theme-roles` |
| Source | `src/ThemeProfile.cpp` |

| property | type | notes |
|---|---|---|
| `id` | `QString` | Profile id, as registered with ThemeManager. |
| `colors` | `ThemeColors` | 18 named colours. No component contains a hex literal. |
| `roleRadius` | `QHash<SurfaceRole,RadiusStep>` | What each surface's roundness step is. |

### Typography

Text by role, never by number: label(), caption(), applyText(), setInk(), and the TypeRole / Ink vocabulary they resolve against.

```cpp
#include "novu/ui/Text.h"
```

| | |
|---|---|
| Variants | `PageTitle`, `SectionTitle`, `CardTitle`, `Body`, `Caption`, `Label`, `Metric`, `Mono`, `Eyebrow` |
| States | `Default`, `Secondary`, `Muted`, `Accent`, `Success`, `Warning`, `Danger`, `Info` |
| API | `role`, `ink`, `bold` |
| Currently used by | novuspace |
| Designed for | novuspace, novuai, novuexplorer, novuhub, novuvoice, novutrade |
| Playground | `run.bat typography` |
| Source | `src/Text.cpp` |

| property | type | notes |
|---|---|---|
| `role` | `enum` | Size and weight together. |
| `ink` | `enum` | Semantic colour. |
| `bold` | `bool` | Overrides the role's weight. |

### WindowShell

The frame every Novu window and dialog wears: title bar, three controls, border, radius, drag and resize. Applications choose semantics, never chrome.

```cpp
#include "novu/ui/WindowShell.h"
```

| | |
|---|---|
| Variants | `AppWindow`, `ToolWindow`, `Dialog` |
| States | `Active`, `Inactive`, `Maximized`, `Restored`, `Close hover` |
| API | `title`, `icon`, `resizable`, `minimizable`, `maximizable`, `kind` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace, novuai, novuexplorer, novuhub, novuvoice, novutrade |
| Playground | `run.bat window-shell` |
| Source | `src/WindowShell.cpp` |

| property | type | notes |
|---|---|---|
| `title` | `QString` | Caption and the OS window title together. Elides; never widens the window. |
| `icon` | `QIcon` | Application mark, drawn at the profile's icon size and the display's pixel ratio. |
| `resizable` | `bool` | false for a fixed dialog. Also clears the OS maximize box, so Aero Snap agrees with the bar. |
| `minimizable` | `bool` | Hides Minimize without moving anything else. |
| `maximizable` | `bool` | Hides Maximize and disables double-click on the bar. |
| `kind` | `enum` | Sets the default capabilities and the radius role. |


## Input

### Button

The single button used across all Novu applications. Variant selects colour only — geometry is identical, and computed rather than styled.

```cpp
#include "novu/ui/Button.h"
```

| | |
|---|---|
| Variants | `Primary`, `Secondary`, `Ghost`, `Success`, `Warning`, `Danger` |
| States | `Normal`, `Hover`, `Pressed`, `Focused`, `Disabled` |
| API | `text`, `variant`, `glyph`, `enabled`, `novuSize` |
| Currently used by | novuadmin, novuspace |
| Designed for | novuspace, novuai, novuexplorer, novuhub, novuvoice, novutrade |
| Playground | `run.bat button` |
| Source | `src/Button.cpp` |

| property | type | notes |
|---|---|---|
| `text` | `QString` | Caption. sizeHint() is measured from it. |
| `variant` | `enum` | Colour role. Never changes geometry. |
| `glyph` | `enum` | A leading icon from the Novu set, redrawn per variant and theme. |
| `enabled` | `bool` | Disabled buttons drop to the muted palette. |
| `novuSize` | `enum` | Geometry variant for row actions; composes with any variant. |

### Chip / ChipBar

A small on/off label, and the wrapping strip of them used to choose which columns a table shows. The strip carries no background of its own.

```cpp
#include "novu/ui/Chip.h"
```

| | |
|---|---|
| Variants | `On`, `Off`, `Bar with caption`, `Bar without caption` |
| States | `On`, `Off`, `Disabled`, `Hover` |
| API | `text`, `on`, `items`, `caption` |
| Currently used by | novuspace |
| Designed for | novuspace, novutrade |
| Playground | `run.bat chip` |
| Source | `src/Chip.cpp` |

| property | type | notes |
|---|---|---|
| `text` | `QString` | Chip caption. |
| `on` | `bool` | Initial checked state. |
| `items` | `QStringList` | ChipBar contents, in declared order. |
| `caption` | `QString` | Laid out as the first item so it wraps with the chips. |

### Field

One labelled input and what is wrong with it. Caption, control, and a hint that an error replaces. Caption and message are painted; only the control is a widget.

```cpp
#include "novu/ui/Field.h"
```

| | |
|---|---|
| Variants | `Hint`, `Error`, `Bare` |
| States | `Valid`, `Invalid`, `Disabled` |
| API | `caption`, `hint`, `error`, `clearsErrorOnEdit` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace, novuadmin, novuai |
| Playground | `run.bat field` |
| Source | `src/Field.cpp` |

| property | type | notes |
|---|---|---|
| `caption` | `QString` | Painted above the control, uppercase and tracked. |
| `hint` | `QString` | The line under the control while nothing is wrong. |
| `error` | `QString` | Replaces the hint and marks the control. Empty clears. |
| `clearsErrorOnEdit` | `bool` | Off only for a field whose validity the server decides. |

### Fields

ComboBox, and the lineEdit() / checkBox() factories. They set what a stylesheet cannot — height, font and an optional fixed width — and nothing else.

```cpp
#include "novu/ui/Fields.h"
```

| | |
|---|---|
| Variants | `ComboBox`, `LineEdit`, `CheckBox` |
| States | `Normal`, `Focused`, `Disabled`, `Empty`, `Populated` |
| API | `items`, `placeholder`, `width`, `checked` |
| Currently used by | novuspace |
| Designed for | novuspace, novuai, novuexplorer, novuhub, novuvoice, novutrade |
| Playground | `run.bat fields` |
| Source | `src/Fields.cpp` |

| property | type | notes |
|---|---|---|
| `items` | `QStringList` | ComboBox contents. |
| `placeholder` | `QString` | LineEdit placeholder. |
| `width` | `int` | Fixed width; 0 means the control sizes itself. |
| `checked` | `bool` | CheckBox initial state. |

### Form submission

What a dialog does between the press and the answer: busy state that disables the whole footer, and the server's error where the eye is already looking.

```cpp
#include "novu/ui/WindowShell.h"
```

| | |
|---|---|
| Variants | `Idle`, `Busy`, `Failed` |
| States | `Field error`, `Panel notice`, `Submission error` |
| API | `busy`, `formError` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace, novuai |
| Playground | `run.bat form-submission` |
| Source | `src/WindowShell.cpp` |

| property | type | notes |
|---|---|---|
| `busy` | `bool` | Disables every footer button and stops Escape closing it. |
| `formError` | `QString` | Shown above the footer. Setting one always clears busy. |

### FormRow

Label on the left at a fixed column width, control on the right, optional muted hint. Returns a layout and creates no wrapper.

```cpp
#include "novu/ui/Form.h"
```

| | |
|---|---|
| Variants | `With control`, `With hint`, `Without control` |
| States | `Normal`, `Disabled` |
| API | `caption`, `control`, `hint` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace, novuai, novuexplorer, novuhub, novutrade |
| Playground | `run.bat form-row` |
| Source | `src/Form.cpp` |

| property | type | notes |
|---|---|---|
| `caption` | `QString` | Left column text. |
| `control` | `QWidget*` | Any control; reparented to the row's owner. |
| `hint` | `QString` | Muted note after the control. |

### IconButton

A button that is a glyph and nothing else. Square at every control size, self-painted, with the product accent as its checked state.

```cpp
#include "novu/ui/IconButton.h"
```

| | |
|---|---|
| Variants | `Outlined`, `Ghost` |
| States | `Normal`, `Hover`, `Pressed`, `Checked`, `Focused`, `Disabled` |
| API | `glyph`, `variant`, `checkable`, `destructive` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat icon-button` |
| Source | `src/IconButton.cpp` |

| property | type | notes |
|---|---|---|
| `glyph` | `enum` | Which outline the button carries. |
| `variant` | `enum` | Whether it has a plate at rest. |
| `checkable` | `bool` | Checked is the accent: something that stays on. |
| `destructive` | `bool` | Colours on hover rather than at rest. |

### SearchBar

The search field every list panel needs, with the profile's control geometry and a clear button decided once rather than per panel.

```cpp
#include "novu/ui/Form.h"
```

| | |
|---|---|
| Variants | `Default placeholder`, `Custom placeholder`, `Fixed width` |
| States | `Empty`, `Populated`, `Focused`, `Disabled` |
| API | `placeholder`, `width` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace, novuai, novuexplorer, novutrade |
| Playground | `run.bat search-bar` |
| Source | `src/Form.cpp` |

| property | type | notes |
|---|---|---|
| `placeholder` | `QString` | Empty takes the shared default. |
| `width` | `int` | 0 lets the layout size it. |

### Segmented

Mutually exclusive choices in one tray, one of them in force. Words or glyphs, painted segments with real press/release hit testing.

```cpp
#include "novu/ui/Segmented.h"
```

| | |
|---|---|
| Variants | `Words`, `Glyphs`, `Small`, `Medium` |
| States | `Normal`, `Hover`, `Pressed`, `Focused`, `Disabled` |
| API | `currentIndex`, `controlSize`, `segmentAttention` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat segmented` |
| Source | `src/Segmented.cpp` |

| property | type | notes |
|---|---|---|
| `currentIndex` | `int` | Which segment is in force. |
| `controlSize` | `enum` | Steps type and glyph together. |
| `segmentAttention` | `bool` | Marks a choice that has a cost, current or not. |

### SettingRow

A name, what it means, and the control that changes it — or a read-only value, or a masked secret, with the validation message under the row.

```cpp
#include "novu/ui/SettingRow.h"
```

| | |
|---|---|
| Variants | `With control`, `Read-only`, `Masked` |
| States | `Normal`, `Error`, `Last in list` |
| API | `hint`, `value`, `masked`, `error`, `controlColumn` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat setting-row` |
| Source | `src/SettingRow.cpp` |

| property | type | notes |
|---|---|---|
| `hint` | `QString` | Wraps, capped at the design's 58ch measure. |
| `value` | `QString` | A read-only value instead of a control; setting one clears the other. |
| `masked` | `bool` | A fixed run of bullets, so the length does not leak. |
| `error` | `QString` | The validation line. Changes the row's styleSpec() too. |
| `controlColumn` | `int` | Fixed, so rows in one panel line up. 0 measures the control. |

### Slider

A value chosen along a range, with the value shown beside it. Self-painted, click-to-position, still a QSlider underneath.

```cpp
#include "novu/ui/Slider.h"
```

| | |
|---|---|
| Variants | `Readout`, `Formatted`, `Bare` |
| States | `Normal`, `Hover`, `Focused`, `Disabled` |
| API | `value`, `formatter`, `showsReadout` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuhub, novuspace |
| Playground | `run.bat slider` |
| Source | `src/Slider.cpp` |

| property | type | notes |
|---|---|---|
| `value` | `int` | QSlider's own, with all its keyboard and wheel behaviour. |
| `formatter` | `std::function` | How the readout is written — percent, seconds, pixels. |
| `showsReadout` | `bool` | Off only when the value is shown elsewhere in the row. |

### Swatch

A row of colours, one chosen — or several, when it is a filter. Painted with hit testing; wraps, and reports its height for a given width.

```cpp
#include "novu/ui/Swatch.h"
```

| | |
|---|---|
| Variants | `Single`, `Multi`, `With none`, `With custom` |
| States | `Normal`, `Hover`, `Chosen` |
| API | `multiSelect`, `current`, `chosen` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuexplorer |
| Playground | `run.bat swatch` |
| Source | `src/Swatch.cpp` |

| property | type | notes |
|---|---|---|
| `multiSelect` | `bool` | A filter asks "any of these"; a tag asks "which one". |
| `current` | `QColor` | Single select. An invalid colour means none. |
| `chosen` | `QList<QColor>` | Multi select. Empty means no filter. |

### Toggle

The switch: something is on, or it is off, and changing it takes effect at once. Self-painted, with the knob sliding on Qt's shared animation driver.

```cpp
#include "novu/ui/Toggle.h"
```

| | |
|---|---|
| Variants | `Off`, `On` |
| States | `Normal`, `Hover`, `Focused`, `Disabled` |
| API | `checked`, `knobPosition` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat toggle` |
| Source | `src/Toggle.cpp` |

| property | type | notes |
|---|---|---|
| `checked` | `bool` | The state it reports, and the one it slides to. |
| `knobPosition` | `double` | Read-only: 0 off, 1 on. The seam a test asserts travel on. |


## Navigation

### BottomPanel

The console strip across the bottom of a window. Holds tabs and pages; owns neither whether it is open nor what is in it — the host answers both.

```cpp
#include "novu/ui/BottomPanel.h"
```

| | |
|---|---|
| Variants | `Expanded`, `Collapsed` |
| States | `Normal`, `Modified tab` |
| API | `expanded`, `expandedHeight`, `currentKey` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuexplorer, novuai |
| Playground | `run.bat bottom-panel` |
| Source | `src/BottomPanel.cpp` |

| property | type | notes |
|---|---|---|
| `expanded` | `bool` | An input. The panel applies it and never changes it. |
| `expandedHeight` | `int` | How tall when open, not counting the strip. The host's number. |
| `currentKey` | `QString` | Which page is showing, by the caller's own identifier. |

### CommandPalette

Every action the application has, reached by name. Subsequence matching with a ranking, so the command Return runs is the one you meant.

```cpp
#include "novu/ui/CommandPalette.h"
```

| | |
|---|---|
| Variants | `Grouped`, `Ranked` |
| States | `Empty filter`, `Filtered`, `No match` |
| API | `commands`, `filter` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuexplorer, novuvoice |
| Playground | `run.bat command-palette` |
| Source | `src/CommandPalette.cpp` |

| property | type | notes |
|---|---|---|
| `commands` | `QVector<Command>` | Given, not discovered: the application knows what it can do. |
| `filter` | `QString` | Matches a subsequence of the name, or the group. |

### Disclosure

A header that puts its content away: an outlined Advanced row inside a form, or a sidebar group with a count and an action that does not collapse it.

```cpp
#include "novu/ui/Disclosure.h"
```

| | |
|---|---|
| Variants | `Inline`, `Block` |
| States | `Open`, `Shut`, `Hover`, `Focused` |
| API | `variant`, `open`, `count`, `hint` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat disclosure` |
| Source | `src/Disclosure.cpp` |

| property | type | notes |
|---|---|---|
| `variant` | `enum` | An outlined form row, or a sidebar caption. |
| `open` | `bool` | Hides the body; never clears it. |
| `count` | `int` | -1 draws nothing. 0 draws a zero, which says something else. |
| `hint` | `QString` | Inline only: what is inside, without opening it. |

### Drawer

The detail view that opens over a list instead of navigating away from it, so the table keeps its filters and its scroll position.

```cpp
#include "novu/ui/Drawer.h"
```

| | |
|---|---|
| Variants | `Trailing`, `Leading`, `With actions` |
| States | `Open`, `Closed`, `Guarded` |
| API | `edge`, `maximumWidth`, `closesOnScrimClick` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat drawer` |
| Source | `src/Drawer.cpp` |

| property | type | notes |
|---|---|---|
| `edge` | `enum` | Which side it is anchored to. |
| `maximumWidth` | `int` | Capped in pixels and as a fraction: both matter. |
| `closesOnScrimClick` | `bool` | False for a drawer holding an edit a stray click would lose. |

### ModuleBar

Top-level navigation in the title bar: the modules an application is made of, with the active one in the accent — the only accented thing in the chrome.

```cpp
#include "novu/ui/ModuleBar.h"
```

| | |
|---|---|
| Variants | `Full`, `Compact` |
| States | `Normal`, `Hover`, `Current`, `Disabled` |
| API | `currentKey`, `compact`, `countTone` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat module-bar` |
| Source | `src/ModuleBar.cpp` |

| property | type | notes |
|---|---|---|
| `currentKey` | `QString` | What moduleChosen() carries. |
| `compact` | `bool` | Glyphs only, with the label moved onto the tooltip. |
| `countTone` | `enum` | Amber when the count needs a person. |

### NavList

A whole sidebar navigation tree in one painted widget: glyphs, labels, counts, status dots, twisties, sections and a collapsed icon mode.

```cpp
#include "novu/ui/NavList.h"
```

| | |
|---|---|
| Variants | `Item`, `Child`, `Section` |
| States | `Normal`, `Hover`, `Current`, `Disabled`, `Expanded`, `Collapsed rail` |
| API | `currentKey`, `countTone`, `collapsed` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat nav-list` |
| Source | `src/NavList.cpp` |

| property | type | notes |
|---|---|---|
| `currentKey` | `QString` | Caller's own identifier; never shown, so it survives translation. |
| `countTone` | `enum` | Amber for a count that needs a person — the design's rule. |
| `collapsed` | `bool` | Hides labels, counts and twisties; centres the glyphs. |

### PageHeader

The band across the top of a page: a breadcrumb, a title, a wrapping description and the page's actions. The most repeated shape in the system.

```cpp
#include "novu/ui/PageHeader.h"
```

| | |
|---|---|
| Variants | `Title only`, `With description`, `With trail`, `Breadcrumb alone` |
| States | `Divider`, `No divider` |
| API | `path`, `description`, `divider` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat page-header` |
| Source | `src/PageHeader.cpp` |

| property | type | notes |
|---|---|---|
| `path` | `QStringList` | The full path; the last segment is dropped as redundant. |
| `description` | `QString` | Wraps, and the header grows to fit it. |
| `divider` | `bool` | Off for a header inside a card. |

### Popover

The menu that drops out of a button: captions, glyphs, a right-aligned meta column, checked and dangerous entries — painted, because QMenu cannot be this.

```cpp
#include "novu/ui/Popover.h"
```

| | |
|---|---|
| Variants | `Item`, `Group`, `Separator`, `Dangerous` |
| States | `Normal`, `Hover`, `Checked`, `Disabled` |
| API | `meta`, `checked`, `dangerous` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat popover` |
| Source | `src/Popover.cpp` |

| property | type | notes |
|---|---|---|
| `meta` | `QString` | The right-aligned note: a shortcut, a count, a version. |
| `checked` | `bool` | The accent, for an option in force. Not a radio group. |
| `dangerous` | `bool` | Ink and hover both go red, for the one entry to avoid. |

### SelectionBar

The tinted strip under a list saying how much is selected and what can be done with it. Hides itself at zero, which is the state every hand-rolled one got wrong.

```cpp
#include "novu/ui/SelectionBar.h"
```

| | |
|---|---|
| States | `Hidden`, `One`, `Many` |
| API | `count`, `noun` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuai |
| Playground | `run.bat selection-bar` |
| Source | `src/SelectionBar.cpp` |

| property | type | notes |
|---|---|---|
| `count` | `int` | Zero hides the bar entirely. |
| `noun` | `QString` | "file" becomes "3 files selected"; the plural is derived. |

### SettingsShell

The settings window as a shape: navigation, page host, headings, grouping and spacing. Owns no setting, no value and no persistence.

```cpp
#include "novu/ui/Settings.h"
```

| | |
|---|---|
| Variants | `With search`, `Grouped`, `Section only` |
| States | `Normal`, `Filtered`, `Invalid field` |
| API | `currentKey`, `searchEnabled`, `navigationWidth` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuai, novuexplorer, novuspace, novuadmin |
| Playground | `run.bat settings-shell` |
| Source | `src/Settings.cpp` |

| property | type | notes |
|---|---|---|
| `currentKey` | `QString` | Which page is showing. View state, so the shell keeps it. |
| `searchEnabled` | `bool` | A filter over the navigation by page title. |
| `navigationWidth` | `int` | One number, so nineteen pages agree. |

### Sidebar

The left rail: an identity head, a connection strip, a scrolling navigation region and a pinned action column. Only the navigation scrolls.

```cpp
#include "novu/ui/Sidebar.h"
```

| | |
|---|---|
| Variants | `Expanded`, `Collapsed`, `With blocks` |
| States | `Online`, `Degraded`, `Offline` |
| API | `collapsed`, `expandedWidth` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat sidebar` |
| Source | `src/Sidebar.cpp` |

| property | type | notes |
|---|---|---|
| `collapsed` | `bool` | Folds the rail to a strip of icons, lists included. |
| `expandedWidth` | `int` | The collapsed width is derived from the mark, not set. |

### StatusBar

The strip along the bottom of a window: session, context and transport, painted as addressable items rather than built from labels.

```cpp
#include "novu/ui/StatusBar.h"
```

| | |
|---|---|
| Variants | `Text`, `Value`, `Mono`, `Status`, `Separator` |
| States | `Online`, `Offline` |
| API | `slot`, `offline` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat status-bar` |
| Source | `src/StatusBar.cpp` |

| property | type | notes |
|---|---|---|
| `slot` | `enum` | Which end an item packs against. |
| `offline` | `bool` | Every transport light goes to danger at once. |

### TabBar

Open documents across the top of a window: per-tab colour and state, drag to reorder, a close box that is its own target, and a new-tab button.

```cpp
#include "novu/ui/TabBar.h"
```

| | |
|---|---|
| Variants | `Idle`, `Loading`, `Failed`, `Modified`, `Asleep` |
| States | `Current`, `Hover`, `Dragging`, `Shrunken` |
| API | `state`, `color`, `reorderable`, `newTabButton` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuexplorer, novuai, novuspace |
| Playground | `run.bat tab-bar` |
| Source | `src/TabBar.cpp` |

| property | type | notes |
|---|---|---|
| `state` | `enum` | One at a time. Two marks is a bug. |
| `color` | `QColor` | Held on the tab, so a reorder carries it. |
| `reorderable` | `bool` | Drag past a threshold, so a click is not a one-pixel drag. |
| `newTabButton` | `bool` | Not every tab bar opens things. |

### TabStrip

A row of buttons where exactly one is active. Not QTabWidget: a strip reports which item is current and leaves the layout to the page.

```cpp
#include "novu/ui/TabStrip.h"
```

| | |
|---|---|
| Variants | `Pill`, `Segment`, `Accent`, `Warning` |
| States | `Active`, `Inactive`, `Hover`, `Disabled` |
| API | `items`, `variant`, `tone`, `current` |
| Currently used by | novuspace |
| Designed for | novuspace, novuai, novuexplorer, novuhub, novuvoice, novutrade |
| Playground | `run.bat tab-strip` |
| Source | `src/TabStrip.cpp` |

| property | type | notes |
|---|---|---|
| `items` | `QStringList` | Captions, in order. |
| `variant` | `enum` | Pill is control height; Segment is compact. |
| `tone` | `enum` | Warning is for a switcher above accent-coloured chips. |
| `current` | `int` | Active index. |

### Tile

A selectable two-line tile: title, subtitle, optional badge. Self-painted, so the selected state is one fill and one border from the profile.

```cpp
#include "novu/ui/Tile.h"
```

| | |
|---|---|
| Variants | `Plain`, `Badged` |
| States | `Unchecked`, `Checked`, `Hover`, `Disabled (badged)` |
| API | `title`, `subtitle`, `checked`, `badge` |
| Currently used by | novuspace |
| Designed for | novuspace, novutrade |
| Playground | `run.bat tile` |
| Source | `src/Tile.cpp` |

| property | type | notes |
|---|---|---|
| `title` | `QString` | First line. |
| `subtitle` | `QString` | Second line, muted. |
| `checked` | `bool` | Selected state. |
| `badge` | `QString` | Non-empty disables the tile — the badge says why. |

### TreeView

A structure of nested things at arbitrary depth, with indent guides, per-row chevrons, marks, counts and tags — flattened and virtualised into one widget.

```cpp
#include "novu/ui/TreeView.h"
```

| | |
|---|---|
| Variants | `Item`, `Group`, `Emphasis` |
| States | `Current`, `Hover`, `Active`, `Detached`, `Dropping`, `Disabled` |
| API | `kind`, `mark`, `compact` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuai |
| Playground | `run.bat tree-view` |
| Source | `src/TreeView.cpp` |

| property | type | notes |
|---|---|---|
| `kind` | `enum` | Typography, and whether it is a caption. |
| `mark` | `enum` | State that is not selection. |
| `compact` | `bool` | A density for the whole tree; per-row heights would make every hit test a search. |


## Data display

### AppMark

The rounded plaque an application, module or workspace is identified by, at the design's three sizes — with the proportions derived rather than tabulated.

```cpp
#include "novu/ui/AppMark.h"
```

| | |
|---|---|
| Variants | `Small`, `Medium`, `Large`, `Initials` |
| States | `Accented`, `Plain` |
| API | `markSize`, `accented` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat app-mark` |
| Source | `src/AppMark.cpp` |

| property | type | notes |
|---|---|---|
| `markSize` | `enum` | 29, 30 and 58 pixels. |
| `accented` | `bool` | The product's own mark, or a generic one beside a row. |

### Avatar

Who someone is, in the space of one glyph. Portrait when there is one, initials when there is not, and a plate colour derived from the name so nothing stores one.

```cpp
#include "novu/ui/Avatar.h"
```

| | |
|---|---|
| Variants | `Round`, `Rounded`, `Portrait`, `Initials` |
| States | `Normal`, `Highlighted` |
| API | `name`, `diameter`, `shape` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuhub, novuvoice, novuai |
| Playground | `run.bat avatar` |
| Source | `src/Avatar.cpp` |

| property | type | notes |
|---|---|---|
| `name` | `QString` | Initials and plate colour are both derived from it. |
| `diameter` | `int` | The type scales with it; there is no size table. |
| `shape` | `enum` | A person is round; a workspace or a machine is not. |

### Badge

A pill-shaped label for status, tag, plan, release and price. Self-painted, so sizeHint() is correct from construction rather than after polish.

```cpp
#include "novu/ui/Badge.h"
```

| | |
|---|---|
| Variants | `Neutral`, `Success`, `Warning`, `Danger`, `Info`, `Accent` |
| States | `Normal` |
| API | `text`, `tone`, `size`, `dot`, `uppercase` |
| Currently used by | novuadmin, novuspace |
| Designed for | novuspace, novuai, novuexplorer, novuhub, novuvoice, novutrade |
| Playground | `run.bat badge` |
| Source | `src/Badge.cpp` |

| property | type | notes |
|---|---|---|
| `text` | `QString` | The whole width of the badge is measured from this. |
| `tone` | `enum` | Meaning, not colour. |
| `size` | `enum` | In a row, or beside a title. |
| `dot` | `bool` | A live status carries one; a classification does not. |
| `uppercase` | `bool` | Applied at paint time; text() still returns what was set. |

### Card

The one surface primitive: background, border, themed radius. Self-painted, and its padding lives on its own layout — there is no inner body widget.

```cpp
#include "novu/ui/Card.h"
```

| | |
|---|---|
| Variants | `Plain surface`, `Titled`, `Titled + subtitle`, `With header trailing` |
| States | `Normal`, `Empty`, `Populated` |
| API | `title`, `subtitle`, `headerVisible`, `headerTrailing` |
| Currently used by | novuspace |
| Designed for | novuspace, novuai, novuexplorer, novuhub, novuvoice, novutrade |
| Playground | `run.bat card` |
| Source | `src/Card.cpp` |

| property | type | notes |
|---|---|---|
| `title` | `QString` | Header title. |
| `subtitle` | `QString` | Muted second line stating what the data is. |
| `headerVisible` | `bool` | A card used as a plain surface drops the header entirely. |
| `headerTrailing` | `QList<QWidget*>` | Trailing widgets on the title row. The card owns the row, the caller owns the widgets. |

### DiffView

A unified diff: tinted added and removed rows, hunk headers in the accent, both sides' line numbers in a gutter. Parses the text format, renders nothing else.

```cpp
#include "novu/ui/DiffView.h"
```

| | |
|---|---|
| Variants | `Context`, `Added`, `Removed`, `Hunk`, `File` |
| States | `With line numbers`, `Without` |
| API | `showLineNumbers` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuai |
| Playground | `run.bat diff-view` |
| Source | `src/DiffView.cpp` |

| property | type | notes |
|---|---|---|
| `showLineNumbers` | `bool` | Off for a narrow panel, where the gutter costs more than it gives. |

### KeyCap

A key drawn as a key. Chords split into separate caps, sequences keep their comma, and the drawing is shared with the tooltip's shortcut chip.

```cpp
#include "novu/ui/Keys.h"
```

| | |
|---|---|
| Variants | `Single`, `Chord`, `Sequence` |
| API | `keys` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat key-cap` |
| Source | `src/Keys.cpp` |

| property | type | notes |
|---|---|---|
| `keys` | `QString` | "Ctrl+Shift+R", or a QKeySequence in the platform's spelling. |

### LogView

A stream of log lines with the level in its own column, filtered by level and by text. Paints only what is on screen and follows the tail unless you scroll away.

```cpp
#include "novu/ui/LogView.h"
```

| | |
|---|---|
| Variants | `Debug`, `Info`, `Warning`, `Error` |
| States | `Following`, `Held`, `Filtered`, `At the cap` |
| API | `minimumLevel`, `filter`, `maximumLines`, `following` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat log-view` |
| Source | `src/LogView.cpp` |

| property | type | notes |
|---|---|---|
| `minimumLevel` | `enum` | Filters the view; never discards what it is not showing. |
| `filter` | `QString` | Case-insensitive substring match against the message. |
| `maximumLines` | `int` | The oldest go first. |
| `following` | `bool` | Set by where the user has scrolled, not by the caller. |

### PropertyRow

Caption left, value right, pushed apart. For read-only detail panels — connection state, versions, ids and paths.

```cpp
#include "novu/ui/Form.h"
```

| | |
|---|---|
| Variants | `Default`, `Toned`, `Monospace` |
| States | `Populated`, `Empty`, `Error` |
| API | `caption`, `value`, `tone`, `mono` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace, novuai, novuexplorer, novutrade |
| Playground | `run.bat property-row` |
| Source | `src/Form.cpp` |

| property | type | notes |
|---|---|---|
| `caption` | `QString` | What the value is. |
| `value` | `QString` | The value. |
| `tone` | `enum` | Ink role for the value. |
| `mono` | `bool` | Monospace, for ids, hashes and paths. |

### ShortcutList

A keyboard reference as groups of labelled shortcuts, with a filter. The whole sheet is one widget.

```cpp
#include "novu/ui/Keys.h"
```

| | |
|---|---|
| Variants | `Group`, `Row` |
| States | `Unfiltered`, `Filtered`, `Empty` |
| API | `filter` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat shortcut-list` |
| Source | `src/Keys.cpp` |

| property | type | notes |
|---|---|---|
| `filter` | `QString` | Matches label or keys; empty captions are dropped. |

### StatCard

A caption and one large number, plus MiniStat and statGrid() for a metric grid inside a card that is already a surface.

```cpp
#include "novu/ui/StatCard.h"
```

| | |
|---|---|
| Variants | `StatCard`, `MiniStat`, `statGrid()` |
| States | `Default`, `Success`, `Warning`, `Danger`, `Empty` |
| API | `caption`, `value`, `tone`, `columns` |
| Currently used by | novuspace |
| Designed for | novuspace, novuai, novutrade |
| Playground | `run.bat stat-card` |
| Source | `src/StatCard.cpp` |

| property | type | notes |
|---|---|---|
| `caption` | `QString` | What the number is. |
| `value` | `QString` | The number. setValue() updates in place. |
| `tone` | `enum` | Ink role for the value. |
| `columns` | `int` | statGrid() column count. |

### StatusDot

The dot that says something is live. Breathes on the shared clock, cross-fades between states, and takes a tone rather than a colour.

```cpp
#include "novu/ui/StatusDot.h"
```

| | |
|---|---|
| Variants | `Neutral`, `Success`, `Warning`, `Danger`, `Info`, `Accent` |
| States | `Still`, `Pulsing` |
| API | `tone`, `pulsing`, `diameter` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat status-dot` |
| Source | `src/StatusDot.cpp` |

| property | type | notes |
|---|---|---|
| `tone` | `enum` | Meaning. Changing it cross-fades. |
| `pulsing` | `bool` | Whether this one is reporting something live. |
| `diameter` | `int` | The dot; the halo sits outside it, so layout does not move. |

### TableView

The table used across all Novu applications. It owns column layout entirely: a page declares what each column is, never how wide it becomes.

```cpp
#include "novu/ui/TableView.h"
```

| | |
|---|---|
| Variants | `Text column`, `Content column` |
| States | `Empty`, `Populated`, `Error`, `Disabled row actions` |
| API | `columns`, `ColumnRole` |
| Currently used by | novuadmin, novuspace |
| Designed for | novuspace, novuexplorer, novuhub, novutrade |
| Playground | `run.bat table-view` |
| Source | `src/TableView.cpp` |

| property | type | notes |
|---|---|---|
| `columns` | `QVector<QPair<QString,ColumnRole>>` | Caption and role per column. Declared once, never inferred. |
| `ColumnRole` | `enum` | Text absorbs leftover width; Content is exactly as wide as its widest cell. |


## Feedback

### Dialog

A question wearing the Novu frame: a tone-marked head, a body, and a footer whose button order is the component's decision rather than the page's.

```cpp
#include "novu/ui/WindowShell.h"
```

| | |
|---|---|
| Variants | `Neutral`, `Accent`, `Warning`, `Danger` |
| States | `confirm()`, `inform()`, `reportError()` |
| API | `tone`, `note` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace, novuai |
| Playground | `run.bat dialog` |
| Source | `src/WindowShell.cpp` |

| property | type | notes |
|---|---|---|
| `tone` | `enum` | What kind of question this is, before the title is read. |
| `note` | `QString` | Wraps, and the head grows to fit it. |

### EmptyState

What a panel shows instead of a table when there is nothing to show: a glyph, what is missing, and what would make it appear.

```cpp
#include "novu/ui/Feedback.h"
```

| | |
|---|---|
| Variants | `Glyph + title + subtitle` |
| States | `Normal` |
| API | `glyph`, `title`, `subtitle` |
| Currently used by | novuspace |
| Designed for | novuspace, novuai, novuexplorer, novutrade |
| Playground | `run.bat empty-state` |
| Source | `src/Feedback.cpp` |

| property | type | notes |
|---|---|---|
| `glyph` | `QString` | Segoe Fluent / MDL2 code point. |
| `title` | `QString` | What is missing. |
| `subtitle` | `QString` | What would make it appear. Not optional in practice. |

### FeedbackForm

Send feedback: type, title, description, and an explicit account of what else is attached. Produces a FeedbackRequest and hands it to the consumer — NovuUI knows nothing about transport.

```cpp
#include "novu/ui/FeedbackForm.h"
```

| | |
|---|---|
| Variants | `Bug`, `Suggestion`, `General`, `FeedbackDialog` |
| States | `Empty`, `Editing`, `Validation error`, `Sending`, `Success`, `Submission error` |
| API | `context`, `type`, `includeApplicationInfo`, `includeLogs`, `state` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | — |
| Playground | `run.bat feedback-form` |
| Source | `src/FeedbackForm.cpp` |

| property | type | notes |
|---|---|---|
| `context` | `FeedbackContext` | Application, version, platform, workspace and an optional log callback. Supplied by the consumer; never discovered here. |
| `type` | `enum` | Chosen with a TabStrip. |
| `includeApplicationInfo` | `bool` | On by default, and the values it would send are listed underneath. |
| `includeLogs` | `bool` | Off by default. Disabled with an explanation when the consumer supplies no log callback. |
| `state` | `enum` | Sending locks the fields and refuses a second submit. |

### HelpOverlay

An explanatory layer over a live window: coloured bubbles joined to what they explain by dashed leaders. The application underneath keeps working.

```cpp
#include "novu/ui/HelpOverlay.h"
```

| | |
|---|---|
| Variants | `Accent`, `Info`, `Success`, `Warning`, `Danger`, `Neutral` |
| States | `Shown`, `Hidden`, `Target hidden`, `Clamped to frame` |
| API | `title`, `body`, `placement`, `tone`, `noteWidth` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | — |
| Playground | `run.bat help-overlay` |
| Source | `src/HelpOverlay.cpp` |

| property | type | notes |
|---|---|---|
| `title` | `QString` | The note's heading, drawn in the note's own colour. |
| `body` | `QString` | One or two sentences. Wraps; the bubble grows to fit. |
| `placement` | `enum` | Auto measures the free space on each side and takes the largest. |
| `tone` | `enum` | Border, title and leader. Colours the semantic palette; Accent is the application's. |
| `noteWidth` | `int` | One width for every note: bubbles sized to their own sentence read as clutter. |

### InfoBanner

A tinted strip carrying one sentence of context, optionally with an action. Painted rather than styled, so a tone costs no stylesheet rule.

```cpp
#include "novu/ui/Feedback.h"
```

| | |
|---|---|
| Variants | `Neutral`, `Success`, `Warning`, `Danger`, `Info`, `With trailing action` |
| States | `Normal`, `Disabled action` |
| API | `text`, `tone`, `trailing` |
| Currently used by | novuspace |
| Designed for | novuspace, novuai, novuexplorer, novutrade |
| Playground | `run.bat info-banner` |
| Source | `src/Feedback.cpp` |

| property | type | notes |
|---|---|---|
| `text` | `QString` | One sentence. |
| `tone` | `enum` | Tint and text colour. |
| `trailing` | `QWidget*` | Optional action, owned by the caller. |

### ProgressBar

A proportion: how much is done, or how much is used. Capacity bars colour themselves by threshold; an unmeasured reading is drawn differently from zero.

```cpp
#include "novu/ui/Feedback.h"
```

| | |
|---|---|
| Variants | `Accent`, `Threshold`, `Success`, `Warning`, `Danger` |
| States | `0%`, `Partial`, `100%`, `Running`, `Unknown`, `Error` |
| API | `value`, `tone`, `unknown`, `legend` |
| Currently used by | novuspace |
| Designed for | novuspace, novuexplorer, novuhub, novutrade |
| Playground | `run.bat progress-bar` |
| Source | `src/Feedback.cpp` |

| property | type | notes |
|---|---|---|
| `value` | `int` | 0..100. Clamped, and only repaints on a real change. |
| `tone` | `enum` | Threshold goes amber past 75% and red past 90%. |
| `unknown` | `bool` | An empty track and no fill: not measured, which is not idle. |
| `legend` | `QString` | Used on the left, left on the right. Adds a line to the height. |

### Skeleton

Placeholder plates with the design's shimmer sweeping across them, for when the shape of what is coming is already known. SkeletonList draws a whole table in one widget.

```cpp
#include "novu/ui/Feedback.h"
```

| | |
|---|---|
| Variants | `Text`, `Block`, `SkeletonList` |
| States | `Shimmering`, `Still` |
| API | `line`, `fill`, `columns` |
| Currently used by | novuspace |
| Designed for | novuspace |
| Playground | `run.bat skeleton` |
| Source | `src/Feedback.cpp` |

| property | type | notes |
|---|---|---|
| `line` | `enum` | One line of prose, or a value / heading / control. |
| `fill` | `double` | 0..1 of the width. Vary it, or the plates read as a grid. |
| `columns` | `QVector<double>` | SkeletonList only. Normalised, so  and  agree. |

### Spinner

A faint ring with one bright quarter turning round it, for work whose length is not known. On the shared clock, and it stops when it is told to.

```cpp
#include "novu/ui/Feedback.h"
```

| | |
|---|---|
| Variants | `Small`, `Medium`, `Large` |
| States | `Spinning`, `Stopped` |
| API | `controlSize`, `spinning`, `headColor` |
| Currently used by | novuspace |
| Designed for | novuspace |
| Playground | `run.bat spinner` |
| Source | `src/Feedback.cpp` |

| property | type | notes |
|---|---|---|
| `controlSize` | `enum` | Resolved through the profile's icon metrics. |
| `spinning` | `bool` | False releases the clock subscription, not only the drawing. |
| `headColor` | `QColor` | An override, for a spinner inside a Primary button. |

### StatusLine

A sentence about one operation with a mark in front of it saying where that operation is. The design's .sline.

```cpp
#include "novu/ui/Feedback.h"
```

| | |
|---|---|
| Variants | `Idle`, `Running`, `Done`, `Failed` |
| States | `Idle`, `Running`, `Done`, `Failed` |
| API | `state`, `text` |
| Currently used by | novuspace |
| Designed for | novuspace |
| Playground | `run.bat status-line` |
| Source | `src/Feedback.cpp` |

| property | type | notes |
|---|---|---|
| `state` | `enum` | Where the operation is. Only Running animates. |
| `text` | `QString` | Set with the state, because they always change together. |

### Steps

Where a multi-part operation has got to, as a row of tracked pills. The one progress shape that says which part rather than how far.

```cpp
#include "novu/ui/Feedback.h"
```

| | |
|---|---|
| Variants | `Done`, `Current`, `Pending` |
| States | `In progress`, `Complete` |
| API | `currentIndex` |
| Currently used by | novuspace |
| Designed for | novuspace |
| Playground | `run.bat steps` |
| Source | `src/Feedback.cpp` |

| property | type | notes |
|---|---|---|
| `currentIndex` | `int` | count() is legal and means finished. |

### Toast

The line that says something happened and then goes away. Confirms an outcome the user expected; refuses to be anything a user has to act on.

```cpp
#include "novu/ui/Toast.h"
```

| | |
|---|---|
| Variants | `Neutral`, `Success`, `Warning`, `Danger`, `With an action` |
| States | `Rising`, `Held`, `Stacked` |
| API | `tone`, `duration`, `actionText` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuai |
| Playground | `run.bat toast` |
| Source | `src/Toast.cpp` |

| property | type | notes |
|---|---|---|
| `tone` | `enum` | The mark. Most toasts need no colour. |
| `duration` | `int` | 0 scales with the length of the message. |
| `actionText` | `QString` | Taking the action dismisses the toast. |

### Tooltip

The branded hover tip, in two modes. Off it gives the name and the key; on it explains the control, says what it is doing, and offers the long form.

```cpp
#include "novu/ui/Tooltip.h"
```

| | |
|---|---|
| Variants | `Brief`, `Full`, `Compact` |
| States | `Help off`, `Help on`, `Detail shown` |
| API | `title`, `shortcut`, `brief`, `status`, `detail`, `compact` |
| Currently used by | *nothing yet — built, not adopted* |
| Designed for | novuspace |
| Playground | `run.bat tooltip` |
| Source | `src/Tooltip.cpp` |

| property | type | notes |
|---|---|---|
| `title` | `QString` | The control's name — .tip-t. |
| `shortcut` | `QString` | The key that reaches it, in a chip — .tip-t .sc. |
| `brief` | `QString` | One line on what it does — .tip-s. |
| `status` | `QString` | What it is doing now, in the accent — .tip-st. |
| `detail` | `QString` | The long form, in help mode after a second delay — .tip-d. |
| `compact` | `bool` | A tip about a row: the same card, one step smaller. |

