# Changelog

All notable changes to NovuUI will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

Foundation work for NUI-002 — making NovuUI the single source of truth for
desktop UI. Everything here is additive; existing consumers link unchanged.

### Added

- **`FeedbackForm` / `FeedbackDialog`.** A shared feedback reporter for every
  Novusion application: Bug / Suggestion / General, a title, a description, and
  an explicit account of what else is attached. It knows nothing about
  transport - `submitted()` hands the consumer a `FeedbackRequest` and the form
  waits in `Sending` until `reportSuccess()` or `reportFailure()` answers, so
  NovuUI never learns that NovuServer exists.
  - The consumer supplies `FeedbackContext`: application, version, platform,
    workspace and an optional log callback. Nothing is discovered here.
  - Logs come from a `std::function` rather than a string, so ticking the box
    is what causes them to be read - "include logs" is a decision rather than a
    default that already happened. With no callback the option is disabled and
    says why rather than being silently absent.
  - Six states - Empty, Editing, Invalid, Sending, Sent, Failed. `Sending`
    locks the fields and refuses a second submit at the handler, not only by
    disabling the button: a return key or a queued click would otherwise still
    produce a second request. A failure leaves the report intact and retryable.
  - Outcomes go through `InfoBanner` and the semantic palette, so a success
    here is the same green as a success anywhere else in the product.
  - `FeedbackRequest` is a struct, deliberately: a screenshot, a file
    attachment, a category or a reply address are additions to it rather than
    changes to the signal that carries it.
- **`textArea()`** in `Fields` - the multi-line half of `lineEdit()`, with
  `QPlainTextEdit` rules added to the scope sheet. It had none, so a description
  field arrived with the light system palette: white paper on a dark form. Its
  height is measured in lines of the profile's body font rather than pixels,
  which is a line short at 125% scaling.
- **`HelpOverlay`.** An explanatory layer drawn over a live window: each note is
  a bubble - coloured outline, title, a sentence - joined to the widget it
  explains by a dashed leader ending in an arrow. The overlay is transparent to
  the mouse and dims nothing, so a user reads the note about the toolbar and
  then uses the toolbar; it annotates a running application rather than stopping
  one to explain it. Anchors resolve at paint time from each target's current
  geometry, so notes follow their targets through a resize, and a target that is
  hidden or destroyed takes its note with it rather than leaving a leader
  pointing at nothing. Six tones, all of them colours the profile already
  defines.
- **`ComponentSpec::awaitingAdoption`** for a component built in NovuUI ahead of
  the page that needs it - which the design system asks for. It keeps "no
  application uses this yet" distinguishable from "somebody forgot to record
  membership", so the test that catches the second does not have to allow the
  first.

- **One palette, every application.** `ThemeColors` now *is* the Novu
  application palette rather than a set of defaults, with NovuAI as the
  reference. `novuai`, `novuexplorer`, `novuhub` and `novuvoice` carry an id and
  a display name and not one colour - they are identities a subtree is scoped
  to, not looks. Only three text inks and the accent actually had to move;
  surfaces and borders were already identical across the products.
  `applicationProfilesShareOnePalette()` fails the build if one starts forking.
  **NovuTrade is the single exception**: its slate surfaces and emerald accent
  are the product, not drift.
- **`novuExplorerProfile()` and `novuHubProfile()`**, and
  `isApplicationProfile()` to tell a product from a selectable theme. Both
  applications had been folded into `novuspace` on the grounds that their
  palettes were identical to it - correct about the palette, wrong about the
  identity: a profile is what an application is scoped to and what the gallery
  filters by, so an application without one cannot be asked which components it
  uses.
- **Application component filter in the gallery.** One ON/OFF control beside the
  Theme selector, which is already the application context - no second dropdown.
  Membership is `ComponentSpec::usedBy`, one line per component, and the filter
  is `ComponentRegistry::search(query, applicationId)`, so the gallery holds no
  membership rules of its own. It narrows visibility and nothing else: a
  filtered spec is the same spec. With a selectable theme rather than an
  application selected there is nothing to filter by, and the control disables
  itself and says so.
- **Shared semantic palette.** Success, Warning, Danger and Info are now one set
  of hues for every Novusion application, in `sharedSemanticHues()`, with
  NovuAI's values adopted as the reference. `SemanticPalette` on the profile
  carries the five values a component needs - text, fill, border, hover,
  pressed - built from each hue by `semanticColorFrom()`. An application owns
  its surfaces and its accent; it does not own what "failed" looks like.
- **`SemanticRole`** and `profile.semantic.role()`, so a component names a
  meaning instead of a colour.
- **`ThemeColors::info`**. Info previously had no colour and every component
  reached for `accentLight` - the application accent - so the same status was
  sky blue in NovuTrade, gold in Warm Gray and violet in Dark Purple.

### Changed

- **Badge, InfoBanner, `Ink`, the scope sheet's tone table and Button's
  Danger/Success variants** all resolve through the shared palette. Badge tinted
  its plate at 0.16, Button at 0.14 with a 0.40 border and the sheet at 0.16
  again - three recipes for one idea, which is why a Danger button and a Danger
  badge next to each other did not match.
- **NovuSpace, NovuVoice, Midnight Blue, Warm Gray and Dark Purple no longer
  state status colours.** The shell's olive `#98C379`, NovuVoice's mint
  `#3CCB7F` and Midnight Blue's near-miss `#F85549` were drift, not design.
- **NovuTrade takes the shared status colours too.** Measured rather than
  assumed: on its own `#111827` panel the shared hues reach 5.26, 7.03 and 5.29
  against 4.5 required - better than on the shell's surfaces - so an override
  would have cost contrast as well as consistency. Its identity is its slate
  surfaces and emerald accent, both unchanged. Its **candle** colours are pinned
  explicitly to the web app's `#22C55E` / `#EF4444`: a candle is a price going
  up, not a "success".
- **`novu-light` keeps its own status hues**, the one override in the system and
  the only one the tests permit. The reason is surface polarity, not product
  identity: the shared hues fall to 2.5:1 against white.

### Added - tests

- `everyProfileSharesTheSemanticPalette()`,
  `theOnlyStatusOverrideIsTheLightProfile()` (names the exceptions, so a second
  one is a deliberate act), `semanticColoursReadAgainstTheirOwnSurface()`
  (measures WCAG contrast on each profile's own panel - it caught a profile that
  had skipped `applyDerivedTokens()` and was painting black),
  `infoIsAStatusAndNotTheApplicationAccent()` and
  `badgeTonesResolveThroughTheSharedPalette()`.

- **Six product profiles, brought in from the applications' own repositories.**
  `novuai`, `novuvoice`, `midnight-blue`, `warm-gray`, `dark-purple` and
  `novu-light` in `src/AppProfiles.cpp`. Each is `novuSpaceProfile()` with a
  different palette - a product picks colours, not a control height, a radius or
  a title-bar height - and `applyDerivedTokens()` re-derives table chrome, chart
  series and window chrome against it. `novu-light` is the first light palette
  in the system and needed no conditional anywhere to work.
  NovuExplorer's own default is deliberately absent: it is byte-for-byte
  `novuSpaceProfile()`, so it is a consumer rather than a profile the library
  was missing.
- **`builtinProfiles()`** - one list of what the library ships. `ThemeManager`
  registers from it and the gallery tells a shipped profile from a loaded
  overlay with it, so adding a profile is one entry and nothing else.
- **`applyDerivedTokens()`** is public: an application registering a profile of
  its own sets `colors`, calls it, and gets a frame, tables and charts in the
  new palette. Without it a custom profile silently kept the graphite title bar.
- **`NovuStyle`.** A check indicator is not a widget - it is a primitive the
  style draws - so no stylesheet selector reaches the one in a table cell, a
  tree item or a checkable group box, and a stylesheet cannot put a shape inside
  the one in a plain QCheckBox either. The only state a sheet could express was
  "flood the box with the accent", which is why every application showed a
  filled square. `NovuStyle` draws all of them from tokens: an outline that
  never goes away and a tick, a cross or a dash inside it. Installed by
  `applyStyleSheet()`, so an application that themes a subtree gets it without
  knowing it exists; `installNovuStyle()` is there for a `main()` that needs it
  earlier. Qt does not propagate a style down a subtree, so this is
  deliberately the application style - it still resolves colours through
  `themeFor(widget)`, so one style object serves every product in a host
  process.
- **`CheckBox`** with `Mark::Check` and `Mark::Cross`, announced to the style as
  a dynamic property rather than painted, so a checkable table cell and a
  checkbox widget are one drawing instead of two that have to be kept in step.
- **Signed metrics.** `StatCard` and `MiniStat` tint a value by its own sign:
  a leading `+` takes the success ink, a leading `-` the danger ink, and a
  placeholder dash takes neither. Applied only when the caller named no tone,
  and re-resolved on `setValue()`, so a position crossing zero changes colour
  on its own.
- `docs/ECOSYSTEM_AUDIT.md` — what is shared, what is duplicated across
  NovuExplorer, NovuAI, NovuVoice, NovuHub and NovuSpace, and what NovuUI is
  still missing.

### Fixed

- **Ink was silently dropped inside a themed subtree.** The scope sheet carries
  `QLabel { color: ... }` so a label with no ink of its own is not left with the
  light system palette, and a stylesheet colour outranks a palette one -
  `setInk()` wrote only the palette. Every `Ink` role was therefore ignored
  wherever the sheet was applied: a loss printed in the same colour as a
  profit, muted captions were not muted, and an inactive window title would
  have stayed at full strength. `setInk()` now also writes a single-property
  sheet on the label, which outranks the ancestor's.

- **Shared window chrome.** `WindowShell` installs the Novu frame onto a
  top-level widget; `AppWindow`, `ToolWindow` and `Dialog` are the three
  classes an application constructs. The shell owns the window background,
  border, corner radius, title bar, the three window controls, dragging,
  double-click maximize/restore and resize hit-testing. An application declares
  semantics only — title, icon, resizable, minimizable, maximizable, modal —
  and fills `content()`. Replaces four separate title bars across
  NovuExplorer, NovuHub and NovuSpace, three of which drew a different Close
  button size.
- **`TitleBar`** — icon, caption, leading slot (a menu bar), trailing slot
  (status widgets) and the controls. The caption elides rather than widening
  the window, and hiding a control moves nothing else: a dialog and a main
  window share one bar height and one Close box.
- **`WindowControlButton`** — Minimize, Maximize, Restore and Close as one
  class with one bounding box. Glyphs are drawn rather than typed, so they stay
  crisp at 125% and 150% scaling. Close is neutral until hovered and only then
  turns danger; it is never permanently red, and never larger than Minimize.
- **Window tokens.** `WindowTokens` on `ThemeProfile` carries title-bar height,
  control width and height, bar padding and gap, border width, resize band,
  icon and glyph size, bar background (active and inactive), title ink (active
  and inactive), border, neutral hover and pressed, and danger hover and
  pressed. Colours are derived from each profile's palette, so a new product
  gets correct chrome by defining a palette and nothing else. Geometry is
  identical in every profile, and a test asserts it.
- **`SurfaceRole::Window`** and **`TypeRole::WindowTitle`**, so a product can
  round its windows and its dialogs differently through roles rather than
  through numbers in an application.
- **Native frame integration (Windows).** The shell restores `WS_THICKFRAME`
  under its own chrome and answers `WM_NCCALCSIZE`, `WM_NCHITTEST` and
  `WM_GETMINMAXINFO`, so resizing, Aero Snap, the drop shadow and the taskbar
  keep working while nothing native is drawn. Corner radius and border colour
  go to DWM rather than being painted, which keeps sub-pixel text antialiasing.
  Elsewhere the frame falls back to `QWindow::startSystemResize()`.
- **`Dialog::confirm()`, `Dialog::inform()`, `Dialog::reportError()`** — same
  shell, so the remaining `QMessageBox` calls have somewhere to go.
- `docs/WINDOW_CHROME.md` — the canonical specification, the PySide6 port
  contract for NovuCrawler, migration instructions and the migration list.
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
