# NovuUI — roadmap

Tracks NUI-002: complete the foundation, then migrate applications onto it.
The system itself is described in `DESIGN_SYSTEM.md`.

Status as of 2026-08-14.

---

## Done

### Phase 2 — theme system ✅

`ThemeProfile.h` / `ThemeProfile.cpp`, `ThemeManager.h/.cpp`,
`ThemeScope.h/.cpp`.

- Radius scale (`None…Pill`) + `SurfaceRole` mapping
- `TypeRole` → size / weight / monospace
- `SpacingRole`, `ControlSize`, `BorderRole`
- Chart tokens (grid, axis, bullish, bearish, volume, crosshair, series palette)
- Table tokens (header/row height, stripes, gridline, selection, hover)
- Animation tokens, icon metrics
- Two profiles: `novuspace`, `novutrade`

### Phase 6 — live theme runtime ✅

- `ThemeManager` registry, default profile, `registerProfile()` swaps live
- `ThemeScope` as a dynamic property on an existing root, no wrapper widget
- `ThemeChangeEvent` broadcast via `QApplication::allWidgets()`, no registry
- `themeFor(widget)` resolves by walking parents, no caching

### Phase 8 — shared window chrome ✅

`TitleBar.h/.cpp`, `WindowShell.h/.cpp`, `WindowTokens`, `SurfaceRole::Window`,
`TypeRole::WindowTitle`.

- `WindowShell` composed onto a top-level widget, not inherited — a dialog has
  to stay a `QDialog`
- `AppWindow`, `ToolWindow`, `Dialog`; `Dialog::confirm/inform/reportError`
- `WindowControlButton`: one class, one bounding box, drawn glyphs, Close
  neutral until hovered
- Native frame kept alive under the chrome on Windows (`WS_THICKFRAME`,
  `WM_NCCALCSIZE`, `WM_NCHITTEST`, DWM radius and border); `startSystemResize()`
  fallback elsewhere
- Reference window, tool window and dialog in the gallery under Foundation →
  WindowShell
- 24 tests in `tests/tst_window_chrome.cpp`
- Specification and migration list: `WINDOW_CHROME.md`

### Phase 1a — structural components ✅

| component | widget cost | note |
|---|---|---|
| `Card` | 1 | self-painted; padding on its own layout, no inner body widget |
| `section()` | 0 | styled label into an existing layout |
| `toolbar()` | 0 | returns a layout |
| `pageLayout()` | 0 | returns a layout |
| `splitHeader()` | 0 | returns the actions layout |
| `scrollPage()` | 1 | the viewport a QScrollArea requires; justified in source |

Everything above builds; `NovuAdmin` still links unchanged (the work is
additive).

---

## Remaining

### Phase 1b — feedback components
`EmptyState`, `InfoBanner`, `BusyOverlay`, `StatusIndicator`, `ConfirmationBar`

### Phase 1c — data components
`StatCard`, `TablePanel`, `FormRow`, `PropertyRow`, `SearchBar`

### Phase 3 — migrate existing components
`Button`, `Badge`, `TableView` from the legacy `theme()` global to
`themeFor(this)` + `ThemeChangeEvent`.

> First real regression risk: these three are already consumed by NovuAdmin.
> `Badge` reads the theme inside `paintEvent` (a repaint is enough), while
> `Button` and `TableView` cache their stylesheet at construction and must
> re-apply on the event.

### Phase 4 — NovuTrade migration
Summary, Platform Setup, Trading Bot, Trade Manager, Trade History, Orders,
Positions, Trade Data, AI Analyze, Advanced, Settings.

Entry point: `ThemeScope::apply(workspace, "novutrade")`, then Summary as the
reference page.

Per-page exit criteria, all countable:

```
hex literals in file      -> 0
setStyleSheet() calls     -> 0
new QWidget()             -> reduced; each survivor justified in a comment
clear()/setRowCount(0)    -> 0 on the refresh path
```

Baseline measured before the work started (NovuSpace `src/modules/novutrade`):
68 QWidget containers, 133 layouts, 205 `setStyleSheet` calls, ~380 hex
literals, 45 rebuild-instead-of-update sites.

### Phase 5 — cleanup
Delete `NovuTradeUiKit.h` and every page-local styling helper.

### Phase 7 — audit and report
Migrated components, removed helpers, removed layouts, removed stylesheets,
remaining debt, recommendations for the Designer.

### Phase 9 — window chrome migration

The library side is done; the applications are not. Full table, per file, in
`WINDOW_CHROME.md` §9. Summary:

| step | scope | risk |
|---|---|---|
| 1 | NovuExplorer main window + `NovuPopupWindow` | highest payoff — deletes three copies of the native recipe |
| 2 | NovuHub `HubWindow` | removes the last hardcoded chrome palette |
| 3 | NovuSpace `AppWindow` | hosts the modules; migrate it alone |
| 4 | `QMessageBox` sweeps, one application at a time | mechanical, ~658 sites total |
| 5 | NovuCrawler PySide6 port | mirrors `WINDOW_CHROME.md` §7, no redesign |
| 6 | the Novu file/folder picker | unblocks the `QFileDialog` sweep |

### Then — the other applications
`NovuAdmin` (retire `AdminUiKit.h`), `NovuExplorer`, `NovuMail`. Until
`AdminUiKit` is gone there are still two kits and the second one drifts.

---

## Suggested order

| step | scope | risk |
|---|---|---|
| 1 | Phase 1b + 1c (10 components) | none — additive |
| 2 | Phase 3 | first regression risk (NovuAdmin) |
| 3 | `ThemeScope` + Summary as reference page | visual review checkpoint |
| 4 | remaining 10 NovuTrade pages, two at a time | per-page review |
| 5 | Phase 5 + 7 | cleanup and report |

Step 3 is the point to run the app and confirm the new look before it is
spread across the rest.

---

## Open questions for the NovuAI UI Designer

Raised during design, not yet answered:

1. **"Hot reload of all open modules"** — theme hot reload works today
   (`registerProfile()`). Reloading module *code* means swapping a loaded DLL
   with live widgets inside it, which in practice is closing and rebuilding the
   module: no application restart, but the module's state is lost. Which is
   meant?
2. **Compare mode (Web vs Qt)** — both legacy web frontends are retired. The
   reference has to be reference screenshots or a locally kept React build.
3. **AI Designer** — undefined. Theme generation from a description? Layout
   suggestions? Contrast and accessibility checking? Each is a different
   project.
