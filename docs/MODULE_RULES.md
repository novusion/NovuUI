# NovuUI — Module UI Rules

Novusion applications are moving toward a modular architecture. NovuUI is the
source of truth for shared UI components, visual language, and UI integration
rules used by application modules.

These rules exist so that new modules stay visually and structurally consistent
as the ecosystem grows. They are binding on NovuUI and on every module that
consumes it.

`DESIGN_SYSTEM.md` describes how the library is built; this file describes who
is allowed to build what, and where it goes.

---

## 1. Module ownership

A feature module owns its feature-specific UI and behaviour.

Shared visual primitives and reusable UI patterns do not belong to individual
modules. They belong in NovuUI.

A module may compose NovuUI components into feature-specific screens, but must
not create local replacements for existing shared components.

## 2. Shared first

Before implementing a new UI component inside a module:

1. Check whether NovuUI already provides it.
2. Check whether another Novusion application contains a reusable implementation.
3. If the component is generally reusable, extract and generalise it into NovuUI.
4. Use the NovuUI implementation from the module.

Do not duplicate common controls between modules. The extraction procedure is
`ECOSYSTEM_AUDIT.md` §6.1.

## 3. Module UI boundary

| NovuUI owns | Feature modules own |
|---|---|
| shared components | feature-specific screens |
| shared patterns | feature-specific composition |
| design tokens | feature-specific state |
| typography | business logic |
| semantic colours | application workflows |
| spacing and geometry rules | |
| icons and shared visual assets | |
| common interaction states | |
| shared motion / animation rules | |
| reusable layout primitives | |
| reusable feedback / help UI | |

## 4. Application shell

Modules must not assume ownership of the application shell. A module does not
modify global navigation, the MainWindow structure, unrelated modules,
application startup, or global workspace infrastructure. Those belong to the
host application and its module framework.

## 5. Module independence

A module should be understandable and maintainable largely within its own source
scope. Avoid direct dependencies between unrelated feature modules; shared
functionality needed by several modules moves to a shared or core layer rather
than being copied, or reached through another feature module.

## 6. UI integration

A module exposes its UI through the host application's module contract rather
than inserting itself into MainWindow, navigation or fixed stack indexes.
Navigation and activation are driven by module metadata and the host's Module
Registry.

NovuUI does not own the Module Registry, but module UI must be compatible with
it.

## 7. Themes

Modules use NovuUI theme roles and design tokens. Do not hardcode an application
colour where a semantic or theme token exists.

Application identity and accent may vary by theme. Success, Warning, Danger,
Info and Neutral stay consistent across the ecosystem unless an explicit theme
override is required — see `DESIGN_SYSTEM.md` §3.4a.

## 8. Component states

A reusable component defines and validates the states that apply to it: Normal,
Hover, Pressed, Focused, Selected, Disabled, Loading, Error. Reusable components
are represented in the NovuUI Component Gallery.

## 9. Playground

The Playground is the validation environment for the design system. Reusable
production components are shown using their **actual** NovuUI implementation.
No gallery-only visual copies.

The Playground may filter by application or theme, but filtering must never
produce an application-specific component implementation.

## 10. Application-specific UI

Not every element needs to become a NovuUI component. Keep it local when it is
genuinely feature-specific behaviour or composition; extract it when the
underlying concept is reusable across modules or applications.

Avoid premature abstraction.

## 11. Source organisation

New modules use the structure defined by the host application's module
framework. NovuUI components are organised by **shared UI responsibility**, not
by the application that first wrote them.

```
avoid:   NovuUI/NovuAI/Button      NovuUI/NovuSpace/Button
prefer:  NovuUI/Button             + properties, variants, composition, roles
```

## 12. Dependencies

```
Application
    ↓
Feature Module
    ↓
NovuUI / Shared Core
```

NovuUI must never depend on NovuAI or any other consuming application. A shared
component may originate in one, but once extracted it is independent of it.

## 13. Growth rule

Prefer extending an established shared system over creating a parallel one.
Before introducing a new component, theme token, sidebar pattern, tab system,
dialog, feedback mechanism, overlay or navigation pattern: audit existing NovuUI
and production implementations first.

Controlled reuse, not maximum abstraction.

---

## Where NovuUI stands — audit of 2026-08-30

Mechanical, so it can be re-run rather than believed:

```bash
# §7 — no hardcoded colour outside the palette definitions
grep -rnE '#[0-9a-fA-F]{6}' src/ include/ \
  | grep -vE 'ThemeProfile\.(h|cpp)|AppProfiles\.cpp|Theme\.h'

# §12 — no dependency on a consuming application
grep -rniE '#include.*(novuai|novuspace|novuexplorer|novuhub|novutrade|iai/)' src/ include/

# §9 — no widget class defined in the gallery that could be a copy
grep -rnE 'class [A-Z][A-Za-z]* *: *public *(QWidget|QFrame|QLabel|QPushButton)' playground/

# §8/§9 — what the gallery covers
build-gallery/playground/Release/NovuUIPlayground.exe --list
```

All four came back clean. The one `#10b981` in `StyleSheet.h` is inside a comment
explaining why hardcoding is forbidden.

### Compliant

- **§11** — flat `include/novu/ui/`, no per-application directory.
- **§12** — nothing in NovuUI includes an application header.
- **§9** — the gallery defines no widget of its own. `ExampleLayout` is layout
  helpers only; every visible pixel is drawn by NovuUI.
- **§7** — one palette for every application, `novutrade` the single exception,
  enforced by `applicationProfilesShareOnePalette()`; semantic status shared and
  enforced by `everyProfileSharesTheSemanticPalette()`.
- **§6** — nothing in NovuUI assumes a shell structure. `AppWindow` takes a
  content widget and knows nothing about what fills it, so it is compatible with
  a registry-driven host.

### Judgement calls, recorded rather than hidden

- **Application ids inside the library.** `isApplicationProfile()` names
  `novuspace`, `novuai`, `novuexplorer`, `novuhub`, `novuvoice`, `novutrade`.
  That is product *data* in the theming layer, not a code dependency on any of
  them — NovuUI compiles and links with none of those repositories present. It
  is the closest thing here to a §12 concern, and it is deliberate: an identity
  a subtree can be scoped to has to be named somewhere.
- **`TitleBar` has no registry entry of its own.** It is demonstrated by the
  real component inside the WindowShell page rather than duplicated into one, so
  §8 is met by composition. `Style`, `StyleSheet`, `ThemeManager` and
  `ThemeScope` are infrastructure, not components, and are covered by the Theme
  roles page.

### Gaps against §3

Two things §3 assigns to NovuUI that NovuUI does not yet have:

| gap | state today |
|---|---|
| **icons and shared visual assets** | only `iconFont()`. No icon set, no asset pipeline, so every product still ships its own glyphs. |
| **shared motion / animation rules** | `AnimationTokens` exist on the profile and **nothing consumes them**. A token nobody reads is not a rule. |

Both should be picked up before the modules that need them arrive, per §13.

The component backlog — Toggle, `TreeView` / `ListView`, NavRail, SettingsShell,
log panel, charts, file picker — is in `ECOSYSTEM_AUDIT.md` §4 and §6, and every
one of them is a §2 case: extract from the production implementation, do not
write a look-alike here.
