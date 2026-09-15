# Porting onto NovuUI

Two workflows, and they are not the same job.

| | **A — UI rebuild** | **B — migration** |
|---|---|---|
| you have | an approved design, and a mockup at most | a working application |
| you write | the presentation, new | replacements, one area at a time |
| data | local and fake | the application's real state, untouched |
| risk | none — nothing is in production | regression, every step |
| next up | **NovuAI** | NovuExplorer, NovuAdmin |

Read [README](../README.md) for the library's rules and [THEMING](THEMING.md)
for how colours resolve before either.

---

## A — UI rebuild from an approved design

**This is the NovuAI phase.** The point is a complete, reviewable UI that is not
entangled with anything that can break.

```
    approved design / mockup
            ↓
    new C++/Qt6 presentation
            ↓
    NovuUI components
            ↓
    local / mock data only
            ↓
    visual + interaction approval
            ↓
    backend integration — LATER, separate task
```

### Do not integrate the backend during this phase

No services, no runtime, no providers, no execution, no persistence, no network.
Not "temporarily wired up", not "just to see real data".

The reason is reviewability. A UI with a backend behind it cannot be judged on
its own: every question about a screen becomes a question about whether the data
is right, and every bug is two bugs until somebody separates them. A UI built on
a fixed sample answers "is this the right screen" on its own, which is the only
question this phase is asking.

It is also reversible. A screen built against mock data can be rearranged in an
afternoon. The same screen with a service wired into it cannot.

### Steps

1. **Read the approved design.** It is the authority — not an older prototype,
   not the current application. If the two disagree, the design wins, and if
   the design is silent, ask rather than invent.
2. **Map the areas to components** before writing anything. Produce the table
   `docs/ports/` holds — [NOVUAI_UI_PORT](ports/NOVUAI_UI_PORT.md) is the one
   for NovuAI. Record genuine gaps; do not fill them with new components while
   the library is frozen.
3. **Build the shell first** — `AppWindow`, `TitleBar`, `Sidebar`,
   `BottomPanel`, `StatusBar`. It is the frame everything else is placed in and
   it is the part most likely to move.
4. **One screen at a time**, each with sample data of its own, each reviewable
   on its own.
5. **Keep the sample data in one obvious place.** `SampleData` in the Playground
   is the shape to copy: JSON on disk, read at runtime, no compiled-in fixtures
   scattered through the pages.
6. **Build and run after each screen.** A UI rebuild that compiles once at the
   end is a rewrite with no checkpoints.
7. **Approval**, then a separate task for integration.

### What the UI phase may contain

View state — which tab is current, whether a panel is open, what is selected,
what is typed. That is presentation and it belongs in the presentation.

What it may not contain is anything that outlives the window: no settings
written to disk, no cache, no session, no credentials.

---

## B — migrating an existing application

**NovuExplorer and NovuAdmin, later.** Both are read-only references today.

```
    inventory the current UI
            ↓
    map widgets and patterns onto NovuUI
            ↓
    replace ONE bounded area
            ↓
    application keeps its state and ownership
            ↓
    build + test
            ↓
    visual and behavioural verification
            ↓
    next area
            ↓
    delete the superseded implementation only when no consumer remains
```

### Steps

1. **Inventory.** What the application draws, and with what. `grep` for
   `class .* : public Q` is enough to start.
2. **Map, honestly.** Three outcomes per pattern: it is already a NovuUI
   component; it is a *variant* of one; or it is application-specific and stays.
   Most things are the third.
3. **Pick a bounded area** — one dialog, one panel, one settings page. Bounded
   means you can describe what changed in a sentence.
4. **Replace, do not wrap.** A NovuUI component behind an application-named
   subclass is the migration not happening.
5. **Leave ownership where it is.** The application keeps its model, its state
   and its decisions. A shared component takes state as an input and reports
   intent as a signal — `BottomPanel::setExpanded()` is a setter and
   `expandRequested()` is a request, and that separation is the whole reason it
   can be shared at all.
6. **Build and test.** The application's own tests, not just NovuUI's.
7. **Verify by looking at it**, then continue.
8. **Delete last.** The old implementation goes when its last consumer is gone —
   not when the first one has moved.

### Scope the accent first

A migrated application scopes its subtree once:

```cpp
applyStyleSheet(mainWindow, QStringLiteral("novuexplorer"));
```

If the product has no approved accent yet it wears the family's, and that is
recorded rather than guessed — [THEMING §4](THEMING.md#4-application-accents).
Settling one later is a one-line change in `AppProfiles.cpp` and no application
code moves.

---

## Rules for both

### Which library

**New work uses `NovuUI` (`novu::ui`) only.** `NovusionUI` (`novu::brand`) is
the original shared library and is still what NovuHub and NovuExplorer run on;
it is being migrated away from, not built on. Do not add a dependency on it —
see [DUPLICATION_MAP](DUPLICATION_MAP.md#the-decided-target-state).

### Ownership

| the application owns | NovuUI owns |
|---|---|
| business logic | reusable presentation |
| persistence | reusable controls |
| services and backend communication | reusable interaction behaviour |
| models | semantic styling |
| runtime and domain state | shared motion |
| page and screen definitions | generic structural composites |

### Do not

**Do not copy a NovuUI component into the application.** A copy is a fork, and
it will drift. If something is missing, the component gains a variant.

**Do not subclass to change a colour.** There must be no `NovuAIButton`,
`ExplorerButton` or `AdminBadge`. Colour, accent, icon, text, size, state and
theme are not structural differences:

```cpp
// no
class AdminBadge : public Badge { /* just a different colour */ };

// yes
new Badge(text, Badge::Tone::Danger, parent);
applyStyleSheet(adminWindow, QStringLiteral("novuadmin"));
```

`switchingProfileMovesTheColoursAndNotTheContract()` is the test that keeps this
true: the same class resolves different colours under five profiles while its
token contract stays identical.

**Do not put application state into a shared component.** If a component would
need to know what a session is, what a file is, or what a model provider is, the
boundary is in the wrong place.

**Do not set a stylesheet on a NovuUI component.** It composes its own from the
profile and will overwrite yours at the next theme change.

### When a component is genuinely missing

Record it in the port's mapping document as a gap. Do not add it during a port —
a component extracted to unblock one screen is a component shaped by one screen.
Gaps are collected, reviewed together, and unfrozen deliberately.

---

## NovuAI rebuild — the next phase

**UI only.** The sources are a Python Qt6 mockup and the approved design
material under `g:\Design\design\NovuAI\` plus the shared reference sheet
`design/Shared/references/Novu_App_Design.png`, which draws NovuAI's own
navigation.

Read the mockup. **Do not run it.**

The goal is the complete new NovuAI UI in C++/Qt6 on NovuUI, and initially:

- no backend integration
- no AI runtime
- no provider integration
- no task execution
- no persistence
- no production services

Local, mock or sample data only, and only where a screen needs something to
show. The result must be visually and interactively reviewable before any real
integration starts.

The area-by-area mapping is prepared: [NOVUAI_UI_PORT](ports/NOVUAI_UI_PORT.md).

NovuAI's accent is **unresolved** — it will wear the family cyan until a product
accent is approved. That is a one-line change when it is, and no screen code
changes with it.

---

## Related

- [README](../README.md) · [THEMING](THEMING.md) · [COMPONENTS](COMPONENTS.md)
- [EXTRACTION_STATUS](EXTRACTION_STATUS.md) — what exists, what is deferred
- [DUPLICATION_MAP](DUPLICATION_MAP.md) — the second library, and its consolidation plan
- [NOVUAI_UI_PORT](ports/NOVUAI_UI_PORT.md) — the next phase's mapping
