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
