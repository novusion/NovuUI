# NovuUI — design system architecture

The durable description of how this library is built. Task-specific progress
lives in `ROADMAP.md`; this file describes the system itself.

---

## 1. Position

NovuUI is the single source of truth for desktop UI across every Novusion
application. A generic control exists here once, or it does not exist.

Applications contain **only** domain-specific widgets:

| | |
|---|---|
| **NovuUI** | Card, Section, Toolbar, TablePanel, FormRow, EmptyState, StatCard, BusyOverlay, SearchBar, Badge, Button, StatusIndicator, InfoBanner, ConfirmationBar, PropertyRow, PageLayout, ScrollPage, SplitHeader |
| **NovuTrade** | TradeChart, PositionWidget, OrderBook, CandlestickChart, TradeSignalBadge |
| **NovuAdmin / NovuExplorer / NovuMail** | their own domain widgets only |

When a page needs something generic that does not exist yet, **it is added
here first**. A styling helper function inside an application is the failure
mode this library exists to prevent.

---

## 2. The component contract

Inherited from the original README and still binding:

A component owns **all** of its own presentation and geometry — padding,
spacing, fonts, colours, radius, `sizeHint()`, `minimumSizeHint()`, size
policy, interaction states and painting.

A page owns **none** of it. A page places components into layouts. It must
never:

- call `setFixedWidth()` / `setMinimumWidth()` on a component
- measure text with `QFontMetrics` to size a widget
- set a stylesheet on a component
- compute column widths

If a page needs a size a component does not provide, the fix goes into the
component.

### Why geometry is computed, not styled

Qt does not fold QSS `padding` into `sizeHint()` until the widget is polished.
Anything measured before that — a table sizing columns while populating rows —
reads a too-small hint and clips. Components therefore compute their own size
from their own font and token values, and paint themselves rather than relying
on a stylesheet.

Painting has a second reason: a self-painted component reads its tokens at
paint time, so a live theme change is a repaint rather than a rebuild.

### Container discipline

A `QWidget` or `QFrame` is justified by exactly one of:

1. a visible surface (background, border)
2. a stacking page
3. a scroll viewport
4. a reusable component boundary

Padding and margins are `contentsMargins` on a layout — **never** a wrapper
widget. A container that exists for padding is a design error, and it
multiplies by every instance on screen.

Concretely: `toolbar()`, `pageLayout()` and `splitHeader()` return layouts and
create no widget at all. `scrollPage()` creates one, and says in the source why
(a QScrollArea cannot scroll without a viewport).

---

## 3. Theme system

### 3.1 Profiles, not a global theme

`theme()` as a single process-wide instance cannot work once one process hosts
several products: NovuSpace loads NovuTrade, NovuMail and NovuVault as modules
into its own process, so changing the theme would repaint all of them together.

```
ThemeManager        registry of profiles, default profile, change notification
ThemeProfile        one product's complete visual language
ThemeScope          marks a widget subtree as belonging to a profile
themeFor(widget)    resolves the profile that applies to this widget
```

`ThemeScope` is **not a widget** — it is a dynamic property written onto a
widget that already exists:

```cpp
ThemeScope::apply(novuTradeWorkspace, "novutrade");
```

Everything constructed under that root resolves to the NovuTrade profile while
the shell around it keeps its own. Resolution walks up `parentWidget()` on
every read rather than caching, because a widget can be reparented into another
scope at any time and a cache would then be silently wrong.

### 3.2 Tokens are roles, not values

A component never names a number. It names what it *is*, and the profile
decides:

| token family | roles |
|---|---|
| Radius | `SurfaceRole::{Card, Dialog, Button, Input, Badge, Toolbar, Table, Overlay}` over a `None…Pill` scale |
| Typography | `TypeRole::{PageTitle, SectionTitle, CardTitle, Body, Caption, Label, Metric, Mono}` → size + weight + monospace |
| Spacing | `SpacingRole::{CardPadding, CardGap, SectionGap, ToolbarGap, FormRowGap, TableCellPadding, PagePadding}` |
| Control size | `ControlSize::{Small, Medium, Large}` |
| Border | `BorderRole::{Card, Input, Table, Divider, Focus}` — thickness, separate from colour |
| Chart | grid, axis, bullish, bearish, volume, crosshair, categorical series palette |
| Table | header height, row height, stripes, gridline, selection, hover |
| Animation | `instant / fast / normal / slow` — zero disables motion wholesale |
| Icon | small / medium / large |

This indirection is what lets NovuSpace say "cards are barely rounded" and
NovuTrade say "cards are rounded-xl" without either statement appearing in a
component.

### 3.3 Live changes

`ThemeManager::registerProfile()` replaces a profile and sends a
`ThemeChangeEvent` to every live widget scoped to it. Components react:

- self-painting components (`Badge`, `Card`) call `update()`
- components that cache a stylesheet or geometry re-apply it and
  `updateGeometry()`

There is deliberately **no registry** of themed components. One that forgot to
unregister would crash the next repaint; one that forgot to register would
silently stop following the theme. `QApplication::allWidgets()` cannot go stale.

This is the runtime the future NovuAI UI Designer builds on: editing a token is
`registerProfile()` on a mutated copy.

### 3.4 Shipped profiles

| id | language |
|---|---|
| `novuspace` | graphite (`#1D2228`), petrol accent, near-flat radii |
| `novutrade` | slate/emerald from the trading web app's `tailwind.config.ts` (`#0f172a`, `#111827`, `#10b981`), rounded-lg/xl/2xl |

Applications may register their own; these two exist so a fresh consumer looks
right with no configuration.

---

## 4. Rules for contributors

1. No new UI kits. Anywhere.
2. No styling helper functions inside application modules.
3. A generic component is created here before the page that needs it.
4. No hardcoded colours, radii, spacing, fonts, border widths or icon sizes in
   a component — tokens only.
5. Every new `QWidget`/`QFrame` carries a comment naming which of the four
   justifications in §2 it satisfies.

### Mechanical checks

```bash
# no hex outside the profile definitions
grep -rnE '#[0-9a-fA-F]{6}' src/ include/ | grep -v ThemeProfile.cpp

# pages must not style components
grep -rn 'setStyleSheet(' <application>/src/
```
