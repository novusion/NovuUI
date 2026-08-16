// Tests for the NovuUI component gallery.
//
// What is worth asserting about a visual tool is everything except the
// pixels: that the registry is complete and unambiguous, that every page it
// advertises actually builds, that the sample-data layer degrades instead of
// crashing, that theme switching goes through the real ThemeManager, and that
// none of this can reach a production build.
//
// There is deliberately no screenshot comparison. NovuUI has no image-diff
// infrastructure, and introducing one here would make the gallery depend on a
// baseline nobody maintains.

#include "novu/playground/ComponentRegistry.h"
#include "novu/playground/DevWatcher.h"
#include "novu/playground/ExampleLayout.h"
#include "novu/playground/Paths.h"
#include "novu/playground/SampleData.h"
#include "novu/playground/Scenario.h"
#include "novu/playground/ShadowRun.h"
#include "novu/playground/ThemeCatalog.h"

#include "novu/ui/Theme.h"

#include "novu/ui/Layout.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/ThemeScope.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QScrollArea>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QTest>
#include <QWidget>

using namespace novu::playground;
using namespace novu::ui;

// The same rendering StyleSheet.cpp uses. Kept local rather than exported:
// the sheet's helper is an implementation detail, and a test that reimplements
// it would not notice the two drifting apart — so this asserts the *output*
// against colours the test derives itself.
static QString cssColor(const QColor& color)
{
    return color.alpha() == 255 ? color.name() : rgba(color, color.alphaF());
}

class GalleryTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // registry
    void registryIsPopulated();
    void componentIdsAreUnique();
    void componentMetadataIsComplete();
    void componentSourceFilesExist();
    void categoriesAreOrdered();

    // instantiation
    void everyComponentBuildsInEveryScenario();
    void everyComponentBuildsInEveryProfile();

    // search
    void searchFindsComponents_data();
    void searchFindsComponents();
    void emptySearchReturnsEverything();
    void searchThatMatchesNothingReturnsNothing();

    // scenarios
    void scenarioKeysRoundTrip();
    void unknownScenarioKeyFallsBackToNormal();

    // sample data
    void sampleDataLoads();
    void scenarioShapesRows();
    void missingSampleDataDirectoryFailsGracefully();
    void malformedSampleDataFailsGracefully();
    void unknownSetReturnsNothing();

    // theme
    void novuTradeMatchesTheApprovedMockupTokens();
    void translucentTokensKeepTheirAlphaInTheSheet();
    void scrollBarsFollowTheMockup();
    void scrollAreasAreNeverLeftUnstyled();
    void themeSwitchingUsesTheRealThemeManager();
    void themeOverlayIsRegisteredLive();
    void themeOverlayCannotOverwriteABuiltinProfile();

    // live reload
    void watcherSeesAFileCreatedAfterItStarted();
    void watcherSeparatesSourceChangesFromDataChanges();
    void shippedThemeOverlaysAllLoad();

    // rebuild-and-reload
    void theBuildTreeExecutableIsNotTheOneHoldingTheLock();

    // development-only boundary
    void playgroundIsNotPartOfTheProductionTarget();
    void productionSourcesDoNotReferenceThePlayground();

private:
    // A parent scoped exactly the way the gallery scopes its preview surface,
    // so a page under test resolves the same tokens it will at runtime.
    QWidget* makeSurface(const QString& profileId);

    ComponentRegistry m_registry;
    SampleData m_data;
    QList<QWidget*> m_surfaces;
};

void GalleryTest::initTestCase()
{
    registerBuiltinComponents(m_registry);
    m_data.reload();
    QVERIFY2(m_data.errors().isEmpty(),
             qPrintable(QStringLiteral("sample data did not load: %1")
                            .arg(m_data.errors().join(QStringLiteral("; ")))));
}

QWidget* GalleryTest::makeSurface(const QString& profileId)
{
    auto* surface = new QWidget();
    applyStyleSheet(surface, profileId);
    m_surfaces.append(surface);
    return surface;
}

// ── registry ──────────────────────────────────────────────────────────────

void GalleryTest::registryIsPopulated()
{
    QVERIFY(!m_registry.isEmpty());
    // Every component NovuUI exposes today. The number is asserted as a floor
    // rather than an equality so adding a component does not break the test,
    // while quietly losing one does.
    QVERIFY2(m_registry.count() >= 19,
             qPrintable(QStringLiteral("only %1 components registered").arg(m_registry.count())));
}

void GalleryTest::componentIdsAreUnique()
{
    const QStringList ids = m_registry.ids();
    QCOMPARE(QSet<QString>(ids.begin(), ids.end()).size(), ids.size());

    // And the registry refuses a second registration rather than replacing.
    ComponentSpec duplicate = *m_registry.find(ids.first());
    QString error;
    QVERIFY(!m_registry.add(duplicate, &error));
    QVERIFY(error.contains(QLatin1String("duplicate")));
    QCOMPARE(m_registry.ids().size(), ids.size());
}

void GalleryTest::componentMetadataIsComplete()
{
    for (const ComponentSpec& spec : m_registry.all()) {
        QVERIFY2(spec.isValid(), qPrintable(spec.id));
        QVERIFY2(!spec.description.isEmpty(), qPrintable(spec.id));
        QVERIFY2(!spec.headerPath.isEmpty(), qPrintable(spec.id));
        QVERIFY2(!spec.sourcePath.isEmpty(), qPrintable(spec.id));
        QVERIFY2(!spec.supportedScenarios().isEmpty(), qPrintable(spec.id));
        // The id is what a command line carries; it has to be stable and
        // typeable.
        QVERIFY2(spec.id == spec.id.toLower(), qPrintable(spec.id));
        QVERIFY2(!spec.id.contains(QLatin1Char(' ')), qPrintable(spec.id));
    }
}

void GalleryTest::componentSourceFilesExist()
{
    // "Open source" is only useful while the paths are true. A moved file is
    // otherwise a dead button nobody notices until they press it.
    for (const ComponentSpec& spec : m_registry.all()) {
        QVERIFY2(paths::existsInRepo(spec.headerPath),
                 qPrintable(QStringLiteral("%1: %2").arg(spec.id, spec.headerPath)));
        QVERIFY2(paths::existsInRepo(spec.sourcePath),
                 qPrintable(QStringLiteral("%1: %2").arg(spec.id, spec.sourcePath)));
    }
}

void GalleryTest::categoriesAreOrdered()
{
    const QVector<Category> order = allCategories();
    int previous = -1;
    for (const ComponentSpec& spec : m_registry.all()) {
        const int index = order.indexOf(spec.category);
        QVERIFY(index >= previous);
        previous = index;
    }
    QVERIFY(!m_registry.populatedCategories().isEmpty());
}

// ── instantiation ─────────────────────────────────────────────────────────

void GalleryTest::everyComponentBuildsInEveryScenario()
{
    QWidget* surface = makeSurface(ThemeManager::instance().defaultProfileId());

    for (const ComponentSpec& spec : m_registry.all()) {
        for (const Scenario scenario : spec.supportedScenarios()) {
            PreviewContext ctx;
            ctx.parent = surface;
            ctx.scenario = scenario;
            ctx.data = &m_data;
            ctx.profileId = ThemeManager::instance().defaultProfileId();

            QWidget* page = spec.build(ctx);
            QVERIFY2(page != nullptr,
                     qPrintable(QStringLiteral("%1 / %2 built nothing")
                                    .arg(spec.id, scenarioKey(scenario))));
            QCOMPARE(page->parentWidget(), surface);
            // Forces a layout pass, which is where a bad size hint or a null
            // dereference in sizeHint() surfaces.
            page->adjustSize();
            delete page;
        }
    }
}

void GalleryTest::everyComponentBuildsInEveryProfile()
{
    ThemeCatalog(paths::themeDir()).reload();

    for (const QString& profileId : ThemeCatalog::availableProfileIds()) {
        QWidget* surface = makeSurface(profileId);
        QCOMPARE(ThemeScope::resolve(surface), profileId);

        for (const ComponentSpec& spec : m_registry.all()) {
            PreviewContext ctx;
            ctx.parent = surface;
            ctx.scenario = spec.supportedScenarios().first();
            ctx.data = &m_data;
            ctx.profileId = profileId;

            QWidget* page = spec.build(ctx);
            QVERIFY2(page != nullptr,
                     qPrintable(QStringLiteral("%1 under %2").arg(spec.id, profileId)));
            page->adjustSize();
            delete page;
        }
    }
}

// ── search ────────────────────────────────────────────────────────────────

void GalleryTest::searchFindsComponents_data()
{
    QTest::addColumn<QString>("query");
    QTest::addColumn<QString>("expectedId");

    QTest::newRow("badge") << "badge" << "badge";
    QTest::newRow("table") << "table" << "table-view";
    QTest::newRow("tab") << "tab" << "tab-strip";
    QTest::newRow("card") << "card" << "card";
    QTest::newRow("button") << "button" << "button";
    QTest::newRow("mixed case") << "BuTTon" << "button";
    QTest::newRow("partial") << "prog" << "progress-bar";
    // Found through a keyword rather than the name.
    QTest::newRow("keyword") << "dropdown" << "fields";
    QTest::newRow("keyword colour") << "palette" << "theme-roles";
    QTest::newRow("description") << "wraps onto the next line" << "flow-layout";
}

void GalleryTest::searchFindsComponents()
{
    QFETCH(QString, query);
    QFETCH(QString, expectedId);

    const QVector<ComponentSpec> results = m_registry.search(query);
    QVERIFY2(!results.isEmpty(), qPrintable(query));

    QStringList ids;
    for (const ComponentSpec& spec : results) {
        ids.append(spec.id);
    }
    QVERIFY2(ids.contains(expectedId),
             qPrintable(QStringLiteral("\"%1\" returned [%2], expected %3")
                            .arg(query, ids.join(QStringLiteral(", ")), expectedId)));
}

void GalleryTest::emptySearchReturnsEverything()
{
    QCOMPARE(m_registry.search(QString()).size(), m_registry.count());
    QCOMPARE(m_registry.search(QStringLiteral("   ")).size(), m_registry.count());
}

void GalleryTest::searchThatMatchesNothingReturnsNothing()
{
    QVERIFY(m_registry.search(QStringLiteral("zzzz-not-a-component")).isEmpty());
}

// ── scenarios ─────────────────────────────────────────────────────────────

void GalleryTest::scenarioKeysRoundTrip()
{
    QSet<QString> keys;
    for (const Scenario scenario : allScenarios()) {
        const QString key = scenarioKey(scenario);
        QVERIFY(!key.isEmpty());
        QVERIFY(!keys.contains(key));
        keys.insert(key);

        bool ok = false;
        QCOMPARE(scenarioFromKey(key, &ok), scenario);
        QVERIFY(ok);
        QVERIFY(!scenarioLabel(scenario).isEmpty());
        QVERIFY(!scenarioDescription(scenario).isEmpty());
    }
}

void GalleryTest::unknownScenarioKeyFallsBackToNormal()
{
    // A stale command line written by an older build must not stop the
    // gallery from opening.
    bool ok = true;
    QCOMPARE(scenarioFromKey(QStringLiteral("retired-scenario"), &ok), Scenario::Normal);
    QVERIFY(!ok);
}

// ── sample data ───────────────────────────────────────────────────────────

void GalleryTest::sampleDataLoads()
{
    QVERIFY(m_data.isLoaded());
    const QStringList sets = m_data.setNames();
    for (const char* required : {"users", "trades", "tasks", "files", "metrics",
                                 "exchanges", "text", "columns", "sections", "plans"}) {
        QVERIFY2(sets.contains(QLatin1String(required)), required);
    }
    QVERIFY(!m_data.rows(QStringLiteral("users")).isEmpty());
    QVERIFY(!m_data.values(QStringLiteral("columns")).isEmpty());
    QVERIFY(!m_data.text(QStringLiteral("long_text")).isEmpty());
    QVERIFY(m_data.text(QStringLiteral("long_identifier")).size() > 32);
    QVERIFY(!m_data.optionalFields(QStringLiteral("users")).isEmpty());
}

void GalleryTest::scenarioShapesRows()
{
    const QString set = QStringLiteral("users");
    const int base = m_data.rows(set).size();
    QVERIFY(base > 1);

    QCOMPARE(m_data.rowsFor(set, Scenario::Normal).size(), base);
    QCOMPARE(m_data.rowsFor(set, Scenario::Empty).size(), 0);
    QCOMPARE(m_data.rowsFor(set, Scenario::ErrorState).size(), 0);
    QCOMPARE(m_data.rowsFor(set, Scenario::SingleItem).size(), 1);
    QVERIFY(m_data.rowsFor(set, Scenario::ManyItems).size() > base);
    QCOMPARE(m_data.rowsFor(set, Scenario::LargeDataset).size(),
             SampleData::largeDatasetSize());

    // MissingOptional actually removes the declared fields.
    const QStringList optional = m_data.optionalFields(set);
    const QJsonArray missing = m_data.rowsFor(set, Scenario::MissingOptional);
    QCOMPARE(missing.size(), base);
    for (const QJsonValue& value : missing) {
        const QJsonObject row = value.toObject();
        for (const QString& field : optional) {
            QVERIFY2(!row.contains(field), qPrintable(field));
        }
    }

    // LongText writes the hostile string into the first string field.
    const QJsonArray longText = m_data.rowsFor(set, Scenario::LongText);
    QCOMPARE(longText.size(), base);
    const QString expected = m_data.text(QStringLiteral("long_text"));
    bool found = false;
    const QJsonObject first = longText.first().toObject();
    for (auto it = first.constBegin(); it != first.constEnd(); ++it) {
        found = found || it.value().toString() == expected;
    }
    QVERIFY(found);
}

void GalleryTest::missingSampleDataDirectoryFailsGracefully()
{
    SampleData data(QStringLiteral("J:/definitely/not/a/directory"));
    QVERIFY(!data.reload());
    QVERIFY(!data.isLoaded());
    QCOMPARE(data.errors().size(), 1);
    // And it still answers every question, with nothing.
    QVERIFY(data.setNames().isEmpty());
    QVERIFY(data.rows(QStringLiteral("users")).isEmpty());
    QVERIFY(data.rowsFor(QStringLiteral("users"), Scenario::ManyItems).isEmpty());
    QVERIFY(!data.text(QStringLiteral("long_text")).isEmpty());   // built-in fallback
}

void GalleryTest::malformedSampleDataFailsGracefully()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const auto write = [&dir](const QString& name, const QByteArray& content) {
        QFile file(dir.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(content);
    };
    write(QStringLiteral("broken.json"), "{ \"set\": \"broken\", rows: [ ");
    write(QStringLiteral("not-an-object.json"), "[1, 2, 3]");
    write(QStringLiteral("good.json"),
          "{ \"set\": \"good\", \"rows\": [ { \"name\": \"one\" } ] }");

    SampleData data(dir.path());
    QVERIFY(!data.reload());
    QCOMPARE(data.errors().size(), 2);

    // The one good file is still usable: a broken fixture degrades the page
    // that uses it, not the tool.
    QVERIFY(data.has(QStringLiteral("good")));
    QCOMPARE(data.rows(QStringLiteral("good")).size(), 1);
    QVERIFY(!data.has(QStringLiteral("broken")));

    // And a page can build against it without crashing.
    QWidget* surface = makeSurface(ThemeManager::instance().defaultProfileId());
    for (const ComponentSpec& spec : m_registry.all()) {
        PreviewContext ctx;
        ctx.parent = surface;
        ctx.scenario = spec.supportedScenarios().first();
        ctx.data = &data;
        QWidget* page = spec.build(ctx);
        QVERIFY2(page != nullptr, qPrintable(spec.id));
        delete page;
    }
}

void GalleryTest::unknownSetReturnsNothing()
{
    QVERIFY(!m_data.has(QStringLiteral("no-such-set")));
    QVERIFY(m_data.rows(QStringLiteral("no-such-set")).isEmpty());
    QVERIFY(m_data.values(QStringLiteral("no-such-set")).isEmpty());
    QVERIFY(m_data.optionalFields(QStringLiteral("no-such-set")).isEmpty());
    QVERIFY(m_data.describe(QStringLiteral("no-such-set")).isEmpty());
}

// ── theme ─────────────────────────────────────────────────────────────────

void GalleryTest::novuTradeMatchesTheApprovedMockupTokens()
{
    // Transcribed from the approved mockup,
    // NovuSpace/design/qt6_mockups_graphite/novutrade_ui.py. That file is the
    // authority for this profile; anything here that disagrees with it is a
    // regression in NovuUI, not a difference of opinion.
    const ThemeProfile p = ThemeManager::instance().profile(QStringLiteral("novutrade"));

    // palette
    QCOMPARE(p.colors.windowBg.name().toUpper(),    QStringLiteral("#0F172A"));   // BG
    QCOMPARE(p.colors.panelBg.name().toUpper(),     QStringLiteral("#111827"));   // SURFACE
    QCOMPARE(p.colors.panelBgAlt.name().toUpper(),  QStringLiteral("#0B1220"));   // SURFACE_ALT
    QCOMPARE(p.colors.accent.name().toUpper(),      QStringLiteral("#10B981"));   // PRIMARY
    QCOMPARE(p.colors.accentLight.name().toUpper(), QStringLiteral("#0EA5E9"));   // SECONDARY
    QCOMPARE(p.colors.warning.name().toUpper(),     QStringLiteral("#F59E0B"));   // ACCENT
    QCOMPARE(p.colors.success.name().toUpper(),     QStringLiteral("#22C55E"));
    QCOMPARE(p.colors.danger.name().toUpper(),      QStringLiteral("#EF4444"));
    QCOMPARE(p.colors.textPrimary.name().toUpper(), QStringLiteral("#E3E8F0"));   // TEXT
    QCOMPARE(p.colors.textSecondary.name().toUpper(), QStringLiteral("#9CA3AF")); // TEXT_SEC
    QCOMPARE(p.colors.textDisabled.name().toUpper(), QStringLiteral("#6B7280"));  // MUTED
    QCOMPARE(p.colors.textOnAccent.name().toUpper(), QStringLiteral("#062018"));
    // BORDER = rgba(148,163,184,0.18)
    QCOMPARE(p.colors.border.red(), 148);
    QCOMPARE(p.colors.border.green(), 163);
    QCOMPARE(p.colors.border.blue(), 184);
    QVERIFY(qAbs(p.colors.border.alphaF() - 0.18) < 0.01);

    // SP_XS..SP_XL = 4, 8, 12, 16, 24
    QCOMPARE(p.spacing.xs, 4);
    QCOMPARE(p.spacing.sm, 8);
    QCOMPARE(p.spacing.md, 12);
    QCOMPARE(p.spacing.lg, 16);
    QCOMPARE(p.spacing.xl, 24);

    // R_INPUT, R_BTN, R_CARD, R_DIALOG = 4, 6, 12, 16
    QCOMPARE(p.radius(SurfaceRole::Input),  4);
    QCOMPARE(p.radius(SurfaceRole::Button), 6);
    QCOMPARE(p.radius(SurfaceRole::Card),   12);
    QCOMPARE(p.radius(SurfaceRole::Dialog), 16);

    // F_XS..F_XL = 11, 12, 13, 16, 20
    QCOMPARE(p.type.xs, 11);
    QCOMPARE(p.type.sm, 12);
    QCOMPARE(p.type.md, 13);
    QCOMPARE(p.type.lg, 16);
    QCOMPARE(p.type.xl, 20);

    // CTRL_H, CTRL_H_SM, ROW_H = 32, 26, 34
    QCOMPARE(p.controlHeight(ControlSize::Medium), 32);
    QCOMPARE(p.controlHeight(ControlSize::Small),  26);
    QCOMPARE(p.controls.rowHeight, 34);
    QCOMPARE(p.table.rowHeight, 34);
    QCOMPARE(p.table.headerHeight, 34);

    // Card title is fully bold in this profile; StatCard's value is F_XL.
    QCOMPARE(p.text(TypeRole::CardTitle).weight, 700);
    QCOMPARE(p.text(TypeRole::CardTitle).size, 13);
    QCOMPARE(p.text(TypeRole::Metric).size, 20);
    QCOMPARE(p.text(TypeRole::Caption).size, 12);

    // Card padding p-4, page padding p-6, toolbar gap-2.
    QCOMPARE(p.space(SpacingRole::CardPadding), 16);
    QCOMPARE(p.space(SpacingRole::PagePadding), 24);
    QCOMPARE(p.space(SpacingRole::ToolbarGap),  8);
}

void GalleryTest::translucentTokensKeepTheirAlphaInTheSheet()
{
    // NovuTrade defines its borders as slate at 18% so an outline reads as a
    // hairline over whatever is behind it. QColor::name() drops alpha, so the
    // sheet used to emit #94a3b8 — a near-white line around every input.
    const ThemeProfile trade = ThemeManager::instance().profile(QStringLiteral("novutrade"));
    QVERIFY2(trade.colors.border.alpha() < 255, "the fixture this test guards has changed");

    const QString sheet = styleSheetFor(trade);
    QVERIFY2(!sheet.contains(trade.colors.border.name(), Qt::CaseInsensitive),
             "a translucent border reached the sheet as an opaque hex colour");
    QVERIFY(sheet.contains(rgba(trade.colors.border, trade.colors.border.alphaF())));

    // A profile whose borders are opaque is emitted exactly as before.
    const ThemeProfile space = ThemeManager::instance().profile(QStringLiteral("novuspace"));
    QCOMPARE(space.colors.border.alpha(), 255);
    QVERIFY(styleSheetFor(space).contains(space.colors.border.name()));
}

void GalleryTest::scrollBarsFollowTheMockup()
{
    // SCROLLBAR_QSS in novutrade_qt6.py: transparent groove, 10px wide, thumb
    // in the border colour with a 5px radius. The horizontal bar is 9px/4px,
    // from scroll_x() in novutrade_ui.py.
    for (const QString& id : ThemeCatalog::builtinProfileIds()) {
        const ThemeProfile profile = ThemeManager::instance().profile(id);
        const QString sheet = styleSheetFor(profile);

        QVERIFY(sheet.contains(QLatin1String(
            "QScrollBar:vertical { background: transparent; width: 10px;")));
        QVERIFY(sheet.contains(QLatin1String(
            "QScrollBar:horizontal { background: transparent; height: 9px;")));
        QVERIFY(sheet.contains(QStringLiteral(
            "QScrollBar::handle:vertical { background: %1; border-radius: 5px;")
                                   .arg(cssColor(profile.colors.border))));
        QVERIFY(sheet.contains(QStringLiteral(
            "QScrollBar::handle:horizontal { background: %1; border-radius: 4px;")
                                   .arg(cssColor(profile.colors.border))));
    }
}

void GalleryTest::scrollAreasAreNeverLeftUnstyled()
{
    // scrollPage() built a QScrollArea and never named it, while the only
    // rule for one was QScrollArea#novuScrollArea. The area matched nothing,
    // kept Qt's default palette and painted a grey block inside a navy card —
    // in every application that scrolls a page, not only in the gallery.
    for (const QString& id : ThemeCatalog::builtinProfileIds()) {
        const QString sheet = styleSheetFor(ThemeManager::instance().profile(id));
        QVERIFY2(sheet.contains(QLatin1String("QScrollArea { background: transparent;")),
                 qPrintable(QStringLiteral("%1: a bare QScrollArea matches no rule").arg(id)));
        QVERIFY(sheet.contains(QLatin1String("QScrollArea > QWidget > QWidget")));
    }

    // And the area the library hands out carries the name its sheet targets.
    QWidget* surface = makeSurface(QStringLiteral("novutrade"));
    auto* page = new QWidget(surface);
    QScrollArea* area = nullptr;
    QVBoxLayout* inner = scrollPage(page, &area);
    QVERIFY(inner != nullptr);
    QVERIFY(area != nullptr);
    QCOMPARE(area->objectName(), QString::fromLatin1(name::scrollArea));
    QCOMPARE(area->frameShape(), QFrame::NoFrame);
    QVERIFY(!area->viewport()->autoFillBackground());
}

void GalleryTest::themeSwitchingUsesTheRealThemeManager()
{
    ThemeManager& manager = ThemeManager::instance();
    const QStringList builtins = ThemeCatalog::builtinProfileIds();
    QVERIFY(builtins.size() >= 2);
    for (const QString& id : builtins) {
        QVERIFY2(manager.hasProfile(id), qPrintable(id));
        QVERIFY(ThemeCatalog::availableProfileIds().contains(id));
    }

    // Switching a scope is what the gallery does, and themeFor() must follow
    // it — this is the assertion that the preview is themed by the library
    // rather than by anything local to the playground.
    QWidget* surface = makeSurface(builtins.at(0));
    QCOMPARE(themeFor(surface).id, builtins.at(0));

    auto* child = new QWidget(surface);
    QCOMPARE(themeFor(child).id, builtins.at(0));

    applyStyleSheet(surface, builtins.at(1));
    QCOMPARE(themeFor(surface).id, builtins.at(1));
    QCOMPARE(themeFor(child).id, builtins.at(1));
    QVERIFY(!surface->styleSheet().isEmpty());
    QCOMPARE(surface->styleSheet(), styleSheetFor(manager.profile(builtins.at(1))));
}

void GalleryTest::themeOverlayIsRegisteredLive()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QFile file(dir.filePath(QStringLiteral("overlay.json")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"({
        "id": "gallery-test-overlay",
        "displayName": "Gallery test overlay",
        "base": "novuspace",
        "colors": { "accent": "#FF00FF" },
        "radii": { "medium": 7 },
        "controls": { "height": 41 }
    })");
    file.close();

    ThemeManager& manager = ThemeManager::instance();
    QSignalSpy spy(&manager, &ThemeManager::profileChanged);

    ThemeCatalog catalog(dir.path());
    QVERIFY(catalog.reload());
    QCOMPARE(catalog.overlayIds(), QStringList{QStringLiteral("gallery-test-overlay")});

    // Registered with the real manager, and announced — which is the signal
    // that repaints live widgets.
    QVERIFY(manager.hasProfile(QStringLiteral("gallery-test-overlay")));
    QCOMPARE(spy.count(), 1);

    const ThemeProfile overlay = manager.profile(QStringLiteral("gallery-test-overlay"));
    QCOMPARE(overlay.colors.accent.name().toUpper(), QStringLiteral("#FF00FF"));
    QCOMPARE(overlay.radii.medium, 7);
    QCOMPARE(overlay.controls.height, 41);
    // Everything not patched comes from the base profile.
    QCOMPARE(overlay.colors.panelBg, manager.profile(QStringLiteral("novuspace")).colors.panelBg);

    QWidget* surface = makeSurface(QStringLiteral("gallery-test-overlay"));
    QCOMPARE(themeFor(surface).colors.accent.name().toUpper(), QStringLiteral("#FF00FF"));
}

void GalleryTest::themeOverlayCannotOverwriteABuiltinProfile()
{
    const QString builtin = ThemeCatalog::builtinProfileIds().first();
    const QColor before = ThemeManager::instance().profile(builtin).colors.accent;

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile file(dir.filePath(QStringLiteral("hijack.json")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QStringLiteral(R"({ "id": "%1", "base": "%1",
                                   "colors": { "accent": "#123456" } })")
                   .arg(builtin)
                   .toUtf8());
    file.close();

    ThemeCatalog catalog(dir.path());
    QVERIFY(!catalog.reload());
    QCOMPARE(catalog.overlayIds().size(), 0);
    QCOMPARE(catalog.errors().size(), 1);
    QVERIFY(catalog.errors().first().contains(QLatin1String("built-in")));

    // The product profile is untouched — preview data has not become
    // production data.
    QCOMPARE(ThemeManager::instance().profile(builtin).colors.accent, before);
}

// ── development-only boundary ──────────────────────────────────────────────

void GalleryTest::watcherSeesAFileCreatedAfterItStarted()
{
    // The bug this guards: the watcher only ever held the files that existed
    // when it started. A theme overlay added while the gallery was running
    // was picked up once, through its directory, and then never responded to
    // another save — which is exactly the workflow the tool promises.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    DevWatcher watcher;
    watcher.watchData({dir.path()});

    QSignalSpy spy(&watcher, &DevWatcher::dataChanged);

    const QString path = dir.filePath(QStringLiteral("late.json"));
    const auto write = [&path](const char* content) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write(content);
        file.close();
    };

    write("{ \"set\": \"late\" }");
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 1, 5000);

    // The second save is the one that used to be silent.
    const int afterCreate = spy.count();
    write("{ \"set\": \"late\", \"rows\": [ { \"a\": 1 } ] }");
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() > afterCreate, 5000);
}

void GalleryTest::watcherSeparatesSourceChangesFromDataChanges()
{
    // Conflating the two is how the tool starts lying: a theme tweak that
    // triggers a compiler, or a C++ edit that appears to have taken effect.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("code")));
    const QString codeDir = dir.filePath(QStringLiteral("code"));

    DevWatcher watcher;
    watcher.watchSources({codeDir});

    QSignalSpy sources(&watcher, &DevWatcher::sourcesChanged);
    QSignalSpy data(&watcher, &DevWatcher::dataChanged);

    QFile file(QDir(codeDir).filePath(QStringLiteral("Widget.cpp")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("// change\n");
    file.close();

    QTRY_VERIFY_WITH_TIMEOUT(sources.count() >= 1, 5000);
    QCOMPARE(data.count(), 0);
}

void GalleryTest::shippedThemeOverlaysAllLoad()
{
    // Every overlay in the repository parses and registers. An overlay that
    // silently fails is a theme that is simply missing from the selector,
    // which is what a developer then reports as "where is it".
    const QDir dir(paths::themeDir());
    const QStringList files = dir.entryList({QStringLiteral("*.json")}, QDir::Files);
    QVERIFY2(!files.isEmpty(), qPrintable(dir.absolutePath()));

    ThemeCatalog catalog(paths::themeDir());
    QVERIFY2(catalog.reload(),
             qPrintable(catalog.errors().join(QStringLiteral("; "))));
    QCOMPARE(catalog.overlayIds().size(), files.size());

    for (const QString& id : catalog.overlayIds()) {
        QVERIFY2(ThemeManager::instance().hasProfile(id), qPrintable(id));
        QVERIFY2(ThemeCatalog::availableProfileIds().contains(id), qPrintable(id));
        QVERIFY2(!ThemeCatalog::displayNameFor(id).isEmpty(), qPrintable(id));
    }
}

void GalleryTest::theBuildTreeExecutableIsNotTheOneHoldingTheLock()
{
    // Windows locks a running executable, so a gallery started straight out
    // of the build tree could never relink itself: every Rebuild & reload
    // ended at LNK1104. It therefore hands over to a copy, and a reload
    // restarts the origin rather than the copy.
    const QString build = QDir::cleanPath(paths::buildDir());
    QVERIFY(shadow::pathIsInsideBuildTree(build + QStringLiteral("/playground/Release/x.exe")));
    QVERIFY(!shadow::pathIsInsideBuildTree(QStringLiteral("C:/Program Files/App/x.exe")));
    QVERIFY(!shadow::pathIsInsideBuildTree(shadow::shadowDirectory()
                                           + QStringLiteral("/NovuUIPlayground-run.exe")));
    // A path that merely shares a prefix is not inside the tree.
    QVERIFY(!shadow::pathIsInsideBuildTree(build + QStringLiteral("-other/x.exe")));

    // The origin defaults to this process and is what a reload restarts.
    shadow::setOriginExecutable(QString());
    QCOMPARE(shadow::originExecutable(), QCoreApplication::applicationFilePath());
    const QString origin = build + QStringLiteral("/playground/Release/NovuUIPlayground.exe");
    shadow::setOriginExecutable(origin);
    QCOMPARE(shadow::originExecutable(), origin);
    shadow::setOriginExecutable(QString());
}

void GalleryTest::playgroundIsNotPartOfTheProductionTarget()
{
    QFile file(paths::absoluteInRepo(QStringLiteral("CMakeLists.txt")));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString cmake = QString::fromUtf8(file.readAll());

    // 1. The library target lists no playground source.
    const int start = cmake.indexOf(QLatin1String("add_library(NovuUI STATIC"));
    QVERIFY(start >= 0);
    const int end = cmake.indexOf(QLatin1Char(')'), start);
    QVERIFY(end > start);
    const QString target = cmake.mid(start, end - start);
    QVERIFY2(!target.contains(QLatin1String("playground"), Qt::CaseInsensitive),
             "the NovuUI target lists a playground source");
    QVERIFY2(!target.contains(QLatin1String("sample_data"), Qt::CaseInsensitive),
             "the NovuUI target lists sample data");

    // 2. The subdirectory is reachable only behind the option.
    const int add = cmake.indexOf(QLatin1String("add_subdirectory(playground)"));
    QVERIFY2(add >= 0, "playground is never added");
    const int guard = cmake.lastIndexOf(QLatin1String("if(NOVUUI_BUILD_PLAYGROUND)"), add);
    QVERIFY2(guard >= 0 && guard < add, "add_subdirectory(playground) is not guarded");
    QVERIFY(cmake.contains(QLatin1String("option(NOVUUI_BUILD_PLAYGROUND")));
    QVERIFY(cmake.contains(QLatin1String("option(NOVUUI_BUILD_TESTS")));

    // 3. The option defaults to off for a consumer that add_subdirectory()s
    //    this repository.
    QVERIFY2(cmake.contains(QLatin1String("${PROJECT_IS_TOP_LEVEL})")),
             "the playground option does not default to PROJECT_IS_TOP_LEVEL");
}

void GalleryTest::productionSourcesDoNotReferenceThePlayground()
{
    // A production file that included a playground header would compile fine
    // in a developer's tree and fail in every application.
    const QStringList directories = {paths::repoRoot() + QStringLiteral("/src"),
                                     paths::repoRoot() + QStringLiteral("/include/novu/ui")};
    int inspected = 0;
    for (const QString& directory : directories) {
        QDir dir(directory);
        QVERIFY2(dir.exists(), qPrintable(directory));
        const QStringList files = dir.entryList({QStringLiteral("*.cpp"), QStringLiteral("*.h")},
                                                QDir::Files);
        for (const QString& name : files) {
            QFile file(dir.filePath(name));
            QVERIFY(file.open(QIODevice::ReadOnly));
            const QString content = QString::fromUtf8(file.readAll());
            QVERIFY2(!content.contains(QLatin1String("novu/playground")),
                     qPrintable(QStringLiteral("%1 includes a playground header").arg(name)));
            QVERIFY2(!content.contains(QLatin1String("sample_data")),
                     qPrintable(QStringLiteral("%1 references sample data").arg(name)));
            ++inspected;
        }
    }
    QVERIFY(inspected > 20);
}

QTEST_MAIN(GalleryTest)
#include "tst_gallery.moc"
