# NovuUI — design system architecture

The durable description of how this library is built. Task-specific progress
lives in `ROADMAP.md`; this file describes the system itself.

Companion documents:

| file | what it settles |
|---|---|
| `MODULE_RULES.md` | who is allowed to build what, and where it goes — binding on NovuUI and on every consuming module |
| `VOCABULARY.md` | what the words mean: the category taxonomy, tooltip states and levels, and the terms that have been read two ways |
| `ECOSYSTEM_AUDIT.md` | what is shared, what is duplicated across the applications, and the extraction order |
| `EXTRACTION_AUDIT.md` | the per-concept extraction matrix for NovuAI, NovuAdmin and NovuExplorer, and which implementation is authoritative |
| `WINDOW_CHROME.md` | the window frame: components, tokens, platform behaviour, migration |

---

## 1. Position

NovuUI is the single source of truth for desktop UI across every Novusion
application. A generic control exists here once, or it does not exist.

Applications contain **only** domain-specific widgets:

| | |
|---|---|
| **NovuUI** | Card, Section, Toolbar, TablePanel, FormRow, EmptyState, StatCard, BusyOverlay, SearchBar, Badge, Button, StatusIndicator, InfoBanner, ConfirmationBar, PropertyRow, PageLayout, ScrollPage, SplitHeader, AppWindow, ToolWindow, Dialog, TitleBar, WindowControlButton |
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
| Window | title-bar height, control width/height, bar padding and gap, border width, resize band, icon and glyph size, bar background, title ink (active and inactive), border, neutral hover/pressed, danger hover/pressed |

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

## 3.4 One palette, every application

An application does not get a look of its own. It gets the Novu look.

`ThemeColors`' values **are** the palette that NovuAI, NovuSpace, NovuVoice,
NovuExplorer and NovuHub all wear — not "defaults a profile starts from".
`novuAiProfile()`, `novuExplorerProfile()`, `novuHubProfile()` and
`novuVoiceProfile()` carry an id and a display name and not one colour; they
exist so a subtree can be scoped to a product, not so a product can look
different.

Register the identity even when it adds no colour. A profile is no longer a
look, it is a name: it is what a subtree is scoped to, what a window says it
belongs to, and what the gallery filters components by. An application folded
into another's profile cannot be asked which components it uses. `applicationProfilesShareOnePalette()` fails the
build if one starts forking.

**NovuTrade is the single exception**, and a deliberate one: its slate surfaces
and emerald accent are the product. Everything else that had its own palette had
it by accident — NovuExplorer's was already byte-for-byte this one, NovuAI's
differed only in text and accent, NovuVoice's had drifted to cyan, NovuHub's was
the same values uncollected in a stylesheet string.

Selectable **themes** are a different thing from application identities, and are
allowed palettes of their own: Midnight Blue, Warm Gray, Dark Purple and Novu
Light are a user's choice of appearance. `isApplicationProfile()` is the line
between the two.

---

## 3.4a Semantic status is shared; accent is not

Two layers that were being confused, now separated.

An application owns its background, its surfaces, its accent and its selection
styling. It does **not** own what Success, Warning, Danger and Info look like —
those are the same statement in every Novu window, so they are the same colour
in every Novu window.

```
ThemeColors.success / .warning / .danger / .info    the four shared hues
sharedSemanticHues()                                what they are, and the guard
SemanticPalette (profile.semantic)                  text / fill / border / hover / pressed
SemanticRole::{Neutral, Success, Warning, Danger, Info}
```

`applyDerivedTokens()` builds the five values from each hue through one recipe.
Before it there were three — Badge tinted a plate at 0.16, Button at 0.14 with a
0.40 border, the scope sheet at 0.16 again — which is how a Danger button and a
Danger badge stopped matching.

`Info` had no colour at all and every component reached for `accentLight`, the
*application accent*. The same status was therefore sky blue in NovuTrade, gold
in Warm Gray and violet in Dark Purple.

**Neutral is not shared and cannot be.** It is the absence of a status, so it is
drawn in the profile's own secondary ink: one fixed grey would be invisible on
the light profile.

A profile may override a status hue **only when its surface forces it to**,
never to have a look of its own. Exactly one profile does — `novu-light`, where
the shared hues fall to 2.5:1 against white. Two tests enforce this:
`everyProfileSharesTheSemanticPalette()` and
`theOnlyStatusOverrideIsTheLightProfile()`, which names the exceptions so adding
a second is a deliberate act.

---

## 3.5 Window chrome

The frame is a component like any other. `WindowShell` installs it onto a
top-level widget; `AppWindow`, `ToolWindow` and `Dialog` are the three things an
application constructs. An application declares semantics — title, icon,
resizable, minimizable, maximizable, modal — and fills `content()`. It never
draws a title bar, never sizes a window control, and never touches window flags.

The shell is composed rather than inherited because a dialog must be a
`QDialog` and a main window must be a `QWidget`: one base class cannot serve
both, and forcing it is how three applications ended up with three title bars.

`SurfaceRole::Window` and `SurfaceRole::Dialog` let a product round its windows
and its dialogs differently, as two roles rather than as two numbers two
applications picked. Geometry does not vary per profile at all — it is asserted
equal across every registered profile, because a theme is not a licence to
resize the Close button.

Full specification, platform behaviour, the PySide6 port contract and the
migration list: `WINDOW_CHROME.md`.

---

## 4. Rules for contributors

1. No new UI kits. Anywhere.
2. No styling helper functions inside application modules.
3. A generic component is created here before the page that needs it.
4. No hardcoded colours, radii, spacing, fonts, border widths or icon sizes in
   a component — tokens only.
5. Every new `QWidget`/`QFrame` carries a comment naming which of the four
   justifications in §2 it satisfies.
6. No application draws its own window chrome, and no page draws a title bar.
   A window that needs a caption already has one.
7. Semantic status colours are shared. A profile states one of its own only
   when its surface forces it to, and says why in the source.
8. A reusable component is **extracted from a production application**, never
   written fresh in NovuUI as a look-alike. See `ECOSYSTEM_AUDIT.md` §6.1.
9. NovuUI never depends on a consuming application, and is organised by shared
   UI responsibility rather than by the application that first wrote a
   component. See `MODULE_RULES.md` §11 and §12.

### Mechanical checks

```bash
# no hex outside the profile definitions
grep -rnE '#[0-9a-fA-F]{6}' src/ include/ | grep -v ThemeProfile.cpp

# pages must not style components
grep -rn 'setStyleSheet(' <application>/src/

# no application-side window chrome left
grep -rnE 'FramelessWindowHint|WM_NCHITTEST|DwmSetWindowAttribute' <application>/src/
grep -rn 'QMessageBox::' <application>/src/
```
