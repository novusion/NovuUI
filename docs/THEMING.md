# Theming — how a component gets its colours

The one rule everything below serves:

> **A component never owns a product colour.** It asks for a meaning.
> `ThemeProfile` decides what that meaning looks like.

This is what lets one `Button` class be NovuSpace's amber and NovuAI's cyan
without a conditional, a subclass or a second stylesheet — and it is why there
is no `NovuAIButton` in this library and never will be.

---

## 1. The flow

```
    Component                 Button::styleSpec()
        ↓                     "my background is the accent"
    StyleSpec                 token(ColorRole::Accent)
        ↓
    ThemeProfile              novuspace → #D49335   novuai → #2FC6D8
        ↓
    ResolvedStyle             QColor, QFont, radius, border width
        ↓
    paint / stylesheet
```

Resolution happens **on every read**, not once at construction. A widget is
never holding a colour that a theme change could make stale.

---

## 2. The two halves

### StyleSpec — what a component means

`include/novu/ui/StyleSpec.h`

```cpp
StyleSpec Button::styleSpec() const
{
    StyleSpec spec;
    spec.radius      = SurfaceRole::Button;
    spec.borderWidth = BorderRole::Input;
    spec.type        = TypeRole::Body;

    switch (m_variant) {
    case Variant::Primary:
        spec.background      = token(ColorRole::Accent);
        spec.foreground      = token(ColorRole::TextOnAccent);
        spec.hoverBackground = token(ColorRole::AccentLight);
        spec.weight          = QFont::Bold;
        break;

    case Variant::Danger:
        spec.background = statusToken(SemanticRole::Danger, ColorRole::StatusFill);
        spec.foreground = statusToken(SemanticRole::Danger, ColorRole::StatusText);
        spec.border     = statusToken(SemanticRole::Danger, ColorRole::StatusBorder);
        break;
    // …
    }
    return spec;
}
```

Not one hex value. `ColorToken` also carries an alpha multiplier, which is how
the design's `rgba(var(--accent), .12)` is said without inventing a token per
opacity:

```cpp
spec.checkedBackground = token(ColorRole::Accent, 0.28);   // Toggle, when on
```

### ResolvedStyle — what that comes to, here, now

```cpp
const ResolvedStyle style = resolve(spec, this, height());

painter.setBrush(style.background);      // QColor
painter.setPen(QPen(style.border, style.borderWidth));
painter.setFont(style.font);             // QFont, from the type role
painter.drawRoundedRect(box, style.radius, style.radius);   // int, pill resolved
```

`resolve()` takes the widget so it can find the profile the widget's subtree is
scoped to, and a height so a `Pill` radius becomes a number instead of the
marker value.

**Never store a `ResolvedStyle`.** Ask for one, use it, drop it.

---

## 3. What a profile holds

`include/novu/ui/ThemeProfile.h`. Each family is a scale, and components name a
*role* rather than a step.

| family | roles | examples |
|---|---|---|
| `ThemeColors` | — | six surfaces, three text weights, accent, four status hues |
| `RadiusScale` | `SurfaceRole` | `Card`, `Dialog`, `Button`, `Input`, `Badge`, `Window` |
| `SpacingScale` | `SpacingRole` | `CardPadding`, `ToolbarGap`, `PagePadding` |
| `TypeScale` | `TypeRole` | `PageTitle`, `CardTitle`, `Body`, `Caption`, `Mono`, `Eyebrow` |
| `BorderScale` | `BorderRole` | `Card`, `Input`, `Divider`, `Focus` |
| `ControlMetrics` | `ControlSize` | `Small`, `Medium`, `Large` |
| `IconMetrics` | — | small / medium / large glyph sizes |
| `AnimationTokens` | — | transition durations, and the four loop periods |
| `WindowTokens` | — | title bar height, control box, chrome colours |
| `SemanticPalette` | `SemanticRole` | derived from the four status hues |

The indirection is the point: NovuSpace says buttons are 10px round and
NovuTrade says 6px, and no component contains either number.

```cpp
profile.radius(SurfaceRole::Button);   // 10 here, 6 there
profile.space(SpacingRole::CardGap);   // 14 here, 16 there
profile.text(TypeRole::CardTitle);     // {14, 600} here, {13, 700} there
```

### Status keeps separate ink and plate channels

A profile states four **pairs** — `success`/`successPlate`,
`warning`/`warningPlate`, and so on. On the shared dark palette NovuAI uses
the saturated family hue for both channels:

```css
.tag.ok { color:#3CCB7F; background:rgba(60,203,127,.10); border-color:rgba(60,203,127,.40); }
```

The channels stay separate because a light profile needs a darker accessible
ink without changing the semantic tint. `semanticColorFrom()` turns the pair
into the five values a component actually needs, at the role's own alphas:

```cpp
SemanticColor c;
c.text    = ink;
c.fill    = withAlpha(plate, fillAlpha);    // .10 for Success, .12 for Danger
c.border  = withAlpha(plate, borderAlpha);  // .40 for Success, .45 for Danger
c.hover   = withAlpha(plate, fillAlpha * 2.0);
c.pressed = withAlpha(plate, fillAlpha * 2.8);
```

The alphas are not uniform across roles — Danger sits heavier than Success on
purpose, because a pink plate at .10 disappears where a green one reads. See
[DESIGN_CONTRACT §1](DESIGN_CONTRACT.md#1-shared-contract--identical-in-every-authority)
for the full per-role table and its sourcing.

### Family hues are palette, not meaning

`familyHues()` is a separate, ten-colour set — Rose, Amber, Lime, Green, Teal,
Cyan, Blue, Indigo, Purple, Slate — stated once at the values NovuExplorer's
own colour picker uses. They are **not** `SemanticRole`s: the same hue means
different things in different products (purple is *agents* in NovuAI and
NovuSpace's own product-identity mark), so promoting one to a shared role
would write one product's vocabulary into every other. A product assigns the
meaning; the library only promises the hue itself does not drift.
`Swatch::tagPalette()` is this set — a tag is a label a person picks, and must
never coincide with the ink that means a status failed.

---

## 4. Application accents

Each product states its own accent in its own approved mockup — there is no
family accent to fall back to. `novusionBaseProfile()`'s cyan is a stand-in
palette for a product that has not been designed yet, not a default any
product is meant to keep.

`novusionBaseProfile()` supplies the shared surfaces and status hues. Every
application profile is built from it by `applicationProfile()`, which takes
the accent its own mockup states:

```cpp
ThemeProfile novuSpaceProfile();   // amber #D49335 — approved, frozen
ThemeProfile novuAiProfile();      // blue #4DA3FF — approved, frozen
```

### What is settled, and what is not

Full sourcing for each: [DESIGN_CONTRACT §2](DESIGN_CONTRACT.md#2-product-theme-profile--stated-per-product).

| profile | accent | authority |
|---|---|---|
| `novusion` (baseline / stand-in) | `#2FC6D8` | `Novu_Palette.png` — no product claims this value |
| `novuspace` | `#D49335` | the approved NovuSpace mockup, *"chosen in the preset evaluation and frozen"* |
| `novuai` | `#4DA3FF` | `NovuAI/mockups/assets/css/00-tokens.css` — `--acc` |
| `novuexplorer` | `#58B09E` | `NovuExplorer-brand-mockup-v2.html` — `--acc` |
| `novuadmin` | `#3CCB7F` | `NovuAdmin-brand-mockup.html` — `--acc` |
| `novuhub` | `#3CCB7F` | `NovuHub-brand-mockup.html` — `--acc` |
| `novutrade` | `#10B981` | mirrors the trading web app; a deliberate whole-palette exception |
| `novuvoice` | `#C47B85` | `NovuVoice-brand-mockup.html` — `--acc` |

The previously unresolved accents were missed for one bad reason: the search was for a
hex value in the approved PNG *screens*, and every one of them was a stated
token in the mockup's own CSS the whole time. Two near-misses had been
rejected on the way there, correctly:

- The NovuHub prototype's **app-mark** colour is not its UI accent — a mark and
  an accent are different things even when, as here, they turn out to agree.
- `Novu_App_Design.png` draws NovuAI's navigation in the family cyan, but it is
  the shared reference sheet and everything on it wears one accent — evidence
  the value is plausible, not a statement that it is NovuAI's.

NovuVoice starts from the same one-line profile rule as the other products:

```cpp
// src/AppProfiles.cpp
ThemeProfile p = applicationProfile(QStringLiteral("novuvoice"), QStringLiteral("NovuVoice"),
                                    hex("#C47B85"), hex("#DA98A1"), hex("#1B1014"));
```

`applyDerivedTokens()` then re-derives the chart series and the window chrome,
and `applicationProfilesShareOnePalette()` holds everything else identical.

### The one product that owns a status hue

NovuVoice then does one more thing, and it is the only application that does:

```cpp
p.colors.danger = p.colors.dangerPlate = hex("#E5484D");   // --red
```

It is allowed because its accent **is** a shared status hue. `#C47B85` is the
family's Danger and NovuVoice's identity at the same time, so on this one
profile "failed" and "this is NovuVoice" would be the same colour — and its
approved design says outright that *"the accent ... is never a status colour:
a rose element on screen is always identity or selection, never state"*. That
design is also the only one of the six that states a `--red` of its own, for
exactly this reason.

Every other product leaves the four status hues alone, because for every other
product the two layers never meet. Success, Warning and Info are untouched here
too: NovuVoice overrides Danger and nothing else.

The exemption is named in one place — `mayOverrideStatus()` in
`tests/tst_gallery.cpp`, which carries the reason — and
`statusOverridesAreStatedOneByOne()` holds the list to exactly the profiles
that appear there, so a third exemption has to be argued for rather than
quietly added.

### The ink on an accent is measured

`textOnAccent` is not picked to taste. NovuSpace's dark ink clears 6.9:1 on its
amber where white manages 2.6:1; the family cyan is 8.4:1 against 2.1:1. The
test asserts ≥ 4.5:1 for every profile.

---

## 5. Scoping — several products in one process

NovuSpace hosts modules in its own process, so there cannot be one global
theme. `ThemeScope` marks a subtree; `themeFor()` walks up to find it.

```cpp
applyStyleSheet(tradingWorkspace, QStringLiteral("novutrade"));
// every NovuUI widget inside now resolves NovuTrade's tokens
```

Resolution walks the parent chain on every read rather than caching, because a
widget can be reparented into another scope at any time and a cache would then
be quietly wrong. It is a handful of pointer hops.

---

## 6. Switching a theme

`ThemeManager::registerProfile()` replaces a profile and posts a
`ThemeChangeEvent` to every widget scoped to it. Components react by re-reading:

```cpp
bool Toggle::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();          // re-resolves on the next paint
    }
    return QAbstractButton::event(event);
}
```

**No widget is rebuilt.** `aLiveProfileEditReachesWidgetsAlreadyBuilt()` asserts
that editing a live profile moves an existing button's resolved colour while
its token stays the same.

The Playground's **Theme & Styling** page is the live proof — `run.bat
theme-styling`, then change the Theme selector.

---

## 7. States

Interaction states are tokens too, not derived brightness:

```cpp
spec.background        = token(ColorRole::SurfaceHover);
spec.hoverBackground   = token(ColorRole::SurfaceActive);
spec.checkedBackground = token(ColorRole::Accent, 0.28);
spec.disabledForeground = token(ColorRole::TextDisabled);
```

Two conventions worth knowing, both from the approved designs:

- **Selected is the accent.** An active module, a current navigation row, a
  checked `IconButton` and a chosen segment all take the same token, so a
  window has one answer to "what is current".
- **Disabled dims, it does not recolour.** Painted components multiply alpha by
  `0.42` rather than switching to a grey, so a disabled switch still reads as
  on or off.

---

## 8. Testing without screenshots

Every painted component exposes both halves:

```cpp
QCOMPARE(button->styleSpec().background.describe(),
         QStringLiteral("status.danger.fill"));        // the contract

QCOMPARE(button->resolvedStyle().background.name(),
         profile.semantic.danger.fill.name());          // this profile
```

`ColorToken::describe()` returns a stable name — `accent`, `accent@12`,
`status.danger.fill` — and never a colour. There is no image-diff
infrastructure in this repository and none is wanted.

---

## Related

- [README](../README.md) — the library's rules
- [DESIGN_CONTRACT](DESIGN_CONTRACT.md) — every token, sourced against the four authoritative mockups
- [COMPONENTS](COMPONENTS.md) — generated catalogue
- [PORTING_GUIDE](PORTING_GUIDE.md) — using this from an application
- [EXTRACTION_STATUS](EXTRACTION_STATUS.md) — coverage, deferrals, limitations
