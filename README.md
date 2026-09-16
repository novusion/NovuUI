# NovuUI

The shared Qt6 component library every Novusion application draws itself with.

A control exists here once, or it does not exist. Nothing in this library knows
which application is consuming it: there is one `Button`, and NovuSpace's amber
and NovuAI's cyan are two `ThemeProfile`s rather than two classes.

**Status: feature extraction is frozen.** 67 components, 164 tests, clean build.
The library is ready; the applications have not migrated onto it yet. Those are
two different statements and this repository keeps them apart —
[EXTRACTION_STATUS](docs/EXTRACTION_STATUS.md) has the numbers.

| | |
|---|---|
| [docs/COMPONENTS.md](docs/COMPONENTS.md) | every component, generated from the gallery |
| [docs/DESIGN_CONTRACT.md](docs/DESIGN_CONTRACT.md) | every token, sourced against the four authoritative mockups |
| [docs/THEMING.md](docs/THEMING.md) | how a component gets its colours |
| [docs/PORTING_GUIDE.md](docs/PORTING_GUIDE.md) | building an application on this |
| [docs/EXTRACTION_STATUS.md](docs/EXTRACTION_STATUS.md) | coverage, deferrals, known limitations |
| [docs/DUPLICATION_MAP.md](docs/DUPLICATION_MAP.md) | the second library next door, and the plan |
| [docs/MODULE_RULES.md](docs/MODULE_RULES.md) | what belongs here versus in a feature module |

---

## What belongs here

**Yes:** reusable presentation · reusable interaction behaviour · semantic
styling · shared motion · generic structural composites (a settings shell, a
console panel, a window frame).

**No:** business logic · persistence · services · file, transfer or chat models ·
runtime state · domain state · backend communication · anything that knows the
name of a product feature.

The test is not "could this be reused" but "is it reused". A component is
*extracted* from an application that has it, not written here as a look-alike —
see [MODULE_RULES](docs/MODULE_RULES.md). There is a second shared library
beside this one for exactly the reason that rule exists; check
[DUPLICATION_MAP](docs/DUPLICATION_MAP.md) before adding anything.

**This library is the target.** `NovusionUI` (`novu::brand`) is the original
shared library and still what NovuHub and NovuExplorer run on. New projects and
mockups use `NovuUI` only and add no new dependency on it —
[the target state](docs/DUPLICATION_MAP.md#the-decided-target-state).

---

## Using it

```cmake
add_subdirectory(../NovuUI NovuUI)
target_link_libraries(YourApp PRIVATE Novu::UI)
```

```cpp
#include "novu/ui/Button.h"
#include "novu/ui/StyleSheet.h"

using namespace novu::ui;

applyStyleSheet(window, QStringLiteral("novuai"));   // scope the subtree

auto* save = primaryButton(tr("Save"), form);
save->setGlyph(Glyph::Check);
```

`applyStyleSheet()` does two things: it composes the profile's stylesheet onto
the subtree, and it marks the subtree as belonging to that profile so
`themeFor()` resolves correctly inside it. One process can host several
products; nothing here is global.

A whole window:

```cpp
AppWindow window;
window.setTitle(tr("NovuAI"));
window.setIcon(glyphIcon(Glyph::Ai, 18, themeFor(&window).window.titleText));
window.setContentWidget(new Workspace);
window.show();
```

`AppWindow`, `ToolWindow` and `Dialog` wear the same frame. An application says
what a window *is* — title, icon, resizable, modal — and never draws chrome.

---

## Repository

```
include/novu/ui/     public headers, one per component
src/                 implementations
  GlyphTable.inc     the 67 icon outlines, copied from the approved design
playground/          the component gallery — the catalogue and the visual test surface
  src/pages/         one file per component page
tests/               tst_gallery, tst_window_chrome
docs/                the documents linked above
tools/               generate_catalogue.py
run.bat              build, test, list, or open the gallery
```

```
run.bat              build if needed, then open the gallery
run.bat button       open it on a component
run.bat build        build only
run.bat tests        build, then run both suites
run.bat list         print the component registry
run.bat themes       print the theme profiles
```

---

## The contract a component signs

A component owns **all** of its own presentation and geometry — padding,
spacing, font, colours, radius, `sizeHint()`, `minimumSizeHint()`, size policy.

A page owns **none** of it. A page places components into layouts. It must never
`setFixedWidth()` a component, measure text with `QFontMetrics` to size one, set
a stylesheet on one, or compute column widths. If a page needs a size a
component does not provide, the fix goes into the component.

Geometry is computed in `sizeHint()` rather than left to the stylesheet: Qt does
not fold QSS padding into `sizeHint()` until a widget happens to be polished,
and anything measured before that reads a too-small hint and clips.

### Composition

Composites are built from primitives and may not re-implement them. `Settings`
is `NavList` + `PageHeader` + `SettingRow` + `Separator`; `BottomPanel` is
`TabBar` + `IconButton`. A composite that draws its own tab is a bug.

Runs of items — `Segmented`, `ModuleBar`, `NavList`, `TabBar` — derive from
`ItemStrip`, which owns hit testing, hover, press-and-release, arrow keys and
per-item tooltips. There is one implementation of that, not four.

---

## Styling

The short version; [THEMING](docs/THEMING.md) is the long one.

```cpp
StyleSpec spec;
spec.background = statusToken(SemanticRole::Danger, ColorRole::StatusFill);
spec.radius     = SurfaceRole::Button;

const ResolvedStyle style = resolve(spec, this, height());
painter.setBrush(style.background);
```

A component holds **token references** and resolves them on demand. It never
stores a `QColor` that came out of a profile — that is what makes a theme change
a repaint instead of a rebuild, and it is asserted by
`componentsKeepTokensRatherThanResolvedColours()`.

### Variants and states, not classes

Colour, accent, icon, text, size, state and theme are **never** reasons for a
new component:

```cpp
new Badge(text, Badge::Tone::Success, parent);   // yes
class SuccessBadge : public Badge                // no
```

Six badge tones, two sizes, a dot and an uppercase flag are one `Badge` — which
is why the approved design's nine badge CSS classes are one class here.

---

## Painting versus widgets

Widget count is a budget. HTML DOM structure is not QWidget structure.

**Paint it** when it is decoration: labels nobody selects, separators, dots,
badges, glyphs, captions, static metadata, section headings, segment labels.
A settings window of twenty sections would otherwise carry forty extra
`QLabel`s.

**Make it a widget** when there is real interaction to own: focus, keyboard,
text selection, context menu, drag and drop.

A painted component that *is* interactive owns its hit testing properly — and
acts on **press and release in the same item**. Releasing somewhere else is a
cancelled gesture, and a control that fires on release alone fires after the
user has changed their mind. Taking the press also matters inside a title bar,
where ignoring it hands the gesture to the window and the window moves.

---

## Motion

One 30 fps clock for every looping animation in the process:

```cpp
Pulse::instance().subscribe(this);        // repainted while visible
motion::phase(profile.animation.ringMs);  // 0..1, from the monotonic clock
motion::breathe(profile.animation.pulseMs);
```

Reference-counted — the timer runs only while something is subscribed, and
hidden widgets are skipped. Phases come off the wall clock, so every dot in a
window breathes together the way CSS keyframes do.

One-shot transitions use `QVariantAnimation`, which Qt already drives from a
single timer. **No component owns a `QTimer`.** Periods are tokens
(`animation.ringMs`, `pulseMs`, `spinMs`, `shimmerMs`), and a profile with
`normalMs == 0` disables motion everywhere.

---

## Testing

`tests/tst_gallery.cpp` and `tests/tst_window_chrome.cpp`. Run with
`run.bat tests`.

There is **no screenshot comparison** and none is wanted. A painted component is
tested through its contract:

```cpp
QCOMPARE(button->styleSpec().background.describe(), QStringLiteral("accent"));
QCOMPARE(button->resolvedStyle().background.name(), profile.colors.accent.name());
```

Test behaviour that matters — what a component means, what it resolves to, what
it does on press and release, what it emits and what it refuses to decide for
its host. Not that a method returns what it was given.

Interaction is tested for real: `QTest::mousePress` / `mouseRelease` on the
rects a component exposes (`segmentRect()`, `handleRect()`, `swatchRect()`),
which is why those accessors are public.

---

## Adding a component

1. **Check it does not already exist** — here, and in the library recorded in
   [DUPLICATION_MAP](docs/DUPLICATION_MAP.md).
2. **Check it is not a variant** of something that does.
3. `include/novu/ui/Foo.h` + `src/Foo.cpp`, added to `CMakeLists.txt`.
   Expose `styleSpec()` and `resolvedStyle()` if it paints.
4. `playground/src/pages/FooPage.cpp` returning a `ComponentSpec` — declared in
   `pages/Pages.h`, listed in `Components.cpp`, added to
   `playground/CMakeLists.txt`. Show meaningful states and variants, with the
   real component; never a gallery-only mock.
5. `python tools/generate_catalogue.py`.
6. Tests for the contract.
7. `run.bat tests`.

Set `designedFor` to the applications it is meant for. Leave `usedBy` empty
until an application actually includes the header — that field is measured, and
a test fails the build if it claims an application that has not migrated.
