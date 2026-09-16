# The design contract

What NovuUI is allowed to decide, and what the approved mockups decide for it.

**A NovuUI default must never override a value an authoritative HTML states.**
Where this document and the library disagree, the library is wrong.

## The authorities

Current approved mockups. Older `versions/` and `frozen/` snapshots are history,
not authority.

| product | source |
|---|---|
| NovuAI | `design/NovuAI/mockups/assets/css/00-tokens.css` (+ `01-base`, `02-components`, `03-shell`, 23 view sheets) |
| NovuSpace | `design/NovuSpace/mockups/NovuSpace-Trade-brand-mockup.html` |
| NovuAdmin | `design/NovuAdmin/mockups/NovuAdmin-brand-mockup.html` |
| NovuExplorer | `design/NovuExplorer/mockups/NovuExplorer-brand-mockup-v2.html` |
| NovuHub | `design/NovuHub/references/NovuHub-brand-mockup.html` |

All under `g:\Design\`. NovuAI is the only one whose tokens are a separate file;
the others carry a `:root` block in the page.

NovuSpace's own token header states the split this document formalises:

> Surfaces, hairlines and the type scale are lifted verbatim from the NovuHub /
> NovuExplorer / NovuAdmin brand mockups, so NovuSpace reads as the same family.
> **The one thing that identifies the product is `--acc`.**

---

## 1. Shared contract — identical in every authority

These are library values. A product does not get its own.

### Surfaces and ink

| token | value | NovuUI |
|---|---|---|
| `--page` | `#0E1216` | `WindowTokens` page |
| `--window` | `#151A20` | `colors.windowBg` |
| `--bg` | `#1D2228` | `colors.panelBg` |
| `--panel` | `#232A33` | `colors.panelBgAlt` |
| `--card` | `#29323D` | `colors.surfaceHover` |
| `--card-h` | `#313B47` | `colors.surfaceActive` |
| `--text` | `#E7EDF5` | `colors.textPrimary` |
| `--text2` | `#B7C1CE` | `colors.textSecondary` |
| `--text3` | `#8793A3` | `colors.textDisabled` |

4/4. NovuAI and NovuExplorer add `--text4 #68737F`, a fourth weight the other
two do not draw.

### Status — two colours per role, not one

The decisive rule, and the one NovuUI had wrong. `.badge` is stated
**byte-identically** in the NovuSpace and NovuAdmin mockups:

```css
.badge.ok   { color:#6ADB9E; background:rgba(60,203,127,.10);  border-color:rgba(60,203,127,.40);  }
.badge.warn { color:#FFB14D; background:rgba(255,177,77,.10);  border-color:rgba(255,177,77,.40);  }
.badge.bad  { color:#E8A8B0; background:rgba(196,123,133,.12); border-color:rgba(196,123,133,.45); }
.badge.info { color:#4DA3FF; background:rgba(77,163,255,.10);  border-color:rgba(77,163,255,.38);  }
```

The word is written in a light ink; the plate behind it is mixed from a deeper
hue. **They are different values and neither derives from the other.**

| role | ink | plate | fill α | border α |
|---|---|---|---|---|
| Success | `#6ADB9E` | `#3CCB7F` | .10 | .40 |
| Warning | `#FFB14D` | `#FFB14D` | .10 | .40 |
| Danger | `#E8A8B0` | `#C47B85` | .12 | .45 |
| Info | `#4DA3FF` | `#4DA3FF` | .10 | .38 |

Danger sits heavier than Success deliberately: a pink plate at .10 disappears
where a green one reads.

NovuAI states the same shape on `.tag` and differs only in two border alphas
(.42 / .45 / .48). NovuExplorer agrees. The alphas above are the two-product
verbatim set.

> **What was wrong.** `SemanticHues` held the ink alone and `semanticColorFrom()`
> mixed the plate from it — so a Danger badge was plated in `rgba(#E8A8B0,.16)`
> where the design plates it in `rgba(#C47B85,.12)`. Every tint in the library
> was one step light, and the chart's bearish candle took the ink as its body
> fill. Fixed by `SemanticHue { ink, plate, fillAlpha, borderAlpha }`.

### Motion and elevation

| token | value | authorities |
|---|---|---|
| `--t-fast` | `110ms cubic-bezier(.3,.7,.3,1)` | NovuAI, NovuExplorer |
| `--t-med` | `160ms cubic-bezier(.3,.7,.3,1)` | " |
| `--t-slow` | `220ms cubic-bezier(.2,.8,.2,1)` | " |
| `--shadow-sm` | `0 2px 6px rgba(0,0,0,.28)` | " |
| `--shadow-md` | `0 12px 32px rgba(0,0,0,.42)` | " |
| `--shadow-lg` | `0 28px 72px rgba(0,0,0,.55)` | 4/4 — NovuSpace and NovuAdmin state it as their only `--shadow` |

---

## 2. Product theme profile — stated per product

### Accents

Every one of these is a token in the product's own mockup. None is inferred.

| product | `--acc` | `--acc-l` | `--acc-d` | `--ink-on-accent` |
|---|---|---|---|---|
| NovuAI | `#4DA3FF` | `#7DBEFF` | `#2B7FD4` | `#0C151F` |
| NovuSpace | `#D49335` | `#E3B87A` | `#AE792B` | `#10171A` |
| NovuAdmin | `#3CCB7F` | `#6ADB9E` | — | `#151A20` |
| NovuHub | `#3CCB7F` | — | — | `#151A20` |
| NovuExplorer | `#58B09E` | `#77CCB9` | `#3F8F80` | `#10171A` |
| NovuVoice | **unresolved** — no approved mockup exists | | | |

NovuAI's token header names three of them in one sentence: *"NovuAI is blue
(#4DA3FF) where Hub is green (#3CCB7F) and Explorer is teal (#58B09E)"*.
NovuSpace's says its amber was *"chosen in the preset evaluation and frozen"*.

NovuAdmin and NovuHub share the green on purpose — the same operator's tools.
It is also the Success plate; a product may wear a status hue as its accent,
and the layers stay separate because the accent moves per profile and Success
does not.

### Hairlines — two conventions, not one

| | `--hair` | `--hair2` | `--hair3` |
|---|---|---|---|
| NovuAI, NovuExplorer | `#252D36` | `#354150` | `#2C3540` |
| NovuSpace, NovuAdmin | `#29323D` | `#394553` | — |

NovuSpace claims to lift its hairlines "verbatim" from the other three, and
lifted NovuAdmin's rather than NovuExplorer's. Both sets are in the approved
material, so both are legitimate; this is a profile token.

### Chrome and density

| | NovuAI | NovuSpace | NovuAdmin | NovuExplorer |
|---|---|---|---|---|
| titlebar height | 44 | **42** | 44 | 44 |
| statusbar height | 26 | **32** | — | 24 |
| row height | 26 | — | — | 24 |
| sidebar width | 236 | — | — | 216 |
| tree indent step | 17 | — | — | — |
| `--titlebar-top` | `#1F252E` | `#1A1F26` | `#1A1F26` | `#1A1F26` |

**Not a shared density.** NovuAI's own header says it "inherits Explorer's
density", and then states a 26px row where Explorer states 24. The claim is
about intent; the tokens disagree. Chrome metrics are therefore profile tokens
with 44 as the default three of four state.

`--tree-step: 17px` is NovuAI's alone and is a real fix — its header records
that the workspace tree, Coding's files and UI Designer's structure had grown
17, 20 and 13px of their own. One step per profile, read by `TreeView`.

### Radii

`--r-sm 6 / --r-md 8 / --r-lg 11 / --r-xl 14 / --r-pill 999` in NovuAI and
NovuExplorer. NovuSpace and NovuAdmin state no radius scale and write literals
(8, 7, 10, 9, 11, 15, 999). The scale covers most of what they write; the odd
values are individual components, not a second scale.

---

## 3. Shared hues that are **not** semantic roles

`--purple #7345CC`, `--cyan #2FC6D8` and `--yellow #E3C55B` are declared with
identical values wherever they appear, which makes them shared *palette*. They
are not shared *meanings*:

| hue | NovuAI | NovuExplorer | NovuSpace | NovuAdmin |
|---|---|---|---|---|
| purple | `.tag.agent`, `.avatar.agent` | `.chip.st-rev` (Review), product colour | the product's own identity mark | declared, unused |
| cyan | `.tag.know` (knowledge) | `.chip.st-test` (Test), a folder colour | a chart series | declared, unused |
| yellow | declared, **unused** | declared, **unused** | absent | absent |

Three meanings for purple, three for cyan, and no use of yellow anywhere.

NovuSpace states the purple case outright: `design/Shared/design-system`
assigns `#7345CC` **to NovuSpace as a product**, and its mark is drawn with it —
while its UI accent is amber. A product identity colour and a status colour are
different layers.

**Decision: these do not become `SemanticRole`s.** Promoting "agents" or
"knowledge" into the shared palette would write NovuAI's vocabulary into every
product, which is the thing `SemanticRole` exists to prevent. They belong in the
shared *hue* set that a product assigns meaning to — the same place a chart
series and an avatar plate already come from.

---

## 4. Drop target — one generic capability, not four

`.drop-on` is NovuAI's own class, on `.trow` (a row) and `.tree` (a region),
with the design's own reasoning attached:

> Two outcomes, so two different highlights, and never both at once: a row
> lights when the folder will be bound to that workspace, the region lights
> when it will make a new one.

NovuExplorer draws a drop *zone* (`.share-drop`) rather than the same two-state
row/region pair, so this is not four products stating one pattern identically —
it is one product stating it precisely, and a second product with unstated
overlap. Treated as a shared capability anyway: a tree and a workspace panel
accepting drops is not specific to NovuAI, and the alternative was letting a
second consumer invent its own row/region convention.

`DropHighlight` (`include/novu/ui/DropTarget.h`) is the state: one enum, so
lighting both a row and the region is unrepresentable rather than merely
discouraged, and every mutator reports whether the picture changed so a drag
crossing thirty rows repaints once per row rather than on every pixel.
`dropRowSpec()` / `dropRegionSpec()` are the tokens, `paintDropRow()` /
`paintDropRegion()` the paint calls — the same two-weight difference the
design states:

| | fill | border | radius |
|---|---|---|---|
| Row | `accent@20` | `accent` (1px, inset) | small |
| Region | `accent@7` | `accent@55` (heavier, inset) | medium |

Demonstrated on its own gallery page (`DropTargetPage.cpp`) and inside
`ThemeRolesPage`'s "Drop target" card, both painting through the real
functions rather than a mock of them.

---

## 5. Unreconciled — decisions the material does not settle

Recorded rather than guessed. Each is a place where the authorities disagree
and no source says which wins.

### The title bar is 44, except NovuSpace says 42

NovuAI, NovuAdmin and NovuExplorer all state 44. NovuSpace states 42.

NovuSpace's own token header says its chrome is *"lifted verbatim from the
NovuHub / NovuExplorer / NovuAdmin brand mockups"* and that the only thing
identifying the product is its accent — which argues the 42 is drift in that
mockup rather than a decision. But nothing says so outright.

**Taken:** 44 as the library value, and the window-chrome invariant kept —
`controlGeometryIsIdenticalAcrossProfiles()` still holds a profile to one bar
size, because that test exists precisely because three applications once each
picked their own. **Open:** whether NovuSpace's 42 is intended.

### `--tree-step` and `--row-h` disagree with their own rationale

NovuAI's sheet says it *"inherits Explorer's density"* and then states
`--row-h:26px` where Explorer states 24. Both values are kept as each states
them; the sentence is treated as intent and the tokens as fact.

### Yellow is declared and never drawn

`--yellow #E3C55B` is in the NovuAI and NovuExplorer token sets and neither
mockup uses it. It is carried in `familyHues()` because the value agrees
wherever it appears — but nothing establishes what it means.

### Five of ten family hues have no second user

`Lime #A8CC45`, `Indigo #7B87E8` and `Slate #8793A3` come from NovuExplorer's
colour picker alone. They are palette rather than contract until a second
authority draws them.

---

## 6. What a product may still override

A profile overrides a shared value only when its **surface** forces it to, never
to have a look of its own. `applicationProfilesShareOnePalette()` and
`switchingProfileMovesTheColoursAndNotTheContract()` are the tests that hold the
line.

User-selectable *themes* (Midnight Blue, Graphite) are a different thing from a
product and are allowed their own palette — a user's choice of appearance is not
a product's identity.

---

## Related

[THEMING](THEMING.md) · [README](../README.md) ·
[EXTRACTION_STATUS](EXTRACTION_STATUS.md) · [PORTING_GUIDE](PORTING_GUIDE.md)
