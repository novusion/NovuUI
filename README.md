# NovuUI

Shared native Qt component library for every Novu application.

It is the single source of truth for reusable controls. A control exists here
once, or it does not exist. The library contains no application-specific code:
nothing here knows which application is consuming it.

Visual language: `NovuAI/docs/NOVU_DESIGN_SYSTEM.md`.
Styling rules: `NovuAI/design/NOVU_THEME_RULES.md`, `NovuAI/design/FIX_FRAMES_HELP.md`.

## The contract

A component owns **all** of its own presentation and geometry:

- padding, spacing, font, colours, border radius
- `sizeHint()`, `minimumSizeHint()`, size policy

A page owns **none** of it. Pages place components into layouts and nothing
else. Specifically, a page must never:

- call `setFixedWidth()` / `setMinimumWidth()` on a component
- measure text with `QFontMetrics` to size a widget
- set a stylesheet on a component
- compute column widths

If a page needs a size that a component does not provide, the fix goes into the
component.

## Why geometry is not in the stylesheet

Qt does not fold QSS `padding` into `sizeHint()` until the widget happens to be
polished. Anything measured before that — a table sizing its columns while
populating rows — reads a too-small hint and clips the content. Every component
here therefore computes its own size from its own font and padding constants,
and `Badge` paints itself rather than being styled. Geometry has one source of
truth per component.

## Components

| Component | Notes |
|---|---|
| `Button` | Variants: `Primary`, `Secondary`, `Ghost`, `Danger`. Same geometry across variants. Helpers: `primaryButton()`, `secondaryButton()`, `ghostButton()`, `dangerButton()`. |
| `Badge` | Tones: `Neutral`, `Success`, `Warning`, `Danger`, `Info`. Covers status / tag / plan / release / price badges. |
| `TableView` | Owns column layout. Pages declare column *roles*, never widths. |

Phase 1 is intentionally these four only. Quality over surface area.

### TableView

```cpp
using novu::ui::TableView;

table->setColumns({
    {tr("Email"),   TableView::ColumnRole::Text},
    {tr("Name"),    TableView::ColumnRole::Text},
    {tr("Plan"),    TableView::ColumnRole::Content},
    {tr("Status"),  TableView::ColumnRole::Content},
    {tr("Created"), TableView::ColumnRole::Content},
    {tr("Actions"), TableView::ColumnRole::Content},
});

table->setText(row, 0, user.email);
table->setWidget(row, 3, new Badge(tr("Active"), Badge::Tone::Success));
table->setWidgets(row, 5, {ghostButton(tr("Plan")), ghostButton(tr("Reset PW"))});

table->relayoutColumns();   // once, after the rows are populated
```

A column's role is declared once and never inferred from the data. Inferring it
(from text length, alignment, …) makes the same column behave differently
depending on which rows happen to be loaded.

Layout algorithm, run on data change and on resize:

1. Measure every cell — widgets report `sizeHint()`, text is measured with that
   cell's own font — plus the header caption.
2. `columnWidth = max(header, all cells) + padding`.
3. `Content` columns are fixed at that width.
4. Leftover viewport width is shared among `Text` columns in proportion to
   their measured width. If there is no leftover, every column keeps its
   measured width and the view scrolls horizontally. Content is never
   compressed and never elided.

## Consuming it

```cmake
add_subdirectory(../NovuUI NovuUI)   # or FetchContent / submodule
target_link_libraries(MyApp PRIVATE Novu::UI)
```

## Status

Early. Implemented: `Theme`, `Button`, `Badge`, `TableView`.

Planned, in order of need: `ComboBox`, `SearchBox`, `TextField`, `CheckBox`,
`Toolbar`, `EmptyState`, `LoadingIndicator`, `Card`.

Not yet built and not yet verified against a running application.
