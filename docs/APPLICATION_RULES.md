# Novusion Application Architecture Rules

These rules are mandatory for all Novusion applications.

Existing applications that do not comply must be migrated incrementally.
Legacy implementation is not considered an exception to these rules.

Rendered as a page in the NovuUI Gallery (Foundation → Application rules) so
every application team can find them from the same place they find the
components. This file is the authoritative copy; the Gallery page is a
formatted view of it.


## 1. Modular architecture

Every Novusion application must use a modular architecture.

Major product features must be implemented as self-contained modules with:

- clear ownership
- explicit dependencies
- predictable source location
- defined lifecycle
- isolated feature logic
- minimal coupling to the application shell

The application shell must not accumulate feature-specific implementation.

Adding a new feature module should require minimal changes to the application
shell.

If an existing Novusion application is not modular, create a migration/refactor
plan and move it toward the shared modular architecture incrementally.


## 2. Strict frontend / backend separation

Every Novusion application must have a clear separation between:

**Backend** — business logic, data access, storage, networking, services,
background jobs, indexing, application state, integrations, processing.

**Frontend** — presentation, user interaction, view state required for
presentation, rendering through NovuUI.

Frontend must remain a clean user-interface layer.

Frontend code must not directly:

- access databases
- implement persistence
- perform filesystem storage
- contain business logic
- call infrastructure implementation directly
- perform long-running work on the UI thread

Communication between frontend and backend must happen through explicit
interfaces/services/controllers/view-model boundaries appropriate to the
application architecture.


## 3. Hub-owned data locations

NovuHub is the authority for Novusion application data locations.

Applications must not independently decide where persistent user data is
stored.

Hub provides the currently active user's configured storage locations to
applications.

There are only two writable user-data roots:

1. **Database / Data root** — user-selected during installation/setup.
2. **Cache root** — user-selected during installation/setup.

Applications consume these locations from Hub/shared platform configuration.
They must not silently replace them with operating-system defaults.


## 4. Strict write policy

Novusion applications may write persistent/runtime-generated user data ONLY
inside the Hub-provided Data Root or Cache Root.

Writing application data anywhere else is forbidden. This includes, but is
not limited to:

- AppData / LocalAppData / Roaming AppData
- Documents, Downloads, Desktop
- user profile directories
- temporary application-specific directories outside the configured Cache Root
- Qt default application-data locations
- framework/library default storage locations

No component, module, library or application may silently fall back to these
locations.


## 5. Data root

The Hub-provided Data Root is intended for durable user/application data.

Examples: application databases, user-created persistent application data,
persistent workspace data where appropriate, durable application state.

The exact structure beneath the root is owned by the Novusion storage
architecture. Applications must not invent another external persistence root.


## 6. Cache root

The Hub-provided Cache Root is the only location for disposable/generated
data.

Examples: caches, thumbnails, indexes that can be rebuilt, temporary
processing files, generated previews, downloaded temporary artifacts,
transient runtime data requiring filesystem storage.

Cache must be safe to delete and rebuild unless explicitly documented
otherwise. OS temp/AppData must not be used as an undocumented fallback.


## 7. No implicit framework storage

Third-party frameworks and libraries must be audited for implicit filesystem
writes. This explicitly includes Qt facilities that may resolve paths
automatically.

Do not assume that a library is compliant merely because application code did
not explicitly provide an AppData path. If a dependency writes files
automatically, configure, redirect or disable that behavior so all writes
remain inside the approved Data/Cache roots.


## 8. Central storage service

Applications and modules should not manually construct storage paths. Use a
shared storage/path service supplied by the platform/application context.

Conceptually:

```
StorageContext
    dataRoot()
    cacheRoot()
```

and scoped paths derived from those roots, for example:

```
Data Root
  /<user>/
      /NovuAI/
      /NovuSpace/
      /NovuExplorer/

Cache Root
  /<user>/
      /NovuAI/
      /NovuSpace/
      /NovuExplorer/
```

The final structure must follow the Hub/user architecture. Modules request
their owned storage scope from the storage service instead of concatenating
arbitrary absolute paths.


## 9. User context

Storage locations belong to the active Hub user context.

When the active user/account changes, applications must resolve storage
through the new user context. A module must never retain a stale path
belonging to another user. Multi-account support must therefore work through
the same storage boundary.


## 10. No silent fallback

Missing or invalid Hub storage configuration must NOT cause fallback to
`%APPDATA%`, Documents, `HOME`, Qt default paths, the current working
directory, or another arbitrary location.

Fail explicitly or enter an appropriate non-writing/degraded state. A visible
failure is preferable to silently storing data in an unauthorized location.


## 11. Storage classification

Every filesystem write must be classifiable as one of:

- **Durable data** → Data Root
- **Disposable / rebuildable data** → Cache Root

If a write cannot be classified into either category, its architecture must
be reviewed before implementation.


## 12. Module storage ownership

Modules do not own arbitrary filesystem roots. A module receives an
application/module-scoped location from the shared storage service.

Conceptually:

```
storage.dataFor("documents")
storage.cacheFor("web")
storage.cacheFor("preview")
```

This prevents modules from independently creating storage conventions.


## 13. Write audit

Every Novusion application must be auditable for filesystem writes. During
migration, search for at least:

- QStandardPaths
- AppData / LocalAppData / Roaming
- Documents
- HOME / USERPROFILE
- temp/tempPath
- QSettings filesystem backends
- SQLite paths
- log paths
- cache paths
- thumbnail paths
- Chromium/WebEngine profiles and caches
- downloaded/generated files
- crash/reporting output
- third-party library storage

Classify every write as: **DATA**, **CACHE**, **FORBIDDEN**, or **UNKNOWN**.
UNKNOWN is not considered acceptable for a completed migration.


## 14. Development and testing

Tests must verify the storage boundary. At minimum:

- application starts with custom Data Root and Cache Root
- generated files remain under those roots
- switching user context switches storage scope
- invalid roots do not cause OS-directory fallback
- modules cannot silently create AppData/Documents storage

Where practical, automated tests should use temporary test roots and verify
that all expected writes remain contained within them.


## 15. Migration rule

Existing applications that currently write outside the approved roots must be
migrated.

Migration process:

**Audit → Classify → Redirect → Test → Remove legacy fallback → Verify clean
user profile**

Do not merely add the new storage path while leaving old fallback behavior
active.


## Core principle

Novusion owns no hidden user-data locations.

The user chooses where Novusion Data and Cache live. Hub owns those
locations. Applications and modules consume them. Nothing writes elsewhere.

## Standard dropdowns

**All standard dropdowns must use the approved NovuUI dropdown component and visual behavior. Custom dropdown implementations are prohibited unless technically justified.**

- The approved dropdown is `novu::ui::SelectButton` (a pill with the current
  value, opening a `Popover`) - the look of the NovuAI chat's model selector and
  its Single / Multi / Mode selector. A closed `novu::ui::ComboBox` opens the
  same `Popover`, so a form that needs `QComboBox` semantics (items with data,
  `currentIndexChanged`) gets the same list, rows, hover, checked mark and
  placement.
- At most seven rows are shown; the rest scroll (`Popover::setMaximumVisibleItems`).
- The list opens below its control, above it when only that side has room, and
  scrolls on the roomier side when neither does; it opens on the checked item.
- No local stylesheet on a dropdown, no `QMenu` used as a select, no
  hand-made popup list. A searchable box (`ComboBox::setSearchable`) keeps
  Qt's list for its completer - that is the one documented exception.
- Specialised pickers with their own function (hierarchical trees, complex
  multi-select) are not "standard dropdowns" and stay what they are.
- AI agents creating UI follow this rule as well (it belongs in the global
  Qt Development skill).

