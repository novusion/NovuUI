# NovuApp UI audit — what is shared, what is duplicated, what NovuUI is missing

Surveyed 2026-08-29 across `NovuExplorer`, `NovuAI`, `NovuVoice`, `NovuHub`,
`NovuSpace` and `NovuAdmin`.

> **Stale paths.** NovuAI was restructured after this survey: its sources moved
> from `ui/src/**` to `src/frontend/**`, `src/backend/**` and `src/modules/**`.
> Every NovuAI path below is therefore out of date. `EXTRACTION_AUDIT.md`
> (2026-09-14) carries the current ones, and is the audit to act on.

Method, so the numbers can be read honestly: class inventories come from
`class X : public QWidget|QFrame|QDialog|...` across each repository's headers.
Styling counts come from matching Qt class names in stylesheet-shaped source
text — they are an indication of *what each application dresses itself*, not an
exact count of QSS rules. Everything in §4 was checked by opening the file.

---

## 1. The headline

| application | consumes NovuUI | draws its own chrome | own palette |
|---|---|---|---|
| NovuAdmin | yes (`add_subdirectory`) | no | no |
| *(profiles)* | `novuspace` `novuai` `novuexplorer` `novuhub` `novuvoice` — one palette; `novutrade` — the exception |||
| NovuSpace | yes (`add_subdirectory`) | **yes** — `src/ui/AppWindow` | no |
| NovuExplorer | **no** | **yes** — `title_bar_widget` | copy + 4 extra themes |
| NovuAI | **no** | **yes** — `widgets/TitleBar`, `widgets/FramelessDialog` | copy + own text/accent |
| NovuVoice | **no** | **yes** — `ui/TitleBar` | **diverged** — `NovuSpaceTheme.h` |
| NovuHub | **no** | **yes** — inline QSS in `HubWindow` | copy, uncollected hex |

Four of six applications do not link the library that exists to stop this. That
is the finding; everything below is detail.

---

## 2. Palettes — three copies and one real divergence  ✅ resolved

The first read of this was wrong and is worth correcting, because the true
finding is more useful than the one it replaced.

`NovuExplorer`'s default theme is **byte-for-byte** `novuSpaceProfile()` —
`#161A1F`, `#1D2228`, `#287873`, every entry. `NovuAI/ui/src/ui/Theme.h` has the
same surfaces (its own header says "unified with NovuSpace") and differs only in
text and accent. `NovuHub` is the same palette again, uncollected, inline in a
stylesheet string.

So three of the four are not forks — they are *copies*, which is a much cheaper
problem: those products do not need a new theme, they need to link the library.

The one that genuinely diverged is
`NovuVoice/novuvoice-client/app/src/ui/NovuSpaceTheme.h`, a frozen copy of "the
Novu graphite theme" with values that are **not** NovuUI's:

| role | NovuVoice | NovuUI `novuspace` |
|---|---|---|
| window background | `#1a1f25` | `#161A1F` |
| shell background | `#161a1f` | `#1D2228` (panel) |
| card background | `#1f252c` | `#1D2228` |
| accent | `#2fc6d8` (cyan) | `#287873` (petrol) |
| success | `#3ccb7f` | `#98C379` |
| warning | `#d7a63a` | `#C9A26D` |
| danger | `#e05b6b` | `#FF5555` |

The file's own header says "frozen palette … keep containers styled through
object names". It is a fork of the design system, and it drifted.

NovuHub is worse only in that its values are not even collected: `HubWindow.cpp`
carries `#161a1f`, `#2a3038`, `#8793a3`, `#20262d`, `#d98a8a` inline in a
stylesheet string.

NovuExplorer also ships four *user-selectable* themes that existed nowhere else:
Midnight Blue (GitHub dark, `#1F6FEB`), Warm Gray (gold, `#D4A853`), Dark Purple
(`#A855F7`) and NovuSpace Light — the only light palette in the ecosystem.

**Done.** All six are now profiles in the library: `novuai`, `novuvoice`,
`midnight-blue`, `warm-gray`, `dark-purple`, `novu-light`, in
`src/AppProfiles.cpp`, registered by `ThemeManager` and listed by
`builtinProfiles()`. Every one of them is `novuSpaceProfile()` with a different
palette — a product picks colours, not a control height — and
`applyDerivedTokens()` re-derives table chrome, chart series and window chrome
against it. The window-chrome tests iterate every registered profile, so the new
six are already covered by "identical control geometry" and "colours derived
from the palette".

NovuExplorer's own default palette is **not** among them, and never will be: it
was `novuSpaceProfile()` to begin with. The *application* is registered all the
same — `novuexplorer` and `novuhub` are identities carrying no colour.

An earlier pass here concluded "a consumer, not a profile" and left both out
entirely. That was wrong, for a reason only visible once every application
shared one palette: **a profile is no longer a look, it is a name.** An
application without one cannot be scoped to, cannot say which product a window
belongs to, and — the one that made it obvious — cannot be asked which
components it uses.

---

## 2a. One palette — resolved

Following §2, the remaining differences were removed rather than recorded: every
application profile is now the same `ThemeColors`, with NovuAI as the reference.
The only values that actually had to move were three text inks and the accent —
surfaces and borders were already identical.

| | before | after |
|---|---|---|
| `textPrimary` | `#E3E8F0` (shell) / `#D4D9E0` (AI) | `#D4D9E0` |
| `textSecondary` | `#A8B2C3` / `#8B929A` | `#8B929A` |
| `textDisabled` | `#6E7A8E` / `#545B64` | `#545B64` |
| `accent` | `#287873` / `#3D8B8B` / `#2FC6D8` (Voice) | `#3D8B8B` |

NovuVoice's cyan is gone with the rest. An earlier pass here argued it was an
identity worth keeping; the decision is that Novu is one product family, and a
video window is not a different product from a file manager.

`novutrade` keeps everything. `applicationProfilesShareOnePalette()` enforces the
rest, entry by entry.

---

## 3. What each application dresses itself

Top styled Qt classes per application. Read it as "which primitives every
product has had to restyle by hand":

| class | Explorer | AI | Voice | Hub | Space |
|---|---:|---:|---:|---:|---:|
| QPushButton | 96 | 294 | 69 | 51 | 173 |
| QScrollBar | 43 | 27 | 22 | 34 | 23 |
| QLabel | 17 | 224 | 4 | 8 | 52 |
| QLineEdit | 12 | 42 | 8 | 8 | 34 |
| QComboBox | 15 | 28 | — | 8 | 24 |
| QCheckBox | 16 | 26 | — | 10 | 23 |
| QTabBar | 20 | 23 | — | 3 | 26 |
| QMenu | 30 | — | 11 | 4 | 23 |
| QTreeView / QTreeWidget | 63 | — | — | 3 | — |
| QToolButton | 36 | 59 | — | — | 17 |
| QProgressBar | 14 | — | — | 8 | 10 |
| QListWidget | — | 66 | 8 | — | 13 |
| QDialog | 12 | 20 | — | 3 | 16 |

Every one of these already has a NovuUI answer (`Button`, the scope sheet's
scrollbar rules, `Text`, `Fields`, `TabStrip`, `TableView`, `Dialog`) — except
the tree/list column, see §5.

---

## 4. Components that exist more than once

Confirmed by opening the files. "NovuUI" is what the library already has.

### Solved this session

| concept | copies found | NovuUI |
|---|---|---|
| window title bar | `NovuExplorer/src/ui/title_bar_widget`, `NovuAI/ui/src/widgets/TitleBar.h`, `NovuVoice .../ui/TitleBar`, `NovuHub HubWindow` inline, `NovuSpace/src/ui/AppWindow` | `TitleBar` + `WindowShell` |
| frameless dialog | `NovuAI/ui/src/widgets/FramelessDialog.h`, `NovuExplorer/src/ui/novu_popup_window` | `Dialog` |
| check indicator | every app; native filled square in Explorer and AI | `NovuStyle` |

### Already in NovuUI, still duplicated in applications

| concept | copy | NovuUI |
|---|---|---|
| card surface | `NovuAI … Card : QFrame` | `Card` |
| metric cell | `NovuAI … StatCell` | `StatCard` / `MiniStat` |
| tab strip | `NovuAI … TabBar`, `NovuVoice … SessionTabBar` | `TabStrip` |
| info / warning block | `NovuAI … WarningBlockWidget`, `SummaryBlockWidget` | `InfoBanner` |
| icon label | `NovuAI … Glyph : QLabel`, `InfoIcon` | `iconFont()` + `label()` |
| search field | `NovuExplorer … FindBar` | `searchBar()` |

### Shared in practice, missing from NovuUI

Ranked by how many products need it.

| gap | who has one | note |
|---|---|---|
| **Toggle / switch** | `NovuAI … Toggle`, `CheckSquare` | a settings staple in four apps; nothing in NovuUI |
| **Busy / spinner** | `NovuAI … BusyGlyph` | `BusyOverlay` is on the roadmap, unwritten |
| **Navigation rail / sidebar** | `NovuAI … LeftSidebar`, `HubWorkspaceSidebar`, `KnowledgeCategoryRail`; `NovuVoice … RoomsSidebar`; `NovuExplorer … PreviewSidebar` | five implementations of one shape |
| **Settings shell** | `NovuExplorer … SettingsDialog` + 8 `*SettingsWidget`; `NovuAI … SettingsWindow` + 11 `Settings*Page` | rail + pages + footer, twice |
| **Log / console panel** | `NovuExplorer … ConsoleWidget`; `NovuAI … DevModeLogPanel` | same widget, two names |
| **Queue / transfer row** | `NovuExplorer … QueueTransferItemWidget`; `NovuAI … DownloadQueueWidget` | progress + label + action |
| **Breadcrumb / path bar** | `NovuExplorer … BreadcrumbBar`, `PathBarWidget` | one product today, wanted by the picker |
| **Charts** | `NovuAI … Donut`, `LineChart`, `Heatmap` | NovuUI ships chart *tokens* but no chart |
| **File / folder picker** | `NovuExplorer … NovuFileDialog` | the shell exists; the content does not |

---

## 5. Trees and lists are the real hole

NovuExplorer restyles `QTreeView` / `QTreeWidget` 63 times and NovuAI restyles
`QListWidget` 66 times. NovuUI has `TableView` and nothing else for hierarchical
or list data, so both products had no choice.

This is the largest single piece of work in the list and the one most likely to
be got wrong twice more. It should be `TreeView` and `ListView` in NovuUI,
reading the existing `TableTokens`, before either product touches its file tree
again.

---

## 6. Proposed order

Each step is a slice that leaves the tree working.

| # | work | why first |
|---|---|---|
| ~~1~~ | ~~product profiles in NovuUI~~ — **done**; the applications still have to adopt them and delete their own copies | ended the palette fork |
| 2 | `Toggle`, `BusyOverlay`, `StatusIndicator` | small, additive, unblocks the settings shells |
| 3 | `TreeView` + `ListView` on `TableTokens` | §5 — the biggest hole |
| 4 | `NavRail` (sidebar) | five copies collapse into one |
| 5 | `SettingsShell` (rail + pages + footer) | consumes 2 and 4; deletes ~20 files across two apps |
| 6 | window chrome migration | `WINDOW_CHROME.md` §8 — independent, can run in parallel |
| 7 | charts on `ChartTokens` | largest, least urgent |
| 8 | file/folder picker into `Dialog` | needs 3 |

Steps 1–2 are additive and carry no regression risk. Step 3 is the first one
that needs a product to be re-tested.

---

## 6.1 How a component gets extracted

Binding for every entry in §6. A shared component is **not** a new visual copy
written in NovuUI from what the applications look like — it is the production
implementation, moved.

For each reusable component found:

1. Find the canonical implementation — the best one that actually ships.
2. Move or refactor its reusable part into NovuUI.
3. Keep its current behaviour and states. **This phase preserves appearance**;
   visual redesign comes after the production components are centralised.
4. Rewire the original application onto the NovuUI component.
5. Delete the local duplicate once nothing needs it.
6. *Then* add it to the gallery.

Where several applications have one: compare them, pick the best common base,
and express the differences as properties, variants or composition. Do not ship
one component per application.

The gallery must never contain its own copy of a production widget. It renders
the same NovuUI components the applications use — that is the only thing that
makes it a regression surface rather than a picture of one.

**The Playground's own sidebar is not the shared Sidebar.** It is the gallery
application's navigation, it has a different job from a production sidebar such
as NovuAI's, and it must not be mistaken for the canonical component. Production
sidebar patterns are audited and extracted independently; the Playground sidebar
may stay Playground-specific, and the gallery must *also* demonstrate the real
shared Sidebar once it is extracted.

---

## 6.2 Component membership

`ComponentSpec::usedBy` records which applications use a component, by profile
id. The gallery's application filter reads it and holds no membership rules of
its own; `everyComponentDeclaresItsApplications()` fails if a spec forgets.

It is **declared, not measured** — most applications do not link NovuUI yet, so
there is nothing to measure until they do. The lists are seeded from §3 and §4
of this document and become fact as each application migrates. NovuExplorer and
NovuHub have no profile of their own, so their usage is recorded under
`novuspace`.

---

## 7. The gallery

The component gallery renders whatever is in the registry, so every component
moved into NovuUI by the steps above appears in it automatically — that is the
point of the registry, and no gallery code changes for any of them.

What the gallery cannot show today is *who uses what*. A "Coverage" page fed by
a small checked-in table — component × application, with the file each product
still has its own copy in — would make the audit above a live document instead
of a snapshot that goes stale the week after it is written. That is worth doing
once §6 is under way and the numbers start moving.
