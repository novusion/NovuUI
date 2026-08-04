# Changelog

All notable changes to NovuUI will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
