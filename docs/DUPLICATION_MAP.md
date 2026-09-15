# Two shared UI libraries

Found 2026-09-14, during the NovuSpace extraction.

`j:\Github\NovusionUI` is not a placeholder. It is a **complete, shipping,
shared Qt6 component library**, moved with its history out of
`NovuHub/src/ui/brand/`, in namespace `novu::brand`, and **NovuHub and
NovuExplorer both link it today**.

`j:\Github\NovuUI` — this repository — is a second shared component library
with the same purpose, in namespace `novu::ui`.

Neither knows the other exists. Between them they implement the same twenty
components twice.

This document exists so the consolidation is a decision taken once with the
whole picture in front of it, rather than twenty decisions taken one component
at a time by whoever touches them next.

## Why this was not obvious

The task that started this work says "NovusionUI" throughout and the working
directory is `NovuUI`. Those read as one name for one thing. They are not.

## The map

`NovusionUI` (`novu::brand`) against `NovuUI` (`novu::ui`):

| NovusionUI | NovuUI | State |
|---|---|---|
| `Pulse` (BrandPulse.h) | `Pulse` (Motion.h) | **Duplicate.** Same 30 fps shared clock, same reference counting, same `ringAngle()` / `glow()` phase arithmetic. NovuUI's was written on 2026-09-14 from the NovuHub *Python* prototype without knowing the C++ one existed. |
| `Hairline` (BrandWidgets.h) | `Separator` | **Duplicate.** |
| `IconButton` (Controls.h) | `IconButton` | **Duplicate.** Both square, both painted, both with an accent "in force" state. |
| `Pill`, `Tag`, `StatusPill` (BrandWidgets.h), `StatusBadge` | `Badge` + `pill()` + `statusBadge()` | **Duplicate**, and NovusionUI splits into four classes what NovuUI carries as one class with a `Tone` and two flags. |
| `Dot` (BrandWidgets.h) | — | Only in NovusionUI. NovuSpace needs it (`.livedot`). |
| `Chip`, `SwatchRow` (Chips.h) | `Chip`, `ChipBar` | **Duplicate.** |
| `SegmentedToggle` (Controls.h) | `Segmented` | **Near-duplicate.** NovusionUI's takes glyphs only; NovuUI's takes glyphs or words, because the NovuSpace design needs words (`.chipset` density presets). |
| `Toggle` (BrandPage.h) | `Toggle` | **Duplicate.** |
| `Select`, `Slider` (BrandPage.h) | `ComboBox`; — | Duplicate / missing. |
| `BrandButton`, `BackButton` | `Button` | **Duplicate.** |
| `Card` (BrandWidgets.h) | `Card` | **Duplicate.** |
| `BrandLabel` | `label()` (Text.h) | **Duplicate.** |
| `FlowLayout` (BrandFlowLayout.h) | `FlowLayout` | **Duplicate.** |
| `Theme` (BrandTheme.h) | `ThemeManager` + `ThemeProfile` | **Duplicate, and they disagree.** NovusionUI has one live theme singleton; NovuUI has per-subtree profiles resolved by walking parents, which is what lets one process host several products at once. |
| `Menu`, `MenuItem` (BrandMenu.h), `PopupMenu` | — | Only in NovusionUI. NovuSpace needs it (`.popover` / `.pitem`). |
| `Tooltip`, `HelpTip` | `HelpOverlay` (different thing) | Only in NovusionUI. NovuSpace needs it (`.tip`). |
| `CommandPalette` | — | Only in NovusionUI. NovuSpace has `.omni`. |
| `FloatingWindow` | — | Only in NovusionUI. NovuSpace needs it (`.floatwin`). |
| `NativeFrame`, `WindowShadow` | `WindowShell` (has its own native frame) | **Duplicate**, and this one is the expensive kind: two separate answers to Windows `WM_NCCALCSIZE` / `WM_NCHITTEST`. |
| `NovuAppMark` | — | Only in NovusionUI. |
| `NavRail` (Controls.h) | — | Only in NovusionUI. NovuSpace needs it (`.rail-nav`). |
| `Page`, `SectionGrid`, `SettingsBlock`, `ThemeOption` (BrandPage.h) | `Layout.h`, `Form.h` | Overlapping. |
| `Avatar`, `HoverArea`, `CheckMark` | — | Only in NovusionUI. |

Roughly **fifteen duplicated components**, and about **ten that exist only in
NovusionUI and that the NovuSpace design needs**.

## What each one is better at

Not a tie, and not a rout either.

**NovusionUI is ahead on coverage.** Menu, Tooltip, FloatingWindow,
CommandPalette, NavRail, AppMark and the window shadow are written, shipping
and being used by two applications. NovuUI has none of them.

**NovuUI is ahead on architecture.** Four things it has that NovusionUI does
not:

- **Profiles instead of a singleton.** `ThemeScope` marks a subtree as
  belonging to a product and `themeFor(widget)` resolves by walking up. One
  process can host NovuTrade, NovuMail and NovuVault at once and draw each in
  its own palette. A single live `Theme` cannot do that.
- **Tokens instead of colours.** `StyleSpec` / `ResolvedStyle`: a component
  holds role references and resolves them on demand, so a theme change is a
  repaint rather than a rebuild.
- **An inspection seam.** `styleSpec()` and `resolvedStyle()` let a test assert
  what a painted component means *and* what it comes to, with no screenshot.
- **A gallery.** Every component is previewable, in every profile, under
  scenarios, with the source one click away.

## The recommendation

**Consolidate into NovuUI, by porting NovusionUI's components onto NovuUI's
token system** — not by merging the two trees and not by picking one wholesale.

In that order:

1. **Stop the bleeding.** No new component in either library until its
   counterpart has been checked. That rule is already in
   `docs/MODULE_RULES.md`: a shared component is *extracted* from a shipping
   application, never written here as a look-alike. It was not followed on
   2026-09-14 because nobody knew there was anything to extract from.
2. **Port what only NovusionUI has**, in the order NovuSpace needs it: `Menu`,
   `Tooltip`, `Dot`, `NavRail`, `FloatingWindow`. Each arrives on `StyleSpec`
   with a gallery page and an inspection seam.
3. **Retire the duplicates** one at a time, each with NovuHub and NovuExplorer
   switched over in the same change, so there is never a period where both
   exist and both are used.
4. **The window frame last**, and carefully. Two native-frame implementations
   is the one duplicate that can break a window's resizing, snapping and drop
   shadow on a specific Windows build, and it is the hardest to test.

The alternative — keep both and draw a boundary — was considered and is worse:
the boundary would run through the middle of `Button`, `Badge` and `Theme`, and
a design system with two buttons in it is not a design system.

## And a third duplication, inside NovuUI itself

Found while looking for a place to put the shared painted-item primitive.

NovuUI has **three** answers to "a run of mutually exclusive choices":

| | What it is | Items are |
|---|---|---|
| `TabStrip` | `Variant::Pill` (section switcher) and `Variant::Segment` (view switcher above a table) | one `QPushButton` each |
| `Segmented` | The design's `.chipset`, words or glyphs | painted |
| `Chip` / `ChipBar` | An on/off label, and a wrapping strip of them | one `QPushButton` each |

`TabStrip::Variant::Segment` and `Segmented` are the same control. Add
NovusionUI's `SegmentedToggle` and `Chip` and there are **five** implementations
of one idea across the two libraries.

`TabStrip` and `ChipBar` are also the widget-per-item pattern the architecture
invariants exist to remove: a four-item strip is four `QPushButton`s, four
polish passes and four places for a stray stylesheet to land, for four labels
nobody can select or drag.

### What this blocks

The invariants say that when a second painted component needs per-item
interaction, the hit testing moves into a shared primitive rather than being
written twice. `Segmented` is the first; the popover menu (`.popover` /
`.pitem`, eighteen uses in the NovuSpace markup) is the second, and the module
bar and the side rail are the third and fourth.

So the next change is **not** another component. It is:

1. Extract the item-run mechanics out of `Segmented` — layout along an axis,
   `indexAt()`, hover, press-and-release-in-the-same-item, arrow keys,
   per-item tooltips — into one internal base.
2. Build `Menu` on it.
3. Fold `TabStrip` and `ChipBar` onto it, and delete their per-item buttons.

Doing it in that order means the primitive is shaped by two real callers before
anything is built on top of it, and no component is written twice on the way.

## What has been written on the wrong side of this

Everything added to NovuUI on 2026-09-14 that duplicates NovusionUI:

- `Motion.h` / `Motion.cpp` (`Pulse`) — duplicates `BrandPulse.h`
- `Separator.h` / `.cpp` — duplicates `Hairline`
- `IconButton.h` / `.cpp` — duplicates `Controls.h`'s `IconButton`
- `Toggle.h` / `.cpp` — duplicates `BrandPage.h`'s `Toggle`
- `Segmented.h` / `.cpp` — near-duplicates `SegmentedToggle`

None of it is wasted if the recommendation above is taken: all five are the
token-driven, inspectable versions the consolidation wants anyway, and all five
are already in the gallery. But they are a second implementation *today*, and
that is worth saying plainly rather than leaving to be discovered again.

`Icon.h` / `Icon.cpp` and the glyph table are **not** duplicates: NovusionUI
draws icons from `QIcon` resources through `drawTintedIcon()`, and has no
outline set of its own.


---

## Where this stands at the extraction freeze (2026-09-15)

The internal duplication above is **resolved by direction rather than by
deletion**: `Segmented`, `ModuleBar`, `NavList` and `TabBar` all derive from the
shared `ItemStrip`, which is the primitive this document asked for.
`TabStrip::Variant::Segment` and `Chip`/`ChipBar` still hold a widget per item
and are now marked in their own headers as superseded, because NovuSpace
consumes both today and a freeze is the wrong moment to rewrite a live
consumer's component.

The **two-library** duplication is unresolved and deliberately so: consolidating
`NovusionUI` into `NovuUI` means switching NovuHub and NovuExplorer over in the
same change, which is application migration. That is the next phase.

See `docs/EXTRACTION_STATUS.md`.


---

# Consolidation plan

## The decided target state

```
    NovuHub ───────┐
    NovuExplorer ──┤
    NovuAI ────────┼──→  NovuUI  (novu::ui)
    NovuSpace ─────┤
    NovuAdmin ─────┘

    NovusionUI (novu::brand)  →  0 consumers  →  remove
```

`NovuUI` is the target. `NovusionUI` is not an old copy to be tidied away — it
is the library NovuHub and NovuExplorer run on **today**, wired in by path:

```
NovuHub/CMakeLists.txt:24       set(NOVUSION_UI_DIR ".../../NovusionUI")
NovuExplorer/CMakeLists.txt:73  set(NOVUSION_UI_DIR ".../../NovusionUI")
```

with 52 files across the two applications including `novusion/ui/*` or naming
`novu::brand`. Moving or renaming that directory fails both builds at configure
time, before a single file compiles. It goes when the last consumer has left,
and not before.

### The rule for anything new

**New projects and mockups use `NovuUI` only. Do not add a new dependency on
`NovusionUI`.**

That includes the NovuAI rebuild. Every dependency added to the old library is
one more thing to unpick later, and the point of the target state above is that
the number only goes down.


**A plan only.** Nothing below has been done, and none of it may be done as part
of documentation work: every step switches a consumer, which is application
migration. See [PORTING_GUIDE §B](PORTING_GUIDE.md#b--migrating-an-existing-application).

## What is superseded, and by what

| NovusionUI (`novu::brand`) | replaced by | consumers today |
|---|---|---|
| `Pulse` (BrandPulse.h) | `novu::ui::Pulse` (Motion.h) | NovuHub, NovuExplorer |
| `Hairline` | `Separator` | NovuHub, NovuExplorer |
| `IconButton` | `IconButton` | NovuHub, NovuExplorer |
| `Pill`, `Tag`, `StatusPill`, `StatusBadge` | `Badge` + `pill()` + `statusBadge()` | NovuHub, NovuExplorer |
| `Dot` | `StatusDot` | NovuHub, NovuExplorer |
| `Chip`, `SwatchRow` | `Chip` / `ChipBar`, `Swatch` | NovuExplorer |
| `SegmentedToggle` | `Segmented` | NovuHub |
| `Toggle` (BrandPage.h) | `Toggle` | NovuHub |
| `Select`, `Slider` | `ComboBox`, `Slider` | NovuHub |
| `BrandButton`, `BackButton` | `Button` | NovuHub, NovuExplorer |
| `Card` | `Card` | NovuHub, NovuExplorer |
| `BrandLabel` | `label()` (Text.h) | NovuHub, NovuExplorer |
| `FlowLayout` | `FlowLayout` | NovuHub |
| `Avatar` | `Avatar` | NovuHub |
| `Menu`, `MenuItem`, `PopupMenu` | `Popover` | NovuHub, NovuExplorer |
| `Tooltip`, `HelpTip` | `Tooltip` | NovuHub, NovuExplorer |
| `CommandPalette` | `CommandPalette` | NovuExplorer |
| `FloatingWindow` | `FloatingPanel` / `ToolWindow` | NovuHub |
| `NavRail` | `NavList` / `Sidebar` | NovuHub |
| `Page`, `SectionGrid`, `SettingsBlock`, `ThemeOption` | `SettingsShell` / `SettingsPage` / `SettingsSection` | NovuHub |
| `Theme` (BrandTheme.h) | `ThemeProfile` + `ThemeManager` + `ThemeScope` | NovuHub, NovuExplorer |
| `NativeFrame`, `WindowShadow` | `WindowShell` | NovuHub, NovuExplorer |
| `NovuAppMark` | `AppMark` | NovuHub |
| `drawTintedIcon()` | `paintGlyph()` / `glyphIcon()` | NovuHub, NovuExplorer |

`HoverArea` and `CheckMark` have no direct replacement and are small enough to
fold into their call sites when the surrounding component moves.

## Order

Bottom-up, so nothing is ever half-migrated:

1. **Theme.** `BrandTheme::Theme` → `ThemeProfile` + `applyStyleSheet()` on each
   window. Everything else reads colours through it, so it goes first or every
   later step migrates a component into the wrong palette. Both applications in
   one change; it is wide but shallow.
2. **Motion.** `brand::Pulse` → `novu::ui::Pulse`. Small, and it removes one of
   two 30 fps timers per window.
3. **Leaves.** `Hairline`, `Dot`, `Pill`/`Tag`/`StatusPill`, `BrandLabel`,
   `Avatar`, `FlowLayout`. No dependants; one commit each.
4. **Controls.** `BrandButton`, `Toggle`, `Select`, `Slider`, `IconButton`,
   `Chip`, `SegmentedToggle`.
5. **Surfaces.** `Card`, then `Menu`/`PopupMenu`, `Tooltip`, `CommandPalette`.
6. **Structure.** `NavRail`, `Page`/`SettingsBlock`, `FloatingWindow`.
7. **The window frame, last and alone.** `NativeFrame` + `WindowShadow` →
   `WindowShell`. Two implementations answer `WM_NCCALCSIZE` and `WM_NCHITTEST`;
   getting this wrong breaks resizing, Aero Snap and the drop shadow on a
   specific Windows build, and it is the hardest to test. Its own change, its
   own verification, on its own.

## When a NovusionUI file may be deleted

When **no consumer remains** — NovuHub and NovuExplorer both migrated off that
component, checked by grep, with both applications building and their tests
green. Not when the first consumer moves.

`NovusionUI` as a repository goes when its last header does.

## What this plan does not decide

Whether `NovuUI` should be renamed `NovusionUI`. The task brief uses the company
name; this repository, its namespace `novu::ui` and its CMake target `Novu::UI`
do not. It is a mechanical change across every consumer and it is **not** part of
consolidation — flagged, not scheduled.
