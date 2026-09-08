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
| `Card` | The one surface primitive: background, border, themed radius. Self-painted. Padding lives on its own layout — there is no inner "body" widget. |
| `section()` | Styled heading added to an existing layout. Creates no container. |
| `toolbar()` | A row of controls with the profile's gap. Returns a layout. |
| `pageLayout()` | A page's outer layout with page padding and card gap. Returns a layout. |
| `splitHeader()` | Title/subtitle on the left, actions on the right. Returns the actions layout. |
| `scrollPage()` | Scrollable page. Creates the one widget a QScrollArea requires. |
| `AppWindow`, `ToolWindow`, `Dialog` | The shared window frame. An application declares title, icon and capabilities; it never draws chrome. See `docs/WINDOW_CHROME.md`. |
| `TitleBar` | Icon, caption, leading/trailing slots and the three window controls. Owned by the shell — a page never builds one. |
| `FeedbackForm`, `FeedbackDialog` | Send feedback: type, title, description, and an explicit account of what else is attached. Produces a `FeedbackRequest` and hands it to the consumer — NovuUI knows nothing about transport. |
| `textArea()` | The multi-line half of `lineEdit()`. Plain text, height measured in lines. |
| `HelpOverlay` | An explanatory layer over a live window: coloured bubbles joined to what they explain by dashed leaders. Transparent to the mouse, so the application keeps working while the help is up. |
| `WindowControlButton` | Minimize, Maximize/Restore and Close as one class, one bounding box. Close is neutral until hovered, then danger. |

Still to come — see `docs/ROADMAP.md`: `EmptyState`, `InfoBanner`,
`BusyOverlay`, `StatusIndicator`, `ConfirmationBar`, `StatCard`, `TablePanel`,
`FormRow`, `PropertyRow`, `SearchBar`.

## Rules

`docs/MODULE_RULES.md` is binding: what belongs in NovuUI versus in a feature
module, the dependency direction, and the rule that a shared component is
*extracted* from a production application rather than written here as a
look-alike. Read it before adding a component.

## Theming

The library supports several products in one process. `ThemeManager` holds
profiles, `ThemeScope` marks a widget subtree as belonging to one, and
`themeFor(widget)` resolves it by walking up the parents:

```cpp
ThemeScope::apply(novuTradeWorkspace, "novutrade");
```

Components read **roles**, never numbers — `radius(SurfaceRole::Card)`,
`text(TypeRole::CardTitle)`, `space(SpacingRole::CardPadding)`. Profiles decide
the values, so the same component is near-flat graphite in NovuSpace and
rounded emerald in NovuTrade with no conditional anywhere.

Theme changes propagate live: `registerProfile()` sends a `ThemeChangeEvent` to
every affected widget, no restart and no component registry.

Full description: `docs/DESIGN_SYSTEM.md`.

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

That gives you exactly one target, `NovuUI`. The component gallery and the
tests are guarded by `NOVUUI_BUILD_PLAYGROUND` / `NOVUUI_BUILD_TESTS`, which
default to "NovuUI is the project being built", so a consumer never
configures them.

## The component gallery

`playground/` is a standalone development executable that renders the real
components — the same classes an application links — with their variants,
states and layout-stress scenarios, under any registered theme profile. It is
the visual test surface: edit a component, press **Rebuild & reload**, see the
result. Editing sample data or a theme overlay needs no rebuild at all.

```bash
cmake -S . -B build && cmake --build build --config Release
build/playground/Release/NovuUIPlayground
```

Full description, and how to add a component to it: `playground/README.md`.

## Status

Early. Implemented: `Theme`, `Button`, `Badge`, `TableView`.

Planned, in order of need: `ComboBox`, `SearchBox`, `TextField`, `CheckBox`,
`Toolbar`, `EmptyState`, `LoadingIndicator`, `Card`.

Not yet built and not yet verified against a running application.
