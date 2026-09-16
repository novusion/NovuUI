# NovuAI — UI port mapping

**Prepared, not started.** This is the input to the NovuAI UI rebuild; no NovuAI
code has been written and NovuUI is frozen. Read
[PORTING_GUIDE §A](../PORTING_GUIDE.md#a--ui-rebuild-from-an-approved-design)
first — the rebuild is **UI only**: no backend, no runtime, no providers, no
persistence.

## Sources

| | |
|---|---|
| approved screens | `g:\Design\design\NovuAI\mockups\*.png` — 38 authored screens |
| brand mockup | `NovuAI-brand-mockup.html` (+ `versions/17` is the latest) |
| shared reference | `design/Shared/references/Novu_App_Design.png` |
| shared palette | `design/Shared/design-system/Novu_Palette.png` |
| current C++ | `j:\Github\NovuAI\src\frontend\**` — **read-only**, not migrated |
| Python mockup | read it; **do not run it** |

Screens: Chat · Coding · Documents (v2, v3, Notes, NotesBoard) · Flow (Kanban,
Gantt, Timeline, Calendar, Summary, Structure, Agent Workload, Sidebar Sections)
· Models (Local) · Learn · Agent Design · Source Control · Task Panel · Release
Center (v1, v2) · Debug Launcher · Summary View · What's New · Hub AI-Core
Bridge · Settings ×10 (General, Server, Runtime, Local AI, Cloud AI, API Keys,
Data, DevTools, Integrations, Sync/Share).

## Which library

`NovuUI` (`novu::ui`), and nothing else. `NovusionUI` (`novu::brand`) is the
original shared library that NovuHub and NovuExplorer still run on; the NovuAI
rebuild must not depend on it —
[DUPLICATION_MAP](../DUPLICATION_MAP.md#the-decided-target-state).

## Theme

NovuAI scopes its window to the `novuai` profile:

```cpp
applyStyleSheet(mainWindow, QStringLiteral("novuai"));
```

Surfaces, text and status come from the shared palette. The accent is
**`#4DA3FF`**, stated in the mockup's own tokens rather than read off a
screenshot — `assets/css/00-tokens.css`: `--acc:#4DA3FF; --acc-l:#7DBEFF;
--acc-d:#2B7FD4; --ink-on-accent:#0C151F`. This was recorded unresolved once,
for the wrong reason: the earlier search was for a hex in the PNG screens, and
the value was in the CSS the whole time. See
[THEMING §4](../THEMING.md#4-application-accents).

---

## Area mapping

`—` in **Gap** means nothing is missing.

### Shell

| NovuAI pattern | NovuUI | Mock data | Stays local | Gap |
|---|---|---|---|---|
| Application window, frame, resize | `AppWindow` / `WindowShell` | — | — | — |
| Title bar: mark, name, version, window controls | `TitleBar`, `AppMark`, `glyphIcon()` | version string | — | — |
| Product switcher (NovuAI · NovuBuilder · NovuExplorer · …) | `ModuleBar` | product list | which products are installed | — |
| Utility row (Find, Favourites, Diagnostics, Console, Downloads, About, Settings) | `ModuleBar` or `TabStrip` | — | what each opens | — |
| Status bar: workspace, branch, role, sync, transfer rates | `StatusBar`, `StatusDot`, `StatusLine` | fixed strings | sync state | — |
| Left sidebar | `Sidebar` + `NavList` | workspace tree | tree model | — |
| Workspace / Explorer switch in the sidebar | `Segmented` or `TabStrip` | — | — | — |
| Right rail (Task Queue, Commits, Git) | `Sidebar` (right edge) + `Card` | sample tasks/commits | — | — |
| Bottom panel (Console, Output) | `BottomPanel` + `LogView` | canned log lines | session lifecycle | — |
| Splitters between the three columns | `Splitter` | — | saved sizes | — |
| Detached / floating windows | `FloatingPanel`, `ToolWindow` | — | what may detach | — |

### Navigation and content

| NovuAI pattern | NovuUI | Mock data | Stays local | Gap |
|---|---|---|---|---|
| Workspace tree with counts and status dots | `NavList` (`setCount`, `setStatus`) or `TreeView` | tree JSON | the model | — |
| Quick actions block in the sidebar | `NavList` section + `Button` | — | actions | — |
| Document tabs (closeable, reorderable, dirty) | `TabBar` (`State::Modified`, `Loading`, `Failed`) | open documents | document model | — |
| Section switcher inside a page | `TabStrip` | — | — | — |
| Page heading, description, actions | `PageHeader` | — | — | — |
| Breadcrumb | `Breadcrumb` (in `PageHeader.h`) | path | — | — |
| Command palette | `CommandPalette` | command list | the commands | — |
| Collapsible sections | `Disclosure` | — | — | — |

### Chat

| NovuAI pattern | NovuUI | Mock data | Stays local | Gap |
|---|---|---|---|---|
| Composer header: agent / model / mode pickers | `PageHeader` actions + `ComboBox` | model list | provider registry | — |
| Message list | **application** — scrolling list of message widgets | canned conversation | rendering, streaming | see note |
| Message bubble, author, timestamp | **application** | — | — | see note |
| Input box + Send | `textArea()` + `Button` (+ `Popover` for the split menu) | — | send | — |
| Context / Summary / Short selects | `Segmented` or `ComboBox` | — | — | — |
| Attach / mention / grid icon buttons | `IconButton` | — | — | — |
| Token and context readout, progress | `StatusLine`, `ProgressBar` | numbers | counting | — |
| "Task created…" inline notice | `InfoBanner` | — | — | — |

> **Note — the message list stays in the application.** A chat transcript is
> NovuAI's domain: authorship, streaming, tool calls, code blocks, retries. The
> *surfaces* around it are shared (`Card`, `Badge`, `Avatar`, `StatusDot`,
> `Separator`, `LogView` for tool output), the transcript itself is not. It
> would be a `NovuAIChatView` in NovuAI, and that is correct.

### Coding, Source Control, Release

| NovuAI pattern | NovuUI | Mock data | Stays local | Gap |
|---|---|---|---|---|
| Diff view | `DiffView` | sample hunks | the diff | — |
| Changed-file list with `+`/`−` and status | `NavList` or `TableView` + `Badge` | file list | VCS state | — |
| Commit list | `Card` + `Badge` + `PropertyRow` | commits | git | — |
| Branch / staged / Commit panel | `Card`, `Field`, `Button`, `Popover` | — | git | — |
| File tree | `TreeView` | tree JSON | the model | — |
| Task queue cards, priority | `Card` + `Badge` + `IconButton` | tasks | scheduling | — |
| Current task: progress and subtask checklist | `ProgressBar`, `CheckBox`, `SettingRow` | — | execution | — |
| Release Center | `TableView`, `Steps`, `Badge` | releases | release logic | — |

### Flow, Documents, Models, Learn, Agent Design

| NovuAI pattern | NovuUI | Mock data | Stays local | Gap |
|---|---|---|---|---|
| Kanban board | `Card` + `Badge` + drag | columns/cards | board model, DnD rules | **GAP — no board or drag primitive** |
| Gantt / Timeline | — | — | — | **GAP — domain visualisation, likely stays local** |
| Calendar | — | — | — | **GAP — no calendar; decide at port time** |
| Flow summary tiles | `StatCard`, `Tile` | metrics | — | — |
| Structure / agent workload charts | — | — | charts | **GAP — no chart component; NovuSpace keeps its own too** |
| Documents list and notes board | `TableView`, `Card`, `FlowLayout` | documents | the model | — |
| Models (local): list, size, status, actions | `TableView` + `Badge` + `StatusDot` + `IconButton` | model list | runtime | — |
| Learn | `Card`, `PageHeader`, `Disclosure` | content | content model | — |
| Agent Design canvas | — | — | — | **GAP — node canvas is domain UI, stays local** |
| Debug Launcher | `Card`, `Field`, `Button`, `LogView` | — | launching | — |

### Settings — ten pages

| NovuAI pattern | NovuUI | Mock data | Stays local | Gap |
|---|---|---|---|---|
| Settings window shell, navigation, search | `SettingsShell` | page list | — | — |
| Page heading and description | `SettingsPage` → `PageHeader` | — | — | — |
| Grouped sections | `SettingsSection` | — | — | — |
| Name + hint + control rows | `SettingRow` | current values | the settings model | — |
| Text, number, path fields with validation | `Field` (`setError`, `setRequired`) | — | validation rules | — |
| On/off | `Toggle` | — | — | — |
| Ranges (context size, temperature, retries) | `Slider` | — | — | — |
| Choice of a few | `Segmented`; of many | `ComboBox` | — | — | — |
| Secrets (API keys) | `SettingRow::setMasked()` | — | storage | — |
| Destructive actions (clear data, sign out) | `Button` Danger + `Dialog::confirm()` | — | what it deletes | — |
| Per-provider blocks | `Card` + `SettingsSection` | providers | provider registry | — |

### Overlays and states

| NovuAI pattern | NovuUI | Mock data | Stays local | Gap |
|---|---|---|---|---|
| Context menus, overflow menus | `Popover` | items | actions | — |
| Tooltips | `Tooltip` | — | — | — |
| Dialogs: create, edit, confirm, error | `Dialog` (heading, tone, busy, form error, `confirm()`) | — | what is submitted | — |
| Sign-in / gate | `GateCard` | — | auth | — |
| Toasts | `Toast` | — | what is announced | — |
| Loading | `Skeleton`, `Spinner`, `ProgressBar` | — | — | — |
| Empty | `EmptyState` | — | copy | — |
| Error | `InfoBanner` Danger, `Dialog::reportError()` | — | the error | — |
| Keyboard shortcuts | `KeyCap`, `ShortcutList` | bindings | bindings | — |
| What's New | — | — | — | **DEFERRED** — [EXTRACTION_STATUS §5](../EXTRACTION_STATUS.md) |

---

## Gaps

Recorded, not built — the library is frozen and a component shaped by one screen
is a component shaped wrong.

| gap | first seen | likely disposition |
|---|---|---|
| Kanban board + card drag | Flow Kanban | **decide at port time** — may be a NovuUI primitive if a second application wants one; NovuAI-only otherwise |
| Gantt / Timeline | Flow | **app-specific** — domain visualisation |
| Calendar | Flow Calendar | **decide at port time** |
| Charts | Flow Structure, Agent Workload | **app-specific for now** — NovuSpace keeps its own charts too, so there is no second consumer yet |
| Agent Design node canvas | Agent Design | **app-specific** |
| Chat transcript view | Chat | **app-specific** — see the note above |
| What's New | Release Center | **DEFERRED** — one consumer, one schema |

None blocks the rebuild: every one of them is either a screen NovuAI owns or a
decision that is cheaper after the shell exists.

---

## Suggested order

1. **Shell** — `AppWindow`, `TitleBar`, `ModuleBar`, `Sidebar`, `StatusBar`,
   `Splitter`. Reviewable on its own, and everything else lands inside it.
2. **Settings** — ten pages on `SettingsShell`, the largest area with the
   fewest unknowns and no gaps at all.
3. **Chat** — the shell around a placeholder transcript.
4. **Coding / Source Control** — `DiffView`, `TabBar`, file tree.
5. **Models, Documents, Learn** — mostly `TableView` and `Card`.
6. **Flow** — last, because it carries every gap.

Each step: build, run, review. Nothing integrated.

---

## Related

[PORTING_GUIDE](../PORTING_GUIDE.md) · [THEMING](../THEMING.md) ·
[COMPONENTS](../COMPONENTS.md) · [EXTRACTION_STATUS](../EXTRACTION_STATUS.md)
