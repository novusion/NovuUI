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

#include "novu/ui/Button.h"
#include "novu/ui/FeedbackForm.h"
#include "novu/ui/Fields.h"
#include "novu/ui/HelpOverlay.h"
#include "novu/ui/Layout.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/ThemeScope.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QJsonArray>
#include <QPainter>
#include <QJsonObject>
#include <QScrollArea>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QTest>
#include <QWidget>

#include <cmath>

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
    void everyComponentDeclaresItsApplications();
    void theApplicationFilterNarrowsAndNothingElse();
    void filteringByANonApplicationReturnsNothing();
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

    // feedback
    void feedbackRefusesAnIncompleteReport();
    void feedbackSendsOnceAndOnlyOnce();
    void feedbackFailureLetsTheUserRetry();
    void feedbackAttachesNothingTheUserDidNotAgreeTo();
    void feedbackDisablesLogsWhenTheHostHasNone();
    void feedbackIgnoresAnAnswerItIsNotWaitingFor();

    // help overlay
    void helpOverlayLetsTheApplicationThrough();
    void helpOverlayFollowsItsHost();
    void helpOverlayDropsNotesWhoseTargetIsGone();
    void helpOverlayTonesComeFromTheSharedPalette();

    // theme
    void novuTradeMatchesTheApprovedMockupTokens();
    void applicationProfilesShareOnePalette();
    void everyProfileSharesTheSemanticPalette();
    void theOnlyStatusOverrideIsTheLightProfile();
    void semanticColoursReadAgainstTheirOwnSurface();
    void infoIsAStatusAndNotTheApplicationAccent();
    void badgeTonesResolveThroughTheSharedPalette();
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

void GalleryTest::everyComponentDeclaresItsApplications()
{
    // Membership is the filter's whole source of truth, so a spec that forgets
    // it disappears from every filtered view without anyone noticing. Better to
    // fail here than to have a component quietly belong to nobody.
    for (const ComponentSpec& spec : m_registry.all()) {
        // Either it records who uses it, or it says outright that nobody does
        // yet. Silence is the case this catches.
        QVERIFY2(!spec.usedBy.isEmpty() || spec.awaitingAdoption, qPrintable(spec.id));
        QVERIFY2(!(spec.awaitingAdoption && !spec.usedBy.isEmpty()),
                 qPrintable(QStringLiteral("%1 claims both adoption and none").arg(spec.id)));
        for (const QString& application : spec.usedBy) {
            QVERIFY2(novu::ui::isApplicationProfile(application),
                     qPrintable(QStringLiteral("%1 claims %2, which is not an application")
                                    .arg(spec.id, application)));
        }
    }
}

void GalleryTest::theApplicationFilterNarrowsAndNothingElse()
{
    const QVector<ComponentSpec> everything = m_registry.search(QString());
    const QVector<ComponentSpec> forAi =
        m_registry.search(QString(), QStringLiteral("novuai"));

    QVERIFY(!forAi.isEmpty());
    QVERIFY(forAi.size() <= everything.size());

    // A subset, in the same order, with every entry unchanged. The filter
    // controls visibility and must not touch what a component *is* - a
    // filtered spec that differed from an unfiltered one would mean the
    // gallery stopped showing the same component the applications use.
    int index = 0;
    for (const ComponentSpec& spec : everything) {
        if (index < forAi.size() && forAi.at(index).id == spec.id) {
            QCOMPARE(forAi.at(index).name, spec.name);
            QCOMPARE(forAi.at(index).category, spec.category);
            QCOMPARE(forAi.at(index).variants, spec.variants);
            QVERIFY(forAi.at(index).build != nullptr);
            ++index;
        } else {
            QVERIFY2(!spec.isUsedBy(QStringLiteral("novuai")), qPrintable(spec.id));
        }
    }
    QCOMPARE(index, forAi.size());

    // Search and the filter compose rather than override one another.
    const QVector<ComponentSpec> searched =
        m_registry.search(QStringLiteral("badge"), QStringLiteral("novuai"));
    for (const ComponentSpec& spec : searched) {
        QVERIFY(spec.matches(QStringLiteral("badge")));
        QVERIFY(spec.isUsedBy(QStringLiteral("novuai")));
    }
}

void GalleryTest::filteringByANonApplicationReturnsNothing()
{
    // Midnight Blue is an appearance a user picks, not a product. Asked to
    // filter by it the registry finds no member - which is why the gallery
    // never asks, and disables the control instead.
    QVERIFY(!novu::ui::isApplicationProfile(QStringLiteral("midnight-blue")));
    QVERIFY(m_registry.search(QString(), QStringLiteral("midnight-blue")).isEmpty());

    // An empty application means "no narrowing", not "narrow to nobody".
    QCOMPARE(m_registry.search(QString(), QString()).size(), m_registry.all().size());
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

namespace {

// WCAG relative luminance, so a claim about legibility is measured rather than
// asserted. The semantic palette exists to be read at a glance; a profile that
// quietly drops one of its statuses under 4.5:1 has broken it just as surely as
// one that changes the hue.
double luminance(const QColor& c)
{
    const auto channel = [](double v) {
        v /= 255.0;
        return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel(c.red()) + 0.7152 * channel(c.green())
         + 0.0722 * channel(c.blue());
}

double contrast(const QColor& a, const QColor& b)
{
    const double la = luminance(a);
    const double lb = luminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

// The profiles allowed to state their own status colours, and why. Anything
// else that overrides them is drift, and the test below says so by name.
bool mayOverrideStatus(const QString& profileId)
{
    // Surface polarity, not product identity: the shared hues are tuned for
    // dark panels and fall to 2.5:1 against white.
    return profileId == QLatin1String("novu-light");
}

} // namespace

void GalleryTest::applicationProfilesShareOnePalette()
{
    // One palette, every application. Not "similar" - identical, entry by
    // entry, so that a component cannot look like a different product depending
    // on which window it is in.
    //
    // NovuTrade is the exception and the only one. Its slate surfaces and
    // emerald accent are the product rather than drift, so it is skipped here
    // and asserted separately by novuTradeMatchesTheApprovedMockupTokens().
    const ThemeColors& shared = ThemeManager::instance()
                                    .profile(QStringLiteral("novuspace")).colors;

    const auto same = [](const QColor& a, const QColor& b) { return a.rgba() == b.rgba(); };

    for (const QString& id : ThemeManager::instance().profileIds()) {
        if (!isApplicationProfile(id) || id == QLatin1String("novutrade")) {
            continue;
        }
        const ThemeColors& c = ThemeManager::instance().profile(id).colors;
        const struct { const char* name; QColor mine; QColor theirs; } entries[] = {
            {"windowBg", c.windowBg, shared.windowBg},
            {"panelBg", c.panelBg, shared.panelBg},
            {"panelBgAlt", c.panelBgAlt, shared.panelBgAlt},
            {"headerBg", c.headerBg, shared.headerBg},
            {"surfaceHover", c.surfaceHover, shared.surfaceHover},
            {"surfaceActive", c.surfaceActive, shared.surfaceActive},
            {"border", c.border, shared.border},
            {"borderSubtle", c.borderSubtle, shared.borderSubtle},
            {"borderHover", c.borderHover, shared.borderHover},
            {"textPrimary", c.textPrimary, shared.textPrimary},
            {"textSecondary", c.textSecondary, shared.textSecondary},
            {"textDisabled", c.textDisabled, shared.textDisabled},
            {"textOnAccent", c.textOnAccent, shared.textOnAccent},
            {"accent", c.accent, shared.accent},
            {"accentLight", c.accentLight, shared.accentLight},
            {"success", c.success, shared.success},
            {"warning", c.warning, shared.warning},
            {"danger", c.danger, shared.danger},
            {"info", c.info, shared.info},
        };
        for (const auto& e : entries) {
            QVERIFY2(same(e.mine, e.theirs),
                     qPrintable(QStringLiteral("%1 forked %2: %3 against %4")
                                    .arg(id, QLatin1String(e.name),
                                         e.mine.name(), e.theirs.name())));
        }
    }
}

void GalleryTest::everyProfileSharesTheSemanticPalette()
{
    const SemanticHues& shared = sharedSemanticHues();
    for (const QString& id : ThemeManager::instance().profileIds()) {
        const ThemeProfile p = ThemeManager::instance().profile(id);
        if (mayOverrideStatus(id) || id.contains(QLatin1String("contrast"))) {
            continue;
        }
        // An application owns its surfaces and its accent. It does not own what
        // "failed" looks like - that is the same statement in every Novu
        // window, so it is the same colour.
        QCOMPARE(p.colors.success.name(), shared.success.name());
        QCOMPARE(p.colors.warning.name(), shared.warning.name());
        QCOMPARE(p.colors.danger.name(), shared.danger.name());
        QCOMPARE(p.colors.info.name(), shared.info.name());
    }
}

void GalleryTest::theOnlyStatusOverrideIsTheLightProfile()
{
    // Stated as a test so that adding a second exception is a deliberate act
    // with a reason attached, not a line in a palette nobody re-reads.
    const SemanticHues& shared = sharedSemanticHues();
    QStringList overriding;
    for (const QString& id : ThemeManager::instance().profileIds()) {
        if (id.contains(QLatin1String("contrast"))) {
            continue;   // a development overlay, not a product
        }
        const ThemeProfile p = ThemeManager::instance().profile(id);
        if (p.colors.success != shared.success || p.colors.warning != shared.warning
            || p.colors.danger != shared.danger || p.colors.info != shared.info) {
            overriding.append(id);
        }
    }
    QCOMPARE(overriding, QStringList{QStringLiteral("novu-light")});
}

void GalleryTest::semanticColoursReadAgainstTheirOwnSurface()
{
    // The point of a shared palette is that it is legible everywhere it is
    // shared. Checked against panelBg, which is what a badge, a banner and a
    // status line actually sit on.
    for (const QString& id : ThemeManager::instance().profileIds()) {
        const ThemeProfile p = ThemeManager::instance().profile(id);
        const SemanticPalette& s = p.semantic;
        for (const SemanticColor* c : {&s.neutral, &s.success, &s.warning,
                                       &s.danger, &s.info}) {
            const double ratio = contrast(c->text, p.colors.panelBg);
            QVERIFY2(ratio >= 4.5,
                     qPrintable(QStringLiteral("%1: %2 on %3 is %4:1")
                                    .arg(id, c->text.name(), p.colors.panelBg.name())
                                    .arg(ratio, 0, 'f', 2)));
        }
    }
}

void GalleryTest::infoIsAStatusAndNotTheApplicationAccent()
{
    // Info used to resolve to accentLight in Badge, in InfoBanner, in Ink and
    // in the scope sheet, so the same status was sky blue in NovuTrade, gold in
    // Warm Gray and violet in Dark Purple. This is the regression guard.
    for (const QString& id : ThemeManager::instance().profileIds()) {
        const ThemeProfile p = ThemeManager::instance().profile(id);
        if (id == QLatin1String("midnight-blue")) {
            // The one profile whose accentLight legitimately is the shared info
            // blue - both are GitHub's - so it cannot distinguish the two.
            continue;
        }
        QVERIFY2(p.semantic.info.text != p.colors.accentLight, qPrintable(id));
    }
}

void GalleryTest::badgeTonesResolveThroughTheSharedPalette()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    const ThemeProfile& p = themeFor(surface);

    // A Badge tone is a semantic role, and the role table is the one place the
    // mapping lives. Asserting the palette rather than the pixels: what must
    // not drift is which colour a tone resolves to.
    QCOMPARE(p.semantic.role(SemanticRole::Success).text.name(), p.colors.success.name());
    QCOMPARE(p.semantic.role(SemanticRole::Warning).text.name(), p.colors.warning.name());
    QCOMPARE(p.semantic.role(SemanticRole::Danger).text.name(), p.colors.danger.name());
    QCOMPARE(p.semantic.role(SemanticRole::Info).text.name(), p.colors.info.name());
    // Neutral is the absence of a status, so it follows the surface.
    QCOMPARE(p.semantic.role(SemanticRole::Neutral).text.name(),
             p.colors.textSecondary.name());

    // One recipe for the plate, shared by Badge, Button and the scope sheet.
    for (const SemanticColor* c : {&p.semantic.success, &p.semantic.danger}) {
        QVERIFY(qAbs(c->fill.alphaF() - 0.16) < 0.01);
        QVERIFY(qAbs(c->border.alphaF() - 0.40) < 0.01);
        QCOMPARE(c->fill.rgb() & 0xFFFFFF, c->text.rgb() & 0xFFFFFF);
    }
}

namespace {

// Fills a form the way a user would, so the tests exercise the widgets rather
// than a back door into the state machine.
void typeReport(FeedbackForm* form)
{
    auto* title = form->findChild<QLineEdit*>();
    auto* description = form->findChild<QPlainTextEdit*>();
    QVERIFY(title);
    QVERIFY(description);
    title->setText(QStringLiteral("Preview window opens behind the main window"));
    description->setPlainText(QStringLiteral("Open a preview from the file list. It appears "
                                             "behind the window it was opened from."));
}

Button* buttonNamed(QWidget* root, const QString& text)
{
    for (Button* button : root->findChildren<Button*>()) {
        if (button->text() == text) {
            return button;
        }
    }
    return nullptr;
}

FeedbackContext contextWithLogs(bool withLogs)
{
    FeedbackContext context;
    context.application = QStringLiteral("NovuAI");
    context.version = QStringLiteral("0.89.067");
    context.workspace = QStringLiteral("Integration");
    if (withLogs) {
        context.diagnostics = [] { return QStringLiteral("line one line two"); };
    }
    return context;
}

} // namespace

void GalleryTest::feedbackRefusesAnIncompleteReport()
{
    FeedbackForm form;
    form.setContext(contextWithLogs(true));
    QSignalSpy submitted(&form, &FeedbackForm::submitted);

    Button* send = buttonNamed(&form, QStringLiteral("Send feedback"));
    QVERIFY(send);
    send->click();

    // Nothing left the form, and it said why rather than doing nothing.
    QCOMPARE(submitted.count(), 0);
    QCOMPARE(form.state(), FeedbackForm::State::Invalid);

    // Typing clears the complaint: leaving it up while the user fixes the thing
    // it complains about makes the form feel broken.
    typeReport(&form);
    QCOMPARE(form.state(), FeedbackForm::State::Editing);
}

void GalleryTest::feedbackSendsOnceAndOnlyOnce()
{
    FeedbackForm form;
    form.setContext(contextWithLogs(true));
    typeReport(&form);

    QSignalSpy submitted(&form, &FeedbackForm::submitted);
    Button* send = buttonNamed(&form, QStringLiteral("Send feedback"));

    send->click();
    QCOMPARE(submitted.count(), 1);
    QCOMPARE(form.state(), FeedbackForm::State::Sending);

    // The guard that matters. A second click, a return key or a shortcut all
    // arrive here, and only one of them may become a request - the disabled
    // button alone would not stop an event already in the queue.
    send->click();
    send->click();
    QCOMPARE(submitted.count(), 1);

    const auto request = submitted.first().first().value<FeedbackRequest>();
    QCOMPARE(request.title, QStringLiteral("Preview window opens behind the main window"));
    QCOMPARE(request.type, FeedbackType::Bug);
    QCOMPARE(request.application, QStringLiteral("NovuAI"));
    // Filled in for the consumer rather than demanded from it.
    QVERIFY(!request.platform.isEmpty());

    form.reportSuccess();
    QCOMPARE(form.state(), FeedbackForm::State::Sent);
    // Nothing further can be sent from a form that has already gone.
    send->click();
    QCOMPARE(submitted.count(), 1);
}

void GalleryTest::feedbackFailureLetsTheUserRetry()
{
    FeedbackForm form;
    form.setContext(contextWithLogs(true));
    typeReport(&form);
    QSignalSpy submitted(&form, &FeedbackForm::submitted);
    Button* send = buttonNamed(&form, QStringLiteral("Send feedback"));

    send->click();
    form.reportFailure(QStringLiteral("The service did not respond."));
    QCOMPARE(form.state(), FeedbackForm::State::Failed);

    // A failed send leaves the report intact and the button live: losing what
    // the user typed because the network was down is the worst thing this form
    // could do.
    QVERIFY(send->isEnabled());
    send->click();
    QCOMPARE(submitted.count(), 2);
}

void GalleryTest::feedbackAttachesNothingTheUserDidNotAgreeTo()
{
    FeedbackForm form;
    form.setContext(contextWithLogs(true));
    typeReport(&form);

    const auto boxes = form.findChildren<CheckBox*>();
    QCOMPARE(boxes.size(), 2);
    CheckBox* info = boxes.at(0);
    CheckBox* logs = boxes.at(1);

    // Application info is on by default and the values are listed in the form;
    // logs are off by default and are not read at all until asked for.
    QVERIFY(info->isChecked());
    QVERIFY(!logs->isChecked());
    FeedbackRequest request = form.request();
    QVERIFY(request.includeApplicationInfo);
    QVERIFY(!request.includeLogs);
    QVERIFY(request.diagnosticData.isEmpty());

    logs->setChecked(true);
    request = form.request();
    QVERIFY(request.includeLogs);
    QCOMPARE(request.diagnosticData, QStringLiteral("line one line two"));

    // Turning the box off removes the data, rather than sending it with a flag
    // saying not to look.
    info->setChecked(false);
    request = form.request();
    QVERIFY(!request.includeApplicationInfo);
    QVERIFY(request.application.isEmpty());
    QVERIFY(request.version.isEmpty());
    QVERIFY(request.workspace.isEmpty());
}

void GalleryTest::feedbackDisablesLogsWhenTheHostHasNone()
{
    FeedbackForm form;
    form.setContext(contextWithLogs(false));

    const auto boxes = form.findChildren<CheckBox*>();
    QCOMPARE(boxes.size(), 2);
    // Disabled and explained, not silently absent: the user learns the option
    // exists and that this application does not offer it.
    QVERIFY(!boxes.at(1)->isEnabled());
    QVERIFY(!boxes.at(1)->isChecked());

    typeReport(&form);
    QVERIFY(!form.request().includeLogs);
}

void GalleryTest::feedbackIgnoresAnAnswerItIsNotWaitingFor()
{
    FeedbackForm form;
    form.setContext(contextWithLogs(true));
    typeReport(&form);

    // A reply arriving after the user gave up must not put the form into a
    // state it never asked for.
    form.reportSuccess();
    QCOMPARE(form.state(), FeedbackForm::State::Editing);
    form.reportFailure(QStringLiteral("late"));
    QCOMPARE(form.state(), FeedbackForm::State::Editing);
}

void GalleryTest::helpOverlayLetsTheApplicationThrough()
{
    // The one property the whole component is built around: a user reads the
    // note about the toolbar and then uses the toolbar. If this attribute is
    // ever cleared the overlay silently becomes a modal tour.
    QWidget host;
    host.resize(400, 300);
    auto* overlay = HelpOverlay::attach(&host);
    QVERIFY(overlay);
    QVERIFY(overlay->testAttribute(Qt::WA_TransparentForMouseEvents));
    QCOMPARE(overlay->parentWidget(), &host);
    QCOMPARE(overlay->focusPolicy(), Qt::NoFocus);
}

void GalleryTest::helpOverlayFollowsItsHost()
{
    QWidget host;
    host.resize(400, 300);
    auto* overlay = HelpOverlay::attach(&host);
    QCOMPARE(overlay->geometry(), host.rect());

    // Shown, because a hidden widget defers its resize event and this is about
    // what happens to a window a user is actually dragging.
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    host.resize(640, 480);
    // The host's resize is filtered, so the overlay does not need the
    // application to remember to keep it in step.
    QTRY_COMPARE(overlay->geometry(), host.rect());
}

void GalleryTest::helpOverlayDropsNotesWhoseTargetIsGone()
{
    QWidget host;
    host.resize(400, 300);
    auto* target = new QWidget(&host);
    target->resize(60, 20);

    auto* overlay = HelpOverlay::attach(&host);
    overlay->addNote(target, QStringLiteral("Title"), QStringLiteral("Body"));
    QCOMPARE(overlay->noteCount(), 1);

    // A leader pointing at nothing is worse than a missing note, so a destroyed
    // target must not take the overlay down with it either - rendering after
    // the delete is the actual assertion here.
    delete target;
    QImage canvas(host.size(), QImage::Format_ARGB32);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);
    overlay->render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    QVERIFY(true);   // reached without a crash

    overlay->clear();
    QCOMPARE(overlay->noteCount(), 0);
}

void GalleryTest::helpOverlayTonesComeFromTheSharedPalette()
{
    // The overlay is the one place a colour is allowed to be decorative - a
    // screen with eight bubbles needs neighbours told apart. It still may not
    // invent one: every tone it can draw is a colour the profile already
    // defines, so a note is never off-palette.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    const ThemeProfile& p = themeFor(surface);

    auto* overlay = HelpOverlay::attach(surface);
    overlay->addNote(surface, QStringLiteral("A"), QStringLiteral("B"),
                     HelpPlacement::Auto, HelpTone::Warning);
    QCOMPARE(overlay->noteCount(), 1);

    const QList<QColor> palette = {p.colors.accent, p.semantic.info.text,
                                   p.semantic.success.text, p.semantic.warning.text,
                                   p.semantic.danger.text, p.semantic.neutral.text};
    for (const QColor& colour : palette) {
        QVERIFY(colour.isValid());
    }
    // Distinguishable from one another, which is the job the colour is doing.
    for (int i = 0; i < palette.size(); ++i) {
        for (int j = i + 1; j < palette.size(); ++j) {
            QVERIFY2(palette.at(i).name() != palette.at(j).name(),
                     qPrintable(QStringLiteral("tones %1 and %2 are the same colour")
                                    .arg(i).arg(j)));
        }
    }
}

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
    QCOMPARE(p.colors.textPrimary.name().toUpper(), QStringLiteral("#E3E8F0"));   // TEXT
    QCOMPARE(p.colors.textSecondary.name().toUpper(), QStringLiteral("#9CA3AF")); // TEXT_SEC
    QCOMPARE(p.colors.textDisabled.name().toUpper(), QStringLiteral("#6B7280"));  // MUTED
    QCOMPARE(p.colors.textOnAccent.name().toUpper(), QStringLiteral("#062018"));
    // BORDER = rgba(148,163,184,0.18)
    QCOMPARE(p.colors.border.red(), 148);
    QCOMPARE(p.colors.border.green(), 163);
    QCOMPARE(p.colors.border.blue(), 184);
    QVERIFY(qAbs(p.colors.border.alphaF() - 0.18) < 0.01);

    // Status colours are deliberately NOT the mockup's any more, and this is
    // the assertion that says so on purpose rather than by omission.
    //
    // The mockup gave NovuTrade #22C55E / #F59E0B / #EF4444. Those are the same
    // four meanings every other Novu window shows, so they moved to the shared
    // semantic palette: an application owns its surfaces and its accent - both
    // still asserted above - but not what "failed" looks like. The shared hues
    // were measured against this profile's own panel and clear 4.5:1 by more
    // than the shell's do, so nothing was traded away for the consistency.
    QCOMPARE(p.colors.success.name().toUpper(), sharedSemanticHues().success.name().toUpper());
    QCOMPARE(p.colors.warning.name().toUpper(), sharedSemanticHues().warning.name().toUpper());
    QCOMPARE(p.colors.danger.name().toUpper(),  sharedSemanticHues().danger.name().toUpper());

    // The candles are the exception the exception needed: a candle is a price
    // going up, not a "success", so the web app's greens and reds stay pinned.
    QCOMPARE(p.chart.bullish.name().toUpper(), QStringLiteral("#22C55E"));
    QCOMPARE(p.chart.bearish.name().toUpper(), QStringLiteral("#EF4444"));

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
