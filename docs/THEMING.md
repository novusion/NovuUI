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

### Status is derived, not listed

A profile states four hues. `semanticColorFrom()` turns each into the five
values a component actually needs — text, fill, border, hover, pressed — so a
Danger badge and a Danger button cannot drift apart:

```cpp
SemanticColor c;
c.text    = hue;
c.fill    = withAlpha(hue, 0.16);
c.border  = withAlpha(hue, 0.40);
c.hover   = withAlpha(hue, 0.24);
c.pressed = withAlpha(hue, 0.32);
```

---

## 4. Application accents

The approved shared palette — `design/Shared/design-system/Novu_Palette.png` —
fixes the surfaces, the text ramp and the four status colours **for the whole
family**. A product is identified by exactly one colour on top of them: its
accent.

`novusionBaseProfile()` is that palette. Every application profile is built
from it by `applicationProfile()`, which takes an optional accent:

```cpp
ThemeProfile novuSpaceProfile();   // amber #D49335 — approved, frozen
ThemeProfile novuAiProfile();      // family accent — UNRESOLVED
```

### What is settled, and what is not

| profile | accent | authority |
|---|---|---|
| `novusion` (baseline) | `#2FC6D8` | `Novu_Palette.png` — ACCENT & BRAND |
| `novuspace` | `#D49335` | the approved NovuSpace mockup, *"chosen in the preset evaluation and frozen"* |
| `novutrade` | `#10B981` | mirrors the trading web app; a deliberate whole-palette exception |
| `novuai` | family | **UNRESOLVED** |
| `novuexplorer` | family | **UNRESOLVED** |
| `novuadmin` | family | **UNRESOLVED** |
| `novuhub` | family | **UNRESOLVED** |
| `novuvoice` | family | **UNRESOLVED** |

**Unresolved means unresolved.** Those five keep the family cyan because no
approved product accent was found for them, and a colour that looks plausible
is not an approval. Two near-misses, both rejected on purpose:

- The NovuHub prototype gives each product an **app-mark** colour (NovuAI blue,
  NovuHub green, NovuExplorer teal). A mark colour is not a UI accent.
- `Novu_App_Design.png` draws NovuAI's own navigation in the family cyan — but
  it is the shared reference sheet, and everything on it is one accent.

Settling one is a one-line change:

```cpp
// src/AppProfiles.cpp
return applicationProfile(QStringLiteral("novuai"), QStringLiteral("NovuAI"),
                          QColor("#4DA3FF"), QColor("#7CBEFF"), QColor("#0B1420"));
```

`applyDerivedTokens()` then re-derives the chart series and the window chrome,
and `applicationProfilesShareOnePalette()` holds everything else identical.

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
- [COMPONENTS](COMPONENTS.md) — generated catalogue
- [PORTING_GUIDE](PORTING_GUIDE.md) — using this from an application
- [EXTRACTION_STATUS](EXTRACTION_STATUS.md) — coverage, deferrals, limitations
