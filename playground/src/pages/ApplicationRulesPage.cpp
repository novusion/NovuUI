#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Card.h"
#include "novu/ui/Text.h"

#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// One rule, one card - the same "one concept per card" shape every other
// Gallery page uses, so this reads as part of the Gallery rather than as a
// document pasted into it. Returns the card's body so a rule with more than
// one paragraph can add further note()s.
QVBoxLayout* ruleCard(QWidget* page, QVBoxLayout* column, const QString& number,
                      const QString& title, const QString& lead)
{
    Card* card = exampleCard(page, QStringLiteral("%1. %2").arg(number, title), QString());
    QVBoxLayout* body = card->body();
    note(body, card, lead);
    column->addWidget(card);
    return body;
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(
            page, QStringLiteral("Novusion Application Architecture Rules"),
            QStringLiteral("Mandatory for every Novusion application."));
        note(card->body(), card,
             QStringLiteral("Existing applications that do not comply must be migrated "
                            "incrementally. Legacy implementation is not considered an "
                            "exception to these rules. The authoritative copy of this "
                            "document is docs/APPLICATION_RULES.md - this page is a "
                            "formatted view of it, kept beside the components every "
                            "application already comes here for."));
        column->addWidget(card);
    }

    ruleCard(page, column, QStringLiteral("1"), QStringLiteral("Modular architecture"),
             QStringLiteral("Every Novusion application must use a modular architecture. "
                            "Major product features must be implemented as self-contained "
                            "modules with:\n"
                            "• clear ownership\n"
                            "• explicit dependencies\n"
                            "• predictable source location\n"
                            "• defined lifecycle\n"
                            "• isolated feature logic\n"
                            "• minimal coupling to the application shell\n\n"
                            "The application shell must not accumulate feature-specific "
                            "implementation. Adding a new feature module should require "
                            "minimal changes to the application shell. If an existing "
                            "application is not modular, create a migration/refactor plan "
                            "and move it toward the shared modular architecture "
                            "incrementally."));

    {
        Card* card = exampleCard(page, QStringLiteral("2. Strict frontend / backend separation"),
                                 QString());
        QVBoxLayout* body = card->body();
        note(body, card,
             QStringLiteral("Every application must have a clear separation between the "
                            "two."));
        note(body, card,
             QStringLiteral("Backend: business logic, data access, storage, networking, "
                            "services, background jobs, indexing, application state, "
                            "integrations, processing."));
        note(body, card,
             QStringLiteral("Frontend: presentation, user interaction, view state required "
                            "for presentation, rendering through NovuUI. It must remain a "
                            "clean user-interface layer and must not directly:\n"
                            "• access databases\n"
                            "• implement persistence\n"
                            "• perform filesystem storage\n"
                            "• contain business logic\n"
                            "• call infrastructure implementation directly\n"
                            "• perform long-running work on the UI thread"));
        note(body, card,
             QStringLiteral("Communication between frontend and backend must happen "
                            "through explicit interfaces/services/controllers/view-model "
                            "boundaries appropriate to the application architecture."));
        column->addWidget(card);
    }

    ruleCard(page, column, QStringLiteral("3"), QStringLiteral("Hub-owned data locations"),
             QStringLiteral("NovuHub is the authority for Novusion application data "
                            "locations. Applications must not independently decide where "
                            "persistent user data is stored - Hub provides the currently "
                            "active user's configured storage locations to applications. "
                            "There are only two writable user-data roots: the Database / "
                            "Data root and the Cache root, both user-selected during "
                            "installation/setup. Applications consume these from Hub/shared "
                            "platform configuration and must not silently replace them "
                            "with operating-system defaults."));

    ruleCard(page, column, QStringLiteral("4"), QStringLiteral("Strict write policy"),
             QStringLiteral("Novusion applications may write persistent/runtime-generated "
                            "user data ONLY inside the Hub-provided Data Root or Cache "
                            "Root. Writing application data anywhere else is forbidden, "
                            "including but not limited to:\n"
                            "• AppData / LocalAppData / Roaming AppData\n"
                            "• Documents, Downloads, Desktop\n"
                            "• user profile directories\n"
                            "• temporary directories outside the configured Cache Root\n"
                            "• Qt default application-data locations\n"
                            "• framework/library default storage locations\n\n"
                            "No component, module, library or application may silently "
                            "fall back to these locations."));

    ruleCard(page, column, QStringLiteral("5"), QStringLiteral("Data root"),
             QStringLiteral("Intended for durable user/application data: application "
                            "databases, user-created persistent application data, "
                            "persistent workspace data where appropriate, durable "
                            "application state. The exact structure beneath the root is "
                            "owned by the Novusion storage architecture - applications "
                            "must not invent another external persistence root."));

    ruleCard(page, column, QStringLiteral("6"), QStringLiteral("Cache root"),
             QStringLiteral("The only location for disposable/generated data: caches, "
                            "thumbnails, indexes that can be rebuilt, temporary processing "
                            "files, generated previews, downloaded temporary artifacts, "
                            "transient runtime data requiring filesystem storage. Cache "
                            "must be safe to delete and rebuild unless explicitly "
                            "documented otherwise. OS temp/AppData must not be used as an "
                            "undocumented fallback."));

    ruleCard(page, column, QStringLiteral("7"), QStringLiteral("No implicit framework storage"),
             QStringLiteral("Third-party frameworks and libraries must be audited for "
                            "implicit filesystem writes - this explicitly includes Qt "
                            "facilities that may resolve paths automatically. Do not "
                            "assume a library is compliant merely because application code "
                            "did not explicitly provide an AppData path. If a dependency "
                            "writes files automatically, configure, redirect or disable "
                            "that behavior so all writes remain inside the approved "
                            "Data/Cache roots."));

    ruleCard(page, column, QStringLiteral("8"), QStringLiteral("Central storage service"),
             QStringLiteral("Applications and modules should not manually construct "
                            "storage paths. Use a shared storage/path service supplied by "
                            "the platform/application context - conceptually a "
                            "StorageContext with dataRoot() and cacheRoot(), and scoped "
                            "paths derived from those roots (Data Root/<user>/NovuSpace/, "
                            "Cache Root/<user>/NovuSpace/, and so on per application). The "
                            "final structure follows the Hub/user architecture; modules "
                            "request their owned storage scope from the storage service "
                            "instead of concatenating arbitrary absolute paths."));

    ruleCard(page, column, QStringLiteral("9"), QStringLiteral("User context"),
             QStringLiteral("Storage locations belong to the active Hub user context. "
                            "When the active user/account changes, applications must "
                            "resolve storage through the new user context - a module must "
                            "never retain a stale path belonging to another user. "
                            "Multi-account support must work through the same storage "
                            "boundary."));

    ruleCard(page, column, QStringLiteral("10"), QStringLiteral("No silent fallback"),
             QStringLiteral("Missing or invalid Hub storage configuration must NOT cause "
                            "fallback to %APPDATA%, Documents, HOME, Qt default paths, the "
                            "current working directory, or another arbitrary location. "
                            "Fail explicitly or enter an appropriate non-writing/degraded "
                            "state - a visible failure is preferable to silently storing "
                            "data in an unauthorized location."));

    ruleCard(page, column, QStringLiteral("11"), QStringLiteral("Storage classification"),
             QStringLiteral("Every filesystem write must be classifiable as durable data "
                            "(→ Data Root) or disposable/rebuildable data (→ "
                            "Cache Root). If a write cannot be classified into either "
                            "category, its architecture must be reviewed before "
                            "implementation."));

    ruleCard(page, column, QStringLiteral("12"), QStringLiteral("Module storage ownership"),
             QStringLiteral("Modules do not own arbitrary filesystem roots. A module "
                            "receives an application/module-scoped location from the "
                            "shared storage service - conceptually storage.dataFor"
                            "(\"documents\"), storage.cacheFor(\"web\"), storage.cacheFor"
                            "(\"preview\"). This prevents modules from independently "
                            "creating storage conventions."));

    ruleCard(page, column, QStringLiteral("13"), QStringLiteral("Write audit"),
             QStringLiteral("Every application must be auditable for filesystem writes. "
                            "During migration, search for at least QStandardPaths, "
                            "AppData/LocalAppData/Roaming, Documents, HOME/USERPROFILE, "
                            "temp paths, QSettings filesystem backends, SQLite paths, log "
                            "paths, cache paths, thumbnail paths, Chromium/WebEngine "
                            "profiles and caches, downloaded/generated files, crash/"
                            "reporting output, and third-party library storage. Classify "
                            "every write as DATA, CACHE, FORBIDDEN or UNKNOWN - UNKNOWN is "
                            "not acceptable for a completed migration."));

    ruleCard(page, column, QStringLiteral("14"), QStringLiteral("Development and testing"),
             QStringLiteral("Tests must verify the storage boundary: the application "
                            "starts with a custom Data Root and Cache Root, generated "
                            "files remain under those roots, switching user context "
                            "switches storage scope, invalid roots do not cause "
                            "OS-directory fallback, and modules cannot silently create "
                            "AppData/Documents storage. Where practical, automated tests "
                            "should use temporary test roots and verify that all expected "
                            "writes remain contained within them."));

    ruleCard(page, column, QStringLiteral("15"), QStringLiteral("Migration rule"),
             QStringLiteral("Existing applications that currently write outside the "
                            "approved roots must be migrated: Audit → Classify → "
                            "Redirect → Test → Remove legacy fallback → "
                            "Verify clean user profile. Do not merely add the new storage "
                            "path while leaving old fallback behavior active."));

    {
        Card* card = exampleCard(page, QStringLiteral("Core principle"), QString());
        note(card->body(), card,
             QStringLiteral("Novusion owns no hidden user-data locations. The user "
                            "chooses where Novusion Data and Cache live. Hub owns those "
                            "locations. Applications and modules consume them. Nothing "
                            "writes elsewhere."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec applicationRulesComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("application-rules");
    spec.name = QStringLiteral("Application rules");
    spec.category = Category::Foundation;
    spec.description = QStringLiteral(
        "The Novusion Application Architecture Rules - modular architecture, strict "
        "frontend/backend separation, and the Hub-owned Data Root / Cache Root storage "
        "boundary every application must stay inside. Mandatory, not a suggestion.");
    spec.headerPath = QStringLiteral("docs/APPLICATION_RULES.md");
    spec.sourcePath = QStringLiteral("playground/src/pages/ApplicationRulesPage.cpp");
    spec.designedFor = {QStringLiteral("novuspace"), QStringLiteral("novuai"),
                        QStringLiteral("novuexplorer"), QStringLiteral("novuadmin"),
                        QStringLiteral("novuhub")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Normal")};
    spec.states = {QStringLiteral("Normal")};
    spec.keywords = {QStringLiteral("rules"), QStringLiteral("architecture"),
                     QStringLiteral("guidelines"), QStringLiteral("storage"),
                     QStringLiteral("data root"), QStringLiteral("cache root"),
                     QStringLiteral("hub"), QStringLiteral("migration"),
                     QStringLiteral("frontend"), QStringLiteral("backend")};
    spec.properties = {};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
