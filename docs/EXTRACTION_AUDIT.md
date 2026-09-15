# Extraction audit — NovuAI, NovuAdmin, NovuExplorer → NovuUI

The audit that must precede migration. It decides, per UI concept, which
implementation is authoritative and whether the concept is shared at all.

**Nothing has been migrated.** This document is the input to that work.

Surveyed 2026-09-14.

---

## 0. Two things to settle before any code moves

**The library is called `NovuUI`.** The task brief says *NovusionUI*. This
repository is `NovuUI`, the namespace is `novu::ui`, and `Novu::UI` is the
CMake target consumed by NovuAdmin and NovuSpace today. Renaming it is a
mechanical but wide change across every consumer, and it is not part of
extraction. **Flagged for a decision; this document uses NovuUI throughout.**

**NovuAI was restructured since the previous audit.** Its sources moved from
`ui/src/**` to `src/frontend/**`, `src/backend/**` and `src/modules/**`, with
only `ui/tests` left behind. Every NovuAI path in `ECOSYSTEM_AUDIT.md` is
therefore stale; the paths below are current.

---

## 1. Method, and how far it goes

Class inventories come from `class X : public Q…` across each repository's
headers. Usage counts come from matching identifiers in sources — they say
*how much of a thing an application does*, not how well.

The chrome, theme and semantic layers were read in depth during earlier work.
The sidebar, tab, menu, settings and panel implementations were **inventoried,
not read line by line**. That is enough to classify and to name the likely
authoritative implementation; it is not enough to write the generalised API.
Step 1 of every SHARED row below is "read the winning implementation properly".

Design references under `g:\Design\` were surveyed for which concepts have an
approved design at all — see §4.

---

## 2. Extraction matrix

| UI concept | NovuAI | NovuAdmin | NovuExplorer | Existing NovuUI | Action |
|---|---|---|---|---|---|
| **Title bar** | `src/frontend/widgets/TitleBar`, `FramelessDialog` | none — native `QMainWindow` | `src/ui/title_bar_widget`, `main_window` `nativeEvent`, `novu_popup_window` | `TitleBar`, `WindowShell`, `AppWindow`/`ToolWindow`/`Dialog` | **DUPLICATE** — shared exists, migrate all three |
| **Sidebar** | `LeftSidebar`, `HubWorkspaceSidebar`, `KnowledgeCategoryRail` | none | `PreviewSidebar`, `ConsoleCommandsSidebar`, `RailSplit` | none | **SHARED** — NovuAI authoritative |
| **Right rail** | `RightSidebar`, `KnowledgeCategoryRail` | none | `RailSplit`, `PreviewSidebar` | none | **SHARED** — same component mirrored, not a second one |
| **Tabs** | `EditorTabWidget : QTabWidget` | none | `ColoredTabBar : QTabBar`, `ConsoleTabWidget`, `ReorderableTabHelper` | `TabStrip` — *a segmented switcher, a different concept* | **NEEDS_API_DESIGN** — Explorer authoritative |
| **Buttons** | QSS on `QPushButton`, 294 sites | consumes `novu::ui::Button` | QSS, 96 sites | `Button`, `NovuStyle` | **ALREADY_SHARED** — migrate AI and Explorer |
| **Menus** | `QMenu` 111 sites, own QSS | — | `QMenu` 70 sites, own QSS | **nothing** | **SHARED** — new; NovuUI styles no menu at all |
| **Tooltips** | `setToolTip` 255 sites | 11 sites | 65 sites, own `QToolTip` QSS | **nothing** | **SHARED** — new |
| **Dialogs** | `FramelessDialog` + ~8 | **14 `Create*`/`Edit*` dialogs** | ~8 + `NovuPopupWindow` | `Dialog`, `FeedbackDialog` | **DUPLICATE** (shell) **+ SHARED** (`FormDialog`, from NovuAdmin) |
| **Settings shell** | `SettingsWindow` + 11 `Settings*Page` (legacy `*SettingsTab` alongside) | none | `SettingsDialog` + 8 `*SettingsWidget` | none | **SHARED** — NovuAI authoritative |
| **Bottom panel** | `DevModeLogPanel` | none | `ConsoleWidget`, `ConsoleTabWidget`, `ConsoleStrip`, `ConsoleSessionStrip` | none | **NEEDS_API_DESIGN** — shell only, see §3 |
| **What's New** | `src/modules/whats_new/WhatsNewManager` + `metadata/novuai/whats_new.json` + generator | none | none | none | **NEEDS_API_DESIGN** — presentation shared, feed stays app-owned |
| **About** | not found in code | none | `src/ui/shell/ReleaseInfo` | none | **APP_SPECIFIC** — one and a half implementations is not a pattern yet |
| **Empty / error / loading** | ad-hoc | ad-hoc | ad-hoc | `EmptyState`, `InfoBanner`, `ProgressBar` | **ALREADY_SHARED** — migrate |
| **Theme / tokens** | `src/frontend/ui/Theme.h` | consumes NovuUI | `src/ui/theme/theme_manager` | `ThemeProfile`, one palette, semantic palette | **DUPLICATE** — migrate |
| **Help** | `src/backend/core/help/*`, `HelpDockWindow` | none | none | `HelpOverlay` — *annotation, a different concept* | **APP_SPECIFIC** for now |

---

## 3. Per-concept notes that change what gets built

### Menus and tooltips are the biggest untouched gap

NovuUI's scope sheet styles `QCheckBox QComboBox QCompleter QDateEdit QFrame
QHeaderView QLabel QLineEdit QPlainTextEdit QPushButton QScrollArea QScrollBar
QTableWidget` — and **not `QMenu`, not `QToolTip`**. Two applications style
them independently, 181 menu sites between them.

This is the one row where the brief's "existing NovuUI Menu/Popover behavior
should remain the reference" does not apply: there is no existing behaviour to
be the reference. It has to be extracted, and NovuExplorer's is the larger
implementation (context menus on the file list, nested submenus).

### `TabStrip` is not a tab bar

NovuUI's `TabStrip` is "a row of buttons where exactly one is active" — a
section switcher. What NovuAI and NovuExplorer have is **document tabs**:
closeable, reorderable, coloured, with a context menu. Explorer's
`ColoredTabBar` + `ReorderableTabHelper` is the complete one.

Extending `TabStrip` to cover both would produce a component with two
personalities. The recommendation is a separate `DocumentTabs`, and to say so
in `TabStrip`'s header so the next person does not merge them.

### NovuAdmin is where the dialog pattern actually lives

Fourteen `Create*` / `Edit*` dialogs in one application, all the same shape:
a form, validation, a cancel/confirm footer. NovuAI and NovuExplorer have
one-off dialogs. If a shared `FormDialog` is extracted from anywhere, it is
from NovuAdmin — and NovuAdmin already consumes NovuUI, so it is also the
lowest-risk migration.

### The Console: extract the shell, leave the state

The brief singles this out and the inventory agrees it is the right worry.
`ConsoleWidget` + `ConsoleTabWidget` + `ConsoleStrip` + `ConsoleSessionStrip`
is four classes carrying session lifecycle and persistence, not a panel.

What is generic is *a dockable bottom panel with tabs and a strip*. What is not
is which sessions exist, whether the panel is open, and what is written to
disk. A shared `BottomPanel` must take visibility as an input and report
intent as a signal — it must not own an `isOpen` of its own, or Explorer ends
up with two answers to that question.

### Sidebar and right rail are one component

`LeftSidebar` / `RightSidebar` in NovuAI, `PreviewSidebar` / `RailSplit` in
Explorer. A rail is a sidebar on the other edge. One component with an edge
property, not two.

### What's New: the presentation is shared, the feed is not

NovuAI's is data-driven — a JSON file and a generator. That is the right shape,
and the presentation (a list of releases with sections) generalises. The feed,
its schema and where it is fetched from stay in the application.

No other application has one, so this is **NEEDS_API_DESIGN**: build it when
the second consumer exists, or the API will be NovuAI's file format wearing a
generic name.

---

## 4. What `g:\Design\` contributes

Organised as `design/<App>/{mockups,branding,shots,references}` with
`design/Shared/{design-system,branding,icons,references}`, mirrored by runnable
PySide6 prototypes under `prototypes/qt6/`. `prototypes/qt6/Shared/novu_ui.py`
is the frozen graphite theme every prototype imports.

| application | what exists | use in this task |
|---|---|---|
| NovuAI | many authored screens — Chat, Coding, Documents v2/v3, Flow, Learn, Agent Design, Debug Launcher, Release Center, What's New v01/v02 | richest source for sidebar, rail and What's New intent |
| NovuExplorer | a 13-part HTML brand mockup series | brand/appearance reference, not behaviour |
| NovuAdmin | **brand mockup only** | no approved screen design — production is the reference |

`Dark_Blue/` is explicitly a pre-graphite historical snapshot. Per the stated
priority order it ranks below current production and must not be used to
resolve anything.

**Consequence for the matrix:** NovuAdmin has no approved design, so for every
NovuAdmin row the current production implementation *is* the reference. And
because `g:\Design\` holds a What's New design that production does not
implement in two applications, that row stays NEEDS_API_DESIGN rather than
becoming a redesign.

Nothing here becomes a runtime dependency. The prototypes' `novu_ui.py` is a
*reference implementation of the same palette NovuUI already holds*, not a
source to import.

---

## 5. Recommended order

Follows the brief's sequence, narrowed to what this audit found.

| # | work | why here | risk |
|---|---|---|---|
| 1 | **Theme/tokens** — Explorer and NovuAI onto `ThemeProfile` | both palettes are already the NovuUI palette; deletes two copies | low, wide |
| 2 | **Buttons, empty/error/loading** | ALREADY_SHARED, just not consumed | low |
| 3 | **Menus + tooltips** | biggest gap, and everything below inherits the look | medium — new component |
| 4 | **Dialog shell + `FormDialog`** | NovuAdmin consumes NovuUI already | low |
| 5 | **Sidebar / rail** | five implementations collapse to one | medium |
| 6 | **Document tabs** | Explorer authoritative | medium |
| 7 | **Settings shell** | consumes 2, 4 and 5 | high — 19 pages across two apps |
| 8 | **Title bar / window chrome** | shared exists and is tested; independent of the rest | medium, can run in parallel |
| 9 | **Bottom panel shell** | needs the state boundary of §3 agreed first | highest |

Steps 1–4 are additive or already-built and can keep every build green. Step 7
is the one that should not start until 2, 4 and 5 have landed.

---

## 6. Explicitly not extracted

| | why |
|---|---|
| NovuAdmin's `*Panel` classes | domain screens — Applications, Bots, Orders, Plans. Composition, not components. |
| Explorer Console session/persistence | authoritative application state, see §3 |
| NovuAI `Help*` | one application; `HelpOverlay` is a different concept and must not absorb it |
| About | one and a half implementations |
| `KnowledgeCategoryRail`, `FileTreeWidget`, `AgentTimelineWidget`, … | feature content that *sits in* a shared surface |

---

## 7. Open questions

1. **`NovusionUI` vs `NovuUI`** — rename, or is the brief using the company
   name informally? Blocks nothing in the audit; blocks every header in a
   migration.
2. **Document tabs vs `TabStrip`** — confirm these stay two components.
3. **Bottom panel state boundary** — the shared panel takes visibility as an
   input and emits intent, and Explorer keeps ownership. Worth agreeing
   explicitly before step 9, because getting it wrong is exactly the
   regression the brief warns about.
4. **What's New** — build now against NovuAI's format, or wait for a second
   consumer? Building now risks a generic name over one application's schema.
