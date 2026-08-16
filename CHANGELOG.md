# Changelog

All notable changes to NovuUI will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

Foundation work for NUI-002 — making NovuUI the single source of truth for
desktop UI. Everything here is additive; existing consumers link unchanged.

### Added

- **Theme profiles.** `ThemeProfile` describes one product's complete visual
  language: colours, radius scale, spacing, typography, control metrics,
  borders, chart tokens, table tokens, animation durations and icon sizes. Two
  profiles ship: `novuspace` (graphite, near-flat) and `novutrade` (slate and
  emerald from the trading web app, rounded).
- **Theme scopes.** `ThemeScope::apply(root, profileId)` marks a widget subtree
  as belonging to a profile; `themeFor(widget)` resolves it by walking up the
  parents. This replaces the single process-wide theme, which could not work
  once NovuSpace hosted several products as modules in its own process.
- **Token roles.** Components name what they are — `SurfaceRole::Card`,
  `TypeRole::CardTitle`, `SpacingRole::CardPadding`, `BorderRole::Input` — and
  the profile decides the value. No component contains a colour, radius,
  spacing, font size or border width.
- **Live theme changes.** `ThemeManager::registerProfile()` broadcasts a
  `ThemeChangeEvent` to every affected live widget. No restart, and
  deliberately no registry of themed components. This is the runtime the
  planned UI Designer edits against.
- **`Card`** — the one surface primitive, self-painted, themed radius, padding
  on its own layout. Replaces card helpers that created a frame plus an inner
  body widget whose only purpose was padding.
- **`section()`, `toolbar()`, `pageLayout()`, `splitHeader()`** — layout
  constructs that create no widget at all, and **`scrollPage()`**, which
  creates only the viewport a `QScrollArea` requires.
- `docs/DESIGN_SYSTEM.md` and `docs/ROADMAP.md`.

### Added — development only

- **Component gallery** (`playground/`). A standalone executable that renders
  the real NovuUI components through the real `Novu::UI` target — no
  reimplementation, no mockup. Sixteen previewable components across five
  categories, ten layout-stress scenarios, search, per-component metadata,
  "Open source", live theme switching through the real `ThemeManager`, and a
  build-and-reload action with an explicit `Ready / Rebuild needed / Building /
  Reloading / Build failed` state. A failed build keeps the last render that
  compiled. The gallery's own chrome is built from NovuUI, so a regression in
  `Tile`, `Badge`, `Button` or `InfoBanner` shows up the moment it opens.
  See `playground/README.md`.
- **Sample data and theme overlays** as JSON under `playground/data/`. Editing
  either is a reload, not a rebuild: a theme overlay is applied by handing a
  `ThemeProfile` back to `registerProfile()`. An overlay may not take the id of
  a built-in profile.
- **`tests/tst_gallery.cpp`** — registry completeness and uniqueness,
  instantiation of every component in every scenario and every profile, search,
  graceful failure on missing or malformed fixtures, theme switching through
  the real manager, and the development-only build boundary.
- `NOVUUI_BUILD_PLAYGROUND` and `NOVUUI_BUILD_TESTS`, both defaulting to
  `PROJECT_IS_TOP_LEVEL`. An application that consumes this repository with
  `add_subdirectory()` gets exactly one target, `NovuUI` — unchanged.

### Changed

- `Button` reads the theme profile instead of the process-wide `theme()`, and
  gained a `Success` variant and `setControlSize()`. Colour, radius, control
  height and type size now all come from the profile, so a primary button in
  NovuTrade is the profile's emerald rather than the graphite accent that the
  global theme happened to hold.
- `Badge` and `TableView` likewise read the profile. These three were written
  before profiles existed and were the only components still bound to the
  global palette — which is why a module could look themed everywhere except
  its buttons, badges and tables.
- `Card` builds its label fonts through `roleFont()` rather than editing the
  label's inherited font. The old path never named a family, so a card title
  took its family from the application default and only its size and weight
  from the profile.
- `withAlpha(QColor, double)` in `Theme.h`, for components that hand a tint to
  a painter rather than to a stylesheet.

### Fixed

- A card title lost the font its card had set. Applying a scope stylesheet to
  a page re-polishes its widgets and drops fonts assigned during construction,
  so the title rendered at the default size and weight while every other label
  on the page kept its role. `Card` now re-applies on `PolishRequest` and
  `ParentChange`, as `Button` does. A hidden subtitle kept its font throughout,
  because a hidden widget is not polished — which is what made the cause
  visible.
- `novutrade` profile: `SurfaceRole::Button` resolved to 8px and
  `SurfaceRole::Input` to 6px, against the approved mockup's 6px and 4px. The
  comments beside them claimed the values the scale did not have.
- `novutrade` profile: `table.headerHeight` was 36 where the mockup draws the
  header as one more 34px row of the grid.
- `novutrade` profile: `success` is `#16a34a`. `#22c55e` read as a grassy
  yellow-green against the slate surfaces.
- `Badge`'s neutral tone was tinted at 0.12 where every other tone uses 0.16,
  so it read as a bare word beside a row of filled pills.

## [0.1.0] - 2026-08-05

Initial release. Phase 1 of the shared native UI framework: the single source of
truth for reusable controls across all Novu applications.

### Added

- `Novu::UI` CMake target — a standalone Qt 6 static library with no
  application-specific code.
- `Theme` — design tokens (colours, spacing, typography) from
  `NOVU_DESIGN_SYSTEM.md`, resolved at runtime through a `ThemeColors` struct so
  no widget hardcodes a hex value.
- `Button` — variants `Primary`, `Secondary`, `Ghost`, `Danger` with identical
  geometry rules across variants, plus `primaryButton()` / `secondaryButton()` /
  `ghostButton()` / `dangerButton()` helpers.
- `Badge` — tones `Neutral`, `Success`, `Warning`, `Danger`, `Info`, covering
  status, tag, plan, release and price badges.
- `TableView` — owns the complete column layout algorithm.

### Design decisions

- **Geometry lives in the component, never in a stylesheet.** Qt does not fold
  QSS `padding` into `sizeHint()` until a widget is polished, so anything
  measured before that (a table sizing columns while populating rows) reads a
  too-small hint and clips its content. `Button::sizeHint()` is therefore
  computed from the button's own font and padding, and `Badge` paints itself
  instead of being styled.
- **Column roles are declared, never inferred.** A page states whether a column
  is `Text` or `Content`; the table decides the width. Inferring the role from
  data (text length, alignment) makes the same column behave differently
  depending on which rows happen to be loaded.
- **Deterministic column layout.** Measure every cell (widgets report
  `sizeHint()`, text is measured with that cell's own font) and the header, take
  `max + padding`, fix `Content` columns at that width, then share any leftover
  viewport width among `Text` columns. When there is no leftover the view
  scrolls horizontally — content is never compressed and never elided.
- **No painted filler.** Unused width past the last column is transparent;
  `QHeaderView` itself carries no background, only its sections do.
- **No alternating row colours.** The alternate colour is drawn by the item
  delegate and does not extend under cell widgets, which makes badge and button
  cells read as dark blocks. Row separation uses grid lines instead.

### Contract

Components own padding, spacing, font, colours, border radius, `sizeHint()`,
`minimumSizeHint()` and size policy. Pages compose layouts and nothing else —
they never measure text, set widget widths, or compute column widths.
