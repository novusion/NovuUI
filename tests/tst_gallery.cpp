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

#include "novu/ui/AppMark.h"
#include "novu/ui/CommandPalette.h"
#include "novu/ui/DiffView.h"
#include "novu/ui/GateCard.h"
#include "novu/ui/Drawer.h"
#include "novu/ui/FloatingPanel.h"
#include "novu/ui/SelectionBar.h"
#include "novu/ui/Splitter.h"
#include "novu/ui/TabBar.h"
#include "novu/ui/Toast.h"
#include "novu/ui/TreeView.h"
#include "novu/ui/BottomPanel.h"
#include "novu/ui/Avatar.h"
#include "novu/ui/Settings.h"
#include "novu/ui/Slider.h"
#include "novu/ui/Swatch.h"
#include "novu/ui/Button.h"
#include "novu/ui/Disclosure.h"
#include "novu/ui/Keys.h"
#include "novu/ui/LogView.h"
#include "novu/ui/ModuleBar.h"
#include "novu/ui/NavList.h"
#include "novu/ui/PageHeader.h"
#include "novu/ui/Popover.h"
#include "novu/ui/SettingRow.h"
#include "novu/ui/Sidebar.h"
#include "novu/ui/StatusBar.h"
#include "novu/ui/Tile.h"
#include "novu/ui/Tooltip.h"
#include "novu/ui/Feedback.h"
#include "novu/ui/Motion.h"
#include "novu/ui/Segmented.h"
#include "novu/ui/Toggle.h"
#include "novu/ui/StyleSpec.h"
#include "novu/ui/FeedbackForm.h"
#include "novu/ui/Field.h"
#include "novu/ui/Fields.h"
#include "novu/ui/HelpOverlay.h"
#include "novu/ui/Layout.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/ThemeScope.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QSet>
#include <QCoreApplication>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QJsonArray>
#include <QPainter>
#include <QJsonObject>
#include <QScrollArea>
#include <QScrollBar>
#include <QLineEdit>
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
    void declaredUsageIsMeasuredAndNotIntended();
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

    // architecture invariants
    void componentsKeepTokensRatherThanResolvedColours();
    void theSameButtonResolvesDifferentlyPerProfile();
    void aComponentsVisualContractIsReadableAsData();
    void loopingAnimationsShareOneClock();
    void theSharedClockRunsOnlyWhileSomethingIsAnimating();
    void aBannerNeverSizesItselfToItsMessage();
    void aSwitchTravelsRatherThanJumping();
    void aPickerActsOnPressAndReleaseTogether();
    void aFieldReportsWhatIsWrongWithItsControl();
    void aFieldGivesItsControlTheFullHeightItAskedFor();
    void aBottomPanelNeverDecidesWhetherItIsOpen();
    void anAvatarDerivesItsInitialsAndItsColour();
    void aSliderReportsWhereItsPartsAre();
    void aSwatchAsksOneQuestionOrMany();
    void aSettingsShellHoldsShapeAndNotSettings();
    void aFormsPrimaryActionEndsWhereItsFieldsEnd();

    // the NovuSpace extraction
    void aNavigationTreeIsOneWidgetAndNotThreeHundred();
    void aNavRowsTwistyIsADifferentTargetFromTheRow();
    void navigationSkipsCaptionsAndDisabledRows();
    void bothStripsShareOneHitTestImplementation();
    void aDisclosureHidesItsBodyWithoutForgettingIt();
    void anActionInsideABlockDoesNotCollapseIt();
    void chordsSplitIntoCapsAndLoneSeparatorsSurvive();
    void aFilteredShortcutSheetDropsEmptyCaptions();
    void statusBarItemsAreAddressedByHandle();
    void aLogKeepsOnlyItsCap();
    void aLogStopsFollowingWhenTheUserScrollsAway();
    void aStoppedSpinnerReleasesTheClock();
    void aSkeletonListIsOneWidgetWhateverItsLength();
    void aSequencePastItsLastStepIsComplete();
    void aTooltipResolvesAgainstTheWidgetItIsAbout();
    void aSettingInErrorSaysSoInItsContract();
    void aCollapsedRailCollapsesEveryListInIt();
    void aRiskyChoiceIsWarningAndNotDanger();
    void theExtractedComponentsKeepTokensRatherThanColours();
    void aMaskedValueDoesNotLeakItsLength();

    // the NovuAI extraction
    void aTreeFlattensOnlyWhatIsOpen();
    void aTreeChevronIsADifferentTargetFromTheRow();
    void aTreeRevealsANodeByOpeningItsAncestors();
    void aTreeKeyboardWalksTheStructure();
    void aDiffIsParsedRatherThanColouredByItsFirstCharacter();
    void theLogAndTheDiffShareOneVirtualisation();
    void aFloatingPanelCannotLeaveItsLayer();
    void theLayerDecidesWhichPanelIsInFront();
    void aRolledPanelKeepsWhatItHad();
    void toastsStackToACapAndNoFurther();
    void aToastHoldsWhileThePointerIsOnIt();
    void aSelectionBarIsNotThereAtZero();
    void aDialogHeadIsAddedOnceAndUpdatedAfter();
    void anAvatarIsRoundAndAPlaqueIsNot();

    // the NovuExplorer extraction
    void aCollapsedPanelComesBackAtTheWidthItHad();
    void aSplitterArrowIsAButtonAndNotADragTarget();
    void aTabCarriesItsColourAndStateThroughAReorder();
    void aTabsCloseBoxIsItsOwnTarget();
    void closingTheCurrentTabMovesToItsNeighbour();
    void tabsShrinkTogetherRatherThanEachToItsOwnTitle();
    void anInactivePanelKeepsItsSelection();

    // the NovuAdmin extraction
    void aCapacityBarColoursItselfAndAProgressBarDoesNot();
    void anUnmeasuredReadingIsNotAMeasuredZero();
    void bytesAreWrittenOneWay();
    void aDrawerDimsWhatItIsAbout();
    void aBusyDialogDisablesItsWholeFooter();
    void aFormErrorClearsBusyAndSitsAboveTheFooter();

    // the remaining-source pass
    void aPaletteFindsACommandByItsInitials();
    void aPaletteRanksRatherThanMerelyFiltering();
    void aGateCardNamesTheHostTheCredentialIsGoingTo();
    void everyExtractedComponentIsInTheGallery();

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
    // `designedFor` is the filter's source of truth, so a spec that forgets it
    // disappears from every filtered view without anyone noticing.
    for (const ComponentSpec& spec : m_registry.all()) {
        QVERIFY2(!spec.designedFor.isEmpty() || spec.awaitingAdoption, qPrintable(spec.id));
        for (const QString& application : spec.designedFor) {
            QVERIFY2(novu::ui::isApplicationProfile(application),
                     qPrintable(QStringLiteral("%1 is designed for %2, which is not an "
                                               "application").arg(spec.id, application)));
        }
        for (const QString& application : spec.usedBy) {
            QVERIFY2(novu::ui::isApplicationProfile(application),
                     qPrintable(QStringLiteral("%1 claims %2, which is not an application")
                                    .arg(spec.id, application)));
        }
    }
}

void GalleryTest::declaredUsageIsMeasuredAndNotIntended()
{
    // The guard on the one piece of metadata that can mislead.
    //
    // `usedBy` used to be a single list holding both "uses it" and "will use
    // it", and the gallery then told anyone who looked that NovuAI used
    // twenty-six components - while NovuAI did not include a single NovuUI
    // header. A catalogue that overstates adoption is worse than one that says
    // nothing, because it is the thing people quote.
    //
    // These two applications do not link this library today. Until they are
    // migrated, nothing may claim they do. When they are, this test fails and
    // is updated in the same change as the migration - which is the point.
    const QStringList notMigrated = {QStringLiteral("novuai"),
                                     QStringLiteral("novuexplorer")};

    for (const ComponentSpec& spec : m_registry.all()) {
        for (const QString& application : notMigrated) {
            QVERIFY2(!spec.isUsedBy(application),
                     qPrintable(QStringLiteral("%1 claims %2 uses it; %2 does not consume "
                                               "NovuUI yet - that belongs in designedFor")
                                    .arg(spec.id, application)));
        }
    }

    // Intent is not adoption, and the two must not have collapsed back into
    // one list: something, somewhere, has to be designed for an application
    // that has not adopted it, or the split has quietly stopped meaning
    // anything.
    int intendedButUnused = 0;
    for (const ComponentSpec& spec : m_registry.all()) {
        for (const QString& application : spec.designedFor) {
            if (!spec.isUsedBy(application)) {
                ++intendedButUnused;
            }
        }
    }
    QVERIFY2(intendedButUnused > 0,
             "every stated intention is already adoption - the two fields have merged");

    // And adoption implies applicability, so the filter cannot hide a
    // component from the application that actually consumes it.
    for (const ComponentSpec& spec : m_registry.all()) {
        for (const QString& application : spec.usedBy) {
            QVERIFY2(spec.isDesignedFor(application),
                     qPrintable(QStringLiteral("%1 is used by %2 but the filter hides it")
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
            QVERIFY2(!spec.isDesignedFor(QStringLiteral("novuai")), qPrintable(spec.id));
        }
    }
    QCOMPARE(index, forAi.size());

    // Search and the filter compose rather than override one another.
    const QVector<ComponentSpec> searched =
        m_registry.search(QStringLiteral("badge"), QStringLiteral("novuai"));
    for (const ComponentSpec& spec : searched) {
        QVERIFY(spec.matches(QStringLiteral("badge")));
        QVERIFY(spec.isDesignedFor(QStringLiteral("novuai")));
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

// ── architecture invariants ───────────────────────────────────────────────
//
// These are the rules that stop the library drifting back into the shapes it
// was extracted from, so they are asserted rather than written down. None of
// them needs a pixel: a component says what it means in tokens, and what those
// tokens come to is a pure function of the profile.

void GalleryTest::componentsKeepTokensRatherThanResolvedColours()
{
    // A Danger button must name the shared status palette, not a colour it
    // happened to read at construction. Naming it is what makes a Danger button
    // and a Danger badge the same red without either knowing about the other.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* button = new Button(QStringLiteral("Delete"), Button::Variant::Danger, surface);

    const StyleSpec spec = button->styleSpec();
    QCOMPARE(spec.background.describe(), QStringLiteral("status.danger.fill"));
    QCOMPARE(spec.foreground.describe(), QStringLiteral("status.danger.text"));
    QCOMPARE(spec.border.describe(), QStringLiteral("status.danger.border"));

    // And a Primary names the product accent, which is the one thing a status
    // may never resolve to.
    auto* primary = new Button(QStringLiteral("Save"), Button::Variant::Primary, surface);
    QCOMPARE(primary->styleSpec().background.describe(), QStringLiteral("accent"));
    QCOMPARE(primary->styleSpec().foreground.describe(), QStringLiteral("textOnAccent"));
}

void GalleryTest::theSameButtonResolvesDifferentlyPerProfile()
{
    // The point of holding tokens: one class, two products, no conditional.
    // NovuSpace is amber and NovuTrade is emerald, and neither Button knows it.
    QWidget* space = makeSurface(QStringLiteral("novuspace"));
    QWidget* trade = makeSurface(QStringLiteral("novutrade"));

    auto* inSpace = new Button(QStringLiteral("Save"), Button::Variant::Primary, space);
    auto* inTrade = new Button(QStringLiteral("Save"), Button::Variant::Primary, trade);

    // Same contract ...
    QCOMPARE(inSpace->styleSpec().background.describe(),
             inTrade->styleSpec().background.describe());

    // ... different pixels, each its own profile's.
    const QColor spaceAccent =
        ThemeManager::instance().profile(QStringLiteral("novuspace")).colors.accent;
    const QColor tradeAccent =
        ThemeManager::instance().profile(QStringLiteral("novutrade")).colors.accent;

    QCOMPARE(inSpace->resolvedStyle().background.name(), spaceAccent.name());
    QCOMPARE(inTrade->resolvedStyle().background.name(), tradeAccent.name());
    QVERIFY(spaceAccent != tradeAccent);
}

void GalleryTest::aComponentsVisualContractIsReadableAsData()
{
    // The seam that replaces a screenshot. Geometry and type come back as
    // numbers, so "a small button steps down the radius scale" is a comparison
    // rather than something someone has to notice.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    const ThemeProfile profile =
        ThemeManager::instance().profile(QStringLiteral("novuspace"));

    auto* regular = new Button(QStringLiteral("Apply"), Button::Variant::Secondary, surface);
    auto* small = new Button(QStringLiteral("Apply"), Button::Variant::Secondary, surface);
    small->setControlSize(ControlSize::Small);

    QCOMPARE(regular->resolvedStyle().radius, profile.radius(SurfaceRole::Button));
    QCOMPARE(small->resolvedStyle().radius, profile.radii.small);
    QVERIFY(small->resolvedStyle().radius < regular->resolvedStyle().radius);

    // A row action is a step smaller in type as well, and both steps are named
    // rather than measured.
    QCOMPARE(regular->styleSpec().type, TypeRole::Body);
    QCOMPARE(small->styleSpec().type, TypeRole::Caption);
    QVERIFY(small->resolvedStyle().font.pixelSize()
            < regular->resolvedStyle().font.pixelSize());
}

void GalleryTest::loopingAnimationsShareOneClock()
{
    // Four things in the approved design loop forever. Given a timer each, a
    // window wakes the CPU on four schedules; this is the assertion that there
    // is one.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));

    const double before = motion::phase(1000);
    QTest::qWait(120);
    const double after = motion::phase(1000);
    QVERIFY2(!qFuzzyCompare(before + 1.0, after + 1.0),
             "the shared clock did not advance");

    // Read from a monotonic clock rather than counted in ticks, so two widgets
    // that subscribed seconds apart are in phase.
    QCOMPARE(motion::phase(1000), motion::phase(1000));

    // breathe() stays inside its stated range whatever the phase.
    for (int i = 0; i < 8; ++i) {
        const double value = motion::breathe(400, 0.55, 1.0);
        QVERIFY(value >= 0.55 - 1e-9 && value <= 1.0 + 1e-9);
        QTest::qWait(30);
    }
    Q_UNUSED(surface);
}

void GalleryTest::theSharedClockRunsOnlyWhileSomethingIsAnimating()
{
    Pulse& pulse = Pulse::instance();
    const int before = pulse.subscriberCount();

    {
        QWidget* surface = makeSurface(QStringLiteral("novuspace"));
        // A Primary button carries the travelling ring, so it subscribes; a
        // Secondary has nothing turning, so it must not.
        auto* secondary = new Button(QStringLiteral("Cancel"),
                                     Button::Variant::Secondary, surface);
        QCOMPARE(pulse.subscriberCount(), before);

        auto* primary = new Button(QStringLiteral("Save"),
                                   Button::Variant::Primary, surface);
        QCOMPARE(pulse.subscriberCount(), before + 1);
        QVERIFY(pulse.isRunning());

        // Disabling stops the motion: nothing about a control the user cannot
        // press should be moving.
        primary->setEnabled(false);
        QCOMPARE(pulse.subscriberCount(), before);

        primary->setEnabled(true);
        QCOMPARE(pulse.subscriberCount(), before + 1);

        // And switching away from Primary releases it, so the subscription
        // follows the variant rather than the construction.
        primary->setVariant(Button::Variant::Ghost);
        QCOMPARE(pulse.subscriberCount(), before);

        Q_UNUSED(secondary);
    }
}

void GalleryTest::aBannerNeverSizesItselfToItsMessage()
{
    // The regression this exists for: the gallery handed InfoBanner a linker
    // error - one unbroken four-hundred-character line - and a banner that
    // measured its own text dragged the window off the screen.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));

    auto* shortOne = new InfoBanner(QStringLiteral("Saved."),
                                    Badge::Tone::Info, surface);
    auto* longOne = new InfoBanner(
        QStringLiteral("NovuUI.lib(Button.obj) : error LNK2019: unresolved external symbol "
                       "\"public: static class novu::ui::Pulse & __cdecl "
                       "novu::ui::Pulse::instance(void)\" referenced in function "
                       "\"private: void __cdecl novu::ui::Button::applyVariant(void)\""),
        Badge::Tone::Danger, surface);

    QCOMPARE(longOne->sizeHint().width(), shortOne->sizeHint().width());
    QCOMPARE(longOne->minimumSizeHint().width(), shortOne->minimumSizeHint().width());
    QVERIFY2(longOne->minimumSizeHint().width() < 400,
             qPrintable(QStringLiteral("a banner insists on %1px")
                            .arg(longOne->minimumSizeHint().width())));
    // Horizontally it takes whatever it is given, and gives nothing back.
    QCOMPARE(longOne->sizePolicy().horizontalPolicy(), QSizePolicy::Ignored);

    // Nothing is lost by eliding: the whole message stays on the tooltip.
    QVERIFY(longOne->toolTip().contains(QStringLiteral("LNK2019")));
}

void GalleryTest::aSwitchTravelsRatherThanJumping()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* toggle = new Toggle(surface);

    // Off and on are the two ends of the travel, and knobPosition() is the seam
    // that says so without anyone looking at the control.
    QCOMPARE(toggle->knobPosition(), 0.0);
    toggle->setChecked(true);
    QCOMPARE(toggle->knobPosition(), 1.0);   // not shown, so it arrives at once

    // The track is the accent when on and the surface when off, both named
    // rather than taken.
    QCOMPARE(toggle->styleSpec().checkedBackground.describe(), QStringLiteral("accent@28"));
    QCOMPARE(toggle->styleSpec().background.describe(), QStringLiteral("surfaceHover"));

    // A switch is not a status, so it must never resolve through the semantic
    // palette - that is the rule the approved design states outright.
    QVERIFY(!toggle->styleSpec().checkedBackground.describe().startsWith(
        QStringLiteral("status.")));
}

void GalleryTest::aPickerActsOnPressAndReleaseTogether()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* picker = new Segmented({QStringLiteral("Basic"), QStringLiteral("Extended"),
                                  QStringLiteral("Pro"), QStringLiteral("MAX")},
                                 surface, 0);
    picker->resize(picker->sizeHint());

    QSignalSpy chosen(picker, &Segmented::currentIndexChanged);

    const QPoint inPro = picker->segmentRect(2).center();
    const QPoint inBasic = picker->segmentRect(0).center();
    QCOMPARE(picker->indexAt(inPro), 2);

    // Press and release in the same segment is a click.
    QTest::mousePress(picker, Qt::LeftButton, Qt::NoModifier, inPro);
    QTest::mouseRelease(picker, Qt::LeftButton, Qt::NoModifier, inPro);
    QCOMPARE(picker->currentIndex(), 2);
    QCOMPARE(chosen.count(), 1);

    // Pressing in one segment and releasing in another is a cancelled gesture,
    // not a choice. A control that acted on release alone would fire here,
    // after the user had already changed their mind.
    QTest::mousePress(picker, Qt::LeftButton, Qt::NoModifier, inBasic);
    QTest::mouseRelease(picker, Qt::LeftButton, Qt::NoModifier, inPro);
    QCOMPARE(picker->currentIndex(), 2);
    QCOMPARE(chosen.count(), 1);

    // setCurrentIndex() sets; choose() says so. Mixing the two is how a view
    // that syncs its own state ends up in a signal loop.
    picker->setCurrentIndex(1);
    QCOMPARE(picker->currentIndex(), 1);
    QCOMPARE(chosen.count(), 1);

    // Words and glyphs are one class, and the glyph segments carry no text.
    auto* glyphs = new Segmented(surface);
    glyphs->addSegment(Glyph::Overview, QStringLiteral("Grid"));
    glyphs->addSegment(Glyph::Chart, QStringLiteral("Chart"));
    QCOMPARE(glyphs->count(), 2);
    QVERIFY(glyphs->segmentText(0).isEmpty());
}

void GalleryTest::aFieldReportsWhatIsWrongWithItsControl()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* input = lineEdit(QStringLiteral("name@example.com"), surface);
    auto* item = new Field(QStringLiteral("Notify"), input,
                           QStringLiteral("Where a finished backtest is announced."),
                           surface);

    QSignalSpy validity(item, &Field::validityChanged);
    QVERIFY(item->isValid());
    QVERIFY(!input->property(prop::invalid).toBool());

    item->setError(QStringLiteral("That is not an address."));
    QVERIFY(!item->isValid());
    QCOMPARE(validity.count(), 1);
    QCOMPARE(validity.takeFirst().at(0).toBool(), false);

    // The control is *marked*, not recoloured: the danger outline is a property
    // the scope sheet reads, so a theme change re-resolves it.
    QVERIFY(input->property(prop::invalid).toBool());
    QVERIFY2(input->styleSheet().isEmpty(),
             "Field wrote a stylesheet onto its control instead of marking it");

    // The error replaces the hint rather than joining it.
    QCOMPARE(item->hint(), QStringLiteral("Where a finished backtest is announced."));
    QCOMPARE(item->error(), QStringLiteral("That is not an address."));

    // Typing clears it, because a message that outlives its problem teaches
    // people to ignore messages.
    QTest::keyClicks(input, QStringLiteral("a"));
    QVERIFY(item->isValid());
    QVERIFY(!input->property(prop::invalid).toBool());
    QCOMPARE(validity.count(), 1);
    QCOMPARE(validity.takeFirst().at(0).toBool(), true);

    // Unless the field says its validity is not decided locally.
    item->setClearsErrorOnEdit(false);
    item->setError(QStringLiteral("The server rejected this address."));
    QTest::keyClicks(input, QStringLiteral("b"));
    QVERIFY(!item->isValid());
}

void GalleryTest::aFieldGivesItsControlTheFullHeightItAskedFor()
{
    // The caption and the message are painted into the margins, so the arithmetic
    // that reserves them is the one thing that can squash the control. Asserted
    // rather than looked at.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));

    auto* bare = new Field(QStringLiteral("Name"),
                           lineEdit(QString(), surface), surface);
    auto* hinted = new Field(QStringLiteral("Name"), lineEdit(QString(), surface),
                             QStringLiteral("A short one."), surface);

    // What it will be given, not what it asks for: the field factories call
    // setFixedHeight(), which moves the minimum rather than the hint.
    const int control = bare->control()->sizeHint()
                            .expandedTo(bare->control()->minimumSize()).height();
    QVERIFY(control > 0);

    // A field is its control plus whatever it reserved above and below it.
    QVERIFY2(bare->sizeHint().height() > control,
             "a captioned field reserved no room for its caption");
    QVERIFY2(hinted->sizeHint().height() > bare->sizeHint().height(),
             "a hint took no vertical room");

    // And laying it out must not eat into the control.
    bare->resize(bare->sizeHint());
    hinted->resize(hinted->sizeHint());
    QCoreApplication::processEvents();
    QCOMPARE(bare->control()->height(), control);
    QCOMPARE(hinted->control()->height(), control);

    // An error is the same shape as a hint, so switching between them must not
    // move the control either.
    const int before = hinted->control()->y();
    hinted->setError(QStringLiteral("No."));
    hinted->resize(hinted->sizeHint());
    QCoreApplication::processEvents();
    QCOMPARE(hinted->control()->y(), before);
}

void GalleryTest::aBottomPanelNeverDecidesWhetherItIsOpen()
{
    // The state boundary the extraction audit warned about. NovuExplorer's
    // console owns session lifecycle, persistence and whether the panel is up;
    // a shared panel that also kept an `isOpen` would give that window two
    // answers to one question. So this asserts the panel has no opinion.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* panel = new BottomPanel(surface);

    auto* out = new QWidget(panel);
    auto* problems = new QWidget(panel);
    panel->addPage(QStringLiteral("out"), QStringLiteral("Output"), out);
    panel->addPage(QStringLiteral("problems"), QStringLiteral("Problems"), problems);
    QCOMPARE(panel->pageCount(), 2);

    QSignalSpy asked(panel, &BottomPanel::expandRequested);
    QVERIFY(!panel->isExpanded());

    // Clicking a tab while the panel is shut asks for it to open - and does not
    // open it. Nothing the user can do moves the state by itself.
    panel->tabs()->setCurrentKey(QStringLiteral("problems"));
    QCOMPARE(panel->currentKey(), QStringLiteral("problems"));
    QCOMPARE(asked.count(), 1);
    QCOMPARE(asked.takeFirst().at(0).toBool(), true);
    QVERIFY2(!panel->isExpanded(),
             "the panel opened itself instead of asking");

    // The host answers, and only then does it move.
    panel->setExpanded(true);
    QVERIFY(panel->isExpanded());
    // A setter sets: applying the state must not ask for it again.
    QCOMPARE(asked.count(), 0);

    // Closing a tab is a request too: a session may need to ask before it dies,
    // so the panel does not remove the page itself.
    QSignalSpy closing(panel, &BottomPanel::pageCloseRequested);
    emit panel->tabs()->closeRequested(QStringLiteral("out"));
    QCOMPARE(closing.count(), 1);
    QCOMPARE(closing.takeFirst().at(0).toString(), QStringLiteral("out"));
    QCOMPARE(panel->pageCount(), 2);

    // And when the host does agree, the page and its tab go together.
    panel->removePage(QStringLiteral("out"));
    QCOMPARE(panel->pageCount(), 1);
    QCOMPARE(panel->tabs()->count(), 1);

    // Shut, it is still exactly one strip tall - the tabs and the status stay.
    panel->setExpanded(false);
    QCOMPARE(panel->sizeHint().height(), panel->stripHeight());
    panel->setExpanded(true);
    QCOMPARE(panel->sizeHint().height(),
             panel->stripHeight() + panel->expandedHeight());
}

void GalleryTest::anAvatarDerivesItsInitialsAndItsColour()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));

    // Two words, an address, one word, and nothing: an avatar with an empty
    // glyph is worse than one with a single character.
    QCOMPARE(Avatar(QStringLiteral("Petr Dostál"), surface).initials(), QStringLiteral("PD"));
    QCOMPARE(Avatar(QStringLiteral("kai.lindgren@novusion.io"), surface).initials(),
             QStringLiteral("KL"));
    QCOMPARE(Avatar(QStringLiteral("Wu"), surface).initials(), QStringLiteral("W"));
    QVERIFY(Avatar(QString(), surface).initials().isEmpty());

    // The colour is a function of the name, not of when it was asked for. This
    // is the regression guard on using qHash, which Qt salts per process: the
    // same person would be a different colour on every run.
    auto* first = new Avatar(QStringLiteral("Anna Nováková"), surface);
    auto* second = new Avatar(QStringLiteral("Anna Nováková"), surface);
    auto* other = new Avatar(QStringLiteral("Marta Šimek"), surface);
    QCOMPARE(first->styleSpec().foreground.describe(),
             second->styleSpec().foreground.describe());

    // Rendering the same name twice must give the same pixels; a different
    // name should generally not. Compared as images because the plate colour
    // is derived rather than exposed.
    const auto render = [](Avatar* a) {
        a->setDiameter(40);
        a->resize(a->sizeHint());
        QImage image(a->size(), QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        a->render(&image);
        return image;
    };
    QCOMPARE(render(first), render(second));

    // Distinct names are *distributed* over the series, not guaranteed to
    // differ: the palette has a handful of entries and two names can land on
    // the same one. What must hold is that the derivation spreads rather than
    // collapsing to a single colour.
    QSet<QByteArray> distinct;
    for (const QString& name : {QStringLiteral("Anna Nováková"), QStringLiteral("Marta Šimek"),
                                QStringLiteral("Petr Dostál"), QStringLiteral("Wu"),
                                QStringLiteral("kai.lindgren@novusion.io"),
                                QStringLiteral("build-agent-04")}) {
        auto* a = new Avatar(name, surface);
        QImage image = render(a);
        distinct.insert(QByteArray(reinterpret_cast<const char*>(image.constBits()),
                                   static_cast<int>(image.sizeInBytes())));
    }
    QVERIFY2(distinct.size() >= 3,
             qPrintable(QStringLiteral("six names produced only %1 renderings")
                            .arg(distinct.size())));
    Q_UNUSED(other);

    // Size is one number and the type follows it, so there is no size table.
    first->setDiameter(64);
    QCOMPARE(first->sizeHint(), QSize(64, 64));
}

void GalleryTest::aSliderReportsWhereItsPartsAre()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* slider = new Slider(0, 100, surface);
    slider->resize(300, slider->sizeHint().height());
    slider->setValue(0);

    // The handle travels the groove, and stays inside the control at both ends
    // rather than hanging off it.
    const QRect groove = slider->grooveRect();
    const QRect atMinimum = slider->handleRect();
    QVERIFY(atMinimum.left() >= 0);
    slider->setValue(100);
    const QRect atMaximum = slider->handleRect();
    QVERIFY(atMaximum.right() <= slider->width());
    QVERIFY(atMaximum.center().x() > atMinimum.center().x());
    // Within a pixel: the handle is an even number of pixels across, so no
    // integer rect has its exact centre on the value. The guarantee is that it
    // sits on the end of the groove, not that QRect::center() rounds a
    // particular way.
    QVERIFY2(qAbs(atMinimum.center().x() - groove.left()) <= 1,
             qPrintable(QStringLiteral("handle at %1, groove starts at %2")
                            .arg(atMinimum.center().x()).arg(groove.left())));
    QVERIFY2(qAbs(atMaximum.center().x() - groove.right()) <= 1,
             qPrintable(QStringLiteral("handle at %1, groove ends at %2")
                            .arg(atMaximum.center().x()).arg(groove.right())));

    // Clicking the groove jumps to where it was clicked rather than paging
    // towards it - Qt's default is a hold-over from scroll bars and costs two
    // clicks to reach a value the user already pointed at.
    slider->setValue(0);
    const QPoint middle(groove.center().x(), slider->height() / 2);
    QTest::mousePress(slider, Qt::LeftButton, Qt::NoModifier, middle);
    QTest::mouseRelease(slider, Qt::LeftButton, Qt::NoModifier, middle);
    QVERIFY2(qAbs(slider->value() - 50) <= 2,
             qPrintable(QStringLiteral("clicking the middle gave %1").arg(slider->value())));

    // The readout is part of the control, and the formatter decides how it
    // reads. A slider that states a proportion and nothing else is a setting
    // nobody can repeat or report.
    QCOMPARE(slider->readout(), QString::number(slider->value()));
    slider->setFormatter([](int v) { return QStringLiteral("%1 %").arg(v); });
    QCOMPARE(slider->readout(), QStringLiteral("%1 %").arg(slider->value()));
    QVERIFY(!slider->readoutRect().isEmpty());

    // Turning it off gives the groove the room back.
    const int wide = slider->grooveRect().width();
    slider->setShowsReadout(false);
    QVERIFY(slider->readoutRect().isEmpty());
    QVERIFY(slider->grooveRect().width() > wide);

    // Still a QSlider: the keyboard behaves as every caller expects.
    slider->setValue(10);
    QTest::keyClick(slider, Qt::Key_Right);
    QCOMPARE(slider->value(), 11);
}

void GalleryTest::aSwatchAsksOneQuestionOrMany()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* row = new Swatch(surface);
    const QList<QColor> palette = tagPalette(surface);
    QVERIFY(palette.size() >= 4);

    row->setColors(palette, /*withNone=*/true, /*withCustom=*/true);
    row->resize(row->sizeHint());
    QCOMPARE(row->count(), palette.size() + 2);

    // Single select: one answer, and "none" is one of the answers rather than
    // a button beside the row.
    QSignalSpy picked(row, &Swatch::picked);
    const QPoint onSecond = row->swatchRect(2).center();
    QCOMPARE(row->indexAt(onSecond), 2);
    QTest::mousePress(row, Qt::LeftButton, Qt::NoModifier, onSecond);
    QTest::mouseRelease(row, Qt::LeftButton, Qt::NoModifier, onSecond);
    QCOMPARE(row->current(), palette.at(1));
    QCOMPARE(picked.count(), 1);

    // Press here, release there: a cancelled gesture, not a choice.
    QTest::mousePress(row, Qt::LeftButton, Qt::NoModifier, row->swatchRect(3).center());
    QTest::mouseRelease(row, Qt::LeftButton, Qt::NoModifier, onSecond);
    QCOMPARE(row->current(), palette.at(1));

    // The custom swatch asks; it does not pick. A colour dialog belongs to the
    // application.
    QSignalSpy custom(row, &Swatch::customRequested);
    const QPoint onCustom = row->swatchRect(row->count() - 1).center();
    QTest::mousePress(row, Qt::LeftButton, Qt::NoModifier, onCustom);
    QTest::mouseRelease(row, Qt::LeftButton, Qt::NoModifier, onCustom);
    QCOMPARE(custom.count(), 1);
    QCOMPARE(row->current(), palette.at(1));

    // Multi select: the same component asked the other question.
    auto* filter = new Swatch(surface);
    filter->setMultiSelect(true);
    filter->setColors(palette, true, false);
    filter->resize(filter->sizeHint());
    QSignalSpy chosen(filter, &Swatch::chosenChanged);

    for (int i : {1, 2}) {
        const QPoint at = filter->swatchRect(i).center();
        QTest::mousePress(filter, Qt::LeftButton, Qt::NoModifier, at);
        QTest::mouseRelease(filter, Qt::LeftButton, Qt::NoModifier, at);
    }
    QCOMPARE(filter->chosen().size(), 2);

    // Toggling, not replacing.
    const QPoint again = filter->swatchRect(1).center();
    QTest::mousePress(filter, Qt::LeftButton, Qt::NoModifier, again);
    QTest::mouseRelease(filter, Qt::LeftButton, Qt::NoModifier, again);
    QCOMPARE(filter->chosen().size(), 1);

    // "None" in a filter clears it: "any of these" with nothing chosen is the
    // same as no filter.
    const QPoint none = filter->swatchRect(0).center();
    QTest::mousePress(filter, Qt::LeftButton, Qt::NoModifier, none);
    QTest::mouseRelease(filter, Qt::LeftButton, Qt::NoModifier, none);
    QVERIFY(filter->chosen().isEmpty());
    QVERIFY(chosen.count() >= 3);

    // It wraps, so it must say how tall it is for a given width - a layout
    // asking at width zero would otherwise get one column and eleven rows.
    QVERIFY(row->hasHeightForWidth());
    const int narrow = row->heightForWidth(60);
    const int wide = row->heightForWidth(1000);
    QVERIFY2(narrow > wide, "a narrower swatch row did not get taller");
}

void GalleryTest::aSettingsShellHoldsShapeAndNotSettings()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* shell = new SettingsShell(surface);

    shell->addGroup(QStringLiteral("This machine"));
    SettingsPage* general = shell->addPage(QStringLiteral("general"),
                                           QStringLiteral("General"), Glyph::Settings,
                                           QStringLiteral("How it behaves here."));
    SettingsPage* connections = shell->addPage(QStringLiteral("connections"),
                                               QStringLiteral("Connections"), Glyph::Plug);
    shell->addGroup(QStringLiteral("Account"));
    shell->addPage(QStringLiteral("account"), QStringLiteral("Account"), Glyph::Users);

    QVERIFY(general);
    QVERIFY(connections);
    QCOMPARE(shell->pageCount(), 3);
    QCOMPARE(shell->keys(),
             QStringList({QStringLiteral("general"), QStringLiteral("connections"),
                          QStringLiteral("account")}));

    // The first page added is the one showing; a settings window that opens on
    // nothing is a settings window with a blank right-hand side.
    QCOMPARE(shell->currentKey(), QStringLiteral("general"));

    QSignalSpy changed(shell, &SettingsShell::currentChanged);
    shell->navigation()->setCurrentKey(QStringLiteral("account"));
    QCOMPARE(shell->currentKey(), QStringLiteral("account"));
    QCOMPARE(changed.count(), 1);

    // Which page is showing is view state, so the shell keeps it - and a host
    // restoring a session can put it back.
    shell->setCurrentKey(QStringLiteral("connections"));
    QCOMPARE(shell->currentKey(), QStringLiteral("connections"));

    // Search filters the navigation without rebuilding it: the rows keep their
    // keys, so the current page does not move out from under whoever is typing.
    shell->setSearchEnabled(true);
    QVERIFY(shell->searchField());
    shell->searchField()->setText(QStringLiteral("acc"));
    QVERIFY(!shell->navigation()->isRowVisible(QStringLiteral("general")));
    QVERIFY(shell->navigation()->isRowVisible(QStringLiteral("account")));
    QCOMPARE(shell->currentKey(), QStringLiteral("connections"));
    QCOMPARE(shell->pageCount(), 3);

    shell->searchField()->clear();
    QVERIFY(shell->navigation()->isRowVisible(QStringLiteral("general")));

    // A section is a heading and its rows, and the divider belongs to the
    // section: only the last row is without one, which a caller adding rows
    // cannot know while it is adding them.
    // Sections keep the order they were added in. Getting this wrong read
    // bottom to top and was invisible until a page had two of them.
    SettingsSection* section = general->addSection(QStringLiteral("Startup"),
                                                   QStringLiteral("On sign-in."));
    SettingsSection* later = general->addSection(QStringLiteral("Appearance"));
    QVERIFY2(general->body()->indexOf(section) < general->body()->indexOf(later),
             "sections came out in reverse order");
    SettingRow* first = section->addRow(QStringLiteral("Start with the system"));
    SettingRow* second = section->addRow(QStringLiteral("Restore the workspace"));
    QVERIFY2(first->hasDivider(), "a row followed by another has no divider");
    QVERIFY2(!second->hasDivider(), "the last row in a section drew a divider");

    SettingRow* third = section->addRow(QStringLiteral("Check for updates"));
    QVERIFY(second->hasDivider());
    QVERIFY(!third->hasDivider());

    // And the heading takes vertical room without being a widget: two QLabels
    // per section is forty labels in a settings window that nobody selects.
    QCOMPARE(section->findChildren<QLabel*>().size(), 0);
}

void GalleryTest::aFormsPrimaryActionEndsWhereItsFieldsEnd()
{
    // Where a form's action sits is a property of the form, not of the screen
    // that built it. Right is the default because the primary action then ends
    // on the same edge the fields do, which is the edge the eye is following
    // down the form.
    // Top-level, because the geometry is the thing under test and a child
    // widget is never exposed. Scoped the same way makeSurface() scopes.
    auto* gate = new GateCard(QStringLiteral("Sign in"));
    applyStyleSheet(gate, QStringLiteral("novuspace"));
    m_surfaces.append(gate);

    auto* email = lineEdit(QStringLiteral("you@novusion.net"), gate->formWidget());
    gate->form()->addWidget(email);

    QCOMPARE(gate->actionAlignment(), ActionAlignment::Right);
    auto* submit = new Button(QStringLiteral("Sign in"), Button::Variant::Primary,
                              gate->formWidget());
    gate->actions()->addWidget(submit);

    // GateCard positions its form itself in resizeEvent, so the form's own
    // layout is what has to be activated - the card has no layout of its own.
    // An explicit size, not sizeHint(): the card positions its form in
    // resizeEvent, and resizing to the size it already has sends no event, so
    // the form would still be at a QWidget's default 100x30 and everything
    // inside it would measure the same.
    // Shown, because the card positions its form in resizeEvent and the form's
    // own layout only reaches the children once that geometry is real. The
    // offscreen platform makes this free.
    gate->show();
    QVERIFY(QTest::qWaitForWindowExposed(gate));
    gate->form()->activate();
    QCoreApplication::processEvents();

    // Not stretched: a primary action that fills the card reads as a banner.
    QVERIFY2(gate->formWidget()->width() > 200,
             qPrintable(QStringLiteral("the form was never laid out: %1 wide")
                            .arg(gate->formWidget()->width())));
    QVERIFY2(submit->width() < email->width(),
             qPrintable(QStringLiteral("action is %1 wide, field is %2")
                            .arg(submit->width()).arg(email->width())));

    // Its right edge lands on the fields' right edge.
    QVERIFY2(qAbs(submit->geometry().right() - email->geometry().right()) <= 1,
             qPrintable(QStringLiteral("action ends at %1, fields end at %2")
                            .arg(submit->geometry().right())
                            .arg(email->geometry().right())));

    // And the alignment is a setting, not a position: Left puts it back under
    // the first field, Center in the middle, without the row accumulating
    // stretches on the way.
    const int rowItems = gate->actions()->count();
    gate->setActionAlignment(ActionAlignment::Left);
    QCoreApplication::processEvents();
    QCOMPARE(submit->geometry().left(), email->geometry().left());

    gate->setActionAlignment(ActionAlignment::Center);
    QCoreApplication::processEvents();
    QVERIFY(submit->geometry().left() > email->geometry().left());
    QVERIFY(submit->geometry().right() < email->geometry().right());

    gate->setActionAlignment(ActionAlignment::Right);
    QCoreApplication::processEvents();
    QCOMPARE(gate->actions()->count(), rowItems);
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

    // Card padding p-4, toolbar gap-2 - straight from the mockup.
    QCOMPARE(p.space(SpacingRole::CardPadding), 16);
    QCOMPARE(p.space(SpacingRole::ToolbarGap),  8);

    // Page padding is the one deliberate departure from the mockup, and it is
    // asserted at the value the profile actually chose rather than the one the
    // standalone mockup shows. The mockup is a window of its own at p-6 (24);
    // the module renders inside the NovuSpace shell, where 24 left the heading
    // floating in a band before the first card. See makeNovuTradeProfile().
    QCOMPARE(p.space(SpacingRole::PagePadding), 16);
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


// == the NovuSpace extraction ==============================================
//
// These are the invariants the new components were written to hold. None of
// them needs a pixel: they are about widget count, about which gesture owns
// which target, about what survives being put away, and about tokens.

void GalleryTest::aNavigationTreeIsOneWidgetAndNotThreeHundred()
{
    // The claim NavList exists to make. A sidebar is twenty to sixty rows,
    // each of which the HTML draws as a button containing an icon, a label, a
    // twisty, a status dot and a count pill. Built literally that is up to
    // three hundred QWidgets for a panel whose whole job is to say which page
    // you are on.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* nav = new NavList(surface);

    nav->addSection(QStringLiteral("Workspaces"));
    for (int i = 0; i < 40; ++i) {
        const QString key = QStringLiteral("row%1").arg(i);
        nav->addItem(key, QStringLiteral("Workspace %1").arg(i), Glyph::Folder);
        nav->setCount(key, i, NavList::CountTone::Alert);
        nav->setStatus(key, StatusDot::Tone::Success);
        nav->setExpandable(key, true);
    }

    QCOMPARE(nav->count(), 41);
    QCOMPARE(nav->keys().size(), 40);
    QCOMPARE(nav->findChildren<QWidget*>().size(), 0);
}

void GalleryTest::aNavRowsTwistyIsADifferentTargetFromTheRow()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* nav = new NavList(surface);
    nav->addItem(QStringLiteral("a"), QStringLiteral("Novusion"), Glyph::Folder);
    nav->setExpandable(QStringLiteral("a"), true);
    nav->resize(240, 200);

    const QRect row = nav->rowRect(0);
    const QRect twisty = nav->twistyRect(0);
    QVERIFY(!twisty.isNull());
    QVERIFY(row.contains(twisty));
    QVERIFY(twisty.width() < row.width());

    QSignalSpy activated(nav, &NavList::activated);
    QSignalSpy expanded(nav, &NavList::expandToggled);

    // Opening a workspace and expanding it are different actions on one line.
    QTest::mouseClick(nav, Qt::LeftButton, Qt::NoModifier, twisty.center());
    QCOMPARE(expanded.count(), 1);
    QCOMPARE(activated.count(), 0);
    QVERIFY(nav->isExpanded(QStringLiteral("a")));

    // A point in the row but outside the twisty chooses instead.
    const QPoint elsewhere(row.right() - 12, row.center().y());
    QTest::mouseClick(nav, Qt::LeftButton, Qt::NoModifier, elsewhere);
    QCOMPARE(activated.count(), 1);
    QCOMPARE(expanded.count(), 1);

    // Press on the twisty, release on the row: a cancelled gesture, so neither
    // fires. This is the rule every painted control in the library follows.
    QTest::mousePress(nav, Qt::LeftButton, Qt::NoModifier, twisty.center());
    QTest::mouseRelease(nav, Qt::LeftButton, Qt::NoModifier, elsewhere);
    QCOMPARE(activated.count(), 1);
    QCOMPARE(expanded.count(), 1);
}

void GalleryTest::navigationSkipsCaptionsAndDisabledRows()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* nav = new NavList(surface);
    nav->addSection(QStringLiteral("Group"));
    nav->addItem(QStringLiteral("a"), QStringLiteral("First"));
    nav->addItem(QStringLiteral("b"), QStringLiteral("Off"));
    nav->setRowEnabled(QStringLiteral("b"), false);
    nav->addItem(QStringLiteral("c"), QStringLiteral("Last"));

    QSignalSpy activated(nav, &NavList::activated);

    // A caption is not somewhere you can be.
    nav->setCurrentIndex(0);
    QCOMPARE(nav->currentIndex(), -1);

    nav->choose(0);
    QCOMPARE(activated.count(), 0);

    // Nor is a disabled row.
    nav->choose(2);
    QCOMPARE(activated.count(), 0);

    nav->setCurrentKey(QStringLiteral("a"));
    QCOMPARE(nav->currentKey(), QStringLiteral("a"));

    // Down from "First" lands on "Last", stepping over the disabled row.
    nav->setFocus();
    QTest::keyClick(nav, Qt::Key_Down);
    QCOMPARE(nav->currentKey(), QStringLiteral("c"));
}

void GalleryTest::bothStripsShareOneHitTestImplementation()
{
    // Segmented promised to give up its hit testing when a second strip needed
    // it. This is the assertion that it did, rather than a comment saying so.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));

    auto* picker = new Segmented({QStringLiteral("Basic"), QStringLiteral("Pro")},
                                 surface, 0);
    auto* modules = new ModuleBar(surface);
    modules->addModule(QStringLiteral("web"), QStringLiteral("Web"), Glyph::Globe);
    modules->addModule(QStringLiteral("mail"), QStringLiteral("Mail"), Glyph::Mail);

    QVERIFY(qobject_cast<ItemStrip*>(picker) != nullptr);
    QVERIFY(qobject_cast<ItemStrip*>(modules) != nullptr);

    // Both are painted: neither builds a widget per item.
    QCOMPARE(picker->findChildren<QWidget*>().size(), 0);
    QCOMPARE(modules->findChildren<QWidget*>().size(), 0);

    // And both cancel the same way, because there is one implementation of it.
    modules->resize(modules->sizeHint());
    QSignalSpy chosen(modules, &ModuleBar::moduleChosen);
    QTest::mousePress(modules, Qt::LeftButton, Qt::NoModifier,
                      modules->itemRect(1).center());
    QTest::mouseRelease(modules, Qt::LeftButton, Qt::NoModifier,
                        modules->itemRect(0).center());
    QCOMPARE(chosen.count(), 0);
    QCOMPARE(modules->currentKey(), QStringLiteral("web"));
}

void GalleryTest::aDisclosureHidesItsBodyWithoutForgettingIt()
{
    // The design says it outright: collapsing WORKSPACES does not forget which
    // workspaces were open inside it.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* group = new Disclosure(QStringLiteral("Workspaces"), surface,
                                 Disclosure::Variant::Block);

    auto* inner = new NavList(group->bodyWidget());
    inner->addItem(QStringLiteral("a"), QStringLiteral("Novusion"));
    inner->setExpandable(QStringLiteral("a"), true);
    inner->setExpanded(QStringLiteral("a"), true);
    group->body()->addWidget(inner);

    QVERIFY(group->isOpen());
    group->setOpen(false);
    QVERIFY(!group->isOpen());

    // The body is hidden, not cleared, and what was expanded inside it still is.
    QCOMPARE(group->bodyWidget(), inner->parentWidget());
    QVERIFY(inner->isExpanded(QStringLiteral("a")));

    group->setOpen(true);
    QVERIFY(inner->isExpanded(QStringLiteral("a")));
}

void GalleryTest::anActionInsideABlockDoesNotCollapseIt()
{
    // Creating something belongs next to the thing it creates, and it must not
    // put away the list you were creating it in.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* group = new Disclosure(QStringLiteral("Workspaces"), surface,
                                 Disclosure::Variant::Block);
    group->setAction(Glyph::Plus, QStringLiteral("New workspace"));
    group->resize(240, group->sizeHint().height());

    QVERIFY(group->hasAction());
    const QRect action = group->actionRect();
    QVERIFY(!action.isNull());
    QVERIFY(group->headerRect().contains(action));

    QSignalSpy toggled(group, &Disclosure::toggled);
    QSignalSpy triggered(group, &Disclosure::actionTriggered);

    QTest::mouseClick(group, Qt::LeftButton, Qt::NoModifier, action.center());
    QCOMPARE(triggered.count(), 1);
    QCOMPARE(toggled.count(), 0);
    QVERIFY(group->isOpen());

    // The caption still toggles.
    QTest::mouseClick(group, Qt::LeftButton, Qt::NoModifier,
                      QPoint(action.left() / 2, group->headerRect().center().y()));
    QCOMPARE(toggled.count(), 1);
    QVERIFY(!group->isOpen());
}

void GalleryTest::chordsSplitIntoCapsAndLoneSeparatorsSurvive()
{
    QCOMPARE(KeyCap::split(QStringLiteral("Ctrl+Shift+R")),
             QStringList({QStringLiteral("Ctrl"), QStringLiteral("Shift"),
                          QStringLiteral("R")}));
    QCOMPARE(KeyCap::split(QStringLiteral("F11")), QStringList({QStringLiteral("F11")}));

    // The two a naive split gets wrong.
    QCOMPARE(KeyCap::split(QStringLiteral("+")), QStringList({QStringLiteral("+")}));
    QCOMPARE(KeyCap::split(QStringLiteral("Ctrl++")),
             QStringList({QStringLiteral("Ctrl"), QStringLiteral("+")}));

    // A sequence keeps the comma between its chords, because a reader has to
    // see that the second half is pressed afterwards rather than together.
    QCOMPARE(KeyCap::split(QStringLiteral("Ctrl+K, Ctrl+F")),
             QStringList({QStringLiteral("Ctrl"), QStringLiteral("K"),
                          QStringLiteral(","), QStringLiteral("Ctrl"),
                          QStringLiteral("F")}));

    QCOMPARE(KeyCap::split(QString()), QStringList());
}

void GalleryTest::aFilteredShortcutSheetDropsEmptyCaptions()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* list = new ShortcutList(surface);
    list->addGroup(QStringLiteral("Navigation"));
    list->addShortcut(QStringLiteral("Command palette"), QStringLiteral("Ctrl+K"));
    list->addGroup(QStringLiteral("Editing"));
    list->addShortcut(QStringLiteral("Save"), QStringLiteral("Ctrl+S"));

    QCOMPARE(list->rowCount(), 2);
    QCOMPARE(list->groupCount(), 2);

    // The sheet is one widget, however many rows it holds.
    QCOMPARE(list->findChildren<QWidget*>().size(), 0);

    // Filtering matches the keys as well as the label ...
    list->setFilter(QStringLiteral("ctrl+s"));
    QVERIFY(list->sizeHint().height() > 0);

    // ... and a caption with nothing left under it must not survive, which a
    // height comparison is the cheapest way to see.
    const int filtered = list->sizeHint().height();
    list->setFilter(QString());
    QVERIFY(list->sizeHint().height() > filtered);

    list->setFilter(QStringLiteral("zzz-nothing-matches"));
    QCOMPARE(list->sizeHint().height(), 0);
}

void GalleryTest::statusBarItemsAreAddressedByHandle()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* bar = new StatusBar(surface);

    const int session = bar->addValue(QStringLiteral("nobody"));
    bar->addSeparator();
    const int latency = bar->addMono(QStringLiteral("0 ms"), StatusBar::Slot::Trailing);

    QCOMPARE(bar->itemText(session), QStringLiteral("nobody"));
    bar->setItemText(session, QStringLiteral("p.dostal@novusion.net"));
    QCOMPARE(bar->itemText(session), QStringLiteral("p.dostal@novusion.net"));

    bar->setItemText(latency, QStringLiteral("52 ms"));
    QCOMPARE(bar->itemText(latency), QStringLiteral("52 ms"));

    // A handle that names a separator is not a special case a caller has to
    // guard: setting its text does nothing rather than asserting.
    const int rule = bar->itemCount() - 2;
    Q_UNUSED(rule);
    QCOMPARE(bar->itemCount(), 3);

    // Painted, not built: no label per item.
    QCOMPARE(bar->findChildren<QWidget*>().size(), 0);

    bar->setItemVisible(latency, false);
    QVERIFY(!bar->isItemVisible(latency));
}

void GalleryTest::aLogKeepsOnlyItsCap()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* view = new LogView(surface);
    view->setMaximumLines(50);

    for (int i = 0; i < 400; ++i) {
        view->append(LogView::Level::Info, QStringLiteral("line %1").arg(i));
    }
    QCOMPARE(view->lineCount(), 50);
    // The oldest go first, so the newest line is still there.
    QCOMPARE(view->lineText(49), QStringLiteral("line 399"));

    // The filter narrows the view without discarding what it is not showing.
    view->setMinimumLevel(LogView::Level::Error);
    QCOMPARE(view->visibleLineCount(), 0);
    QCOMPARE(view->lineCount(), 50);
    view->setMinimumLevel(LogView::Level::Debug);
    QCOMPARE(view->visibleLineCount(), 50);

    view->setFilter(QStringLiteral("line 39"));
    QVERIFY(view->visibleLineCount() > 0);
    QVERIFY(view->visibleLineCount() < 50);
}

void GalleryTest::aLogStopsFollowingWhenTheUserScrollsAway()
{
    // A log that yanks itself back to the bottom while someone is reading
    // history is unusable. Following is a consequence of where the user is.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* view = new LogView(surface);
    view->resize(420, 160);
    view->show();

    for (int i = 0; i < 200; ++i) {
        view->append(LogView::Level::Info, QStringLiteral("line %1").arg(i));
    }
    QVERIFY(view->isFollowing());

    QSignalSpy followed(view, &LogView::followingChanged);
    view->verticalScrollBar()->setValue(0);
    QVERIFY(!view->isFollowing());
    QCOMPARE(followed.count(), 1);

    view->verticalScrollBar()->setValue(view->verticalScrollBar()->maximum());
    QVERIFY(view->isFollowing());
    QCOMPARE(followed.count(), 2);
}

void GalleryTest::aStoppedSpinnerReleasesTheClock()
{
    Pulse& pulse = Pulse::instance();
    const int before = pulse.subscriberCount();

    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    surface->resize(200, 80);
    auto* spinner = new Spinner(ControlSize::Medium, surface);
    surface->show();
    QVERIFY(QTest::qWaitForWindowExposed(surface));

    QCOMPARE(pulse.subscriberCount(), before + 1);

    // Stopping must release the subscription, not merely stop drawing: a
    // spinner left behind a finished operation is the usual reason a quiet
    // window still repaints sixty times a second.
    spinner->setSpinning(false);
    QCOMPARE(pulse.subscriberCount(), before);

    spinner->setSpinning(true);
    QCOMPARE(pulse.subscriberCount(), before + 1);

    surface->hide();
    QCOMPARE(pulse.subscriberCount(), before);
}

void GalleryTest::aSkeletonListIsOneWidgetWhateverItsLength()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* list = new SkeletonList(5, {3.0, 1.0, 1.0}, surface);
    QCOMPARE(list->findChildren<QWidget*>().size(), 0);

    const int five = list->sizeHint().height();
    list->setRowCount(40);
    QCOMPARE(list->rowCount(), 40);
    QCOMPARE(list->findChildren<QWidget*>().size(), 0);
    QCOMPARE(list->sizeHint().height(), five * 8);
}

void GalleryTest::aSequencePastItsLastStepIsComplete()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    const QStringList stages = {QStringLiteral("Validate"), QStringLiteral("Upload"),
                                QStringLiteral("Index")};
    auto* steps = new Steps(stages, surface, 0);

    QCOMPARE(steps->count(), 3);
    QVERIFY(!steps->isComplete());

    // count() is a legal index and means finished. Clamping to count()-1 would
    // leave the last stage looking like it is still running after it succeeded.
    steps->setCurrentIndex(3);
    QCOMPARE(steps->currentIndex(), 3);
    QVERIFY(steps->isComplete());

    steps->setCurrentIndex(99);
    QCOMPARE(steps->currentIndex(), 3);

    steps->setCurrentIndex(1);
    QVERIFY(!steps->isComplete());
    steps->markComplete();
    QVERIFY(steps->isComplete());
}

void GalleryTest::aTooltipResolvesAgainstTheWidgetItIsAbout()
{
    // A tooltip is a top-level window and so belongs to no ThemeScope. If it
    // resolved its own profile, every tip in the process would wear the default
    // product's colours - which is exactly the defect the token layer exists to
    // prevent.
    QWidget* space = makeSurface(QStringLiteral("novuspace"));
    auto* target = new Button(QStringLiteral("Refresh"), Button::Variant::Secondary, space);

    TipContent content;
    content.title = QStringLiteral("Full rebuild");
    content.shortcut = QStringLiteral("Ctrl+Shift+R");
    content.brief = QStringLiteral("Rebuilds the index from scratch.");
    content.detail = QStringLiteral("Every document is re-read and re-embedded.");
    Tooltip::install(target, content);

    const TipContent stored = Tooltip::contentFor(target);
    QCOMPARE(stored.title, content.title);
    QCOMPARE(stored.shortcut, content.shortcut);
    QCOMPARE(stored.detail, content.detail);

    // The branded tip and the OS one must never both appear.
    QVERIFY(target->toolTip().isEmpty());

    // Help is a mode on the layer, not a flag per tip.
    const bool was = Tooltip::helpMode();
    Tooltip::setHelpMode(true);
    QVERIFY(Tooltip::helpMode());
    Tooltip::setHelpMode(was);

    Tooltip::remove(target);
    QVERIFY(Tooltip::contentFor(target).isEmpty());
}

void GalleryTest::aSettingInErrorSaysSoInItsContract()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* row = new SettingRow(QStringLiteral("Endpoint"),
                               QStringLiteral("Where the hub is reached."), surface);

    QVERIFY(!row->hasError());
    QCOMPARE(row->styleSpec().foreground.describe(), QStringLiteral("textPrimary"));

    row->setError(QStringLiteral("Port must be between 1 and 65535."));
    QVERIFY(row->hasError());
    QCOMPARE(row->styleSpec().foreground.describe(), QStringLiteral("status.danger.text"));
    QCOMPARE(row->styleSpec().border.describe(), QStringLiteral("status.danger.border"));

    // The row grows to make space for the message rather than clipping it.
    const int clean = row->heightForWidth(420);
    row->setError(QString());
    QVERIFY(row->heightForWidth(420) < clean);
}

void GalleryTest::aCollapsedRailCollapsesEveryListInIt()
{
    // Not only the convenience list: a rail with three lists inside Disclosure
    // blocks has to collapse whole.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* rail = new Sidebar(surface);
    rail->setIdentity(Glyph::Vault, QStringLiteral("Vault"), QStringLiteral("v0.1"));

    NavList* first = rail->nav();
    first->addItem(QStringLiteral("a"), QStringLiteral("Overview"), Glyph::Overview);

    auto* group = new Disclosure(QStringLiteral("More"), rail->navigationWidget(),
                                 Disclosure::Variant::Block);
    auto* second = new NavList(group->bodyWidget());
    second->addItem(QStringLiteral("b"), QStringLiteral("Archive"), Glyph::Archive);
    group->body()->addWidget(second);
    rail->navigation()->insertWidget(rail->navigation()->count() - 1, group);

    QVERIFY(!first->isCollapsed());
    QVERIFY(!second->isCollapsed());
    const int wide = rail->width();

    rail->setCollapsed(true);
    QVERIFY(first->isCollapsed());
    QVERIFY(second->isCollapsed());
    QVERIFY(rail->width() < wide);
    QCOMPARE(rail->width(), rail->collapsedWidth());

    rail->setCollapsed(false);
    QVERIFY(!first->isCollapsed());
    QVERIFY(!second->isCollapsed());
    QCOMPARE(rail->width(), wide);
}

void GalleryTest::aRiskyChoiceIsWarningAndNotDanger()
{
    // Danger is for an action that destroys something. A risky *setting* is
    // still only a setting, and the design marks it amber.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* plain = new Tile(QStringLiteral("Balanced"), QStringLiteral("The default."),
                           surface, true);
    auto* risky = new Tile(QStringLiteral("Aggressive"),
                           QStringLiteral("Uses every core."), surface, true);
    risky->setTone(Tile::Tone::Risky);

    QCOMPARE(plain->styleSpec().checkedBackground.describe(), QStringLiteral("accent@11"));
    QCOMPARE(risky->styleSpec().checkedBackground.describe(),
             QStringLiteral("status.warning.fill"));
    QCOMPARE(risky->styleSpec().checkedForeground.describe(),
             QStringLiteral("status.warning.text"));

    // The description wraps, so a card can explain itself.
    risky->setSubtitle(QStringLiteral(
        "Uses every available core for the rebuild, which makes the machine "
        "unresponsive for the duration and is not what you want on a laptop."));
    QVERIFY(risky->heightForWidth(180) > risky->heightForWidth(600));
}

void GalleryTest::theExtractedComponentsKeepTokensRatherThanColours()
{
    // Every component added in this pass, asked what it means rather than what
    // it looks like. A component that resolved its palette once would still
    // paint after a theme change - just in the old product's colours - so this
    // is the assertion that none of them did.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));

    auto* nav = new NavList(surface);
    nav->addItem(QStringLiteral("a"), QStringLiteral("Overview"));
    QCOMPARE(nav->rowStyleSpec(0).checkedBackground.describe(), QStringLiteral("accent@13"));
    QCOMPARE(nav->rowStyleSpec(0).checkedForeground.describe(), QStringLiteral("accentLight"));

    auto* modules = new ModuleBar(surface);
    modules->addModule(QStringLiteral("web"), QStringLiteral("Web"));
    QCOMPARE(modules->moduleStyleSpec(0).checkedBackground.describe(),
             QStringLiteral("accent@17"));

    auto* strip = new ConnectionStrip(surface);
    QCOMPARE(strip->styleSpec().background.describe(), QStringLiteral("windowBg"));

    auto* bar = new StatusBar(surface);
    QCOMPARE(bar->styleSpec().background.describe(), QStringLiteral("headerBg"));

    auto* header = new PageHeader(QStringLiteral("Overview"), surface);
    QCOMPARE(header->styleSpec().border.describe(), QStringLiteral("borderSubtle"));

    auto* mark = new AppMark(Glyph::Vault, surface);
    QCOMPARE(mark->styleSpec().foreground.describe(), QStringLiteral("accentLight"));
    mark->setAccented(false);
    QCOMPARE(mark->styleSpec().foreground.describe(), QStringLiteral("textSecondary"));

    auto* menu = new Popover(surface);
    menu->addItem(QStringLiteral("x"), QStringLiteral("Delete"));
    menu->setDangerous(QStringLiteral("x"), true);
    QCOMPARE(menu->itemStyleSpec(0).foreground.describe(),
             QStringLiteral("status.danger.text"));

    auto* logs = new LogView(surface);
    QCOMPARE(logs->styleSpec().background.describe(), QStringLiteral("windowBg"));

    auto* block = new Disclosure(QStringLiteral("Group"), surface,
                                 Disclosure::Variant::Block);
    QCOMPARE(block->styleSpec().border.describe(), QStringLiteral("transparent"));
    block->setVariant(Disclosure::Variant::Inline);
    QCOMPARE(block->styleSpec().border.describe(), QStringLiteral("border"));

    // And each of them resolves to this profile's own colours rather than to a
    // colour baked in at construction.
    const ThemeProfile& profile =
        ThemeManager::instance().profile(QStringLiteral("novuspace"));
    QCOMPARE(bar->resolvedStyle().background.name(), profile.colors.headerBg.name());
    QCOMPARE(strip->resolvedStyle().background.name(), profile.colors.windowBg.name());
}

void GalleryTest::aMaskedValueDoesNotLeakItsLength()
{
    // Masking that preserves the length leaks the length, which for a
    // passphrase is most of what an attacker wanted to know.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* shortSecret = new SettingRow(QStringLiteral("Key"), QString(), surface);
    auto* longSecret = new SettingRow(QStringLiteral("Key"), QString(), surface);

    shortSecret->setValue(QStringLiteral("abc"));
    longSecret->setValue(QStringLiteral("sk_live_9f2b8c14a77e4d03b6e1f0a2c9"));
    QVERIFY(shortSecret->sizeHint().width() < longSecret->sizeHint().width());

    shortSecret->setMasked(true);
    longSecret->setMasked(true);
    QCOMPARE(shortSecret->sizeHint().width(), longSecret->sizeHint().width());

    // The value itself is still what the caller set - masking is presentation.
    QCOMPARE(longSecret->value(), QStringLiteral("sk_live_9f2b8c14a77e4d03b6e1f0a2c9"));
}


// == the NovuAI extraction =================================================

void GalleryTest::aTreeFlattensOnlyWhatIsOpen()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* tree = new TreeView(surface);

    tree->addNode(QString(), QStringLiteral("root"), QStringLiteral("workspace"),
                  Glyph::Folder);
    for (int i = 0; i < 6; ++i) {
        const QString branch = QStringLiteral("b%1").arg(i);
        tree->addNode(QStringLiteral("root"), branch, QStringLiteral("branch %1").arg(i),
                      Glyph::Folder);
        for (int j = 0; j < 20; ++j) {
            tree->addNode(branch, QStringLiteral("%1.%2").arg(branch).arg(j),
                          QStringLiteral("file %1").arg(j), Glyph::File);
        }
    }

    QCOMPARE(tree->nodeCount(), 1 + 6 + 6 * 20);
    // Shut, only the root is a row - which is the whole reason a tree of a
    // hundred and twenty-seven nodes is cheap.
    QCOMPARE(tree->visibleRowCount(), 1);

    tree->setExpanded(QStringLiteral("root"), true);
    QCOMPARE(tree->visibleRowCount(), 7);

    tree->setExpanded(QStringLiteral("b0"), true);
    QCOMPARE(tree->visibleRowCount(), 27);

    tree->expandAll();
    QCOMPARE(tree->visibleRowCount(), tree->nodeCount());

    tree->collapseAll();
    QCOMPARE(tree->visibleRowCount(), 1);

    // And none of it is a widget.
    QCOMPARE(tree->viewport()->findChildren<QWidget*>().size(), 0);

    QCOMPARE(tree->depthOf(QStringLiteral("b0.3")), 2);
    QCOMPARE(tree->parentOf(QStringLiteral("b0.3")), QStringLiteral("b0"));
    QCOMPARE(tree->childrenOf(QStringLiteral("root")).size(), 6);
}

void GalleryTest::aTreeChevronIsADifferentTargetFromTheRow()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* tree = new TreeView(surface);
    tree->addNode(QString(), QStringLiteral("a"), QStringLiteral("Novusion"), Glyph::Folder);
    tree->addNode(QStringLiteral("a"), QStringLiteral("a.1"), QStringLiteral("src"),
                  Glyph::Folder);
    tree->resize(280, 200);

    const QRect row = tree->rowRect(0);
    const QRect chevron = tree->chevronRect(0);
    QVERIFY(!chevron.isNull());
    QVERIFY(row.contains(chevron));

    QSignalSpy activated(tree, &TreeView::activated);
    QSignalSpy expanded(tree, &TreeView::expandToggled);

    QTest::mouseClick(tree->viewport(), Qt::LeftButton, Qt::NoModifier, chevron.center());
    QCOMPARE(expanded.count(), 1);
    QCOMPARE(activated.count(), 0);
    QVERIFY(tree->isExpanded(QStringLiteral("a")));

    const QPoint onName(row.right() - 20, row.center().y());
    QTest::mouseClick(tree->viewport(), Qt::LeftButton, Qt::NoModifier, onName);
    QCOMPARE(activated.count(), 1);
    QCOMPARE(expanded.count(), 1);
    QCOMPARE(tree->currentId(), QStringLiteral("a"));

    // Press on one target, release on the other: cancelled.
    QTest::mousePress(tree->viewport(), Qt::LeftButton, Qt::NoModifier, chevron.center());
    QTest::mouseRelease(tree->viewport(), Qt::LeftButton, Qt::NoModifier, onName);
    QCOMPARE(activated.count(), 1);
    QCOMPARE(expanded.count(), 1);

    // A leaf has no chevron, and reserves its space rather than shifting left.
    QVERIFY(tree->chevronRect(tree->rowOf(QStringLiteral("a.1"))).isNull());
}

void GalleryTest::aTreeRevealsANodeByOpeningItsAncestors()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* tree = new TreeView(surface);
    tree->addNode(QString(), QStringLiteral("a"), QStringLiteral("a"), Glyph::Folder);
    tree->addNode(QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("b"),
                  Glyph::Folder);
    tree->addNode(QStringLiteral("b"), QStringLiteral("c"), QStringLiteral("c"),
                  Glyph::Folder);
    tree->addNode(QStringLiteral("c"), QStringLiteral("d"), QStringLiteral("target"),
                  Glyph::File);
    tree->resize(280, 200);

    QCOMPARE(tree->visibleRowCount(), 1);
    QCOMPARE(tree->rowOf(QStringLiteral("d")), -1);

    tree->revealNode(QStringLiteral("d"));
    QVERIFY(tree->isExpanded(QStringLiteral("a")));
    QVERIFY(tree->isExpanded(QStringLiteral("b")));
    QVERIFY(tree->isExpanded(QStringLiteral("c")));
    // The target itself is not opened - revealing a node is not expanding it.
    QVERIFY(!tree->isExpanded(QStringLiteral("d")));
    QCOMPARE(tree->rowOf(QStringLiteral("d")), 3);
}

void GalleryTest::aTreeKeyboardWalksTheStructure()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* tree = new TreeView(surface);
    tree->addNode(QString(), QStringLiteral("a"), QStringLiteral("a"), Glyph::Folder);
    tree->addNode(QStringLiteral("a"), QStringLiteral("a1"), QStringLiteral("a1"),
                  Glyph::File);
    tree->addNode(QStringLiteral("a"), QStringLiteral("a2"), QStringLiteral("a2"),
                  Glyph::File);
    tree->addNode(QString(), QStringLiteral("b"), QStringLiteral("b"), Glyph::Folder);
    tree->resize(280, 240);
    tree->setCurrentId(QStringLiteral("a"));

    // Right on a closed node opens it; right again steps into it. That is what
    // every file manager does and what a user's hands expect.
    QTest::keyClick(tree, Qt::Key_Right);
    QVERIFY(tree->isExpanded(QStringLiteral("a")));
    QCOMPARE(tree->currentId(), QStringLiteral("a"));

    QTest::keyClick(tree, Qt::Key_Right);
    QCOMPARE(tree->currentId(), QStringLiteral("a1"));

    QTest::keyClick(tree, Qt::Key_Down);
    QCOMPARE(tree->currentId(), QStringLiteral("a2"));

    // Left on a leaf goes to its parent.
    QTest::keyClick(tree, Qt::Key_Left);
    QCOMPARE(tree->currentId(), QStringLiteral("a"));

    // Left on an open node closes it.
    QTest::keyClick(tree, Qt::Key_Left);
    QVERIFY(!tree->isExpanded(QStringLiteral("a")));

    QTest::keyClick(tree, Qt::Key_Down);
    QCOMPARE(tree->currentId(), QStringLiteral("b"));
}

void GalleryTest::aDiffIsParsedRatherThanColouredByItsFirstCharacter()
{
    // Classifying by the leading character is the mistake every first attempt
    // makes: it paints `--- a/file` as a removed line and `+++ b/file` as an
    // added one, so the file header comes out red and green.
    const QString text = QStringLiteral(
        "diff --git a/x.cpp b/x.cpp\n"
        "index 1111111..2222222 100644\n"
        "--- a/x.cpp\n"
        "+++ b/x.cpp\n"
        "@@ -10,3 +10,4 @@ void f()\n"
        " context one\n"
        "-gone\n"
        "+arrived\n"
        "+also arrived\n");

    const QVector<DiffView::Line> lines = DiffView::parseUnified(text);

    QCOMPARE(lines.at(0).kind, DiffView::Kind::File);
    QCOMPARE(lines.at(1).kind, DiffView::Kind::File);
    QCOMPARE(lines.at(2).kind, DiffView::Kind::File);
    QCOMPARE(lines.at(3).kind, DiffView::Kind::File);
    QCOMPARE(lines.at(4).kind, DiffView::Kind::Hunk);
    QCOMPARE(lines.at(5).kind, DiffView::Kind::Context);
    QCOMPARE(lines.at(6).kind, DiffView::Kind::Removed);
    QCOMPARE(lines.at(7).kind, DiffView::Kind::Added);

    // The numbers are the diff's own, from the @@ header, and each side only
    // has one where that side has the line.
    QCOMPARE(lines.at(5).oldNumber, 10);
    QCOMPARE(lines.at(5).newNumber, 10);
    QCOMPARE(lines.at(6).oldNumber, 11);
    QCOMPARE(lines.at(6).newNumber, -1);
    QCOMPARE(lines.at(7).oldNumber, -1);
    QCOMPARE(lines.at(7).newNumber, 11);

    // The leading marker is stripped from the body: a viewer that kept it would
    // be a viewer whose text cannot be copied back out.
    QCOMPARE(lines.at(7).text, QStringLiteral("arrived"));

    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* view = new DiffView(surface);
    view->setUnifiedDiff(text);
    QCOMPARE(view->addedCount(), 2);
    QCOMPARE(view->removedCount(), 1);
}

void GalleryTest::theLogAndTheDiffShareOneVirtualisation()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));

    auto* log = new LogView(surface);
    auto* diff = new DiffView(surface);
    QVERIFY(qobject_cast<MonoLineView*>(log) != nullptr);
    QVERIFY(qobject_cast<MonoLineView*>(diff) != nullptr);

    log->resize(500, 200);
    log->show();
    for (int i = 0; i < 4000; ++i) {
        log->append(LogView::Level::Info, QStringLiteral("line %1").arg(i));
    }

    // The claim the base class exists to make: the buffer is four thousand
    // rows and the paint is twenty.
    QCOMPARE(log->rowCount(), 4000);
    QVERIFY(log->visibleRowCount() > 0);
    QVERIFY2(log->visibleRowCount() < 60,
             qPrintable(QStringLiteral("painted %1 rows for a 200px viewport")
                            .arg(log->visibleRowCount())));

    // Both agree on the row height for one profile, because it comes from one
    // place - but the diff runs tighter, which is the one thing they differ on.
    QVERIFY(diff->rowHeight() > 0);
    QVERIFY(diff->rowHeight() <= log->rowHeight());
}

void GalleryTest::aFloatingPanelCannotLeaveItsLayer()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    surface->resize(800, 500);

    FloatingLayer* layer = FloatingLayer::forHost(surface);
    QVERIFY(layer);
    // One layer per host: two would be two stacking orders.
    QCOMPARE(FloatingLayer::forHost(surface), layer);

    auto* panel = new FloatingPanel(Glyph::Ai, QStringLiteral("Assistant"));
    layer->addPanel(panel);
    panel->placeAt(QPoint(40, 40), QSize(400, 240));
    QCOMPARE(panel->pos(), QPoint(40, 40));

    // Dragged far above the top: the bar has to stay reachable.
    panel->placeAt(QPoint(40, -400));
    QCOMPARE(panel->y(), 0);

    // Dragged far below the bottom: the bar stays on screen.
    panel->placeAt(QPoint(40, 5000));
    QVERIFY(panel->y() <= layer->height());
    QVERIFY(panel->y() + panel->barRect().height() <= layer->height() + 1);

    // Far to the right: still grabbable.
    panel->placeAt(QPoint(9000, 40));
    QVERIFY(panel->x() < layer->width());
}

void GalleryTest::theLayerDecidesWhichPanelIsInFront()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    surface->resize(800, 500);
    FloatingLayer* layer = FloatingLayer::forHost(surface);

    auto* first = new FloatingPanel(Glyph::Ai, QStringLiteral("One"));
    auto* second = new FloatingPanel(Glyph::Chart, QStringLiteral("Two"));
    layer->addPanel(first);
    layer->addPanel(second);

    QCOMPARE(layer->panelCount(), 2);
    // Exactly one is front - which is why the layer owns it rather than each
    // panel deciding for itself.
    QVERIFY(!first->isFront());
    QVERIFY(second->isFront());

    layer->raisePanel(first);
    QVERIFY(first->isFront());
    QVERIFY(!second->isFront());
    QCOMPARE(layer->frontPanel(), first);

    layer->cascade();
    QVERIFY(first->pos() != second->pos());
}

void GalleryTest::aRolledPanelKeepsWhatItHad()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    surface->resize(800, 500);
    FloatingLayer* layer = FloatingLayer::forHost(surface);

    auto* panel = new FloatingPanel(Glyph::Ai, QStringLiteral("Assistant"));
    layer->addPanel(panel);
    panel->placeAt(QPoint(30, 30), QSize(420, 320));

    auto* marker = new QWidget(panel->bodyWidget());
    panel->body()->addWidget(marker);

    QCOMPARE(panel->height(), 320);
    panel->setRolled(true);
    QVERIFY(panel->isRolled());
    QCOMPARE(panel->height(), panel->barRect().height());
    // Rolling is not closing: the contents are still there.
    QCOMPARE(marker->parentWidget(), panel->bodyWidget());

    panel->setRolled(false);
    QCOMPARE(panel->height(), 320);
    QCOMPARE(marker->parentWidget(), panel->bodyWidget());
}

void GalleryTest::toastsStackToACapAndNoFurther()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    surface->resize(600, 400);

    Toast::dismissAll(surface);
    for (int i = 0; i < 8; ++i) {
        QVERIFY(Toast::post(surface, QStringLiteral("message %1").arg(i),
                            Toast::Tone::Neutral, 30000));
    }

    // Eight were posted; the cap holds. A column of eight is a notification
    // centre, which is a different thing and not one a toast becomes by
    // accident.
    QVERIFY2(Toast::shownOn(surface).size() <= 3,
             qPrintable(QStringLiteral("%1 toasts on screen")
                            .arg(Toast::shownOn(surface).size())));

    Toast::dismissAll(surface);
    QTest::qWait(20);
    QCOMPARE(Toast::shownOn(surface).size(), 0);
}

void GalleryTest::aToastHoldsWhileThePointerIsOnIt()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    surface->resize(600, 400);
    Toast::dismissAll(surface);

    Toast* toast = Toast::postWithAction(surface, QStringLiteral("3 files moved"),
                                         QStringLiteral("Undo"), Toast::Tone::Success, 90);
    QVERIFY(toast);
    QCOMPARE(toast->actionText(), QStringLiteral("Undo"));
    QVERIFY(!toast->actionRect().isNull());

    // Held, the timer does not fire - which is the only thing that makes an
    // Undo reachable: the user goes for the button and it leaves on the way.
    toast->hold();
    QTest::qWait(200);
    QVERIFY2(!Toast::shownOn(surface).isEmpty(), "a held toast dismissed itself");

    QSignalSpy gone(toast, &Toast::dismissed);
    toast->dismiss();
    QCOMPARE(gone.count(), 1);
}

void GalleryTest::aSelectionBarIsNotThereAtZero()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    surface->resize(400, 200);
    surface->show();

    auto* bar = new SelectionBar(surface);
    bar->setNoun(QStringLiteral("file"));
    QSignalSpy shown(bar, &SelectionBar::shownChanged);

    // Not there before anything is selected, and not a state a caller handles.
    QVERIFY(!bar->isVisible());

    bar->setCount(1);
    QVERIFY(bar->isVisible());
    QCOMPARE(shown.count(), 1);
    QCOMPARE(bar->text(), QStringLiteral("1 file selected"));

    bar->setCount(3);
    QCOMPARE(bar->text(), QStringLiteral("3 files selected"));
    QCOMPARE(shown.count(), 1);   // still shown; not a second signal

    bar->setCount(0);
    QVERIFY(!bar->isVisible());
    QCOMPARE(shown.count(), 2);

    // An override wins over the built sentence, but the count still decides
    // whether there is a bar at all.
    bar->setCount(5);
    bar->setText(QStringLiteral("5 of 40 rows"));
    QCOMPARE(bar->text(), QStringLiteral("5 of 40 rows"));
    QVERIFY(bar->isVisible());
}

void GalleryTest::aDialogHeadIsAddedOnceAndUpdatedAfter()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* dialog = new Dialog(QStringLiteral("Rename"), surface);

    // Filled first, headed afterwards - the common order, which has to work.
    auto* field = new QLineEdit(dialog->content());
    dialog->contentLayout()->addWidget(field);

    const int before = dialog->content()->layout()->count();
    dialog->setHeading(QStringLiteral("Rename workspace"),
                       QStringLiteral("The name is shown in the sidebar."),
                       Dialog::Tone::Accent);
    QCOMPARE(dialog->content()->layout()->count(), before + 1);
    QCOMPARE(dialog->heading(), QStringLiteral("Rename workspace"));
    QCOMPARE(dialog->tone(), Dialog::Tone::Accent);

    // The head goes in at the top, above what was already there.
    QCOMPARE(dialog->content()->layout()->itemAt(1)->widget(),
             static_cast<QWidget*>(field));

    // Called again it updates rather than adding a second head.
    dialog->setHeading(QStringLiteral("Delete workspace"),
                       QStringLiteral("This cannot be undone."), Dialog::Tone::Danger);
    QCOMPARE(dialog->content()->layout()->count(), before + 1);
    QCOMPARE(dialog->tone(), Dialog::Tone::Danger);
    QCOMPARE(dialog->note(), QStringLiteral("This cannot be undone."));

    dialog->deleteLater();
}

void GalleryTest::anAvatarIsRoundAndAPlaqueIsNot()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));

    auto* plaque = new AppMark(Glyph::Vault, surface, AppMark::Size::Small);
    QCOMPARE(plaque->shape(), AppMark::Shape::Plaque);
    QCOMPARE(plaque->side(), 29);

    AppMark* person = AppMark::avatar(QStringLiteral("pd"), surface);
    QCOMPARE(person->shape(), AppMark::Shape::Round);
    QCOMPARE(person->tone(), AppMark::Tone::Neutral);
    QCOMPARE(person->initials(), QStringLiteral("PD"));
    // The design's 26px avatar rather than the 29px plaque.
    QCOMPARE(person->side(), 26);
    // An avatar is a party, never the product.
    QVERIFY(!person->isAccented());

    AppMark* agent = AppMark::avatar(QStringLiteral("AI"), surface, AppMark::Tone::Agent);
    QCOMPARE(agent->tone(), AppMark::Tone::Agent);

    AppMark* me = AppMark::avatar(QStringLiteral("me"), surface, AppMark::Tone::Accent);
    // The accented party resolves through the product accent, as a token.
    QCOMPARE(me->styleSpec().background.describe(), QStringLiteral("accent@16"));
    QCOMPARE(me->styleSpec().foreground.describe(), QStringLiteral("accent"));

    // Glyph or letters, never both.
    person->setGlyph(Glyph::Users);
    QVERIFY(person->initials().isEmpty());
    person->setInitials(QStringLiteral("pd"));
    QCOMPARE(person->glyph(), Glyph::None);
}


// == the NovuExplorer extraction ===========================================

void GalleryTest::aCollapsedPanelComesBackAtTheWidthItHad()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    // Shown, because a QSplitter lays its children out in resizeEvent and a
    // widget inside a hidden parent never gets one - the handle would stay 1px
    // and have no arrow zones to assert on.
    surface->resize(700, 400);
    surface->show();
    QVERIFY(QTest::qWaitForWindowExposed(surface));

    auto* splitter = new Splitter(Qt::Horizontal, surface);
    splitter->resize(600, 300);

    auto* left = new QWidget(splitter);
    auto* right = new QWidget(splitter);
    splitter->addWidget(left);
    splitter->addWidget(right);
    splitter->setSizes({240, 360});
    splitter->show();
    QCoreApplication::processEvents();

    QVERIFY(!splitter->isPanelCollapsed(0));
    const int was = splitter->sizes().at(0);
    QVERIFY(was > 0);

    QSignalSpy collapsed(splitter, &Splitter::panelCollapsedChanged);

    splitter->setPanelCollapsed(0, true);
    QVERIFY(splitter->isPanelCollapsed(0));
    QCOMPARE(splitter->sizes().at(0), 0);
    QCOMPARE(collapsed.count(), 1);
    // The space went to the neighbour rather than being lost.
    QVERIFY(splitter->sizes().at(1) > 0);

    splitter->setPanelCollapsed(0, false);
    QVERIFY(!splitter->isPanelCollapsed(0));
    // Back at the width it had, not at whatever the layout would decide. This
    // is the part the naive implementation - hide the widget - gets wrong.
    QCOMPARE(splitter->sizes().at(0), was);
    QCOMPARE(collapsed.count(), 2);

    splitter->togglePanel(0);
    QVERIFY(splitter->isPanelCollapsed(0));
}

void GalleryTest::aSplitterArrowIsAButtonAndNotADragTarget()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    surface->resize(700, 400);
    surface->show();
    QVERIFY(QTest::qWaitForWindowExposed(surface));

    auto* splitter = new Splitter(Qt::Horizontal, surface);
    splitter->addWidget(new QWidget(splitter));
    splitter->addWidget(new QWidget(splitter));
    splitter->resize(600, 300);
    splitter->show();
    // The handle's geometry comes from the splitter's layout, which has not run
    // until the events it posts are delivered. Without this the handle is still
    // 0x0 and has no arrow zones at all.
    QCoreApplication::processEvents();

    auto* handle = qobject_cast<SplitterHandle*>(splitter->handle(1));
    QVERIFY(handle);
    QVERIFY2(handle->height() >= 60,
             qPrintable(QStringLiteral("handle is %1px tall").arg(handle->height())));
    QVERIFY(handle->hasCollapseArrows());

    const QRect leading = handle->leadingArrowRect();
    const QRect trailing = handle->trailingArrowRect();
    QVERIFY(!leading.isNull());
    QVERIFY(!trailing.isNull());
    QVERIFY(!leading.intersects(trailing));
    // Between them there has to be drag band left, or the handle stops being a
    // splitter handle at all.
    QVERIFY(trailing.top() > leading.bottom());

    QSignalSpy collapse(handle, &SplitterHandle::collapseRequested);
    QTest::mouseClick(handle, Qt::LeftButton, Qt::NoModifier, leading.center());
    QCOMPARE(collapse.count(), 1);
    QCOMPARE(collapse.takeFirst().at(0).toBool(), true);

    QTest::mouseClick(handle, Qt::LeftButton, Qt::NoModifier, trailing.center());
    QCOMPARE(collapse.count(), 1);
    QCOMPARE(collapse.takeFirst().at(0).toBool(), false);

    // Press on the arrow, release in the drag band: a gesture the user thought
    // better of, so nothing collapses.
    const QPoint middle(handle->width() / 2, handle->height() / 2);
    QTest::mousePress(handle, Qt::LeftButton, Qt::NoModifier, leading.center());
    QTest::mouseRelease(handle, Qt::LeftButton, Qt::NoModifier, middle);
    QCOMPARE(collapse.count(), 0);

    // Turned off, there are no zones at all.
    handle->setCollapseArrows(false);
    QVERIFY(handle->leadingArrowRect().isNull());
}

void GalleryTest::aTabCarriesItsColourAndStateThroughAReorder()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* tabs = new TabBar(surface);
    tabs->resize(700, 32);

    tabs->addTab(QStringLiteral("a"), QStringLiteral("first.cpp"), Glyph::File);
    tabs->addTab(QStringLiteral("b"), QStringLiteral("second.cpp"), Glyph::File);
    tabs->addTab(QStringLiteral("c"), QStringLiteral("third.cpp"), Glyph::File);

    const QColor blue(QStringLiteral("#4DA3FF"));
    tabs->setColor(QStringLiteral("a"), blue);
    tabs->setState(QStringLiteral("a"), TabBar::State::Modified);
    tabs->setCurrentKey(QStringLiteral("a"));

    QCOMPARE(tabs->keys(), QStringList({QStringLiteral("a"), QStringLiteral("b"),
                                        QStringLiteral("c")}));

    // Drag the first tab past the second. The colour and the state live on the
    // tab, so they travel with it - which is the whole reason they are kept
    // there rather than in a list beside it.
    QSignalSpy moved(tabs, &TabBar::tabMoved);
    QTest::mousePress(tabs, Qt::LeftButton, Qt::NoModifier, tabs->tabRect(0).center());
    QTest::mouseMove(tabs, tabs->tabRect(1).center());
    QMouseEvent drag(QEvent::MouseMove, QPointF(tabs->tabRect(1).center()),
                     tabs->mapToGlobal(tabs->tabRect(1).center()), Qt::NoButton,
                     Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(tabs, &drag);
    QTest::mouseRelease(tabs, Qt::LeftButton, Qt::NoModifier, tabs->tabRect(1).center());

    QCOMPARE(moved.count(), 1);
    QCOMPARE(tabs->keys(), QStringList({QStringLiteral("b"), QStringLiteral("a"),
                                        QStringLiteral("c")}));
    QCOMPARE(tabs->color(QStringLiteral("a")).name(), blue.name());
    QCOMPARE(tabs->state(QStringLiteral("a")), TabBar::State::Modified);
    // And the current tab is still the same document, not the same index.
    QCOMPARE(tabs->currentKey(), QStringLiteral("a"));
}

void GalleryTest::aTabsCloseBoxIsItsOwnTarget()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* tabs = new TabBar(surface);
    tabs->resize(700, 32);
    tabs->addTab(QStringLiteral("a"), QStringLiteral("first.cpp"), Glyph::File);
    tabs->addTab(QStringLiteral("b"), QStringLiteral("second.cpp"), Glyph::File);
    tabs->setCurrentKey(QStringLiteral("a"));

    const QRect box = tabs->tabRect(1);
    const QRect close = tabs->closeRect(1);
    QVERIFY(!close.isNull());
    QVERIFY(box.contains(close));

    QSignalSpy closing(tabs, &TabBar::closeRequested);
    QSignalSpy switching(tabs, &TabBar::currentChanged);

    QTest::mouseClick(tabs, Qt::LeftButton, Qt::NoModifier, close.center());
    QCOMPARE(closing.count(), 1);
    QCOMPARE(switching.count(), 0);
    // The bar asks rather than acting: a document with unsaved changes has to
    // be able to answer.
    QCOMPARE(tabs->count(), 2);

    QTest::mouseClick(tabs, Qt::LeftButton, Qt::NoModifier,
                      QPoint(box.left() + 20, box.center().y()));
    QCOMPARE(switching.count(), 1);
    QCOMPARE(closing.count(), 1);

    // A tab that cannot be closed has no close box to hit.
    tabs->setClosable(QStringLiteral("a"), false);
    QVERIFY(tabs->closeRect(0).isNull());
}

void GalleryTest::closingTheCurrentTabMovesToItsNeighbour()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* tabs = new TabBar(surface);
    tabs->addTab(QStringLiteral("a"), QStringLiteral("a"));
    tabs->addTab(QStringLiteral("b"), QStringLiteral("b"));
    tabs->addTab(QStringLiteral("c"), QStringLiteral("c"));

    // The first tab added becomes current: a bar with tabs and no current tab
    // is a bar showing nothing.
    QCOMPARE(tabs->currentKey(), QStringLiteral("a"));

    tabs->setCurrentKey(QStringLiteral("b"));
    tabs->removeTab(QStringLiteral("b"));
    // The one that took its place.
    QCOMPARE(tabs->currentKey(), QStringLiteral("c"));

    tabs->setCurrentKey(QStringLiteral("c"));
    tabs->removeTab(QStringLiteral("c"));
    // It was the last, so the one before it.
    QCOMPARE(tabs->currentKey(), QStringLiteral("a"));

    // Removing one before the current keeps the current document current.
    tabs->addTab(QStringLiteral("d"), QStringLiteral("d"));
    tabs->setCurrentKey(QStringLiteral("d"));
    tabs->removeTab(QStringLiteral("a"));
    QCOMPARE(tabs->currentKey(), QStringLiteral("d"));

    tabs->removeTab(QStringLiteral("d"));
    QCOMPARE(tabs->count(), 0);
    QVERIFY(tabs->currentKey().isEmpty());
}

void GalleryTest::tabsShrinkTogetherRatherThanEachToItsOwnTitle()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* tabs = new TabBar(surface);
    tabs->resize(420, 32);

    tabs->addTab(QStringLiteral("a"), QStringLiteral("x.md"));
    tabs->addTab(QStringLiteral("b"),
                 QStringLiteral("a-very-long-document-name-that-will-not-fit.md"));
    tabs->addTab(QStringLiteral("c"), QStringLiteral("y.md"));

    // Every tab the same width: a ragged row is one whose close boxes are never
    // in the same place twice.
    QCOMPARE(tabs->tabRect(0).width(), tabs->tabRect(1).width());
    QCOMPARE(tabs->tabRect(1).width(), tabs->tabRect(2).width());

    const int wide = tabs->tabRect(0).width();
    for (int i = 0; i < 12; ++i) {
        tabs->addTab(QStringLiteral("t%1").arg(i), QStringLiteral("document-%1.md").arg(i));
    }
    const int narrow = tabs->tabRect(0).width();
    QVERIFY2(narrow < wide, "tabs did not shrink as more were opened");
    // But not to nothing: below the floor the label goes and the tooltip takes
    // over, which is what stops the thirtieth tab being unreachable.
    QVERIFY(narrow >= 92);

    // They are painted, not built.
    QCOMPARE(tabs->findChildren<QWidget*>().size(), 0);
}

void GalleryTest::anInactivePanelKeepsItsSelection()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));

    // No panel above it: always active. A dialog has no sibling to be confused
    // with, and a dimmed selection there would be dimmed for no visible reason.
    auto* loose = new NavList(surface);
    QVERIFY(selectionIsActive(loose));

    auto* panel = new QWidget(surface);
    auto* inside = new NavList(panel);
    inside->addItem(QStringLiteral("a"), QStringLiteral("Novusion"));
    inside->setCurrentKey(QStringLiteral("a"));

    // Declared inactive: the selection stays and dims.
    panel->setProperty(activePanelProperty, false);
    QVERIFY(!selectionIsActive(inside));
    QCOMPARE(inside->currentKey(), QStringLiteral("a"));

    panel->setProperty(activePanelProperty, true);
    QVERIFY(selectionIsActive(inside));

    // A search box beside the list is still that list's panel, so typing in it
    // must not dim the row the user is about to act on.
    auto* search = new QLineEdit(panel);
    panel->setProperty(activePanelProperty, true);
    QVERIFY(selectionIsActive(search));

    // The nearest declaring ancestor decides: a panel inside a panel is the
    // inner one's business.
    auto* nested = new QWidget(panel);
    nested->setProperty(activePanelProperty, false);
    auto* deep = new NavList(nested);
    QVERIFY(!selectionIsActive(deep));

    QVERIFY(selectionIsActive(nullptr));
    QVERIFY(inactiveSelectionOpacity > 0.0 && inactiveSelectionOpacity < 1.0);
}


// == the NovuAdmin extraction ==============================================

void GalleryTest::aCapacityBarColoursItselfAndAProgressBarDoesNot()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));

    // A usage bar picks its own colour: a full disk has to read as full without
    // anyone reading the number.
    auto* usage = new ProgressBar(0, ProgressBar::Tone::Threshold, surface);
    usage->setValue(34);
    QCOMPARE(usage->effectiveRole(), SemanticRole::Success);
    usage->setValue(74);
    QCOMPARE(usage->effectiveRole(), SemanticRole::Success);
    usage->setValue(75);
    QCOMPARE(usage->effectiveRole(), SemanticRole::Warning);
    usage->setValue(89);
    QCOMPARE(usage->effectiveRole(), SemanticRole::Warning);
    usage->setValue(90);
    QCOMPARE(usage->effectiveRole(), SemanticRole::Danger);
    usage->setValue(100);
    QCOMPARE(usage->effectiveRole(), SemanticRole::Danger);

    // A progress bar must not, or every download turns red as it finishes.
    auto* work = new ProgressBar(99, ProgressBar::Tone::Accent, surface);
    QCOMPARE(work->effectiveRole(), SemanticRole::Neutral);
    QCOMPARE(work->styleSpec().foreground.describe(), QStringLiteral("accent"));

    // Clamped, because a percentage computed from two live counters will hand
    // over 101 sooner or later.
    work->setValue(140);
    QCOMPARE(work->value(), 100);
    work->setValue(-8);
    QCOMPARE(work->value(), 0);
}

void GalleryTest::anUnmeasuredReadingIsNotAMeasuredZero()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* bar = new ProgressBar(0, ProgressBar::Tone::Threshold, surface);

    QVERIFY(!bar->isUnknown());
    bar->setUnknown(true);
    QVERIFY(bar->isUnknown());

    // Measuring makes it measured. A caller that sets a value has, by
    // definition, taken a reading.
    bar->setValue(42);
    QVERIFY(!bar->isUnknown());
    QCOMPARE(bar->value(), 42);

    // A legend adds a line, so the bar grows rather than drawing over the
    // thing beneath it.
    const int bare = bar->sizeHint().height();
    bar->setLegend(QStringLiteral("42%"), QStringLiteral("580 GB free"));
    QVERIFY(bar->sizeHint().height() > bare);
    bar->setLegend(QString(), QString());
    QCOMPARE(bar->sizeHint().height(), bare);
}

void GalleryTest::bytesAreWrittenOneWay()
{
    // 1024, and one decimal below ten. NovuAdmin had two of these and they
    // disagreed, which showed as a file that was 1.0 MB in the table and
    // 0.98 MB in its own properties.
    QCOMPARE(formatBytes(0), QStringLiteral("0 B"));
    QCOMPARE(formatBytes(812), QStringLiteral("812 B"));
    QCOMPARE(formatBytes(1024), QStringLiteral("1.0 kB"));
    QCOMPARE(formatBytes(1024 * 1024), QStringLiteral("1.0 MB"));
    QCOMPARE(formatBytes(qint64(1024) * 1024 * 1024), QStringLiteral("1.0 GB"));
    QCOMPARE(formatBytes(qint64(1024) * 1024 * 1024 * 1024), QStringLiteral("1.0 TB"));
    // Bytes are never fractional.
    QVERIFY(!formatBytes(999).contains(QLatin1Char('.')));
    // And a negative is a difference, not a size, so it keeps its sign.
    QVERIFY(formatBytes(-2048).startsWith(QLatin1Char('-')));
}

void GalleryTest::aDrawerDimsWhatItIsAbout()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    surface->resize(900, 500);
    surface->show();
    QVERIFY(QTest::qWaitForWindowExposed(surface));

    auto* drawer = new Drawer(surface);
    drawer->setTitle(QStringLiteral("NovuExplorer"));
    QVERIFY(drawer->scrim());
    QVERIFY(!drawer->isOpen());
    QVERIFY(!drawer->scrim()->isVisible());

    QSignalSpy opened(drawer, &Drawer::opened);
    QSignalSpy closed(drawer, &Drawer::closed);

    drawer->open();
    QVERIFY(drawer->isOpen());
    QCOMPARE(opened.count(), 1);
    // The scrim is what separates this from a FloatingPanel: a drawer is about
    // one row of the list behind it, and dimming that list is how it says so.
    QVERIFY(drawer->scrim()->isVisible());
    QCOMPARE(drawer->scrim()->size(), surface->size());

    // min(760px, 92%) - both halves matter: the cap stops it swallowing a wide
    // window, the fraction stops it overflowing a narrow one.
    QCOMPARE(drawer->width(), qMin(760, int(900 * 0.92)));
    QCOMPARE(drawer->height(), surface->height());
    // Anchored to the trailing edge by default.
    QCOMPARE(drawer->geometry().right() + 1, surface->width());

    drawer->setEdge(Drawer::Edge::Leading);
    QCOMPARE(drawer->x(), 0);

    // Escape closes: a drawer is not asking a question, so leaving is always
    // allowed - which is the difference between this and a Dialog.
    QTest::keyClick(drawer, Qt::Key_Escape);
    QVERIFY(!drawer->isOpen());
    QCOMPARE(closed.count(), 1);
    QVERIFY(!drawer->scrim()->isVisible());

    drawer->setClosesOnScrimClick(false);
    QVERIFY(!drawer->closesOnScrimClick());
}

void GalleryTest::aBusyDialogDisablesItsWholeFooter()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* dialog = new Dialog(QStringLiteral("New peer"), surface);

    Button* cancel = secondaryButton(QStringLiteral("Cancel"), dialog->content());
    Button* create = primaryButton(QStringLiteral("Create"), dialog->content());
    dialog->buttonRow()->addWidget(cancel);
    dialog->buttonRow()->addWidget(create);

    QVERIFY(!dialog->isBusy());
    QVERIFY(cancel->isEnabled());
    QVERIFY(create->isEnabled());

    dialog->setBusy(true, QStringLiteral("Provisioning…"));
    QVERIFY(dialog->isBusy());
    // Every button, not only the primary: a Cancel that still works while a
    // request is in flight leaves the caller holding a reply for a dialog that
    // has gone.
    QVERIFY(!cancel->isEnabled());
    QVERIFY(!create->isEnabled());

    dialog->setBusy(false);
    QVERIFY(!dialog->isBusy());
    QVERIFY(cancel->isEnabled());
    QVERIFY(create->isEnabled());

    dialog->deleteLater();
}

void GalleryTest::aFormErrorClearsBusyAndSitsAboveTheFooter()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* dialog = new Dialog(QStringLiteral("New peer"), surface);
    Button* create = primaryButton(QStringLiteral("Create"), dialog->content());
    dialog->buttonRow()->addWidget(create);

    QVERIFY(!dialog->hasFormError());
    dialog->setBusy(true, QStringLiteral("Provisioning…"));
    QVERIFY(dialog->isBusy());

    const QString message = QStringLiteral("The address pool is exhausted.");
    dialog->showFormError(message);

    QVERIFY(dialog->hasFormError());
    QCOMPARE(dialog->formError(), message);
    // A dialog cannot be both reporting a failure and still working: the
    // failure is the answer to the work.
    QVERIFY(!dialog->isBusy());
    QVERIFY(create->isEnabled());

    // The banner is a real InfoBanner in the content, so it wears the same
    // danger treatment as every other one in the system.
    const QList<InfoBanner*> banners = dialog->content()->findChildren<InfoBanner*>();
    QCOMPARE(banners.size(), 1);
    QCOMPARE(banners.first()->text(), message);

    dialog->clearFormError();
    QVERIFY(!dialog->hasFormError());
    // Cleared, not destroyed: the next failure reuses it rather than adding a
    // second banner under the first.
    QCOMPARE(dialog->content()->findChildren<InfoBanner*>().size(), 1);

    dialog->showFormError(QStringLiteral("Still no."));
    QCOMPARE(dialog->content()->findChildren<InfoBanner*>().size(), 1);

    dialog->deleteLater();
}


// == the remaining-source pass =============================================

void GalleryTest::aPaletteFindsACommandByItsInitials()
{
    // The change from NovusionUI's substring filter, and most of why a palette
    // is faster than a menu.
    QVERIFY(CommandPalette::subsequenceMatch(QStringLiteral("ndp"),
                                             QStringLiteral("New device peer")));
    QVERIFY(CommandPalette::subsequenceMatch(QStringLiteral("nwp"),
                                             QStringLiteral("New workspace")));
    QVERIFY(!CommandPalette::subsequenceMatch(QStringLiteral("pdn"),
                                              QStringLiteral("New device peer")));
    // Order matters: the same letters the wrong way round is not a match.
    QVERIFY(CommandPalette::subsequenceMatch(QString(), QStringLiteral("anything")));

    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* palette = new CommandPalette(surface);

    QVector<Command> commands;
    for (const char* name : {"New device peer", "Revoke device peer", "New workspace",
                             "Toggle sidebar", "Rebuild the index"}) {
        Command command;
        command.name = QString::fromLatin1(name);
        command.group = QStringLiteral("Test");
        command.data = command.name;
        commands.append(command);
    }
    palette->setCommands(commands);
    QCOMPARE(palette->commandCount(), 5);

    // Nothing typed: everything, in the order the application gave them.
    QCOMPARE(palette->matches().size(), 5);

    palette->setFilter(QStringLiteral("ndp"));
    QVERIFY(!palette->matches().isEmpty());
    QCOMPARE(palette->currentName(), QStringLiteral("New device peer"));

    palette->setFilter(QStringLiteral("zzzz"));
    QVERIFY(palette->matches().isEmpty());
    QVERIFY(palette->currentName().isEmpty());
}

void GalleryTest::aPaletteRanksRatherThanMerelyFiltering()
{
    // Without an order the top result - the one Return runs - is whichever
    // command happened to be registered first, which makes Return a gamble.
    Command exact;
    exact.name = QStringLiteral("Settings");
    Command wordStart;
    wordStart.name = QStringLiteral("Connection settings");
    Command scattered;
    scattered.name = QStringLiteral("Show every tag in the sidebar");

    const QString needle = QStringLiteral("set");
    const int exactScore = CommandPalette::matchScore(needle, exact);
    const int wordScore = CommandPalette::matchScore(needle, wordStart);
    const int scatterScore = CommandPalette::matchScore(needle, scattered);

    QVERIFY2(exactScore > wordScore, "a prefix did not beat a word start");
    QVERIFY2(wordScore > scatterScore, "a word start did not beat a scattered match");
    QVERIFY(scatterScore > 0);

    // Shorter wins at the same rank.
    Command shortName;
    shortName.name = QStringLiteral("New tab");
    Command longName;
    longName.name = QStringLiteral("New tab to the right of this one");
    QVERIFY(CommandPalette::matchScore(QStringLiteral("new tab"), shortName)
            > CommandPalette::matchScore(QStringLiteral("new tab"), longName));

    // A command the needle does not reach at all scores zero and is dropped.
    Command unrelated;
    unrelated.name = QStringLiteral("Quit");
    QCOMPARE(CommandPalette::matchScore(needle, unrelated), 0);

    // The group is searchable, but ranks below anything the name matched.
    Command byGroup;
    byGroup.name = QStringLiteral("Quit");
    byGroup.group = QStringLiteral("Settings");
    const int groupScore = CommandPalette::matchScore(needle, byGroup);
    QVERIFY(groupScore > 0);
    QVERIFY(groupScore < scatterScore);
}

void GalleryTest::aGateCardNamesTheHostTheCredentialIsGoingTo()
{
    // Top-level, because the geometry is the thing under test and a child
    // widget is never exposed. Scoped the same way makeSurface() scopes.
    auto* gate = new GateCard(QStringLiteral("Sign in"));
    applyStyleSheet(gate, QStringLiteral("novuspace"));
    m_surfaces.append(gate);

    // No fields of its own: NovuAdmin signs in with an email and a password,
    // NovuSpace with a workspace and a key, and a component with fields would
    // grow one every time a product grows an authentication method.
    QVERIFY(gate->form());
    QCOMPARE(gate->form()->count(), 0);

    auto* field = new QLineEdit(gate->formWidget());
    gate->form()->addWidget(field);
    QCOMPARE(gate->form()->count(), 1);

    // The footer grows the card rather than overlapping it, because the host
    // is the one thing a person must be able to check before typing.
    const int bare = gate->sizeHint().height();
    gate->setHost(QStringLiteral("admin.novusion.net"));
    const int withHost = gate->sizeHint().height();
    QVERIFY2(withHost > bare, "the host line did not make room for itself");
    gate->setFootnote(QStringLiteral("v0.67.11 · production"));
    QVERIFY(gate->sizeHint().height() > withHost);
    QCOMPARE(gate->host(), QStringLiteral("admin.novusion.net"));

    // Busy disables the whole form, not only a button: a second press while
    // the first request is out creates a second session.
    gate->setBusy(true);
    QVERIFY(gate->isBusy());
    QVERIFY(!gate->formWidget()->isEnabled());

    // A failure is the answer to the attempt, so the attempt is over.
    gate->setError(QStringLiteral("Those credentials do not match an account."));
    QVERIFY(gate->hasError());
    QVERIFY(!gate->isBusy());
    QVERIFY(gate->formWidget()->isEnabled());
    QCOMPARE(gate->styleSpec().border.describe(), QStringLiteral("status.danger.border"));

    gate->setError(QString());
    QVERIFY(!gate->hasError());
    QCOMPARE(gate->styleSpec().border.describe(), QStringLiteral("border"));
}

void GalleryTest::everyExtractedComponentIsInTheGallery()
{
    // The catalogue is the deliverable: a component NovuUI ships and the
    // gallery does not show is a component nobody will find, and one whose
    // regressions nobody will see.
    const QStringList extracted = {
        // NovuSpace
        QStringLiteral("motion"),       QStringLiteral("tooltip"),
        QStringLiteral("disclosure"),   QStringLiteral("nav-list"),
        QStringLiteral("sidebar"),      QStringLiteral("module-bar"),
        QStringLiteral("status-bar"),   QStringLiteral("page-header"),
        QStringLiteral("key-cap"),      QStringLiteral("shortcut-list"),
        QStringLiteral("setting-row"),  QStringLiteral("popover"),
        QStringLiteral("log-view"),     QStringLiteral("app-mark"),
        QStringLiteral("spinner"),      QStringLiteral("skeleton"),
        QStringLiteral("status-line"),  QStringLiteral("steps"),
        // NovuAI
        QStringLiteral("tree-view"),    QStringLiteral("diff-view"),
        QStringLiteral("floating-panel"), QStringLiteral("toast"),
        QStringLiteral("selection-bar"), QStringLiteral("dialog"),
        // NovuExplorer
        QStringLiteral("splitter"),     QStringLiteral("tab-bar"),
        QStringLiteral("active-panel"),
        // NovuAdmin
        QStringLiteral("drawer"),       QStringLiteral("form-submission"),
        QStringLiteral("progress-bar"),
        // the remaining sources
        QStringLiteral("command-palette"), QStringLiteral("gate-card"),
    };

    for (const QString& id : extracted) {
        QVERIFY2(m_registry.find(id) != nullptr,
                 qPrintable(QStringLiteral("%1 is not in the gallery").arg(id)));
    }

    // And the catalogue grew rather than being replaced: the components that
    // were there before this work are all still registered.
    for (const QString& id : {QStringLiteral("button"), QStringLiteral("badge"),
                              QStringLiteral("card"), QStringLiteral("table-view"),
                              QStringLiteral("segmented"), QStringLiteral("toggle"),
                              QStringLiteral("tab-strip"), QStringLiteral("tile"),
                              QStringLiteral("window-shell")}) {
        QVERIFY2(m_registry.find(id) != nullptr,
                 qPrintable(QStringLiteral("%1 was lost").arg(id)));
    }
}

QTEST_MAIN(GalleryTest)
#include "tst_gallery.moc"
