# NovuUI Component Gallery

A standalone development tool that renders the **real** NovuUI components —
the same classes an application links, built from the same `Novu::UI` target.
It is not a mockup renderer and it contains no reimplementation of anything in
`src/`.

```
edit a NovuUI component  →  Rebuild & reload  →  the change is on screen
edit sample data / theme →  saved             →  the change is on screen
```

---

## Running it

The gallery is built only when NovuUI is the project being configured:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=<Qt>/msvc2022_64
cmake --build build --config Release
build/playground/Release/NovuUIPlayground
```

`--list` prints the registry without opening a window. `--component <id>
--scenario <key> --profile <id>` opens on a specific preview — the same
arguments the gallery writes for itself when it restarts after a build.

An application that consumes this repository with `add_subdirectory()` gets
exactly one target, `NovuUI`. See *Development boundary* below.

---

## What is in it

| | |
|---|---|
| **Foundation** | Typography, Theme roles, Layout, FlowLayout |
| **Input** | Button, Fields, Chip / ChipBar |
| **Navigation** | TabStrip, Tile |
| **Data display** | Badge, Card, StatCard, TableView |
| **Feedback** | EmptyState, InfoBanner, ProgressBar |

Not previewable, and deliberately so: `StyleSheet` (`styleSheetFor()`,
`applyStyleSheet()`, `resolveRadius()`, `repolish()`), `ThemeManager`,
`ThemeScope` and the legacy `Theme` global are infrastructure with no visual
surface of their own. They are not absent from the gallery — every page is
drawn through them, and the *Theme roles* page reads its numbers out of the
live profile.

---

## Architecture

Six concepts, kept apart on purpose, because a UI designer built on top of
this will need to recombine them:

```
ComponentSpec     what a component is: metadata + properties + a builder
Scenario          the situation it is being shown in
SampleData        the data that situation supplies
ThemeProfile      the profile it is drawn with          (NovuUI's own)
PreviewContext    the four of them, handed to a builder
GalleryWindow     navigation, search, status — knows nothing about any page
```

A page receives a `PreviewContext` and returns a widget. It never reaches for
a singleton, never touches the navigation, and never knows a gallery exists.
That is what lets the tests build all sixteen pages against a fixture
directory, and what will later let an inspector build one page twice with two
different property sets.

```
playground/
  include/novu/playground/   public headers of the playground library
  src/
    ComponentRegistry.cpp    the catalog
    Components.cpp           the one list of every component
    SampleData.cpp           JSON fixtures + scenario shaping
    ThemeCatalog.cpp         overlay profiles, registered with the real manager
    GalleryWindow.cpp        the shell
    BuildBridge.cpp          incremental build + restart
    DevWatcher.cpp           source vs data change, kept distinct
    ExampleLayout.cpp        arrangement helpers — no new visual primitive
    pages/                   one file per component
  data/
    sample_data/*.json       development data. Never installed.
    themes/*.json            theme overlays, reloaded without a rebuild
```

### The gallery is built from NovuUI

The navigation entries are `Tile`s. The search box is `lineEdit()`. The
actions are `Button`s. The build status is a `Badge`. A failure is an
`InfoBanner`. There is no `GalleryButton`, no `GalleryCard`, and there will
not be one — a tool built from the library it previews finds the library's
bugs before an application does.

`ExampleLayout` is the one shared file, and it creates no visual primitive:
every pixel it produces comes from `Card`, `section()`, `caption()`, `label()`
and `FlowLayout`. It follows NovuUI's own convention of returning a layout
wherever a layout is enough.

---

## Adding a component to the gallery

Four lines, none of them in the navigation, the search or the theme code:

1. `src/pages/FooPage.cpp` — a `build()` returning a widget, and a
   `fooComponent()` returning a `ComponentSpec`.
2. Declare `fooComponent()` in `src/pages/Pages.h`.
3. Add `&pages::fooComponent` to the list in `src/Components.cpp`.
4. Add the file to `playground/CMakeLists.txt`.

The registry is explicit rather than self-registering: a static registrar in
each translation unit can be dropped by the linker from a static library, and
the component would then be missing from exactly the build nobody checks.

Fill in `variants`, `states`, `keywords`, `properties` and both source paths.
The tests assert that the paths exist, so a moved file is caught rather than
becoming a dead button.

---

## Scenarios

| | |
|---|---|
| Normal | the data a page sees on a good day |
| Empty | nothing to show — the component must say so, not collapse |
| Single item | exposes layouts that only work in a list |
| Many items | enough rows to scroll and to wrap |
| Long text | a field far longer than the design assumed |
| Long identifier | an unbreakable token — a hash, a path, an order id |
| Missing optional data | optional fields absent, as an API actually returns them |
| Error | the load failed; the component shows why, not blankness |
| Disabled | every control disabled |
| Large dataset | enough rows that a per-row measurement becomes visible |

A page declares which of these it implements; the selector offers exactly
those. Shaping happens in `SampleData::rowsFor()`, so every data-driven
component is stressed the same way.

---

## Theme

The gallery uses the real `ThemeManager`, `ThemeProfile` and `ThemeScope`. It
reproduces no colour locally — the *Theme roles* page reads every number out
of the live profile, which is why switching the selector changes it.

The preview surface is a separate scope from the chrome. The developer
switches the profile of what they are inspecting; the frame around it stays
on the default profile, so there is always an unchanged reference on screen.

Switching rebuilds the page as well as repainting it: a `ThemeChangeEvent` is
enough for a self-painted component, but NovuUI components also read geometry
tokens at construction — control heights, paddings, fonts.

### Overlays

`data/themes/*.json` is a JSON patch over a base profile:

```json
{
  "id": "novuspace-contrast",
  "displayName": "NovuSpace — high contrast (dev overlay)",
  "base": "novuspace",
  "colors": { "accent": "#3DA9FC" },
  "radii":  { "medium": 2 },
  "spacing": { "md": 14 },
  "controls": { "height": 36 }
}
```

Save the file and the gallery re-registers it through
`ThemeManager::registerProfile()`, which repaints every live widget in the
scope. No rebuild, no restart.

An overlay may **not** take the id of a built-in profile. The gallery can run
in a process that also hosts real product widgets, and an overlay that
captured `novutrade` would restyle them. A refused overlay is reported in the
problem strip, not applied.

---

## Live preview workflow

| change | what happens |
|---|---|
| sample data JSON | reloaded, page rebuilt, **no compilation** |
| theme overlay JSON | profile re-registered, widgets repainted, **no compilation** |
| NovuUI `.h` / `.cpp` | status goes to *Rebuild needed*; press **Rebuild & reload** |

There is no C++ hot reload here and the tool does not pretend otherwise.
Replacing a static library under a running process with live `QWidget`s built
from it is not something Qt or the loader supports. What is achievable is a
fast incremental build followed by a restart that lands on the same component,
scenario and profile — about a second, and it is what the button does.

The watcher marks source changes rather than building them. An automatic
compiler on every keystroke is how a development tool becomes something you
turn off.

### Build status

`Ready · Rebuild needed · Building · Reloading · Build failed`, shown as a
`Badge`. On failure the preview is **not** replaced: what stays on screen is
the last render that compiled, which is the thing being compared against. The
error goes into the banner and the log strip.

### Environment overrides

| | |
|---|---|
| `NOVUUI_REPO_DIR` | the checkout being previewed |
| `NOVUUI_BUILD_DIR` | the build tree to rebuild |
| `NOVUUI_PLAYGROUND_DATA` | sample data + theme overlays |
| `NOVUUI_EDITOR` | command used by **Open source** (`code -g`, `devenv /edit`, …) |

All four default to values baked in at configure time, so a freshly built
gallery runs with nothing set up. They exist so one built gallery can preview
a second checkout — a worktree, a branch under review.

---

## Development boundary

`NOVUUI_BUILD_PLAYGROUND` and `NOVUUI_BUILD_TESTS` both default to
`PROJECT_IS_TOP_LEVEL`. NovuAdmin and NovuSpace consume this repository with
`add_subdirectory()`, so the default is derived from whether NovuUI is the
project actually being built rather than from a flag a consumer has to
remember to switch off.

Verified rather than asserted: configuring a directory that only does
`add_subdirectory(NovuUI)` yields exactly one target, `NovuUI`. Two tests
enforce the rest — that the library target lists no playground source, that
`add_subdirectory(playground)` is guarded, and that nothing under `src/` or
`include/` includes a playground header or mentions sample data.

Sample data is never installed and never copied next to a binary. It is read
from the source tree, which is also what makes editing a JSON file show up in
a running gallery.

---

## Tests

`tests/tst_gallery.cpp`, run with `ctest` or directly. It covers the registry
(populated, unique ids, complete metadata, source paths that exist),
instantiation of every component in every scenario **and** every profile,
search, scenario round-tripping, sample-data loading and shaping, graceful
failure on a missing or malformed fixture, theme switching through the real
manager, overlay registration and overlay refusal, and the development-only
boundary.

There is deliberately no screenshot comparison: NovuUI has no image-diff
infrastructure, and introducing one here would make the gallery depend on a
baseline nobody maintains.

---

## What this is the foundation for

`ComponentSpec::properties` is populated and read by nothing yet. It exists
because a property inspector retrofitted onto a registry with no notion of
properties means rewriting every page. The next steps, in order:

1. **Property inspector** — render `PropertySpec` as controls, add an
   optional `applyProperties()` to the builder contract.
2. **Per-example selection** — a page currently returns one widget; the
   inspector needs to know which widget in it is selected.
3. **UI designer** — composition on top of the same registry.

Out of scope here, and left that way on purpose: drag-and-drop construction,
generated C++, runtime plugin loading, production integration.
