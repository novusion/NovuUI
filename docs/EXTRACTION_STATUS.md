# NovuUI extraction — final status

Frozen 2026-09-15.

**"Extraction complete" means the shared library is ready. It does not mean the
applications use it.** Two of five consume any of it today, and neither
consumes much. Keeping those two statements apart is the point of this
document.

---

## 1. What is built

62 public headers; **65 components with a gallery page**. The eight headers
without a page of their own are infrastructure rather than visual components —
`ItemStrip` and `MonoLineView` are the two painted base classes their
subclasses are shown through, `TitleBar` is drawn by `WindowShell`, and
`Style`, `StyleSheet`, `StyleSpec`, `ThemeManager` and `ThemeScope` are the
theming machinery the *Theme roles* page covers.

Tests: **153 passing, 0 failing** (128 gallery, 25 window chrome).

---

## 2. Actual usage, today

Measured from each application's own sources: which NovuUI headers it includes,
and whether it names a class from them.

| application | links NovuUI | headers used |
|---|---|---|
| **NovuSpace** | yes | 15 — Badge, Button, Card, Chip, Feedback, Fields, StatCard, StyleSheet, TabStrip, TableView, Text, ThemeManager, ThemeProfile, ThemeScope, Tile |
| **NovuAdmin** | yes | 4 — Badge, Button, TableView, Theme |
| **NovuAI** | **no** | — |
| **NovuExplorer** | **no** | — |
| NovuHub, NovuVoice, NovuTrade | no | — |

So of the ~45 components added during this extraction, **no application
consumes one yet**. That is by instruction — migration is the next phase — but
it is the number that matters when anyone asks whether this is "done".

The gallery reflects this. `ComponentSpec::usedBy` is now measured fact and
`ComponentSpec::designedFor` is stated intent; the two are separate fields,
shown on separate lines, and `declaredUsageIsMeasuredAndNotIntended()` fails
the build if anything claims NovuAI or NovuExplorer as a current consumer.

---

## 3. NovuSpace coverage

The approved design carries **347 CSS classes**. Every one has a disposition:

| | count | what it is |
|---|---|---|
| **COVERED** | 167 | a class a NovuUI component draws today |
| **APP/DOMAIN-SPECIFIC** | 81 | NovuSpace's own screens — the bot builder (`.bb-*`), the mail reader (`.mr-*`), the vault gate (`.sb-*`), the browser surface, the trading charts and the order-book ladder |
| **VARIANT OR STATE** | ~99 | CSS modifiers — `.on`, `.off`, `.open`, `.err`, `.idle`, `.sm`, `.lg`, `.c2`, `.wide` — which in NovuUI are enumerators and flags on the component they modify rather than classes of their own |

There is no unexplained gap. The third row is the architecture working: six badge
tone classes are one `Badge::Tone`, four density classes are one `Segmented`.
Reaching 100% *class* coverage would have meant turning `.ladder` and `.lrow`
into generic components, which is the opposite of what the boundary is for.

---

## 4. Cross-source dispositions

Every pattern the extraction identified, and where it ended up.

| pattern | source | disposition |
|---|---|---|
| Title bar, window frame | all four | **REUSE** — `WindowShell` / `TitleBar` existed; all four migrate onto it |
| Sidebar and right rail | NovuAI, NovuExplorer | **GENERALIZED** — one `Sidebar` with an edge, plus `NavList`; five implementations collapse to two |
| Document tabs | NovuExplorer | **GENERALIZED** — `TabBar`, deliberately *not* `TabStrip` (see §6) |
| Menus | NovuAI, NovuExplorer | **GENERALIZED** — `Popover`; NovuUI styled no menu at all before |
| Tooltips | all | **GENERALIZED** — `Tooltip` |
| Dialogs, forms, validation | NovuAdmin | **GENERALIZED** — `Dialog` gained heading, tone, busy, form error and `confirm()`; `Field` carries per-field validation |
| Settings shell | NovuAI, NovuExplorer | **GENERALIZED** — `SettingsShell` / `SettingsPage` / `SettingsSection` |
| Bottom panel / console | NovuExplorer | **GENERALIZED** — `BottomPanel`, with visibility as an input |
| Empty / error / loading | all | **REUSE** — `EmptyState`, `InfoBanner`, `ProgressBar`, `Skeleton`, `Spinner` |
| Theme and tokens | all | **MERGED** — `ThemeProfile`; two palette copies retire on migration |
| Shared animation clock | NovuHub | **MERGED** — `Pulse`, ported from the prototype |
| Avatar | NovuHub, NovuVoice, NovuAI | **MERGED** — ported from NovusionUI's `Dot`/`Avatar` |
| Slider | NovuHub | **GENERALIZED** — with the readout the design draws |
| Colour swatches | NovuExplorer | **MERGED** — ported from NovusionUI's `SwatchRow` |
| Console session lifecycle | NovuExplorer | **APP_SPECIFIC** — state, not presentation |
| `*Panel` domain screens | NovuAdmin | **APP_SPECIFIC** — composition, not components |
| `Help*`, About | NovuAI, NovuExplorer | **APP_SPECIFIC** — one and a half implementations is not a pattern |
| File tree, agent timeline, knowledge rail | NovuAI, NovuExplorer | **APP_SPECIFIC** — content that sits *in* a shared surface |
| **What's New** | NovuAI only | **DEFERRED** — see §5 |

---

## 5. Deferred

### What's New — insufficient cross-application evidence

One application has one, and it is data-driven from one JSON file with one
schema. The presentation generalises; the feed, its schema and where it is
fetched from do not.

Building it now would put a generic name over NovuAI's file format, and the
second consumer would then arrive to find an API shaped like the first one's
storage. The cost of waiting is that NovuAI keeps its own screen a while
longer. The cost of not waiting is a shared component nobody else can use.

**Reconsider when:** a second application needs one, or the shared contract
becomes clear without one.

---

## 6. Known limitations

**Three components still hold a run of items as one widget each.**
`TabStrip`, `Chip` and `ChipBar` predate `ItemStrip` and keep a `QPushButton`
per item. `Segmented`, `ModuleBar`, `NavList` and `TabBar` are on the shared
painted primitive. `TabStrip::Variant::Segment` and `Segmented` are the same
control drawn twice.

They were left alone deliberately: NovuSpace uses `TabStrip` today, it is one
of the fifteen headers it actually consumes, and rewriting the internals of a
live consumer's component during a freeze is how a freeze stops being one. Both
headers now say so, so the next person does not merge them by accident.

**`usedBy` is header-granular.** A component whose header declares several
classes — `Feedback.h` holds seven — is credited to an application that
includes the header and names any one of them. The measurement errs towards
over-reporting within a header, never across one.

**Three form-row shapes coexist**: `Field` (caption above control, validation),
`SettingRow` (name and hint left, control right), `formRow()` (fixed label
column) and `propertyRow()` (read-only key/value). They are four different
layouts for four different contexts, all present in the approved designs, and
none is a variant of another.

---

## 7. Architecture, verified

- **Tokens, not colours.** Components hold `StyleSpec` — role references — and
  resolve on demand. `componentsKeepTokensRatherThanResolvedColours()` and
  `theSameButtonResolvesDifferentlyPerProfile()` assert it.
- **One animation clock.** `Pulse`, reference-counted, stopped when nothing is
  on screen. One-shot transitions use `QVariantAnimation`, which Qt already
  drives from a single timer. No component owns a `QTimer`.
- **Painted decoration.** Captions, hints, separators, dots, badges, segments,
  menu items and section headings are painted. Widgets exist where interaction
  does.
- **Press and release.** Every painted interactive component acts on press *and*
  release in the same item; `aPickerActsOnPressAndReleaseTogether()` covers the
  cancelled gesture.
- **Testable without screenshots.** `styleSpec()` / `resolvedStyle()` on every
  painted component. There is no image-diff infrastructure and none is wanted.
- **Clean boundary.** No persistence, no network, no process, no database, no
  file model anywhere in `include/novu/ui` or `src/`. The only non-Qt includes
  are `<algorithm>`, `<array>`, `<cmath>`, `<functional>`, `<utility>` and the
  Windows headers the native frame needs.

---

## 8. Freeze

Feature extraction is closed. The next change to this library should be an
application migrating onto it, not another component.

`docs/DUPLICATION_MAP.md` records the second library, `NovusionUI`, and the
consolidation this does not yet perform.
