# NovuUI — vocabulary

What the words mean, so they mean the same thing twice.

This file exists because the same terms kept being re-derived from scratch and
landing differently each time — a component filed under the wrong category, a
"compact" that meant two different things in two sentences. Look here before
naming, filing or classifying anything.

`MODULE_RULES.md` says who may build what. This says what the words are.

---

## 1. Categories

A component is filed by **what it is for**, never by how it behaves. Two
components that both appear on hover can belong to different categories,
because appearing on hover is not a purpose.

| category | for | examples |
|---|---|---|
| **Foundation** | the material everything else is made of: theme, tokens, icons, layout primitives, rules that apply everywhere | Theme & Styling, Icon, Layout, Separator, Motion, Application rules |
| **Window title bar** | the frame a window wears | WindowShell |
| **Navigation** | moving between places, and showing where you are | Sidebar, NavList, TabBar, Disclosure, PageHeader, Drawer |
| **Footer** | the pinned strip at the bottom of a window | StatusBar |
| **Input** | a control a person operates to supply or change a value | Button, IconButton, Slider, Toggle, Select, SearchField |
| **Data display** | showing content the application holds | TableView, StatCard, DiffView, KeyCap |
| **Feedback** | telling the person about **the application**: its state, or the result or progress of something they did | Toast, Spinner, ProgressBar, Skeleton, StatusLine, Steps, EmptyState, InfoBanner, FeedbackForm |
| **Help** | explaining **the interface itself** — contextual guidance, offered rather than reported | Tooltip, HelpOverlay |

### Feedback or Help — the line that keeps getting crossed

> **Feedback reports. Help explains.**

Feedback answers *what just happened* or *what is the application doing*. Help
answers *what is this, and what will it do if I use it*.

A tooltip on hover is Help: nothing happened, nothing changed, and the person
asked. A toast after a save is Feedback: something happened, and the
application is reporting it.

`Tooltip` and `HelpOverlay` were both filed under Feedback and were both wrong.
The test was "it pops up, like a toast does" — behaviour, not purpose.

### Still unsettled

- **`Dialog`** sits in Feedback and is neither: it is a window surface. There
  is no Surfaces category yet. Left where it is rather than moved on one
  person's opinion; raise it before filing the next surface.

---

## 2. Tooltip vocabulary

One component, `novu::ui::Tooltip`. One card, `max-width: 274px`. Everything
below is content or state — never a second widget and never a second style.

### The two states

| state | shows | design |
|---|---|---|
| **compact** | title, shortcut, brief, status, and a line saying there is more | `.tip-t` `.tip-s` `.tip-st` `.tip-more` |
| **full** | the same card with the long form added under a rule | `+ .tip-d` |

**Compact is a state of this card, not a smaller card.** There was briefly a
`TipContent::compact` flag that drew the tip a step down the type scale; the
mockup has no trace of it, and it is gone.

### The three content levels

Field subsets of the one `TipContent`. They change what is *said*, never how it
is drawn.

| level | fields | for |
|---|---|---|
| **LABEL** | `title` (+ short `status`) | identification only — a tab, a path, a row |
| **ACTION** | `title` + `brief` (+ `shortcut`, `status`) | **the default for anything actionable**: buttons, toolbar and title-bar actions, session and console controls |
| **EXTENDED** | all five | exceptional — only where the extra paragraph earns its place |

ACTION's `brief` says **what happens when you activate it**, not what the button
is called again. "Create a workspace for organizing chats, tasks and context",
not "New workspace".

### Help mode

`prefs.tooltipHelp` in the design, and **on by default** there. Off, a tip is
the name and the key alone — which is the OS tooltip this component replaced.
NovuUI defaulted it off, so every rich tip rendered as a bare title and nobody
could see the card the design draws.

### Timings

From the mockup's `prefs`, not invented: hover **320 ms** with help on,
**420 ms** with it off, long form after a further **900 ms**.

---

## 3. Words that have bitten us

| word | means here | does not mean |
|---|---|---|
| **compact** | the tooltip's first state | a smaller card, a denser variant |
| **brief** | one sentence on what a control does | the control's name |
| **status** | what it is doing *right now* | its kind, its type, a badge |
| **detail** | the long form, behind a delay | documentation |
| **designedFor** | which application a component was drawn for | which application uses it |
| **usedBy** | measured adoption — it links the library today | intent, or a plan |
| **child** (NavList) | folds under the row above and draws a leaf marker | a row that is merely indented — that is `depth` |
| **depth** (NavList) | how many levels in a row is drawn | whether it folds |
