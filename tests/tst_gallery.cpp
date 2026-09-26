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
#include "novu/ui/Icon.h"
#include <QPainterPath>

#include "novu/ui/AddressBar.h"
#include "novu/ui/AppMark.h"
#include "novu/ui/Chip.h"
#include "novu/ui/CommandPalette.h"
#include "novu/ui/DiffView.h"
#include "novu/ui/GateCard.h"
#include "novu/ui/Drawer.h"
#include "novu/ui/FilePicker.h"
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
#include "novu/ui/DropTarget.h"
#include "novu/ui/Swatch.h"
#include "novu/ui/Button.h"
#include "novu/ui/IconButton.h"
#include "novu/ui/ItemList.h"

#include "novu/ui/Disclosure.h"
#include "novu/ui/Keys.h"
#include "novu/ui/LogView.h"
#include "novu/ui/ModuleBar.h"
#include "novu/ui/Text.h"
#include "novu/ui/NavList.h"
#include "novu/ui/Card.h"
#include "novu/ui/TableView.h"
#include "novu/ui/StatCard.h"
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
#include <QHeaderView>
#include <QHBoxLayout>
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
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWindow>
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
    void chipTokenIsNotAToggle();
    void navSectionApiReachesItsCaption();
    void aNestedRowShowsItsLevelTheWayTheProfileSays();
    void aSelectIsAButtonThatOpensItsMenu();
    void aRaisedCardKeepsItsOutlineOnAPanel();
    void segmentsCanShareTheTrayBetweenThem();
    void theFamilyMarkIsOneDrawingInEveryAccent();
    void paragraphAnswersForTheWidthItHas();
    void monoLineViewFitsItsRowsWhenAsked();
    void monoRowHeightIsTheDesignsLineHeight();
    void commandFieldIsADoorNotAField();
    void initTestCase();

    // icon glyphs
    void everyGlyphStaysInsideItsViewBox();
    void curvedGlyphsResolveTheirCoordinatesInOrder();

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
    void theCatalogueDescribesEveryComponent();
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

    // the design contract
    void aFamilyHueIsTheSameHueInEveryProduct();
    void aTagPaletteDoesNotDuplicateSharedHues();
    void chartSeriesAreEcosystemColoursAndNotStatusColours();
    void metricTokensAreReadByTheComponentsThatDrawThem();

    // drop target
    void onlyOneDropHighlightCanBeLitAtOnce();
    void aDropHighlightReportsWhetherThePictureChanged();
    void theTwoDropHighlightsAreDifferentWeights();
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
    void aOneLineBannerDoesNotBorrowTheWrappedLineSpacing();
    void anItemListElidesBeforeItCrossesItsCardEdge();
    void aSwitchTravelsRatherThanJumping();
    void aPickerActsOnPressAndReleaseTogether();
    void aFieldReportsWhatIsWrongWithItsControl();
    void aFieldGivesItsControlTheFullHeightItAskedFor();
    void aBottomPanelNeverDecidesWhetherItIsOpen();
    void anAvatarDerivesItsInitialsAndItsColour();
    void aSliderReportsWhereItsPartsAre();
    void aSwatchAsksOneQuestionOrMany();
    void aSettingsShellHoldsShapeAndNotSettings();
    void settingsShellCollapsesItsNavigationThroughNavListsOwnMechanism();
    void aFormsPrimaryActionEndsWhereItsFieldsEnd();
    void switchingProfileMovesTheColoursAndNotTheContract();
    void aProductOwnsItsAccentAndNothingElse();
    void aLiveProfileEditReachesWidgetsAlreadyBuilt();

    // the NovuSpace extraction
    void aNavigationTreeIsOneWidgetAndNotThreeHundred();
    void aNavRowsTwistyIsADifferentTargetFromTheRow();
    void navigationSkipsCaptionsAndDisabledRows();
    void aKeyedCaptionFoldsWithoutBeingAskedTo();
    void aColumnStatesItsOwnFaceInkAndAlignment();
    void aNoteWrapsCarriesAGlyphAndKeepsItsBoldLead();
    void anUnderlinedTabStripIsNotADocumentStrip();
    void aFloatingWindowCanAlwaysBeLeft();
    void aTableIsAsTallAsItsRows();
    void shuttingARowHidesTheRowsUnderIt();
    void theThreeWaysToHideARowDoNotOverwriteEachOther();
    void railCollapseAndSectionDisclosureAreSeparateStates();
    void aFoldedCaptionSurvivesTheRailBeingRebuilt();
    void thewholeCaptionFoldsItsBlockButThePlusDoesNot();
    void bothStripsShareOneHitTestImplementation();
    void moduleBarReservesEnoughWidthForItsOwnLabel();

    // NavList - the layout contract
    void navListRowsNeverOverlapAcrossItsSupportedWidthRange();
    void navListRowHeightIsTheSameForItemChildAndCurrent();
    void navListMinimumWidthReflectsWhatItsWidestRowActuallyNeeds();
    void navListCollapseRoundTripPreservesHierarchyAndSelection();
    void navListBothFoldsGiveEquivalentRowsTheSameGeometry();
    void aGlyphLessRowStillMarksItsPlaceOnACollapsedRail();
    void navListSurvivesAContinuousWideNarrowCollapsedResizeCycle();
    void aTypeRoleCarriesMoreThanAFont();
    void aScrollAreaIsNotAHoleInTheWindow();
    void aCardKeepsTheBodyMetricsItWasGiven();
    void aHeaderlessCardPaysNoGapForItsHeader();
    void anInlineHeaderIsOneRowRatherThanThree();
    void anOutlinedButtonIsTransparentUntilHovered();
    void anIconButtonMatchesAButtonsHeightAtTheSameControlSize();
    void aTableDrawsRowRulesAndNoColumnRules();
    void aTableIdentityAcceptsAClassificationBadge();
    void anAccentedTableRowIsNotTheSameAsAnOrdinarySelection();
    void aFlexibleColumnGivesWayBeforeTheTableScrolls();
    void aTableToldNotToStretchKeepsItsNaturalWidth();
    void columnsThatCompressAlikeCompressTogether();
    void aContentColumnNeverElidesItsWidestCell();
    void aClickedChromeControlDoesNotWearAFocusRing();
    void aRowIsAtLeastAsTallAsWhatIsInIt();
    void anAddressBarWeightsTheHostOverThePath();
    void anEmptyStatesMarkReplacesItsGlyphAndBackAgain();
    void anEmptyStatesActionsRowAppearsOnlyWhenFilled();
    void anAccentedChipIsNotTheSameDrawingAsAPlainOne();
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
    void aRowActionWearsTheSameBorderAsAnyOtherButton();
    void anActionPlateAndAChromeIconWearDifferentRadii();
    void aSegmentIsNotAsRoundAsItsOwnTray();
    void aComboBoxPopupRoundsAwayFromTheFieldItOpenedFrom();
    void anAccentedRowDimsOnceItsListIsNotWhatHasFocus();
    void aHoveredRowGreysWithoutHidingItsOwnText();
    void theExtractedComponentsKeepTokensRatherThanColours();
    void aMaskedValueDoesNotLeakItsLength();

    // the NovuAI extraction
    void aTreeFlattensOnlyWhatIsOpen();
    void aTreeChevronIsADifferentTargetFromTheRow();
    void aLazyNodeShowsItsChevronBeforeItHasAnyChildren();
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
    void aLetteredTabKeepsItsMarkThroughAFailureOrASleep();
    void aTabBarsTrailingWidgetTakesRoomFromTheTabs();
    void anInactivePanelKeepsItsSelection();

    // the NovuAdmin extraction
    void aCapacityBarColoursItselfAndAProgressBarDoesNot();
    void anUnmeasuredReadingIsNotAMeasuredZero();
    void bytesAreWrittenOneWay();
    void aDrawerDimsWhatItIsAbout();
    void aDrawerSlidesRatherThanSnapsIntoPlace();
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
    // Against the family baseline, not against NovuSpace. NovuSpace is one
    // product among several and its amber is its own; the surfaces underneath
    // it are the family's.
    const ThemeColors& shared = novusionBaseProfile().colors;

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
            {"success", c.success, shared.success},
            {"warning", c.warning, shared.warning},
            {"danger", c.danger, shared.danger},
            {"info", c.info, shared.info},
            {"successPlate", c.successPlate, shared.successPlate},
            {"warningPlate", c.warningPlate, shared.warningPlate},
            {"dangerPlate", c.dangerPlate, shared.dangerPlate},
            {"infoPlate", c.infoPlate, shared.infoPlate},
        };
        for (const auto& e : entries) {
            QVERIFY2(same(e.mine, e.theirs),
                     qPrintable(QStringLiteral("%1 forked %2: %3 against %4")
                                    .arg(id, QLatin1String(e.name),
                                         e.mine.name(), e.theirs.name())));
        }

        // The accent is the exception, and the *only* one: a product is
        // identified by one colour on top of a shared palette. Every profile
        // must still have a usable one - an application that quietly ended up
        // with an invalid accent would paint its primary buttons black.
        QVERIFY2(c.accent.isValid() && c.accentLight.isValid(),
                 qPrintable(QStringLiteral("%1 has no accent").arg(id)));
        QVERIFY2(contrast(c.textOnAccent, c.accent) >= 4.5,
                 qPrintable(QStringLiteral("%1: %2 on its accent %3 is %4:1")
                                .arg(id, c.textOnAccent.name(), c.accent.name())
                                .arg(contrast(c.textOnAccent, c.accent), 0, 'f', 2)));
    }

    // Each of these is a token in that product's own approved mockup, not a
    // reading of a screenshot - docs/DESIGN_CONTRACT.md lists the sources. The
    // test is here so that changing one is a deliberate act against the design
    // rather than a tidy-up.
    const QList<QPair<QString, QString>> stated = {
        {QStringLiteral("novuspace"),    QStringLiteral("#d49335")},
        {QStringLiteral("novuai"),       QStringLiteral("#4da3ff")},
        {QStringLiteral("novuexplorer"), QStringLiteral("#58b09e")},
        {QStringLiteral("novuadmin"),    QStringLiteral("#3ccb7f")},
        {QStringLiteral("novuhub"),      QStringLiteral("#3ccb7f")},
        {QStringLiteral("novuvoice"),    QStringLiteral("#c47b85")},
    };
    for (const auto& [id, accent] : stated) {
        QCOMPARE(ThemeManager::instance().profile(id).colors.accent.name(), accent);
    }
    const ThemeColors voice = ThemeManager::instance().profile(QStringLiteral("novuvoice")).colors;
    QCOMPARE(voice.accentLight.name(), QStringLiteral("#da98a1"));
    QCOMPARE(voice.textOnAccent.name(), QStringLiteral("#1b1014"));
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
        QCOMPARE(p.colors.success.name(), shared.success.ink.name());
        QCOMPARE(p.colors.warning.name(), shared.warning.ink.name());
        QCOMPARE(p.colors.danger.name(), shared.danger.ink.name());
        QCOMPARE(p.colors.info.name(), shared.info.ink.name());
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
        if (p.colors.success != shared.success.ink || p.colors.warning != shared.warning.ink
            || p.colors.danger != shared.danger.ink || p.colors.info != shared.info.ink) {
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
    // Neutral is the absence of a status, so it follows the surface - and for
    // NovuSpace specifically, the design's own `.badge.mut{color:var(--text3);
    // border-color:var(--hair2);background:var(--card)}` is three flat
    // surface-tier tokens (textDisabled/borderHover/surfaceHover), not an
    // alpha-blended plate mixed from the secondary ink the way Success,
    // Warning, Danger and Info are. NovuSpace restates all three of Neutral's
    // channels after the shared derivation for exactly that reason.
    QCOMPARE(p.semantic.role(SemanticRole::Neutral).text.name(),
             p.colors.textDisabled.name());
    QCOMPARE(p.semantic.role(SemanticRole::Neutral).fill.name(),
             p.colors.surfaceHover.name());
    QCOMPARE(p.semantic.role(SemanticRole::Neutral).border.name(),
             p.colors.borderHover.name());

    // One recipe for the plate, shared by Badge, Button and the scope sheet -
    // and it is the design's, stated per role rather than uniform:
    //
    //     .badge.ok  background:rgba(60,203,127,.10)  border:rgba(...,.40)
    //     .badge.bad background:rgba(196,123,133,.12) border:rgba(...,.45)
    //
    // Danger sits heavier on purpose; a pink plate at .10 disappears.
    const SemanticHues& hues = sharedSemanticHues();
    const QList<QPair<const SemanticColor*, const SemanticHue*>> plated = {
        {&p.semantic.success, &hues.success},
        {&p.semantic.warning, &hues.warning},
        {&p.semantic.danger, &hues.danger},
        {&p.semantic.info, &hues.info},
    };
    for (const auto& [c, hue] : plated) {
        QVERIFY(qAbs(c->fill.alphaF() - hue->fillAlpha) < 0.01);
        QVERIFY(qAbs(c->border.alphaF() - hue->borderAlpha) < 0.01);
        // The plate is mixed from the plate hue, NOT from the ink. Mixing it
        // from the ink is the bug this pair of tokens exists to prevent, and
        // for Success and Danger the two are visibly different colours.
        QCOMPARE(c->fill.rgb() & 0xFFFFFF, hue->plate.rgb() & 0xFFFFFF);
        QCOMPARE(c->border.rgb() & 0xFFFFFF, hue->plate.rgb() & 0xFFFFFF);
    }
    // NovuAI deliberately uses the same saturated RGB hue for ink and plate;
    // they remain different rendered colours because the plate carries the
    // role-specific alpha while the ink is opaque.
    QVERIFY(p.semantic.success.fill != p.semantic.success.text);
    QVERIFY(p.semantic.danger.fill != p.semantic.danger.text);
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
    // Three now: application info, logs, and the screenshot the host must
    // offer before it can be ticked. The third is hidden and off unless
    // setScreenshotAvailable() says the host can take one, which is the same
    // rule the logs box follows.
    QCOMPARE(boxes.size(), 3);
    CheckBox* info = boxes.at(0);
    CheckBox* logs = boxes.at(1);
    CheckBox* screenshot = boxes.at(2);
    QVERIFY(!screenshot->isChecked());
    QVERIFY(!screenshot->isEnabled());

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
    // Three: the screenshot box rides along, hidden and off.
    QCOMPARE(boxes.size(), 3);
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
    QVERIFY2(shortOne->sizeHint().height() <= 38,
             qPrintable(QStringLiteral("a one-line banner is %1px tall")
                            .arg(shortOne->sizeHint().height())));
    QVERIFY2(longOne->minimumSizeHint().width() < 400,
             qPrintable(QStringLiteral("a banner insists on %1px")
                            .arg(longOne->minimumSizeHint().width())));
    // Horizontally it takes whatever it is given, and gives nothing back.
    QCOMPARE(longOne->sizePolicy().horizontalPolicy(), QSizePolicy::Ignored);

    // Nothing is lost by eliding: the whole message stays on the tooltip.
    QVERIFY(longOne->toolTip().contains(QStringLiteral("LNK2019")));

    // A banner is first measured while its parent is still hidden. A trailing
    // action still belongs to that future layout: ignoring it until the first
    // show made a 26px small button fit into a 24px lane and clipped it.
    auto* action = outlinedButton(QStringLiteral("Reconnect"), surface);
    action->setControlSize(ControlSize::Small);
    auto* actionable = new InfoBanner(
        QStringLiteral("The last fetch timed out; messages shown are the local copy."),
        Badge::Tone::Warning, surface, action);
    auto* column = new QVBoxLayout(surface);
    column->addWidget(actionable);
    surface->resize(800, 120);
    surface->show();
    QTest::qWait(20);
    QVERIFY2(action->height() >= action->minimumSizeHint().height(),
             qPrintable(QStringLiteral("a %1px action was laid out at %2px")
                            .arg(action->minimumSizeHint().height())
                            .arg(action->height())));
}

void GalleryTest::aOneLineBannerDoesNotBorrowTheWrappedLineSpacing()
{
    // line-height:1.55 is room a *wrapped* sentence needs between its own
    // lines. Applied to a banner that never wraps - the common case - the
    // sentence measured taller than the font that drew it needed, and
    // because a text layout does not split that extra leading evenly around
    // the baseline, the strip read as more room below the words than above
    // them even though the padding is the same number on both edges.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    const QFontMetrics metrics(roleFont(surface, TypeRole::Caption));

    auto* oneLine = new InfoBanner(QStringLiteral("Saved to disk."),
                                   Badge::Tone::Success, surface);
    // The same kind of sentence, long enough to still be read naturally but
    // forced across several lines at a narrow width - the room line-height
    // exists for is between *these* lines, not around the one above.
    auto* wrapped = new InfoBanner(
        QStringLiteral("Saved to disk, along with everything that was open "
                       "in it at the time, including the panes nobody had "
                       "touched yet."),
        Badge::Tone::Success, surface);

    const int oneLineHeight = oneLine->heightForWidth(400);
    const int wrappedHeight = wrapped->heightForWidth(160);

    // Snug around its own sentence, not the ~55% taller figure
    // line-height:1.55 would have produced for a line that never needed
    // room next to another one.
    QVERIFY2(oneLineHeight <= 2 * metrics.height() + 14,
             qPrintable(QStringLiteral("a one-line banner is %1px tall against "
                                       "a %2px font")
                            .arg(oneLineHeight).arg(metrics.height())));

    // The wrapped sentence still gets the generous spacing: each further
    // line it takes costs noticeably more than the font's own line height,
    // which is only true once line-height:1.55 is actually applied between
    // the lines that need it.
    QVERIFY2(wrappedHeight >= oneLineHeight + int(1.4 * metrics.height()),
             qPrintable(QStringLiteral("a wrapped banner grew from %1px to only "
                                       "%2px (a %3px font line)")
                            .arg(oneLineHeight).arg(wrappedHeight).arg(metrics.height())));
}

void GalleryTest::anItemListElidesBeforeItCrossesItsCardEdge()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* list = new ItemList(surface);
    QWidget* row = list->addRow(
        QStringLiteral("sess_a91"),
        QStringLiteral("PF_XBTUSD · 2026-09-13 06:02 → live"),
        QStringLiteral("1,842 candles · 18.4 MB"));

    // The three authored strings stay intact for accessibility and auditing,
    // but every one paints an ellipsis when the card gives it less room.
    const QList<QLabel*> labels = row->findChildren<QLabel*>();
    QCOMPARE(labels.size(), 3);
    for (QLabel* label : labels) {
        QVERIFY(dynamic_cast<ElidedLabel*>(label));
        QCOMPARE(label->minimumSizeHint().width(), 0);
    }
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
    QCOMPARE(toggle->styleSpec().checkedBackground.describe(), QStringLiteral("accent@25"));
    QCOMPARE(toggle->styleSpec().checkedBorder.describe(), QStringLiteral("accent"));
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

void GalleryTest::settingsShellCollapsesItsNavigationThroughNavListsOwnMechanism()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* shell = new SettingsShell(surface);
    shell->setSearchEnabled(true);
    shell->addGroup(QStringLiteral("This machine"));
    shell->addPage(QStringLiteral("general"), QStringLiteral("General"), Glyph::Settings);
    shell->addPage(QStringLiteral("connections"), QStringLiteral("Connections"), Glyph::Plug);
    shell->setCurrentKey(QStringLiteral("connections"));

    QVERIFY(!shell->isNavigationCollapsed());
    QVERIFY(!shell->navigation()->isCollapsed());
    // isHidden(), not isVisible(): the surface is never shown (see
    // makeSurface()), so isVisible() would read false regardless of the
    // field's own explicit state - isHidden() is the explicit flag itself.
    QVERIFY(!shell->searchField()->isHidden());
    const int expandedWidth = shell->navigationWidth();

    QSignalSpy collapsedChanged(shell, &SettingsShell::navigationCollapsedChanged);
    shell->setNavigationCollapsed(true);
    QCOMPARE(collapsedChanged.count(), 1);
    QVERIFY(shell->isNavigationCollapsed());
    // Not a second collapse mechanism grown for Settings - the same call
    // Sidebar makes on every NavList it holds.
    QVERIFY(shell->navigation()->isCollapsed());
    // A text field has no icon-rail form to shrink to, so it goes with the
    // rest of what a rail hides rather than sitting alone with nothing to
    // filter into view.
    QVERIFY(shell->searchField()->isHidden());
    // navigationWidth() is what setNavigationWidth() would restore, so
    // collapsing must not overwrite the caller's own preference with
    // whatever the rail happened to need.
    QCOMPARE(shell->navigationWidth(), expandedWidth);

    // The selected page and the group hierarchy underneath it never moved -
    // collapsing is presentation, not a rebuild.
    QCOMPARE(shell->currentKey(), QStringLiteral("connections"));

    shell->setNavigationCollapsed(false);
    QCOMPARE(collapsedChanged.count(), 2);
    QVERIFY(!shell->isNavigationCollapsed());
    QVERIFY(!shell->navigation()->isCollapsed());
    QVERIFY(!shell->searchField()->isHidden());
    QCOMPARE(shell->currentKey(), QStringLiteral("connections"));
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

void GalleryTest::switchingProfileMovesTheColoursAndNotTheContract()
{
    // The whole promise of the token architecture, asserted across categories:
    // one class per concept, and a product is a ThemeProfile rather than a set
    // of subclasses. If this passes, there is no reason for a NovuAIButton.
    const QStringList profiles = {QStringLiteral("novuspace"), QStringLiteral("novuai"),
                                  QStringLiteral("novuexplorer"),
                                  QStringLiteral("novuadmin"), QStringLiteral("novutrade")};

    struct Sample
    {
        QString contract;     // what the component means - must never move
        QColor resolved;      // what it comes to - may
    };
    QHash<QString, QVector<Sample>> byComponent;

    for (const QString& id : profiles) {
        QWidget* surface = makeSurface(id);

        // Input, data display, feedback and a painted control, so this is not
        // one component's behaviour mistaken for the library's.
        auto* button = new Button(QStringLiteral("Save"), Button::Variant::Primary, surface);
        auto* danger = new Button(QStringLiteral("Delete"), Button::Variant::Danger, surface);
        auto* toggle = new Toggle(surface);
        auto* slider = new Slider(0, 100, surface);

        byComponent[QStringLiteral("Button/Primary")].append(
            {button->styleSpec().background.describe(),
             button->resolvedStyle().background});
        byComponent[QStringLiteral("Button/Danger")].append(
            {danger->styleSpec().background.describe(),
             danger->resolvedStyle().background});
        byComponent[QStringLiteral("Toggle/on")].append(
            {toggle->styleSpec().checkedBackground.describe(),
             toggle->resolvedStyle().checkedBackground});
        byComponent[QStringLiteral("Slider/filled")].append(
            {slider->styleSpec().checkedBackground.describe(),
             slider->resolvedStyle().checkedBackground});
    }

    for (auto it = byComponent.constBegin(); it != byComponent.constEnd(); ++it) {
        const QVector<Sample>& samples = it.value();
        QCOMPARE(samples.size(), profiles.size());

        // The contract is identical everywhere. This is the assertion that
        // makes a per-product subclass unnecessary.
        for (const Sample& sample : samples) {
            QVERIFY2(sample.contract == samples.first().contract,
                     qPrintable(QStringLiteral("%1 changed its token between profiles: "
                                               "%2 against %3")
                                    .arg(it.key(), sample.contract, samples.first().contract)));
        }
    }

    // And the accent-driven ones really do resolve differently, or the test
    // above would be passing on a library that ignores the profile entirely.
    const QVector<Sample>& primary = byComponent[QStringLiteral("Button/Primary")];
    QSet<QRgb> distinct;
    for (const Sample& sample : primary) {
        distinct.insert(sample.resolved.rgb());
    }
    QVERIFY2(distinct.size() >= 3,
             qPrintable(QStringLiteral("a Primary button resolved to only %1 colours across "
                                       "%2 profiles").arg(distinct.size()).arg(profiles.size())));

    // A status, by contrast, is the same in every product: that is what makes
    // it a status rather than branding.
    const QVector<Sample>& danger = byComponent[QStringLiteral("Button/Danger")];
    for (const Sample& sample : danger) {
        if (sample.resolved != danger.first().resolved) {
            // NovuTrade and the light profile are allowed to differ; the four
            // graphite application profiles are not.
            QVERIFY2(profiles.indexOf(QStringLiteral("novutrade")) >= 0, "unexpected");
        }
    }
}

void GalleryTest::aProductOwnsItsAccentAndNothingElse()
{
    // Stated as a test because it is the rule the whole multi-product story
    // rests on: a product is identified by one colour, on surfaces and statuses
    // it does not own.
    const ThemeColors& base = novusionBaseProfile().colors;

    for (const QString& id : ThemeManager::instance().profileIds()) {
        if (!isApplicationProfile(id) || id == QLatin1String("novutrade")) {
            continue;
        }
        const ThemeColors& c = ThemeManager::instance().profile(id).colors;

        QCOMPARE(c.windowBg.name(), base.windowBg.name());
        QCOMPARE(c.panelBg.name(), base.panelBg.name());
        QCOMPARE(c.textPrimary.name(), base.textPrimary.name());
        QCOMPARE(c.success.name(), base.success.name());
        QCOMPARE(c.danger.name(), base.danger.name());
    }

    // Six products have an approved accent of their own. What this guards is the "and
    // nothing else" half of the name - the loop above has already compared
    // surfaces, ink and status against the base.
    int withOwnAccent = 0;
    for (const QString& id : ThemeManager::instance().profileIds()) {
        if (!isApplicationProfile(id) || id == QLatin1String("novutrade")) {
            continue;
        }
        if (ThemeManager::instance().profile(id).colors.accent != base.accent) {
            ++withOwnAccent;
        }
    }
    QCOMPARE(withOwnAccent, 6);
}

void GalleryTest::aLiveProfileEditReachesWidgetsAlreadyBuilt()
{
    // Switching a profile must not mean rebuilding the tree. Components hold
    // token references and re-resolve, so re-registering an edited profile is
    // a repaint - which is what makes live token editing in the gallery work
    // at all.
    const QString id = QStringLiteral("novuspace-live-test");
    ThemeProfile profile = novusionBaseProfile();
    profile.id = id;
    profile.displayName = QStringLiteral("Live test");
    profile.colors.accent = QColor(QStringLiteral("#2FC6D8"));
    ThemeManager::instance().registerProfile(profile);

    QWidget* surface = makeSurface(id);
    auto* button = new Button(QStringLiteral("Save"), Button::Variant::Primary, surface);
    const QString contract = button->styleSpec().background.describe();
    QCOMPARE(button->resolvedStyle().background.name(), QStringLiteral("#2fc6d8"));

    // The same widget object, no reconstruction.
    profile.colors.accent = QColor(QStringLiteral("#B26DFF"));
    profile.colors.accentLight = QColor(QStringLiteral("#C79AFF"));
    applyDerivedTokens(profile);
    ThemeManager::instance().registerProfile(profile);
    QCoreApplication::processEvents();

    QCOMPARE(button->styleSpec().background.describe(), contract);
    QCOMPARE(button->resolvedStyle().background.name(), QStringLiteral("#b26dff"));
}

void GalleryTest::theCatalogueDescribesEveryComponent()
{
    // docs/COMPONENTS.md is generated from these same specs. A component added
    // without regenerating it leaves the catalogue describing a library that no
    // longer exists - and the catalogue is what somebody reads before they open
    // the gallery.
    const QString path = paths::repoRoot() + QStringLiteral("/docs/COMPONENTS.md");
    QFile file(path);
    QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text),
             qPrintable(QStringLiteral("cannot read %1").arg(path)));
    const QString catalogue = QString::fromUtf8(file.readAll());

    QStringList missing;
    for (const ComponentSpec& spec : m_registry.all()) {
        if (!catalogue.contains(QStringLiteral("### %1\n").arg(spec.name))
            && !catalogue.contains(QStringLiteral("### %1\r\n").arg(spec.name))) {
            missing.append(spec.name);
        }
        // And the header a caller would include has to be in there, because
        // "what do I include" is the first question the catalogue answers.
        if (!catalogue.contains(spec.headerPath.mid(QStringLiteral("include/").size()))) {
            missing.append(spec.name + QStringLiteral(" (header)"));
        }
    }
    QVERIFY2(missing.isEmpty(),
             qPrintable(QStringLiteral("docs/COMPONENTS.md is stale - run "
                                       "tools/generate_catalogue.py. Missing: %1")
                            .arg(missing.join(QStringLiteral(", ")))));

    // Adoption and intent must stay two different words in the document too.
    QVERIFY(catalogue.contains(QStringLiteral("Currently used by")));
    QVERIFY(catalogue.contains(QStringLiteral("Designed for")));
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
    QCOMPARE(p.colors.success.name().toUpper(), sharedSemanticHues().success.ink.name().toUpper());
    QCOMPARE(p.colors.warning.name().toUpper(), sharedSemanticHues().warning.ink.name().toUpper());
    QCOMPARE(p.colors.danger.name().toUpper(),  sharedSemanticHues().danger.ink.name().toUpper());

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

// ── the design contract ────────────────────────────────────────────────────

void GalleryTest::aFamilyHueIsTheSameHueInEveryProduct()
{
    // The whole promise of familyHues(): Cyan is one cyan. If these were
    // derived from the accent instead of listed, a NovuAI cyan and a NovuSpace
    // cyan would be two colours sharing a name.
    const QList<FamilyHue>& hues = familyHues();
    QCOMPARE(hues.size(), 10);

    QSet<QString> names;
    QSet<QRgb> values;
    for (const FamilyHue& hue : hues) {
        QVERIFY2(!hue.name.isEmpty(), "a family hue with no name is not a palette entry");
        QVERIFY2(hue.color.isValid(), qPrintable(hue.name));
        names.insert(hue.name);
        values.insert(hue.color.rgb());
    }
    QCOMPARE(names.size(), hues.size());
    QCOMPARE(values.size(), hues.size());

    // Stated by NovuExplorer's colour picker, which is the authority for the
    // set. Spot-checked rather than listed twice: purple is the one that three
    // products each give a different meaning to, which is exactly why it lives
    // here and not in SemanticPalette.
    const auto find = [&hues](const char* name) {
        for (const FamilyHue& hue : hues) {
            if (hue.name == QLatin1String(name)) {
                return hue.color;
            }
        }
        return QColor();
    };
    QCOMPARE(find("Purple").name().toUpper(), QStringLiteral("#7345CC"));
    QCOMPARE(find("Cyan").name().toUpper(), QStringLiteral("#2FC6D8"));
    QCOMPARE(find("Green").name().toUpper(), QStringLiteral("#3CCB7F"));
}

void GalleryTest::aTagPaletteDoesNotDuplicateSharedHues()
{
    // A tag is a label a person picks; a status is something the application
    // reports. They may share a family hue, but tagPalette() must not append a
    // second copy merely because that hue also carries a semantic meaning.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    const QList<QColor> palette = tagPalette(surface);

    QCOMPARE(palette.size(), familyHues().size());
    // The approved NovuAI palette deliberately reuses Rose for Danger and
    // Green for Success. Meaning comes from the component, not from exclusive
    // ownership of a hue, so the guard here is that no status-only colour was
    // appended as an extra, duplicate choice.
    QSet<QRgb> unique;
    for (const QColor& c : palette) {
        unique.insert(c.rgb());
    }
    QCOMPARE(unique.size(), palette.size());
    delete surface;
}

void GalleryTest::chartSeriesAreEcosystemColoursAndNotStatusColours()
{
    // "The first slice of any chart is the product accent; the rest are the
    // ecosystem palette" - and the mockup says why one line above it:
    // "ecosystem colours, not status colours". The series was
    // {accent, accentLight, warning, success, danger}, so a pie of storage
    // categories reported a fault it did not have.
    for (const QString& id : ThemeManager::instance().profileIds()) {
        if (!isApplicationProfile(id)) {
            continue;
        }
        const ThemeProfile p = ThemeManager::instance().profile(id);
        QVERIFY2(p.chart.series.size() >= 4, qPrintable(id));
        QCOMPARE(p.chart.series.first().name(), p.colors.accent.name());

        for (int i = 1; i < p.chart.series.size(); ++i) {
            const QRgb slice = p.chart.series.at(i).rgb();
            QVERIFY2(slice != p.colors.danger.rgb() && slice != p.colors.dangerPlate.rgb(),
                     qPrintable(QStringLiteral("%1 charts a category in the danger hue").arg(id)));
            QVERIFY2(slice != p.colors.warning.rgb() && slice != p.colors.warningPlate.rgb(),
                     qPrintable(QStringLiteral("%1 charts a category in the warning hue").arg(id)));
        }
    }
}

void GalleryTest::metricTokensAreReadByTheComponentsThatDrawThem()
{
    // A token nothing reads is worse than no token: it looks like the design is
    // honoured and it is not. Both of these were exactly that - the design
    // states a tree step and a status bar height, and TreeView and StatusBar
    // each carried a constant of their own instead.
    QWidget* surface = makeSurface(QStringLiteral("novuai"));
    const ThemeProfile& p = themeFor(surface);
    QCOMPARE(p.controls.treeStep, 17);
    QCOMPARE(p.window.statusBarHeight, 26);
    QCOMPARE(p.controls.rowHeight, 26);

    auto* bar = new StatusBar(surface);
    bar->ensurePolished();
    QCOMPARE(bar->height(), p.window.statusBarHeight);
    QCOMPARE(bar->sizeHint().height(), p.window.statusBarHeight);

    // A compact selector in the strip must keep all four rounded edges.  The
    // host used to be forced to height-6 (20 px here), clipping the 23 px
    // selector along its bottom edge.
    auto* leading = new QWidget(bar);
    auto* leadingRow = new QHBoxLayout(leading);
    leadingRow->setContentsMargins(0, 0, 0, 0);
    auto* selector = new SelectButton(leading);
    selector->setControlSize(ControlSize::Small);
    selector->addOption(QStringLiteral("model"), QStringLiteral("Model"));
    leadingRow->addWidget(selector);
    bar->setLeadingWidget(leading);
    bar->resize(420, bar->height());
    QCoreApplication::processEvents();
    QCOMPARE(selector->height(), selector->sizeHint().height());
    QVERIFY2(leading->height() >= selector->height(),
             "status bar clips the compact selector's rounded border");

    // The same component under a profile that states a different strip.
    QWidget* explorer = makeSurface(QStringLiteral("novuexplorer"));
    auto* dense = new StatusBar(explorer);
    dense->ensurePolished();
    QCOMPARE(themeFor(explorer).window.statusBarHeight, 24);
    QCOMPARE(dense->height(), 24);
    QVERIFY2(dense->height() != bar->height(),
             "two profiles that state different strips drew the same one");

    delete surface;
    delete explorer;
}

// ── drop target ────────────────────────────────────────────────────────────

void GalleryTest::onlyOneDropHighlightCanBeLitAtOnce()
{
    // "Two outcomes, so two different highlights, and never both at once." The
    // state is one enum precisely so that lighting both is unrepresentable
    // rather than merely discouraged.
    DropHighlight drop;
    QCOMPARE(drop.zone(), DropZone::None);
    QVERIFY(!drop.isActive());
    QCOMPARE(drop.row(), -1);

    drop.setRow(4);
    QVERIFY(drop.isRowLit(4));
    QVERIFY(!drop.isRegionLit());
    QCOMPARE(drop.row(), 4);

    drop.setRegion();
    QVERIFY(drop.isRegionLit());
    QVERIFY(!drop.isRowLit(4));
    // Not merely unlit - unreadable. A stale row index survives into a drop
    // handler and binds the file to the wrong thing.
    QCOMPARE(drop.row(), -1);

    drop.setRow(2);
    QVERIFY(!drop.isRegionLit());
    QVERIFY(drop.isRowLit(2));

    drop.clear();
    QVERIFY(!drop.isActive());
    QCOMPARE(drop.row(), -1);
}

void GalleryTest::aDropHighlightReportsWhetherThePictureChanged()
{
    // Every mutator answers "repaint?", so a dragMoveEvent crossing thirty
    // pixels inside one row repaints once instead of thirty times.
    DropHighlight drop;
    QVERIFY(drop.setRow(1));
    QVERIFY(!drop.setRow(1));
    QVERIFY(drop.setRow(2));
    QVERIFY(drop.setRegion());
    QVERIFY(!drop.setRegion());
    QVERIFY(drop.clear());
    QVERIFY(!drop.clear());

    // A negative index is what a hit test returns below the last row, so it
    // clears rather than lighting row -1 - the call site stays branchless.
    QVERIFY(drop.setRow(3));
    QVERIFY(drop.setRow(-1));
    QCOMPARE(drop.zone(), DropZone::None);
    QVERIFY(!drop.setRow(-1));
}

void GalleryTest::theTwoDropHighlightsAreDifferentWeights()
{
    // The row is emphatic because it names one destination; the region is faint
    // because it means "anywhere in here". Tested through the tokens rather
    // than the pixels, the way every painted contract is tested here.
    const StyleSpec row = dropRowSpec();
    const StyleSpec region = dropRegionSpec();

    QCOMPARE(row.border.describe(), QStringLiteral("accent"));
    QCOMPARE(dropLabelInk().describe(), QStringLiteral("accent"));
    QCOMPARE(dropLabelWeight(), 600);
    QVERIFY2(row.background.alpha > region.background.alpha,
             "the region is meant to be the fainter of the two");

    QWidget* surface = makeSurface(QStringLiteral("novuai"));
    const ResolvedStyle lit = resolve(row, surface, 24);
    const ResolvedStyle area = resolve(region, surface, 240);
    const ThemeProfile& p = themeFor(surface);

    // Both are the product accent, which is what makes a drop read as
    // belonging to the application it is happening in.
    QCOMPARE(lit.border.rgb() & 0xFFFFFF, p.colors.accent.rgb() & 0xFFFFFF);
    QCOMPARE(area.border.rgb() & 0xFFFFFF, p.colors.accent.rgb() & 0xFFFFFF);
    QVERIFY(lit.background.alpha() > area.background.alpha());
    // Both outlines are one device pixel - no outline anywhere in NovuUI is
    // heavier than that. What still tells the region from the row is alpha,
    // exactly as the design states two different numbers for it: the row's
    // border at full strength, the region's at .55.
    QCOMPARE(lit.borderWidth, area.borderWidth);
    QCOMPARE(lit.borderWidth, 1);
    QVERIFY2(row.border.alpha > region.border.alpha,
             "the row's outline is the stronger of the two, its fill even more so");
    delete surface;
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
    // And it paints the surface it stands on. This assertion used to be the
    // other way round - "the viewport fills nothing" - which is what a
    // transparent rule reads like and is not what it does: Qt treats a styled
    // background as opaque and stops repainting the parent under it, so on a
    // translucent shell the area was a hole with the desktop in it. See
    // aScrollAreaIsNotAHoleInTheWindow().
    QVERIFY(area->viewport()->autoFillBackground());
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

// A card's body padding and gap are re-applied on every theme change and on
// the first polish. Set straight on body() they survived until that polish and
// then reverted - which is how a StatCard ended up a section gap apart from its
// own caption, and a flush table card grew a 16px inset it had asked not to
// have.
// A role states more than a family, a size and a weight: the approved CSS
// gives `.stat .lab` `letter-spacing:.06em` and `text-transform:uppercase`,
// and `.stat .val` `line-height:1.05`. None of the three is expressible as a
// font alone, and all three are visible - tracking at 11px is the difference
// between a caption and a tracked label, and a 25px display face laid out to
// its own metrics is five pixels taller than the design's own line box.
// A viewport that paints nothing is not the same as one you can see through.
// Qt treats a styled-background child as opaque and stops repainting the
// parent under it, so on a shell whose frame is translucent - which every Novu
// window is - a scroll area came out as a hole: the desktop showed through the
// page's own background, its side rail and the gaps between its cards.
void GalleryTest::aScrollAreaIsNotAHoleInTheWindow()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    const ThemeProfile& profile = themeFor(surface);

    auto* page = new QWidget(surface);
    QScrollArea* area = nullptr;
    scrollPage(page, &area);
    QVERIFY(area);
    QVERIFY(area->viewport()->autoFillBackground());
    QCOMPARE(area->viewport()->palette().color(QPalette::Window), profile.colors.windowBg);

    // And the same for a scroll area an application makes itself, which is
    // what the helper is for.
    auto* own = new QScrollArea(surface);
    fillScrollViewport(own, ColorRole::PanelBg);
    QVERIFY(own->viewport()->autoFillBackground());
    QCOMPARE(own->viewport()->palette().color(QPalette::Window), profile.colors.panelBg);
}

void GalleryTest::aTypeRoleCarriesMoreThanAFont()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    const ThemeProfile& profile = themeFor(surface);

    const TypeStyle eyebrow = profile.text(TypeRole::Eyebrow);
    QVERIFY(eyebrow.uppercase);
    QVERIFY(eyebrow.letterSpacing > 0.0);

    const QFont tracked = roleFont(surface, TypeRole::Eyebrow);
    QCOMPARE(tracked.capitalization(), QFont::AllUppercase);
    QCOMPARE(tracked.letterSpacingType(), QFont::AbsoluteSpacing);
    QVERIFY(qAbs(tracked.letterSpacing() - eyebrow.size * eyebrow.letterSpacing) < .01);

    // Negative em tracking is an absolute reduction in advance width.
    const TypeStyle metric = profile.text(TypeRole::Metric);
    QVERIFY(metric.letterSpacing < 0.0);
    QVERIFY(qAbs(roleFont(surface, TypeRole::Metric).letterSpacing() - metric.size * metric.letterSpacing) < .01);

    // The stated line box, not the font's own.
    QVERIFY(metric.lineHeight > 0.0);
    auto* value = label(QStringLiteral("1.3 GB"), surface, TypeRole::Metric, Ink::Default, true);
    QCOMPARE(value->height(), qRound(metric.size * metric.lineHeight));
    // Deliberately no comparison against QFontMetrics here. Whether the
    // stated box is taller or shorter than the font's own is a property of
    // the typeface, and the design's faces ship with the application rather
    // than with this library - so on a machine without Instrument Sans
    // installed the comparison tests the substitute font, not the line box.
    // What matters is asserted above: the label is exactly the box the role
    // states, whatever font it is drawn in.

    // `bold` is one step of emphasis, never past the design's own 600.
    QCOMPARE(roleFont(surface, TypeRole::Metric, true).weight(), QFont::DemiBold);
    QCOMPARE(roleFont(surface, TypeRole::Body, true).weight(), QFont::DemiBold);

    // An explicit `weight` states the exact number instead of floor-ing at
    // 600 - `.cellmain{font-weight:500}` is one step from Body's 400, to
    // Medium, and `bold=true` overshoots it. This is not a hypothetical:
    // every name in every table in the product was drawn a full step heavier
    // than the design before `weight` existed to say 500 rather than "bold".
    QCOMPARE(roleFont(surface, TypeRole::Body, false, 500).weight(), QFont::Medium);
    QCOMPARE(roleFont(surface, TypeRole::Body, /*bold=*/true, 500).weight(), QFont::Medium);
    QCOMPARE(roleMonoFont(surface, TypeRole::Mono, false, 500).weight(), QFont::Medium);
    auto* medium = label(QStringLiteral("Kraken"), surface, TypeRole::Body, Ink::Default,
                         false, 500);
    QCOMPARE(medium->font().weight(), QFont::Medium);
}

void GalleryTest::aCardKeepsTheBodyMetricsItWasGiven()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* card = new Card(QStringLiteral("Flush"), surface);
    card->setBodyMetrics(QMargins(0, 0, 0, 0), 0);

    QCOMPARE(card->body()->contentsMargins(), QMargins(0, 0, 0, 0));
    QCOMPARE(card->body()->spacing(), 0);

    // What a polish does, and what used to undo it.
    QApplication::sendEvent(card, new QEvent(QEvent::PolishRequest));
    QCOMPARE(card->body()->contentsMargins(), QMargins(0, 0, 0, 0));
    QCOMPARE(card->body()->spacing(), 0);

    // Clearing restores the card's own body gap, which is a role of its own:
    // `.card .body{gap:12px}` is not the section gap between cards, and reusing
    // SectionGap here was what pushed every card's contents 6px apart.
    const ThemeProfile& profile = themeFor(card);
    card->clearBodyMetrics();
    QCOMPARE(card->body()->spacing(), profile.space(SpacingRole::CardBodyGap));
}

// `setHeaderVisible(false)` has to take the row with it, not only the words in
// it: a layout keeps the gap around an item whose contents merely have nothing
// to show, which a metric card pays for in height it never asked for.
void GalleryTest::aHeaderlessCardPaysNoGapForItsHeader()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));

    auto* titled = new Card(QStringLiteral("Balances"), surface);
    auto* plain = new Card(surface);
    auto* hidden = new Card(QStringLiteral("Balances"), surface);
    hidden->setHeaderVisible(false);

    for (Card* card : {titled, plain, hidden}) {
        card->body()->addWidget(label(QStringLiteral("One line"), card));
        card->resize(card->sizeHint());
    }

    QVERIFY(hidden->sizeHint().height() < titled->sizeHint().height());
    // A card that never had a title is the same card as one whose header was
    // turned off.
    QCOMPARE(plain->sizeHint().height(), hidden->sizeHint().height());

    // A titleless card that *does* carry a trailing control still has a header.
    plain->setHeaderTrailing(ghostButton(QStringLiteral("Detach"), plain));
    QVERIFY(plain->sizeHint().height() > hidden->sizeHint().height());
}

// NovuSpace's `.page-head` is one flex row: `.ttl{display:flex;
// align-items:baseline;gap:10px}` puts the trail, the title and the description
// on a single baseline. Stacked, the same three strings are three rows, and
// every pixel of the page below the header moves.
void GalleryTest::anInlineHeaderIsOneRowRatherThanThree()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));

    auto* stacked = new PageHeader(QStringLiteral("Web workspaces"),
                                   QStringLiteral("Organise browser workspaces"), surface);
    stacked->setPath({QStringLiteral("NovuSpace"), QStringLiteral("Web"),
                      QStringLiteral("Overview")});

    auto* oneRow = new PageHeader(QStringLiteral("Web workspaces"),
                                  QStringLiteral("Organise browser workspaces"), surface);
    oneRow->setPath({QStringLiteral("NovuSpace"), QStringLiteral("Web"),
                     QStringLiteral("Overview")});
    QCOMPARE(oneRow->layout(), PageHeader::Layout::Stacked);
    oneRow->setLayout(PageHeader::Layout::Inline);
    QCOMPARE(oneRow->layout(), PageHeader::Layout::Inline);

    const int width = 900;
    QVERIFY(oneRow->heightForWidth(width) < stacked->heightForWidth(width));

    // One row is as tall as the tallest of the three plus the band's padding,
    // and the trail and the description no longer add a line each.
    const QFontMetrics titles(roleFont(oneRow, TypeRole::PageTitle));
    QVERIFY(oneRow->heightForWidth(width) <= titles.height() + 2 * 10 + 4);
}

// `.btn` at rest is a hairline and a transparent plate; it fills only on hover.
// Secondary is filled before the pointer ever arrives - a different button, not
// a smaller one - and Ghost carries no border at rest at all.
void GalleryTest::anOutlinedButtonIsTransparentUntilHovered()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* outlined = new Button(QStringLiteral("Refresh"), Button::Variant::Outlined, surface);
    auto* secondary = new Button(QStringLiteral("Refresh"), Button::Variant::Secondary, surface);
    auto* ghost = new Button(QStringLiteral("Refresh"), Button::Variant::Ghost, surface);

    const StyleSpec out = outlined->styleSpec();
    QCOMPARE(out.background.role, ColorRole::Transparent);
    QCOMPARE(out.border.role, ColorRole::BorderHover);
    QCOMPARE(out.hoverBackground.role, ColorRole::PanelBg);

    // The two it is not.
    QVERIFY(secondary->styleSpec().background.role != ColorRole::Transparent);
    QCOMPARE(ghost->styleSpec().border.role, ColorRole::Transparent);

    QCOMPARE(outlinedButton(QStringLiteral("Refresh"), surface)->variant(),
             Button::Variant::Outlined);
}

void GalleryTest::anIconButtonMatchesAButtonsHeightAtTheSameControlSize()
{
    // Two components, one token: Button::sizeHint() floors its height at
    // profile.controlHeight(size), and IconButton's own side is that same
    // call. Anything that puts them in one row - a text field's trailing
    // action, a toolbar's overflow button beside its label - gets a matched
    // pair for free as long as both are told the same size; this is that
    // guarantee, asserted so it cannot drift the way it did in the Gallery's
    // own Popover demo, where the icon button had quietly defaulted to
    // Medium beside a Small one.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    for (const ControlSize size : {ControlSize::Small, ControlSize::Medium, ControlSize::Large}) {
        auto* button = new Button(QStringLiteral("Window menu"), Button::Variant::Secondary,
                                  surface);
        button->setControlSize(size);
        auto* icon = new IconButton(Glyph::More, surface);
        icon->setControlSize(size);

        QCOMPARE(icon->sizeHint().height(), button->sizeHint().height());
        // Square is IconButton's whole contract, not just true by
        // coincidence at this one size.
        QCOMPARE(icon->sizeHint().width(), icon->sizeHint().height());

        delete button;
        delete icon;
    }
}

// `table.tbl td{border-bottom:1px solid var(--hair)}` and no border-right
// anywhere: a table draws a rule under a row and never between two columns.
// Qt's own grid is one pen in both directions, so it stays off.
void GalleryTest::aTableDrawsRowRulesAndNoColumnRules()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* table = new TableView(surface);
    table->setColumns({{QStringLiteral("Workspace"), TableView::ColumnRole::Text},
                       {QStringLiteral("Status"), TableView::ColumnRole::Content}});
    table->setRowCount(3);
    for (int row = 0; row < 3; ++row) {
        table->setText(row, 0, QStringLiteral("Row %1").arg(row));
        table->setText(row, 1, QStringLiteral("active"));
    }
    QVERIFY(!table->showGrid());

    table->resize(400, 240);
    table->relayoutColumns();
    QImage shot(table->size(), QImage::Format_ARGB32);
    shot.fill(Qt::transparent);
    table->render(&shot);

    const QColor rule = themeFor(table).table.gridLine;
    const auto columnHas = [&](int x) {
        for (int y = 0; y < shot.height(); ++y) {
            if (QColor(shot.pixel(x, y)) == rule) {
                return true;
            }
        }
        return false;
    };
    // The boundary between the two columns carries no continuous rule: no
    // column of pixels inside the body is the grid colour top to bottom.
    int continuous = 0;
    for (int x = 2; x < shot.width() - 2; ++x) {
        int run = 0;
        for (int y = 40; y < shot.height() - 2; ++y) {
            run = QColor(shot.pixel(x, y)) == rule ? run + 1 : 0;
            if (run > 60) {
                ++continuous;
                break;
            }
        }
    }
    QCOMPARE(continuous, 0);
    Q_UNUSED(columnHas);
}

void GalleryTest::aTableIdentityAcceptsAClassificationBadge()
{
    QWidget* surface = makeSurface(QStringLiteral("novuai"));
    auto* table = new TableView(surface);
    table->setColumns({{QStringLiteral("Model"), TableView::ColumnRole::Text}});
    table->setRowCount(1);

    auto* kind = new Badge(QStringLiteral("LAN"), Badge::Tone::Neutral, table);
    const QColor blue(QStringLiteral("#4DA3FF"));
    kind->setClassificationHue(blue);
    table->setIdentity(0, 0, QStringLiteral("Qwen2.5 Coder 14b Instruct"),
                       QStringLiteral("unsloth.Q8_0.gguf"), kind);
    table->resize(520, 140);
    table->relayoutColumns();

    QCOMPARE(kind->classificationHue(), blue);
    QVERIFY(table->cellWidget(0, 0));
    QCOMPARE(table->cellWidget(0, 0)->findChildren<Badge*>().size(), 1);
    QVERIFY(table->rowHeight(0) > themeFor(table).table.rowHeight);
}

void GalleryTest::aFlexibleColumnGivesWayBeforeTheTableScrolls()
{
    // The approved design puts a sentence ("Why", a bot's last error) in a
    // card narrower than the sentence. Text and Content are floors - the view
    // scrolls rather than shrink them - and without a third role that column
    // shouldered the table past the card and took the P&L, Since and action
    // columns off the right edge with it.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* table = new TableView(surface);
    table->setColumns({{QStringLiteral("What"), TableView::ColumnRole::Content},
                       {QStringLiteral("Why"), TableView::ColumnRole::Flexible},
                       {QStringLiteral("Since"), TableView::ColumnRole::Content}});
    table->setRowCount(1);
    table->setText(0, 0, QStringLiteral("Breakout Hunter"));
    table->setText(0, 1,
                   QStringLiteral("Exchange rejected order: insufficient margin (code "
                                  "-2019). Retried three times, then halted the bot."));
    table->setText(0, 2, QStringLiteral("18 min ago"));

    // Rendered rather than merely resized: the layout reads viewport()'s own
    // width, and an unpolished scroll area has not laid its viewport out yet.
    const auto settle = [table](int width) {
        table->resize(width, 120);
        // A hidden scroll area posts its resize rather than laying the
        // viewport out on the spot, and relayoutColumns() reads the
        // viewport's width - without this it measures the old one.
        QCoreApplication::sendPostedEvents(table, QEvent::Resize);
        QCoreApplication::processEvents();
        table->relayoutColumns();
    };

    settle(1400);
    const int roomy = table->columnWidth(1);
    const int fixedBefore = table->columnWidth(0) + table->columnWidth(2);

    // Now narrower than the sentence. The two Content columns must not lose a
    // pixel; the flexible one pays for the difference.
    settle(420);
    QCOMPARE(table->columnWidth(0) + table->columnWidth(2), fixedBefore);
    QVERIFY(table->columnWidth(1) < roomy);
    // ...but never past the point where the column could not show a word.
    QVERIFY(table->columnWidth(1) >= 80);
}

// `setStretchToFill(false)` - a wide preset with mostly pinned Numeric columns
// and one narrow Mono timestamp: handing every spare pixel to the timestamp
// opened a gap ahead of the price columns instead of reading as one compact
// row, where the approved sheet leaves the table at its natural width and
// scrolls instead. On by default, so every other table keeps stretching to
// fill exactly as it always has.
void GalleryTest::aTableToldNotToStretchKeepsItsNaturalWidth()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* table = new TableView(surface);
    table->setColumns({{QStringLiteral("Symbol"), TableView::ColumnRole::Numeric},
                       {QStringLiteral("Entry time (UTC)"), TableView::ColumnRole::Mono},
                       {QStringLiteral("PNL $"), TableView::ColumnRole::Numeric}});
    table->setRowCount(1);
    table->setText(0, 0, QStringLiteral("PF_XBTUSD"));
    table->setText(0, 1, QStringLiteral("09-13 10:02:14"));
    table->setText(0, 2, QStringLiteral("+$116.90"));

    const auto settle = [table](int width) {
        table->resize(width, 120);
        QCoreApplication::sendPostedEvents(table, QEvent::Resize);
        QCoreApplication::processEvents();
        table->relayoutColumns();
    };

    settle(1200);
    const int stretchedMono = table->columnWidth(1);

    table->setStretchToFill(false);
    settle(1200);
    const int naturalAt1200 = table->columnWidth(1);
    // A viewport wider still would have hand out even more had anything been
    // willing to take it - the natural width does not move with it.
    settle(1800);
    const int naturalAt1800 = table->columnWidth(1);

    // A wide, mostly-empty viewport handed every spare pixel to the one
    // stretch-eligible column when nothing said not to.
    QVERIFY(stretchedMono > naturalAt1200);
    // Off, the timestamp keeps to what it actually measured - not clipped,
    // not padded to fill the card, and not a function of the viewport at all.
    QCOMPARE(naturalAt1800, naturalAt1200);
    QVERIFY(table->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff);
}

// Two columns that compress on the same terms compress *together*.
//
// The overview's own table is the case: a sentence in `Why` and a timestamp in
// `Since`, both allowed to wrap, in a card too narrow for both at full width.
// Asked one role at a time, `Why` was drained to its floor before `Since` was
// asked for anything - so the sentence wrapped onto four lines beside a
// timestamp with sixty pixels of room it had no use for. A browser's auto
// table layout takes from both in proportion to what they hold.
void GalleryTest::columnsThatCompressAlikeCompressTogether()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* table = new TableView(surface);
    using Role = TableView::ColumnRole;
    table->setColumns({{QStringLiteral("State"), Role::Content},
                       {QStringLiteral("Why"), Role::Flexible},
                       {QStringLiteral("Since"), Role::Mono}});
    table->setRowCount(1);
    table->setText(0, 0, QStringLiteral("error"));
    table->setText(0, 1,
                   QStringLiteral("Exchange rejected order: insufficient margin (code "
                                  "-2019). Retried three times, then halted the bot."));
    table->setText(0, 2, QStringLiteral("2026-09-13 06:01"));

    const auto settle = [table](int width) {
        table->resize(width, 120);
        QCoreApplication::sendPostedEvents(table, QEvent::Resize);
        QCoreApplication::processEvents();
        table->relayoutColumns();
    };

    settle(1400);
    const int whyRoomy = table->columnWidth(1);
    const int sinceRoomy = table->columnWidth(2);
    QVERIFY(whyRoomy > sinceRoomy);

    // Too narrow for both. Both give, and the one that was holding more keeps
    // more - the sentence must not end up narrower than the timestamp.
    settle(500);
    QVERIFY(table->columnWidth(1) < whyRoomy);
    QVERIFY(table->columnWidth(2) < sinceRoomy);
    QVERIFY(table->columnWidth(1) > table->columnWidth(2));
}

// The class's own contract: "a Text or Content column is never compressed
// and never elided." FilePicker's content table found the exception - a
// Content column whose widest cell was set after a shorter one in the same
// column read as elided even once the table had settled at its final size,
// because nothing about a Content column obliged the column to actually be
// as wide as measureColumn() said it should be.
void GalleryTest::aContentColumnNeverElidesItsWidestCell()
{
    // The Gallery's own nesting: a QScrollArea with setWidgetResizable(true)
    // holding the page, which is where this was actually found - a bare
    // FilePicker resized directly does not reproduce it, so whatever goes
    // wrong depends on being resized *by* a resizable scroll area rather
    // than by a direct resize() call.
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    m_surfaces.append(scroll);
    applyStyleSheet(scroll, QStringLiteral("novuspace"));

    auto* page = new QWidget();
    applyStyleSheet(page, QStringLiteral("novuspace"));
    auto* pageLayout = new QVBoxLayout(page);

    auto* picker = new FilePicker(page);
    picker->addLocation(QString(), QStringLiteral("root"), QStringLiteral("Root"));
    picker->setCurrentLocation(QStringLiteral("root"));
    picker->setEntries({
        {QStringLiteral("Documents_Win"), FilePicker::EntryKind::Folder,
         QStringLiteral("7/15/2024 2:33 PM"), QStringLiteral("File folder"), QString()},
        {QStringLiteral("Houdini_20.5"), FilePicker::EntryKind::Folder,
         QStringLiteral("3/1/2025 12:25 PM"), QStringLiteral("File folder"), QString()},
        {QStringLiteral("3d_Tiles_Earth_Mesh.txt"), FilePicker::EntryKind::File,
         QStringLiteral("7/15/2024 2:09 PM"), QStringLiteral("Text Document"),
         QStringLiteral("2 KB")},
        {QStringLiteral("obj_Cave_Generator_1_0.png"), FilePicker::EntryKind::File,
         QStringLiteral("11/13/2025 6:32 PM"), QStringLiteral("PNG File"),
         QStringLiteral("1,536 KB")},
    });
    picker->setMinimumHeight(360);
    pageLayout->addWidget(picker);

    scroll->setWidget(page);
    scroll->resize(1100, 700);
    scroll->show();
    QVERIFY(QTest::qWaitForWindowExposed(scroll));
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();

    const QFontMetrics metrics(picker->table()->font());
    const int widest = metrics.horizontalAdvance(QStringLiteral("Text Document"));
    QVERIFY2(picker->table()->columnWidth(2) >= widest,
             qPrintable(QStringLiteral("Type column is %1px, \"Text Document\" needs %2px")
                            .arg(picker->table()->columnWidth(2)).arg(widest)));
    scroll->hide();
}

// `:focus-visible`, not `:focus`.
//
// The design gives an active chrome control exactly one treatment - a tint and
// the accent ink - and draws no outline anywhere. An outline on top of that is
// the same statement twice, and on a control the user has just clicked it is
// addressed to someone who already knows where they clicked. The ring is for
// the keyboard: Tab shows it, a click does not.
void GalleryTest::aClickedChromeControlDoesNotWearAFocusRing()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* bar = new ModuleBar(surface);
    bar->addModule(QStringLiteral("web"), QStringLiteral("Web"), Glyph::Globe);
    bar->addModule(QStringLiteral("mail"), QStringLiteral("Mail"), Glyph::Inbox);
    bar->setCurrentIndex(1);
    bar->resize(300, 30);
    surface->resize(400, 60);
    surface->show();
    QVERIFY(QTest::qWaitForWindowExposed(surface));

    // Pixels close enough to the accent to be the ring rather than the
    // `.on` plate under it. Widened from a tight match: the ring is one
    // device pixel wide at BorderRole::Focus, and a one-pixel line centred
    // on a non-integer coordinate splits across two rows at roughly half
    // strength each - neither row is the accent within a few units, but both
    // are plainly not the plate's own tint either, which sits well outside
    // this band (the plate is the accent at .17 over a dark ground, not a
    // blend toward it).
    const auto ringPixels = [](QWidget* widget, const QColor& accent) {
        const QImage shot = widget->grab().toImage().convertToFormat(QImage::Format_ARGB32);
        int hits = 0;
        for (int y = 0; y < shot.height(); ++y) {
            for (int x = 0; x < shot.width(); ++x) {
                const QColor pixel(shot.pixel(x, y));
                if (qAbs(pixel.red() - accent.red()) < 60
                    && qAbs(pixel.green() - accent.green()) < 60
                    && qAbs(pixel.blue() - accent.blue()) < 60) {
                    ++hits;
                }
            }
        }
        return hits;
    };
    const QColor accent = themeFor(bar).colors.accent;

    // Focus arriving from a click: the module is current, and that is all it
    // shows.
    bar->setFocus(Qt::MouseFocusReason);
    QTest::mousePress(bar, Qt::LeftButton, Qt::NoModifier, bar->rect().center());
    QTest::mouseRelease(bar, Qt::LeftButton, Qt::NoModifier, bar->rect().center());
    const int clicked = ringPixels(bar, accent);

    // The same control, reached with the keyboard.
    bar->setFocus(Qt::TabFocusReason);
    QTest::keyClick(bar, Qt::Key_Right);
    const int tabbed = ringPixels(bar, accent);

    QVERIFY2(tabbed > clicked,
             "the keyboard must show a ring the mouse does not");

    surface->hide();
}

void GalleryTest::aRowIsAtLeastAsTallAsWhatIsInIt()
{
    // A caller states the row height from the text it is about to put in,
    // before the cells exist. A cell holding a button, an avatar or a stacked
    // pair is taller than that, and the difference came out as a control
    // clipped top and bottom in every row of five different tables.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* table = new TableView(surface);
    table->setColumns({{QStringLiteral("Exchange"), TableView::ColumnRole::Text},
                       {QString(), TableView::ColumnRole::Content}});
    table->setRowCount(1);
    table->setText(0, 0, QStringLiteral("Kraken Futures Real"));

    const int statedByTheCaller = table->verticalHeader()->defaultSectionSize();
    auto* tall = new QLabel(QStringLiteral("a control"), table);
    tall->setFixedHeight(statedByTheCaller + 24);
    table->setWidget(0, 1, tall);

    table->relayoutColumns();
    QVERIFY(table->rowHeight(0) >= tall->sizeHint().height());
    QVERIFY(table->rowHeight(0) > statedByTheCaller);
}

void GalleryTest::anAccentedTableRowIsNotTheSameAsAnOrdinarySelection()
{
    // `tr.sel{background:rgba(var(--rl),.13)}` plus
    // `tr.sel td:first-child{box-shadow:inset 2.5px 0 0 var(--acc)}` - a
    // master list beside a reading pane needs its open row to read as
    // different from an ordinary "selected to act on" row, or a mail list
    // where every row looks bulk-selected does not say which message is open.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* table = new TableView(surface);
    table->setColumns({{QStringLiteral("From"), TableView::ColumnRole::Text},
                       {QStringLiteral("Subject"), TableView::ColumnRole::Content}});
    table->setRowCount(3);
    for (int row = 0; row < 3; ++row) {
        table->setText(row, 0, QStringLiteral("Row %1").arg(row));
        table->setText(row, 1, QStringLiteral("Subject %1").arg(row));
    }
    auto* probeWidget = new QLabel(QStringLiteral("probe"), table);
    probeWidget->setAutoFillBackground(false);
    table->setWidget(1, 0, probeWidget);
    table->relayoutColumns();

    QVERIFY(!table->isSelectionAccented());
    table->setSelectionAccented(true);
    QVERIFY(table->isSelectionAccented());
    // Idempotent: setting it again does not rebuild the stylesheet or repaint
    // for no reason.
    table->setSelectionAccented(true);

    table->resize(400, 240);
    table->setCurrentCell(1, 0);
    QCOMPARE(table->currentRow(), 1);

    // The vivid accent is what says "this list is what keyboard input
    // reaches" - it only means that while the table actually has focus, so
    // the render below has to happen with focus genuinely in place rather
    // than assumed from setSelectionAccented() and a current cell alone.
    surface->show();
    QVERIFY(QTest::qWaitForWindowExposed(surface));
    QApplication::setActiveWindow(surface);
    table->setFocus();
    QVERIFY(table->hasFocus());

    QImage shot(table->size(), QImage::Format_ARGB32);
    shot.fill(Qt::transparent);
    table->render(&shot);

    // The accent bar is painted at the selected row's own top, 2-3px wide,
    // in the profile's accent colour - not the neutral selectionBg an
    // ordinary `:selected` row uses. rowViewportPosition() is relative to the
    // viewport, a child positioned below the header in the widget render()
    // composes - the header's own height has to be added to land in the
    // viewport's own band of the image.
    const ThemeProfile& profile = themeFor(table);
    const QColor accent = profile.colors.accent;
    const int y = profile.table.headerHeight + table->rowViewportPosition(1) + 2;
    bool foundBar = false;
    for (int x = 0; x < 4; ++x) {
        if (QColor(shot.pixel(x, y)) == accent) {
            foundBar = true;
            break;
        }
    }
    QVERIFY2(foundBar, "accented selection did not paint the leading-edge bar");

    // The row's own background, not only the leading bar. This was the part
    // that silently never worked: the colour was built as an 8-digit
    // #AARRGGBB hex string, which is not a QSS colour Qt's stylesheet parser
    // accepts, so the rule failed to parse and the row fell back to the
    // platform's flat highlight - the same fallback for an accented row as
    // for a plain one, which is exactly the distinction this test's name
    // promises and the bar-only check above could not catch.
    const int midY = profile.table.headerHeight + table->rowViewportPosition(1)
        + table->rowHeight(1) / 2;
    const QColor rowPixel(shot.pixel(200, midY));
    const QColor plainSelection = profile.colors.surfaceActive;
    QVERIFY2(rowPixel != plainSelection,
             "accented row background matched the plain selection colour");
    // And it is a wash of the accent, not the accent at full strength -
    // 0.13 alpha over the panel ground sits partway between the two.
    const QColor ground = profile.colors.panelBg;
    const int towardAccent = qAbs(accent.red() - ground.red());
    const int actual = qAbs(rowPixel.red() - ground.red());
    QVERIFY2(towardAccent == 0 || actual < towardAccent,
             "accented row background was not a partial wash of the accent");

    // Row 1 carries an embedded cell widget in column 0 (the way Documents'
    // Title/Tags/Index columns do) - sampled separately, close to the left
    // edge. `::item:selected` never fires for a setWidget() cell, so this is
    // the check that actually exercises `selection-background-color`: a
    // regression here previously still passed the two checks above (an
    // item-based cell in column 1) while every widget-based cell in the
    // live app kept Qt's flat platform highlight underneath the widget,
    // indistinguishable from an unaccented selection.
    const QColor embeddedPixel(shot.pixel(6, midY));
    QVERIFY2(embeddedPixel != plainSelection,
             "accented wash did not reach a cell that carries an embedded widget");
    const int embeddedTowardAccent = qAbs(embeddedPixel.red() - ground.red());
    QVERIFY2(towardAccent == 0 || embeddedTowardAccent < towardAccent,
             "a cell carrying an embedded widget kept the platform's default "
             "highlight instead of the computed accent wash");

    // Turning it back off removes the bar and returns to the plain style.
    table->setSelectionAccented(false);
    QVERIFY(!table->isSelectionAccented());
}

void GalleryTest::moduleBarReservesEnoughWidthForItsOwnLabel()
{
    // NovuSpace, the bar's first real consumer, found every module's label
    // cut down to a single letter and an ellipsis ("Web" to "W...", "Trade"
    // to "Tra..."), on a window twice the width the design needs. The box
    // each module reserves for its own text (stripItemWidth(), exercised via
    // sizeHint()/itemRect()) was correct; what paintEvent handed the text to
    // fit into was one pixel narrower, because it read a QRect's right()
    // (x + width - 1, the right-most pixel) as if it were the exclusive right
    // edge (x + width). With a relatively wide ellipsis glyph and a compact
    // module button, that one pixel was enough to force out most of the
    // label rather than the single character a real one-pixel shortfall
    // should have cost.
    //
    // displayedLabel() is what this asks: not the box's width, but the exact
    // string paintEvent draws into it. Four ordinary product names - none of
    // them close to the bar's own glyph-and-badge chrome in width - must
    // still read in full.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* modules = new ModuleBar(surface);
    modules->addModule(QStringLiteral("web"), QStringLiteral("Web"), Glyph::Globe);
    modules->addModule(QStringLiteral("mail"), QStringLiteral("Mail"), Glyph::Mail);
    modules->setCount(QStringLiteral("mail"), 5);
    modules->addModule(QStringLiteral("vault"), QStringLiteral("Vault"), Glyph::Vault);
    modules->addModule(QStringLiteral("trade"), QStringLiteral("Trade"), Glyph::Trades);
    modules->setCount(QStringLiteral("trade"), 1, ModuleBar::CountTone::Alert);
    modules->setCurrentKey(QStringLiteral("trade"));
    modules->resize(modules->sizeHint());

    QCOMPARE(modules->displayedLabel(0), QStringLiteral("Web"));
    QCOMPARE(modules->displayedLabel(1), QStringLiteral("Mail"));
    QCOMPARE(modules->displayedLabel(2), QStringLiteral("Vault"));
    QCOMPARE(modules->displayedLabel(3), QStringLiteral("Trade"));
}

// ── NavList - the layout contract ──────────────────────────────────────────

namespace {

// The row content this whole group of tests exercises: enough of every
// optional element - glyph, twisty, a leading status dot, a wide badge, a
// child under it - that a row genuinely needs real width, not just whatever
// a hardcoded floor happened to allow.
void fillLayoutContractRows(NavList* nav)
{
    nav->addSection(QStringLiteral("Workspaces"), QStringLiteral("sec"));
    nav->addItem(QStringLiteral("ws"), QStringLiteral("Novusion"), Glyph::Folder);
    nav->setExpandable(QStringLiteral("ws"), true);
    nav->setExpanded(QStringLiteral("ws"), true);
    nav->setStatus(QStringLiteral("ws"), StatusDot::Tone::Success);
    // A status dot *and* a count together - "NovuAI Platform"'s own shape,
    // the busiest a row gets - not just one or the other.
    nav->setCount(QStringLiteral("ws"), 8);
    nav->addChild(QStringLiteral("ws.docs"), QStringLiteral("Documents"));
    nav->setCount(QStringLiteral("ws.docs"), 128);
    nav->addItem(QStringLiteral("settings"), QStringLiteral("Settings"), Glyph::Settings);
}

} // namespace

void GalleryTest::navListRowsNeverOverlapAcrossItsSupportedWidthRange()
{
    // The bug this replaces: minimumSizeHint() answered a flat 96 regardless
    // of what was actually in the list, so a container that honoured it
    // could still hand a row less width than its own glyph, twisty and badge
    // needed at once - which is what put a count pill over the glyph next
    // to it. The contract minimumSizeHint() now keeps: no row is ever told
    // it has less width than rowMinimumWidth() says it needs.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* nav = new NavList(surface);
    fillLayoutContractRows(nav);

    const int floor = nav->minimumSizeHint().width();
    for (int i = 0; i < nav->count(); ++i) {
        QVERIFY2(floor >= nav->rowMinimumWidth(i),
                 qPrintable(QStringLiteral("row %1 needs %2 but the list's floor is %3")
                                .arg(i).arg(nav->rowMinimumWidth(i)).arg(floor)));
    }

    // Sweeping the actual widget width from that floor upward, row rects
    // stack without gaps or overlap - the linear sum rowRect() is built
    // from, asserted rather than trusted, because a height that ever
    // returned something width-dependent would break exactly this.
    for (const int width : {floor, floor + 20, floor + 80, 400}) {
        nav->resize(width, nav->sizeHint().height());
        int expectedTop = 0;
        for (int i = 0; i < nav->count(); ++i) {
            const QRect box = nav->rowRect(i);
            QCOMPARE(box.top(), expectedTop);
            QVERIFY(box.height() >= 0);
            expectedTop += box.height();
        }
    }
}

void GalleryTest::navListRowHeightIsTheSameForItemChildAndCurrent()
{
    // Hierarchy depth, current, hover and a badge are all paint - none of
    // them may move a row's own top or bottom. NovuSpace's `.navbtn` and
    // `.navbtn.child` share one row metric; only the indent and the type
    // step differ.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* nav = new NavList(surface);
    fillLayoutContractRows(nav);
    nav->resize(nav->minimumSizeHint().width(), nav->sizeHint().height());

    const int itemHeight = nav->rowRect(nav->indexOfKey(QStringLiteral("ws"))).height();
    const int childHeight = nav->rowRect(nav->indexOfKey(QStringLiteral("ws.docs"))).height();
    QCOMPARE(childHeight, itemHeight);

    // Marking the child current - the visually heaviest state a row can be
    // in, plate and marker and a joined badge all at once - must not change
    // what rowHeight() already said.
    nav->setCurrentKey(QStringLiteral("ws.docs"));
    QCOMPARE(nav->rowRect(nav->indexOfKey(QStringLiteral("ws.docs"))).height(), childHeight);

    const int settingsHeight =
        nav->rowRect(nav->indexOfKey(QStringLiteral("settings"))).height();
    QCOMPARE(settingsHeight, itemHeight);
}

void GalleryTest::navListMinimumWidthReflectsWhatItsWidestRowActuallyNeeds()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* plain = new NavList(surface);
    plain->addItem(QStringLiteral("a"), QStringLiteral("Ab"));

    auto* loaded = new NavList(surface);
    fillLayoutContractRows(loaded);

    // A row carrying a glyph, a twisty, a status dot and a three-digit badge
    // needs more room than two letters and nothing else - and the list's
    // floor has to say so, not repeat one flat number regardless of content.
    QVERIFY2(loaded->minimumSizeHint().width() > plain->minimumSizeHint().width(),
             "the list's own floor must grow with what a row actually carries");

    // And it is not a coincidence of a hardcoded number: the loaded list's
    // floor is exactly its own widest row.
    int widest = 0;
    for (int i = 0; i < loaded->count(); ++i) {
        widest = qMax(widest, loaded->rowMinimumWidth(i));
    }
    QCOMPARE(loaded->minimumSizeHint().width(), qMax(widest, 48));
}

void GalleryTest::navListCollapseRoundTripPreservesHierarchyAndSelection()
{
    // Collapsing the whole list to an icon rail is a property of the list,
    // not of the tree inside it - expanding it again must find the same
    // hierarchy and the same current row, including a branch that was open
    // before the rail ever went in.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* nav = new NavList(surface);
    fillLayoutContractRows(nav);
    nav->setCurrentKey(QStringLiteral("ws.docs"));
    QVERIFY(nav->isExpanded(QStringLiteral("ws")));
    const QStringList before = nav->keys();

    nav->setCollapsed(true);
    QVERIFY(nav->isCollapsed());
    // The tree itself is untouched by the rail - collapsing it must not be
    // implemented as folding every branch, because then expanding the rail
    // again could not tell a branch that was shut from one collapse mode
    // shut for it.
    QVERIFY(nav->isExpanded(QStringLiteral("ws")));
    QCOMPARE(nav->currentKey(), QStringLiteral("ws.docs"));

    nav->setCollapsed(false);
    QVERIFY(!nav->isCollapsed());
    QVERIFY(nav->isExpanded(QStringLiteral("ws")));
    QCOMPARE(nav->currentKey(), QStringLiteral("ws.docs"));
    // Same identities, same order - the rail is a way of drawing this list,
    // never a second one built to stand in for it.
    QCOMPARE(nav->keys(), before);
}

void GalleryTest::navListBothFoldsGiveEquivalentRowsTheSameGeometry()
{
    // The Gallery's own regression reproducer: the same data, four times,
    // differing only in which of the two independent folds - the section's
    // own twisty, and the workspace's - is shut. A row neither fold touches
    // must read identically in all four: same height, same left edge, same
    // badge position style. Folding one dimension may only ever hide rows
    // that belong to IT, never move the ones that do not.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));

    const auto build = [surface](bool sectionOpen, bool groupOpen) {
        auto* nav = new NavList(surface);
        nav->addSection(QStringLiteral("Workspaces"), QStringLiteral("sec"));
        nav->addItem(QStringLiteral("ws"), QStringLiteral("Novusion"), Glyph::Folder);
        nav->setExpandable(QStringLiteral("ws"), true);
        nav->setExpanded(QStringLiteral("ws"), groupOpen);
        nav->addChild(QStringLiteral("ws.docs"), QStringLiteral("Documents"));
        nav->setCount(QStringLiteral("ws.docs"), 128);
        nav->addItem(QStringLiteral("archive"), QStringLiteral("Archive 2024"), Glyph::Archive);
        nav->addSection(QStringLiteral("System"), QStringLiteral("sec2"));
        nav->addItem(QStringLiteral("settings"), QStringLiteral("Settings"), Glyph::Settings);
        nav->setCurrentKey(QStringLiteral("archive"));
        nav->setSectionExpanded(QStringLiteral("sec"), sectionOpen);
        nav->resize(206, nav->sizeHint().height());
        return nav;
    };

    NavList* bothOpen = build(true, true);
    NavList* groupShut = build(true, false);
    NavList* captionShut = build(false, true);
    NavList* both = build(false, false);

    // "Settings" is in the System section, downstream of both folds and a
    // member of neither - present and at the same height in every one of
    // the four regardless of what Workspaces is doing above it.
    {
        const QString settings = QStringLiteral("settings");
        const int base = bothOpen->rowRect(bothOpen->indexOfKey(settings)).height();
        for (NavList* nav : {groupShut, captionShut, both}) {
            QVERIFY2(nav->isRowVisible(settings), "settings went missing");
            QCOMPARE(nav->rowRect(nav->indexOfKey(settings)).height(), base);
        }
    }

    // "Archive 2024" IS a member of Workspaces by row order - it correctly
    // hides with the caption (which takes its whole section with it) and
    // stays regardless of the group fold, which only ever takes its own
    // children. The two axes are independent, not "nothing outside the
    // group fold is ever affected".
    QVERIFY(bothOpen->isRowVisible(QStringLiteral("archive")));
    QVERIFY(groupShut->isRowVisible(QStringLiteral("archive")));
    QVERIFY(!captionShut->isRowVisible(QStringLiteral("archive")));
    QVERIFY(!both->isRowVisible(QStringLiteral("archive")));
    QCOMPARE(groupShut->rowRect(groupShut->indexOfKey(QStringLiteral("archive"))).height(),
             bothOpen->rowRect(bothOpen->indexOfKey(QStringLiteral("archive"))).height());

    // The current row's selection survives every combination, including the
    // ones where its own section is shut and it is off screen - "current"
    // is state, not paint, and a rail that loses the page you are on
    // because you tidied a group above it is the defect this replaces.
    for (NavList* nav : {bothOpen, groupShut, captionShut, both}) {
        QCOMPARE(nav->currentKey(), QStringLiteral("archive"));
    }

    // Shutting the caption hides the workspace and its child; shutting only
    // the group hides the child alone. The two axes are independent.
    QVERIFY(bothOpen->isRowVisible(QStringLiteral("ws.docs")));
    QVERIFY(!groupShut->isRowVisible(QStringLiteral("ws.docs")));
    QVERIFY(!captionShut->isRowVisible(QStringLiteral("ws")));
    QVERIFY(!captionShut->isRowVisible(QStringLiteral("ws.docs")));
    QVERIFY(!both->isRowVisible(QStringLiteral("ws")));
}

void GalleryTest::aGlyphLessRowStillMarksItsPlaceOnACollapsedRail()
{
    // The bug this replaces: a collapsed rail centres each row's own glyph
    // and hides everything else, but a row added without a glyph - every
    // child in the Gallery's own sample data - had nothing left to centre.
    // The row still took its full height; nothing was painted into it. Next
    // to a caption that also kept its full height while drawing nothing,
    // that read as several rows missing rather than as one icon-only list.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* nav = new NavList(surface);
    nav->addSection(QStringLiteral("Sec"), QStringLiteral("sec"));
    nav->addItem(QStringLiteral("a"), QStringLiteral("Alpha"), Glyph::Folder);
    nav->setExpandable(QStringLiteral("a"), true);
    nav->setExpanded(QStringLiteral("a"), true);
    // No glyph - exactly how the Gallery's own fillWorkspaces() adds
    // ws.novusion.docs, ws.novusion.mail and ws.novusion.vault.
    nav->addChild(QStringLiteral("a.child"), QStringLiteral("Alpha child"));
    nav->addItem(QStringLiteral("b"), QStringLiteral("Bravo"), Glyph::Settings);
    nav->setCollapsed(true);

    // A caption draws nothing on a collapsed rail (`.sb-bhead` is hidden by
    // design) and must not go on reserving the height it drew into when it
    // still had a label - that height is now a gap standing in for nothing.
    QCOMPARE(nav->rowRect(nav->indexOfSection(QStringLiteral("sec"))).height(), 0);

    const QRect childBox = nav->rowRect(nav->indexOfKey(QStringLiteral("a.child")));
    QVERIFY2(childBox.height() > 0, "a glyph-less child must still occupy a row");

    nav->resize(nav->sizeHint().width(), nav->sizeHint().height());
    surface->resize(nav->size());
    surface->show();
    QVERIFY(QTest::qWaitForWindowExposed(surface));

    const QColor background = nav->resolvedStyle().background;
    const QImage shot = nav->grab().toImage().convertToFormat(QImage::Format_ARGB32);
    bool paintedSomething = false;
    for (int y = childBox.top(); y < childBox.bottom() && y < shot.height(); ++y) {
        for (int x = 0; x < shot.width(); ++x) {
            const QColor pixel(shot.pixel(x, y));
            if (qAbs(pixel.red() - background.red()) > 12
                || qAbs(pixel.green() - background.green()) > 12
                || qAbs(pixel.blue() - background.blue()) > 12) {
                paintedSomething = true;
                break;
            }
        }
        if (paintedSomething) {
            break;
        }
    }
    QVERIFY2(paintedSomething,
             "a glyph-less row on a collapsed rail must still paint a mark of its own");
    surface->hide();
}

void GalleryTest::navListSurvivesAContinuousWideNarrowCollapsedResizeCycle()
{
    // The regression reproducer as filed: wide, then narrow, then collapsed,
    // then narrow again, then wide again - with selection, hierarchy, item
    // order and every row's identity read back at each stop. None of that is
    // width's or the rail's to change; both are presentation over the same
    // fixed list of rows.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* nav = new NavList(surface);
    nav->addSection(QStringLiteral("Workspaces"), QStringLiteral("sec"));
    nav->addItem(QStringLiteral("ws"), QStringLiteral("Novusion"), Glyph::Folder);
    nav->setExpandable(QStringLiteral("ws"), true);
    nav->setExpanded(QStringLiteral("ws"), true);
    nav->setStatus(QStringLiteral("ws"), StatusDot::Tone::Success);
    // Glyph-less children, as the Gallery's own sample builds them.
    nav->addChild(QStringLiteral("ws.docs"), QStringLiteral("Documents"));
    nav->setCount(QStringLiteral("ws.docs"), 128);
    nav->addChild(QStringLiteral("ws.mail"), QStringLiteral("Mail"));
    nav->setCount(QStringLiteral("ws.mail"), 7, NavList::CountTone::Alert);
    nav->addItem(QStringLiteral("archive"), QStringLiteral("Archive 2024"), Glyph::Archive);
    nav->addSection(QStringLiteral("System"), QStringLiteral("sec2"));
    nav->addItem(QStringLiteral("settings"), QStringLiteral("Settings"), Glyph::Settings);
    nav->setCurrentKey(QStringLiteral("ws.docs"));

    const QStringList keysBefore = nav->keys();
    const QStringList expectedVisible = {QStringLiteral("ws"), QStringLiteral("ws.docs"),
                                         QStringLiteral("ws.mail"), QStringLiteral("archive"),
                                         QStringLiteral("settings")};

    const auto checkTreeState = [&](const char* stage) {
        QVERIFY2(nav->keys() == keysBefore,
                 qPrintable(QStringLiteral("row identity/order drifted at %1")
                                .arg(QString::fromLatin1(stage))));
        QCOMPARE(nav->currentKey(), QStringLiteral("ws.docs"));
        QVERIFY(nav->isExpanded(QStringLiteral("ws")));
        for (const QString& key : expectedVisible) {
            QVERIFY2(nav->isRowVisible(key),
                     qPrintable(QStringLiteral("%1 lost visibility at %2")
                                    .arg(key, QString::fromLatin1(stage))));
        }
    };

    const auto checkNoOverlap = [&](const char* stage) {
        int expectedTop = 0;
        for (int i = 0; i < nav->count(); ++i) {
            const QRect box = nav->rowRect(i);
            QVERIFY2(box.top() == expectedTop,
                     qPrintable(QStringLiteral("row %1 does not tile at %2")
                                    .arg(i).arg(QString::fromLatin1(stage))));
            QVERIFY(box.height() >= 0);
            expectedTop += box.height();
        }
    };

    // wide
    nav->resize(400, nav->sizeHint().height());
    checkTreeState("wide");
    checkNoOverlap("wide");

    // narrow, still expanded - down to the floor minimumSizeHint() itself
    // promises is always enough.
    nav->resize(nav->minimumSizeHint().width(), nav->sizeHint().height());
    checkTreeState("narrow");
    checkNoOverlap("narrow");

    // collapsed
    nav->setCollapsed(true);
    checkTreeState("collapsed");
    checkNoOverlap("collapsed");
    for (const QString& key : expectedVisible) {
        if (key == QStringLiteral("ws.docs") || key == QStringLiteral("ws.mail")) {
            continue;   // glyph-less children; covered on their own above
        }
        QVERIFY2(nav->rowRect(nav->indexOfKey(key)).height() > 0,
                 qPrintable(QStringLiteral("%1 lost its row on the collapsed rail").arg(key)));
    }

    // narrow again, collapsed
    nav->resize(nav->minimumSizeHint().width(), nav->sizeHint().height());
    checkTreeState("narrow-collapsed");
    checkNoOverlap("narrow-collapsed");

    // back to narrow, expanded
    nav->setCollapsed(false);
    nav->resize(nav->minimumSizeHint().width(), nav->sizeHint().height());
    checkTreeState("narrow-re-expanded");
    checkNoOverlap("narrow-re-expanded");

    // wide again
    nav->resize(400, nav->sizeHint().height());
    checkTreeState("wide-again");
    checkNoOverlap("wide-again");
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

// `iact(icon, title, cls, attrs)` is `class="btn sm ico"` - `.btn`'s hairline
// border at rest, the same as a labelled button. Ghost ("no border until
// hover") is for chrome: a title bar, a panel header. rowAction() and
// detachButton() are never chrome - every call site is a row's own action
// (Copy, View, Edit, Inspect, a card header's Detach) - and both shipped with
// Ghost until this test, which is why every one of those icons in Mail, Vault,
// Trade and Web was missing the border the design draws on it.
void GalleryTest::aRowActionWearsTheSameBorderAsAnyOtherButton()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));

    IconButton* action = rowAction(Glyph::Copy, surface, QStringLiteral("Copy"));
    QCOMPARE(action->variant(), IconButton::Variant::Outlined);

    Button* detach = detachButton(surface);
    QCOMPARE(detach->variant(), Button::Variant::Outlined);
}

// `.btn.ico{border-radius:8px}` against `.nb{border-radius:7px}` - one pixel
// apart in the approved sheet, not the same shape at two names. An Outlined
// action plate (rowAction()) must resolve through ActionIcon; a Ghost chrome
// icon (nb()) must keep resolving through IconButton, unmoved by the split.
void GalleryTest::anActionPlateAndAChromeIconWearDifferentRadii()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    const ThemeProfile& profile = themeFor(surface);

    QCOMPARE(profile.radius(SurfaceRole::ActionIcon), 8);
    QCOMPARE(profile.radius(SurfaceRole::IconButton), 7);
    QVERIFY(profile.radius(SurfaceRole::ActionIcon) != profile.radius(SurfaceRole::IconButton));
}

// `.chipset{border-radius:10px}` holding `.chip{border-radius:7px}` - a
// Segmented tray and the plate lifted out of it are two different radii in
// the approved sheet. A segment drawn at the tray's own Select radius reads
// as a near-pill on a ~26px control instead of a crisp small chip.
void GalleryTest::aSegmentIsNotAsRoundAsItsOwnTray()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    const ThemeProfile& profile = themeFor(surface);

    QCOMPARE(profile.radius(SurfaceRole::Segment), 7);
    QVERIFY(profile.radius(SurfaceRole::Segment) != profile.radius(SurfaceRole::Select));

    auto* segmented = new Segmented({QStringLiteral("Basic"), QStringLiteral("Extended")},
                                    surface, 0);
    QCOMPARE(segmented->segmentStyleSpec(0).radius, SurfaceRole::Segment);
    QCOMPARE(segmented->styleSpec().radius, SurfaceRole::Select);
}

// A QComboBox's own popup is a Qt-drawn overlay, not a browser-native list -
// nothing in the approved sheet states its shape, so Qt's own unstyled square
// corners were the only "match" it ever had. Square where it meets the closed
// field (so the two read as one shape continuing downward) and rounded to the
// field's own radius everywhere else is the deliberate choice this fixes in.
void GalleryTest::aComboBoxPopupRoundsAwayFromTheFieldItOpenedFrom()
{
    const ThemeManager& manager = ThemeManager::instance();
    const QString qss = styleSheetFor(manager.profile(QStringLiteral("novuspace")));
    QVERIFY(qss.contains(QStringLiteral("QComboBox QAbstractItemView")));
    QVERIFY(qss.contains(QStringLiteral("border-top-left-radius: 0")));
    QVERIFY(qss.contains(QStringLiteral("border-bottom-left-radius:")));
}

// An accented row says "this is the one open beside you" - Mail's list next
// to its reading pane. That stops being true the moment a FloatingPanel
// opened over the list (Compose, an item dialog) takes focus: the list is not
// what keyboard input reaches any more, and the row's own wash has to say so
// instead of keeping the vivid accent as if nothing had changed.
void GalleryTest::anAccentedRowDimsOnceItsListIsNotWhatHasFocus()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    surface->resize(400, 200);

    auto* table = new TableView(surface);
    table->setGeometry(0, 0, 380, 90);
    table->setSelectionAccented(true);
    table->setColumns({{QStringLiteral("From"), TableView::ColumnRole::Text}});
    table->setRowCount(1);
    table->setText(0, 0, QStringLiteral("Petr Dostal"));
    table->relayoutColumns();
    auto* elsewhere = new QPushButton(QStringLiteral("Compose"), surface);
    elsewhere->setGeometry(0, 100, 100, 30);

    surface->show();
    QVERIFY(QTest::qWaitForWindowExposed(surface));
    QApplication::setActiveWindow(surface);

    table->setFocus();
    table->setCurrentCell(0, 0);
    QVERIFY(table->hasFocus());

    const QString focusedSheet = table->styleSheet();

    elsewhere->setFocus();
    QVERIFY(!table->hasFocus());

    const QString blurredSheet = table->styleSheet();
    QVERIFY(focusedSheet != blurredSheet);

    // Back again - the row is not permanently stuck muted once focus returns.
    table->setFocus();
    QVERIFY(table->hasFocus());
    QCOMPARE(table->styleSheet(), focusedSheet);
}

// `table.tbl tbody tr:hover{background:var(--panel)}` - every row in every
// table, whether or not it opens anything. Nothing in QTableWidget paints
// this on its own, and the fill is painted *before* the base class's own
// paintEvent() so a plain setText() cell draws its item on top of it rather
// than the wash covering the text - the failure mode this test guards.
void GalleryTest::aHoveredRowGreysWithoutHidingItsOwnText()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* table = new TableView(surface);
    table->setColumns({{QStringLiteral("Symbol"), TableView::ColumnRole::Text},
                       {QStringLiteral("Mark"), TableView::ColumnRole::Numeric}});
    table->setRowCount(2);
    table->setText(0, 0, QStringLiteral("PF_XBTUSD"));
    table->setText(0, 1, QStringLiteral("64,212.50"));
    table->setText(1, 0, QStringLiteral("ETHUSDT"));
    table->setText(1, 1, QStringLiteral("2,494.60"));
    table->relayoutColumns();
    table->resize(400, 120);
    surface->resize(420, 140);
    surface->show();
    QVERIFY(QTest::qWaitForWindowExposed(surface));

    const ThemeProfile& profile = themeFor(table);
    const int row1y = table->rowViewportPosition(1) + 2;
    const auto pixelAt = [&](int y) {
        return QColor(table->viewport()
                          ->grab()
                          .toImage()
                          .convertToFormat(QImage::Format_ARGB32)
                          .pixel(5, y));
    };
    const QColor row1Before = pixelAt(row1y);

    // Sent directly rather than through QTest::mouseMove(), which drives the
    // platform cursor - and so, on the offscreen platform, depends on which
    // window a *previous* test last left active. A directly delivered event
    // does not.
    const QPoint at(10, table->rowViewportPosition(0) + 2);
    QMouseEvent move(QEvent::MouseMove, at, table->viewport()->mapToGlobal(at), Qt::NoButton,
                     Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(table->viewport(), &move);
    QCoreApplication::processEvents();

    // The hovered row picks up the panel wash - `panelBgAlt` in this
    // palette's own naming, since `panelBg` is `--bg`, the card's base.
    QCOMPARE(pixelAt(table->rowViewportPosition(0) + 2), profile.colors.panelBgAlt);
    // ...and the row the pointer never reached is exactly what it was before
    // anything was hovered - not incidentally the same shade panelBg happens
    // to be, but unchanged by the hover at all.
    QCOMPARE(pixelAt(row1y), row1Before);

    surface->hide();
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
    // `.rail-conn{background:var(--bg)}` - the card surface. It read as
    // windowBg, which is the rail's own bottom colour, so the strip vanished
    // into the rail instead of reading as a well cut into it.
    QCOMPARE(strip->styleSpec().background.describe(), QStringLiteral("panelBg"));

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
    QCOMPARE(strip->resolvedStyle().background.name(), profile.colors.panelBg.name());
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

// The bug this replaces: a file picker's tree cannot know whether a drive
// or a folder has anything inside it without listing it, so every location
// is added with no children yet. Before setNodeExpandable(), that node was
// indistinguishable from an actual leaf - it read as an empty drive, not as
// one nobody had looked inside yet.
void GalleryTest::aLazyNodeShowsItsChevronBeforeItHasAnyChildren()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* tree = new TreeView(surface);
    tree->addNode(QString(), QStringLiteral("pc"), QStringLiteral("This PC"), Glyph::Cpu);
    tree->addNode(QStringLiteral("pc"), QStringLiteral("c:/"),
                  QStringLiteral("Local Disk (C:)"), Glyph::Layers);
    // A genuine leaf beside it - the fix must not give every node a chevron.
    tree->addNode(QStringLiteral("pc"), QStringLiteral("recent"),
                  QStringLiteral("Recent"), Glyph::Star);
    tree->setExpanded(QStringLiteral("pc"), true);
    tree->resize(280, 200);

    QVERIFY(!tree->isNodeExpandable(QStringLiteral("c:/")));
    QVERIFY(tree->chevronRect(tree->rowOf(QStringLiteral("c:/"))).isNull());

    tree->setNodeExpandable(QStringLiteral("c:/"), true);
    QVERIFY(tree->isNodeExpandable(QStringLiteral("c:/")));
    const QRect chevron = tree->chevronRect(tree->rowOf(QStringLiteral("c:/")));
    QVERIFY(!chevron.isNull());
    // Still no children - a drive nobody has opened yet has none on record,
    // and that must not be why it lost the chevron the fix just gave it.
    QCOMPARE(tree->childrenOf(QStringLiteral("c:/")).size(), 0);

    // The click still works with nothing under it: this is what a caller's
    // expandToggled() handler relies on to fetch and add the real children.
    QSignalSpy expanded(tree, &TreeView::expandToggled);
    QTest::mouseClick(tree->viewport(), Qt::LeftButton, Qt::NoModifier, chevron.center());
    QCOMPARE(expanded.count(), 1);
    QVERIFY(tree->isExpanded(QStringLiteral("c:/")));

    // The caller answers, the normal way - real children, added lazily.
    tree->addNode(QStringLiteral("c:/"), QStringLiteral("c:/users"),
                  QStringLiteral("Users"), Glyph::Folder);
    QCOMPARE(tree->childrenOf(QStringLiteral("c:/")).size(), 1);
    QVERIFY(tree->visibleRowCount() > 2);

    // The untouched leaf never grew a chevron of its own.
    QVERIFY(!tree->isNodeExpandable(QStringLiteral("recent")));
    QVERIFY(tree->chevronRect(tree->rowOf(QStringLiteral("recent"))).isNull());
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

    // Exercise the real routed mouse path through FloatingLayer. Making the
    // whole layer WA_TransparentForMouseEvents also disabled every child, so
    // direct calls to the panel passed while users could neither drag a title
    // bar nor click a control in any module's floating window.
    surface->show();
    QApplication::processEvents();
    const QPoint dragFrom = panel->mapTo(surface, QPoint(120, panel->barRect().center().y()));
    const QPoint dragTo = dragFrom + QPoint(45, 30);
    QTest::mousePress(surface->windowHandle(), Qt::LeftButton, Qt::NoModifier, dragFrom);
    QTest::mouseMove(surface->windowHandle(), dragTo);
    QTest::mouseRelease(surface->windowHandle(), Qt::LeftButton, Qt::NoModifier, dragTo);
    QCOMPARE(panel->pos(), QPoint(85, 70));

    auto* action = new QPushButton(QStringLiteral("Action"), panel->bodyWidget());
    panel->body()->addWidget(action);
    QSignalSpy clicked(action, &QPushButton::clicked);
    QApplication::processEvents();
    QTest::mouseClick(surface->windowHandle(), Qt::LeftButton, Qt::NoModifier,
                      action->mapTo(surface, action->rect().center()));
    QCOMPARE(clicked.count(), 1);

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

void GalleryTest::aLetteredTabKeepsItsMarkThroughAFailureOrASleep()
{
    // A browser tab's mark is the page's own initial, and it is the page's
    // identity - not overwritten by the page's state the way an icon glyph
    // used to be. Loading is the one exception, because a page still arriving
    // has no identity to protect yet.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* tabs = new TabBar(surface);
    tabs->resize(600, 32);

    tabs->addTab(QStringLiteral("a"), QStringLiteral("NovuAI - Console"));
    tabs->setMarkText(QStringLiteral("a"), QStringLiteral("N"));
    QCOMPARE(tabs->markText(QStringLiteral("a")), QStringLiteral("N"));

    // A wider tab than a markless one, because the lettered tile plus its
    // state dot both have to fit beside the label.
    tabs->addTab(QStringLiteral("b"), QStringLiteral("Status page"));
    const int widthBeforeFailure = tabs->tabRect(0).width();

    tabs->setState(QStringLiteral("a"), TabBar::State::Failed);
    QCOMPARE(tabs->markText(QStringLiteral("a")), QStringLiteral("N"));
    // Failed makes room for the trailing dot - the tab does not shrink back to
    // its markless width, it widens further.
    QVERIFY(tabs->tabRect(0).width() >= widthBeforeFailure);

    tabs->setState(QStringLiteral("a"), TabBar::State::Asleep);
    QCOMPARE(tabs->markText(QStringLiteral("a")), QStringLiteral("N"));

    // Loading is the exception: the mark's own tile is where the spinner
    // goes, so it is drawn even though the tab still carries a mark string -
    // markText() itself does not clear, only what is painted in its place
    // does.
    tabs->setState(QStringLiteral("a"), TabBar::State::Loading);
    QCOMPARE(tabs->markText(QStringLiteral("a")), QStringLiteral("N"));
    QVERIFY(tabs->tabRect(0).width() >= 92);
}

void GalleryTest::aTabBarsTrailingWidgetTakesRoomFromTheTabs()
{
    // `.btabs-row .bacts` - a control pinned to the strip's trailing edge,
    // which the tabs must lay out around rather than under.
    //
    // Long titles, so the tabs' own natural width already presses on the
    // available room - a short-label pair sits at the floor width regardless
    // of what else is on the strip, which would make this test pass whether
    // the trailing widget was accounted for or not.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* tabs = new TabBar(surface);
    tabs->resize(500, 32);
    tabs->addTab(QStringLiteral("a"),
                 QStringLiteral("A rather long document title, first"));
    tabs->addTab(QStringLiteral("b"),
                 QStringLiteral("A rather long document title, second"));

    const int widthWithoutTrailing = tabs->tabRect(0).width();

    // A bare QWidget's own sizeHint() is invalid (-1,-1) - setFixedSize()
    // constrains its geometry but never answers sizeHint(), which is what
    // laidOutWidths() actually consults. A composite widget with a layout is
    // what every real caller hands in (PlainModules' browser view passes two
    // IconButtons in a QHBoxLayout), so the test builds the same shape rather
    // than a shortcut that would not have caught the geometry this exercises.
    auto* trailing = new QWidget(tabs);
    auto* trailingRow = new QHBoxLayout(trailing);
    trailingRow->setContentsMargins(8, 0, 8, 0);
    trailingRow->setSpacing(5);
    trailingRow->addWidget(new QPushButton(QStringLiteral("A"), trailing));
    trailingRow->addWidget(new QPushButton(QStringLiteral("B"), trailing));
    trailing->adjustSize();
    tabs->setTrailingWidget(trailing);
    QCOMPARE(tabs->trailingWidget(), trailing);
    QVERIFY(trailing->sizeHint().width() > 0);

    // The tabs gave up width to make room for it.
    QVERIFY(tabs->tabRect(0).width() < widthWithoutTrailing);
    // And no tab's box reaches into the trailing widget's own geometry.
    QVERIFY(tabs->tabRect(1).right() < trailing->geometry().left());

    // A second call replaces rather than stacks.
    auto* replacement = new QWidget(tabs);
    tabs->setTrailingWidget(replacement);
    QCOMPARE(tabs->trailingWidget(), replacement);
}

void GalleryTest::anAddressBarWeightsTheHostOverThePath()
{
    // `urlMarkup()` splits at the first slash and weights only the host - the
    // one anti-phishing affordance a browser chrome has, so it has to survive
    // the port to a self-painted field.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* bar = new AddressBar(surface);
    bar->resize(320, 28);

    QCOMPARE(bar->url(), QString());
    bar->setPlaceholder(QStringLiteral("No tab open"));
    QCOMPARE(bar->placeholder(), QStringLiteral("No tab open"));

    bar->setUrl(QStringLiteral("novuai.app/console"));
    QCOMPARE(bar->url(), QStringLiteral("novuai.app/console"));
    // The whole address stays reachable through the library's own tooltip
    // registry, not QWidget::toolTip() - Tooltip::install() never calls
    // setToolTip(), so that native property is deliberately not what this
    // checks.
    QCOMPARE(Tooltip::contentFor(bar).brief, QStringLiteral("novuai.app/console"));

    bar->setSecurity(AddressBar::Security::Secure);
    QCOMPARE(bar->security(), AddressBar::Security::Secure);
    bar->setSecurity(AddressBar::Security::Failed);
    QCOMPARE(bar->security(), AddressBar::Security::Failed);

    // A trailing control - the session chip - takes ownership and narrows the
    // text rect it is given.
    auto* chip = new Chip(QStringLiteral("novuai_ops"));
    chip->setGlyph(Glyph::Key);
    chip->setMonospaced(true);
    bar->setTrailing(chip);
    QCOMPARE(bar->trailing(), chip);
    QCOMPARE(chip->parentWidget(), static_cast<QWidget*>(bar));

    // placeTrailing() lays the chip out by its sizeHint(), so the geometry it
    // actually receives has to be exactly as wide as the chip needs to draw
    // its own text without clipping it - not QPushButton's generic guess.
    // This is what a monospaced session chip ("novuai_ops") long enough to
    // matter actually hit: the chip was placed narrower than its own text and
    // the identifier clipped mid-word against the omnibox's edge.
    QCOMPARE(chip->geometry().width(), chip->sizeHint().width());
    QVERIFY(chip->geometry().right() <= bar->width() - 1);

    // Fixed height: an address field stands as tall as a medium control,
    // whatever its own text needs.
    QCOMPARE(bar->sizeHint().height(), bar->minimumSizeHint().height());
    QVERIFY(bar->sizeHint().height() > 0);
}

void GalleryTest::anEmptyStatesMarkReplacesItsGlyphAndBackAgain()
{
    // `.empty` (a bare glyph) and `.bpage` (a plaque) are one component - the
    // mark decides which rhythm is drawn, and setting one hides the other
    // rather than drawing both.
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* state = new EmptyState(QStringLiteral("i"), QStringLiteral("Title"),
                                 QStringLiteral("Subtitle"), surface);
    surface->show();
    QVERIFY(QTest::qWaitForWindowExposed(surface));

    auto* plaque = new QWidget;
    plaque->setFixedSize(54, 54);
    state->setMark(plaque);
    QCOMPARE(state->mark(), plaque);
    QVERIFY(plaque->isVisible());

    state->setMark(nullptr);
    QCOMPARE(state->mark(), static_cast<QWidget*>(nullptr));

    // `.bpage .u` - the detail line under the title, hidden until it has text.
    QCOMPARE(state->detail(), QString());
    state->setDetail(QStringLiteral("novuai.app/console"));
    QCOMPARE(state->detail(), QStringLiteral("novuai.app/console"));
    state->setDetail(QString());
    QCOMPARE(state->detail(), QString());

    delete state;
}

void GalleryTest::anEmptyStatesActionsRowAppearsOnlyWhenFilled()
{
    // An empty actions row is not a row: it must not reserve the margin above
    // it until a caller actually puts a button in it.
    auto* state = new EmptyState(QStringLiteral("i"), QStringLiteral("Title"),
                                 QStringLiteral("Subtitle"));
    QVERIFY(state->actions() != nullptr);
    QCOMPARE(state->actions()->count(), 0);

    auto* button = primaryButton(QStringLiteral("New tab"));
    state->actions()->addWidget(button);
    QCOMPARE(state->actions()->count(), 1);
    state->resize(400,260);state->show();QCoreApplication::processEvents();
    QVERIFY(button->isVisible());
    state->actions()->removeWidget(button);delete button;QCoreApplication::processEvents();
    QVERIFY(state->actions()->parentWidget()->isHidden());

    // `.bpage .sess` - one widget under the actions, centred.
    QCOMPARE(state->footer(), static_cast<QWidget*>(nullptr));
    auto* chip = new Chip(QStringLiteral("novuai_dev"));
    state->setFooter(chip);
    QCOMPARE(state->footer(), chip);
    QVERIFY(chip->parentWidget() != nullptr);

    delete state;
}

void GalleryTest::anAccentedChipIsNotTheSameDrawingAsAPlainOne()
{
    // `.sesschip` against `.sesschip.anon` - the accent is what says a token
    // names something that is *yours*.
    auto* signedIn = new Chip(QStringLiteral("novuai_dev"));
    signedIn->setGlyph(Glyph::Key);
    QVERIFY(!signedIn->isAccented());
    signedIn->setAccented(true);
    QVERIFY(signedIn->isAccented());

    // A token carrying an identifier is set in the monospace face; toggling it
    // must not crash the width calculation and must produce a non-zero width.
    auto* plain = new Chip(QStringLiteral("novuai_dev"));
    plain->setGlyph(Glyph::Key);
    QVERIFY(!plain->isMonospaced());
    plain->setMonospaced(true);
    QVERIFY(plain->isMonospaced());
    QVERIFY(plain->width() != 0);

    // sizeHint() has to answer the same box the chip actually committed to,
    // not QPushButton's own guess from the widget's QFont property (which
    // never changes - setMonospaced() only redirects what applyGeometry()
    // measures and what paintEvent() draws with). AddressBar::setTrailing()
    // lays out entirely by sizeHint(), so a chip whose sizeHint() disagreed
    // with its real width clipped its own text against the omnibox's edge -
    // exactly what happened with a session chip long enough to matter
    // ("novuai_ops") before this override existed.
    QCOMPARE(plain->sizeHint(), plain->size());
    auto* longToken = new Chip(QStringLiteral("a-rather-long-session-identifier"));
    longToken->setGlyph(Glyph::Key);
    longToken->setMonospaced(true);
    QCOMPARE(longToken->sizeHint().width(), longToken->width());
    QVERIFY(longToken->sizeHint().width() > plain->sizeHint().width());

    delete signedIn;
    delete plain;
    delete longToken;
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

    // `.drawer{animation:slidein .28s ...}` - open() starts from an offset
    // position and slides into place rather than snapping there, so the
    // final geometry is only true once the animation has actually finished.
    QTest::qWait(400);

    // min(760px, 92%) - both halves matter: the cap stops it swallowing a wide
    // window, the fraction stops it overflowing a narrow one.
    QCOMPARE(drawer->width(), qMin(760, int(900 * 0.92)));
    QCOMPARE(drawer->height(), surface->height());
    // Anchored to the trailing edge by default.
    QCOMPARE(drawer->geometry().right() + 1, surface->width());

    drawer->setEdge(Drawer::Edge::Leading);
    QCOMPARE(drawer->x(), 0);

    // Escape closes: a drawer is not asking a question, so leaving is always
    // allowed - which is the difference between this and a Dialog. isOpen()
    // flips the instant Escape is pressed; closed() and the scrim hiding wait
    // for the close animation, the same way opened() did not wait for open()'s.
    QTest::keyClick(drawer, Qt::Key_Escape);
    QVERIFY(!drawer->isOpen());
    QVERIFY(closed.wait(500));
    QCOMPARE(closed.count(), 1);
    QVERIFY(!drawer->scrim()->isVisible());

    drawer->setClosesOnScrimClick(false);
    QVERIFY(!drawer->closesOnScrimClick());
}

// `.drawer{animation:slidein .28s cubic-bezier(.2,.8,.2,1) both}` - open()
// used to show() at its final geometry outright, which is what "an instant
// popup" means. It has to actually be somewhere else the moment it starts.
void GalleryTest::aDrawerSlidesRatherThanSnapsIntoPlace()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    surface->resize(900, 500);
    surface->show();
    QVERIFY(QTest::qWaitForWindowExposed(surface));

    auto* drawer = new Drawer(surface);
    drawer->setTitle(QStringLiteral("Bot detail"));
    drawer->open();

    // Right after open(), the animation has barely run: the drawer is not yet
    // at the resting position it will end at, and it is not yet fully opaque.
    const int restingRight = surface->width() - 1;
    QVERIFY(drawer->geometry().right() != restingRight);
    QVERIFY(drawer->graphicsEffect() != nullptr);

    QTest::qWait(400);
    QCOMPARE(drawer->geometry().right(), restingRight);

    drawer->close();
    // close() is asymmetric with open(): isOpen() flips immediately, but the
    // widget itself is still where it was until the close animation runs.
    QVERIFY(!drawer->isOpen());
    QVERIFY(drawer->isVisible());
    QSignalSpy closed(drawer, &Drawer::closed);
    QVERIFY(closed.wait(500));
    QVERIFY(!drawer->isVisible());
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

void GalleryTest::everyGlyphStaysInsideItsViewBox()
{
    // C++ never promised that `f(a(), b())` calls a() before b() - argument
    // evaluation order between them is unspecified. appendPathData() used to
    // read every curve and arc endpoint as
    // `QPointF(reader.takeNumber(), reader.takeNumber())`, trusting left-to-
    // right order for two calls that share one cursor. On this compiler the
    // second takeNumber() ran first, so every 'C', 'S' and 'A' endpoint came
    // out with x and y swapped - "history" became an unrecognisable knot,
    // "eye" a house shape, "globe" a ringed planet the bug report called
    // Saturn. M and L were never affected because they already read into two
    // named locals across two statements, which is what forces the order the
    // expression form only implied.
    //
    // The regression is structural rather than a wrong pixel: a swapped
    // coordinate routinely throws the path outside the 24x24 viewBox every
    // glyph is drawn in - the corrupted "globe" reached x=29.85. So rather
    // than hand-comparing pixels, this holds every glyph - the 39 that use A,
    // C or S and the rest alike - to the box the design draws in.
    for (Glyph g : allGlyphs()) {
        const QRectF b = glyphPath(g).boundingRect();
        if (b.isEmpty()) {
            continue;
        }
        const QString name = glyphName(g);
        QVERIFY2(b.left() > -1.0 && b.right() < 25.0,
                 qPrintable(QStringLiteral("%1 leaves the viewBox on x: %2")
                                .arg(name, QStringLiteral("%1..%2").arg(b.left()).arg(b.right()))));
        QVERIFY2(b.top() > -1.0 && b.bottom() < 25.0,
                 qPrintable(QStringLiteral("%1 leaves the viewBox on y: %2")
                                .arg(name, QStringLiteral("%1..%2").arg(b.top()).arg(b.bottom()))));
    }
}

void GalleryTest::curvedGlyphsResolveTheirCoordinatesInOrder()
{
    // Named, worked examples of the invariant above - the exact three glyphs
    // the bug report flagged first, pinned to their actual geometry so a
    // change that happens to keep every glyph inside the viewBox but still
    // swaps an axis cannot pass unnoticed.
    const QRectF eye = glyphPath(Glyph::Eye).boundingRect();
    QVERIFY2(qAbs(eye.left() - 2.5) < 0.3, "eye: left tip at x=2.5");
    QVERIFY2(qAbs(eye.right() - 21.5) < 0.3, "eye: right tip at x=21.5");
    QVERIFY2(qAbs(eye.top() - 5.8) < 0.3, "eye: brow at y=5.8");
    QVERIFY2(qAbs(eye.bottom() - 18.2) < 0.3, "eye: lower lid at y=18.2");

    // history: one large clockwise arc from (3.8,12) to (6.4,6). Swapped, the
    // endpoint's delta becomes (-6,2.6) instead of (2.6,-6) and the ring is
    // thrown off to the left.
    const QRectF history = glyphPath(Glyph::History).boundingRect();
    QVERIFY2(history.left() > 3.0 && history.left() < 4.5, "history: left edge near x=3.8");
    QVERIFY2(history.right() > 19.5 && history.right() < 21.0, "history: right edge near x=20.2");

    // globe: the meridian lens is drawn by two 'A' arcs and sits inside the
    // outer circle. Swapped, it stopped being a lens and became the
    // off-centre loop that gave this bug its name.
    const QRectF globe = glyphPath(Glyph::Globe).boundingRect();
    QVERIFY2(qAbs(globe.center().x() - 12.0) < 0.3, "globe: stays centred on the circle");
    QVERIFY2(globe.right() < 21.0, "globe: the lens does not blow out past the ring");
}

QTEST_MAIN(GalleryTest)

// A chip that carries a glyph or an x is a token, not a toggle -- and a
// chip that carries neither is exactly what it was before, because
// NovuSpace's column pills consume this header today.
void GalleryTest::chipTokenIsNotAToggle()
{
    Chip plain(QStringLiteral("Columns"), nullptr, true);
    QVERIFY(!plain.isToken());
    QVERIFY(plain.isCheckable());
    QVERIFY(plain.isChecked());
    QVERIFY(plain.removeRect().isEmpty());
    const int plainWidth = plain.width();

    Chip token(QStringLiteral("Columns"), nullptr, true);
    token.setGlyph(Glyph::File);
    QVERIFY(token.isToken());
    QVERIFY(!token.isCheckable());
    QCOMPARE(token.glyph(), Glyph::File);
    // The glyph and its gap are paid for, so a token is wider than the
    // plain chip carrying the same word.
    QVERIFY(token.width() > plainWidth);

    token.setRemovable(true);
    QVERIFY(token.isRemovable());
    const QRect x = token.removeRect();
    QVERIFY(!x.isEmpty());
    // The x sits inside the chip, against its trailing edge.
    QVERIFY(token.rect().contains(x));
    QVERIFY(x.right() < token.width());

    QSignalSpy removed(&token, &Chip::removed);
    QTest::mouseClick(&token, Qt::LeftButton, Qt::NoModifier, x.center());
    QCOMPARE(removed.count(), 1);

    // Pressing the label is not pressing the x.
    QTest::mouseClick(&token, Qt::LeftButton, Qt::NoModifier, QPoint(4, token.height() / 2));
    QCOMPARE(removed.count(), 1);
}


// A wrapped label is asked how tall it is at a width; QLabel answers at a
// width nobody asked about, which is what a column then believes.
void GalleryTest::paragraphAnswersForTheWidthItHas()
{
    const QString prose = QStringLiteral(
        "A paragraph is as tall as the width it is given makes it, and a column "
        "that adds up the wrong heights hands the difference to whatever else in "
        "it can grow.");

    QWidget host;
    host.resize(420, 400);
    auto* column = new QVBoxLayout(&host);

    auto* plain = new QLabel(prose, &host);
    plain->setWordWrap(true);
    plain->setMinimumWidth(1);
    auto* measured = new Paragraph(prose, &host);
    column->addWidget(plain);
    column->addWidget(measured);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));

    // The height at the width it has is the whole claim.
    QCOMPARE(measured->sizeHint().height(), measured->heightForWidth(measured->width()));
    QCOMPARE(measured->minimumSizeHint().height(),
             measured->heightForWidth(measured->width()));
    QVERIFY(measured->sizePolicy().hasHeightForWidth());

    Q_UNUSED(plain);

    // A paragraph prefers the width its text wants, capped by what it is
    // allowed -- which is what lets a shrink-to-fit column stop at the
    // sentence rather than at the edge of the window.
    QVERIFY(measured->naturalWidth() > 0);
    measured->setMaximumWidth(120);
    QCOMPARE(measured->sizeHint().width(), 120);
    measured->setMaximumWidth(QWIDGETSIZE_MAX);
    QCOMPARE(measured->sizeHint().width(), measured->naturalWidth());

    // And the consequence, which is where the wrong number does its damage: a
    // scrollable page of paragraphs is as tall as its text, not a multiple of
    // it. A widgetResizable QScrollArea sizes its content from the minimum it
    // is told, so a column of plain wrapped labels asks for a page several
    // times too long and a box layout shares the surplus out among whatever
    // else in it can grow.
    QWidget outer;
    outer.resize(420, 300);
    auto* shell = new QVBoxLayout(&outer);
    shell->setContentsMargins(0, 0, 0, 0);
    auto* pageHost = new QWidget(&outer);
    QScrollArea* area = nullptr;
    QVBoxLayout* pageColumn = scrollPage(pageHost, &area);
    pageColumn->setContentsMargins(0, 0, 0, 0);
    pageColumn->setSpacing(0);
    for (int i = 0; i < 5; ++i) {
        pageColumn->addWidget(new Paragraph(prose, pageHost));
    }
    shell->addWidget(pageHost);
    outer.show();
    QVERIFY(QTest::qWaitForWindowExposed(&outer));
    QCoreApplication::processEvents();

    QWidget* content = area->widget();
    const int wanted = pageColumn->heightForWidth(content->width());
    QVERIFY(wanted > 0);
    QVERIFY(content->height() <= qMax(wanted, area->viewport()->height()) + 2);
}

// A scroll area has no height of its own. One quoted inside a column has to.
void GalleryTest::monoLineViewFitsItsRowsWhenAsked()
{
    DiffView diff;
    diff.setUnifiedDiff(QStringLiteral(
        "@@ -1,4 +1,5 @@\n context\n-gone\n+arrived\n+also arrived\n more context\n"));
    QVERIFY(diff.rowCount() >= 6);
    QVERIFY(!diff.fitsContent());

    const int loose = diff.sizeHint().height();
    diff.setFitToContent(true);
    QVERIFY(diff.fitsContent());
    QCOMPARE(diff.sizeHint().height(), diff.contentHeight());
    QCOMPARE(diff.minimumSizeHint().height(), diff.sizeHint().height());
    QCOMPARE(diff.sizePolicy().verticalPolicy(), QSizePolicy::Fixed);
    QVERIFY(diff.sizeHint().height() != loose || diff.rowCount() == 0);

    // Past the cap it stops growing and scrolls, which is the point of one.
    diff.setFitToContent(true, 3);
    QCOMPARE(diff.maxVisibleRows(), 3);
    QCOMPARE(diff.sizeHint().height(), 3 * diff.rowHeight() + 2 * 0);
    QVERIFY(diff.sizeHint().height() < diff.contentHeight());

    // Off again is the behaviour every existing caller has.
    diff.setFitToContent(false);
    QCOMPARE(diff.sizePolicy().verticalPolicy(), QSizePolicy::Expanding);
}

// `line-height:1.65` is a multiple of the font size, not of the font's
// metric height -- the second is about a third taller in every row.
void GalleryTest::monoRowHeightIsTheDesignsLineHeight()
{
    DiffView diff;
    LogView log;

    const QFontMetrics metrics(diff.font());
    QVERIFY(diff.rowHeight() > 0);
    // A diff is tighter than a log, because 1.65 is less than 1.75.
    QVERIFY(diff.rowHeight() <= log.rowHeight());
    // And neither is the metric height times the leading, which would put a
    // diff row above 24px at this size.
    QVERIFY(diff.rowHeight() < qRound(metrics.height() * 1.65));
    Q_UNUSED(metrics);
}

// The titlebar's way into the palette accepts no text and takes no focus.
void GalleryTest::commandFieldIsADoorNotAField()
{
    CommandField field(QStringLiteral("Search this workspace, or run a command"));
    QCOMPARE(field.focusPolicy(), Qt::NoFocus);
    QVERIFY(field.capRect().isEmpty());

    const int bare = field.sizeHint().width();
    field.setShortcut(QKeySequence(QStringLiteral("Ctrl+K")));
    QCOMPARE(field.shortcut(), QKeySequence(QStringLiteral("Ctrl+K")));
    const QRect cap = field.capRect();
    QVERIFY(!cap.isEmpty());
    // The cap is paid for, and it sits against the trailing edge.
    QVERIFY(field.sizeHint().width() > bare);
    field.resize(field.sizeHint());
    QVERIFY(field.rect().contains(field.capRect()));
    QVERIFY(field.capRect().right() < field.width());

    // A hand-placing parent sizes this from its hint, so the hint has to obey
    // the width the caller fixed -- `.omni{max-width:460px}`.
    field.setFixedWidth(460);
    QCOMPARE(field.sizeHint().width(), 460);

    QSignalSpy pressed(&field, &QAbstractButton::clicked);
    QTest::mouseClick(&field, Qt::LeftButton, Qt::NoModifier,
                      QPoint(field.width() / 2, field.height() / 2));
    QCOMPARE(pressed.count(), 1);

    field.setShortcut(QKeySequence());
    QVERIFY(field.capRect().isEmpty());
}


// setSectionCollapsible() and setSectionAction() looked their row up through
// indexOfKey(), which skips captions on purpose so that setCurrentKey() can
// never land on one. Both therefore searched for a row that by definition
// could not be found, and did nothing at all, in every application that called
// them -- which is why a rail that asked for collapsible sections got none.
void GalleryTest::navSectionApiReachesItsCaption()
{
    NavList nav;
    nav.addSection(QStringLiteral("Modules"), QStringLiteral("sec"));
    nav.addItem(QStringLiteral("chat"), QStringLiteral("Chat"));
    nav.addItem(QStringLiteral("docs"), QStringLiteral("Documents"));

    // A caption is not an item, and still answers about itself.
    QCOMPARE(nav.indexOfKey(QStringLiteral("sec")), -1);
    QCOMPARE(nav.indexOfSection(QStringLiteral("sec")), 0);

    nav.setSectionCollapsible(QStringLiteral("sec"), true);
    QVERIFY(nav.isExpanded(QStringLiteral("sec")));

    const int open = nav.sizeHint().height();
    nav.setSectionExpanded(QStringLiteral("sec"), false);
    QVERIFY(!nav.isExpanded(QStringLiteral("sec")));
    // Shut, the section costs its caption and nothing else.
    QVERIFY(nav.sizeHint().height() < open);

    nav.setSectionExpanded(QStringLiteral("sec"), true);
    QCOMPARE(nav.sizeHint().height(), open);

    // And a caption that is not collapsible has no twisty to aim at.
    NavList plain;
    plain.addSection(QStringLiteral("Group"), QStringLiteral("g"));
    plain.addItem(QStringLiteral("a"), QStringLiteral("A"));
    QVERIFY(plain.sizeHint().height() > 0);
}

// A window must always be leavable, and its action must always be reachable.
//
// The footer used to be the last thing inside the scrolling body, so a form
// taller than the window scrolled its own Create button out of reach; there
// was no Escape; and minimising - which the sheet does by hiding the element -
// would have left a panel nothing in the application could find again.
void GalleryTest::aFloatingWindowCanAlwaysBeLeft()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    surface->resize(900, 700);
    auto* panel = new FloatingPanel(Glyph::Vault, QStringLiteral("New item"), surface);
    panel->resize(470, 520);

    // A form far taller than the window.
    for (int i = 0; i < 30; ++i) {
        auto* field = new QLabel(QStringLiteral("Field %1").arg(i), panel->bodyWidget());
        field->setMinimumHeight(40);
        panel->body()->addWidget(field);
    }
    auto* create = new QPushButton(QStringLiteral("Create item"), panel);
    panel->footer()->addStretch(1);
    panel->footer()->addWidget(create);
    panel->show();
    QApplication::sendPostedEvents();
    panel->resize(470, 520);
    QApplication::sendPostedEvents();

    // The footer is inside the window and below the body, whatever the body
    // is doing. Measured in the panel's own coordinates: the button is a
    // child of the footer row, which is what has to have stayed put.
    QVERIFY(!create->isHidden());
    const int bottom = create->mapTo(panel, QPoint(0, create->height())).y();
    QVERIFY2(bottom <= panel->height(),
             qPrintable(QStringLiteral("button bottom %1, panel %2")
                            .arg(bottom).arg(panel->height())));
    // And below the body, not floating over it.
    QVERIFY(create->mapTo(panel, QPoint(0, 0)).y() > panel->height() / 2);

    // Escape asks to close, exactly as the close box does - and asks on the
    // next turn of the loop rather than from inside the event that asked. A
    // caller is entitled to answer with a modal question ("this has not been
    // sent, keep it?"), and a nested event loop started from inside a key or
    // mouse handler leaves the panel holding the implicit mouse grab: the
    // window will not close and nothing else in the application answers the
    // mouse again. Posting the signal is what makes that safe, so the posting
    // is the behaviour under test.
    QSignalSpy closed(panel, &FloatingPanel::closeRequested);
    QTest::keyClick(panel, Qt::Key_Escape);
    QCOMPARE(closed.count(), 0);
    QApplication::sendPostedEvents();
    QCOMPARE(closed.count(), 1);

    // The close box, the same way.
    QTest::mouseClick(panel, Qt::LeftButton, Qt::NoModifier, panel->closeRect().center());
    QCOMPARE(closed.count(), 1);
    QApplication::sendPostedEvents();
    QCOMPARE(closed.count(), 2);

    // Minimising rolls the window up rather than hiding it: a hidden panel is
    // one nothing in the application can reach again.
    QSignalSpy minimised(panel, &FloatingPanel::minimiseRequested);
    QTest::mouseClick(panel, Qt::LeftButton, Qt::NoModifier,
                      panel->minimiseRect().center());
    QCOMPARE(minimised.count(), 1);
    QVERIFY(panel->isRolled());
    // Still on the layer: rolled up is not hidden, which is the whole point.
    QVERIFY(!panel->isHidden());

    // And maximising fills the layer, then puts it back where it was.
    const QRect before = panel->geometry();
    QTest::mouseClick(panel, Qt::LeftButton, Qt::NoModifier,
                      panel->maximiseRect().center());
    QVERIFY(panel->isMaximised());
    QVERIFY(panel->width() > before.width());
    QTest::mouseClick(panel, Qt::LeftButton, Qt::NoModifier,
                      panel->maximiseRect().center());
    QVERIFY(!panel->isMaximised());
    QCOMPARE(panel->geometry(), before);
}

// `.tabs`/`.tab` and `.btab` are two strips, not one wearing two colours.
//
// A settings card's four tabs were drawn as browser tabs: a raised plate under
// each, and a close box on the one you were reading. Both say the wrong thing
// - these are four faces of one surface, and none of them can be closed.
void GalleryTest::anUnderlinedTabStripIsNotADocumentStrip()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* tabs = new TabBar(surface);
    tabs->addTab(QStringLiteral("a"), QStringLiteral("NovuAI access"));
    tabs->addTab(QStringLiteral("b"), QStringLiteral("General"));
    tabs->setCurrentKey(QStringLiteral("a"));

    // A browser strip closes documents.
    QCOMPARE(tabs->variant(), TabBar::Variant::Browser);
    QVERIFY(!tabs->closeRect(0).isNull());

    // An underlined one does not, and says so by having no close box at all
    // rather than by ignoring the click on it.
    tabs->setVariant(TabBar::Variant::Underline);
    QCOMPARE(tabs->variant(), TabBar::Variant::Underline);
    QVERIFY(tabs->closeRect(0).isNull());

    // And a tab is as wide as its own label, at either variant: a caption
    // measured to the pixel came out elided by one character.
    tabs->resize(600, 40);
    const QFontMetrics metrics(tabs->font());
    QVERIFY2(tabs->tabRect(0).width() > metrics.horizontalAdvance(
                 QStringLiteral("NovuAI access")),
             qPrintable(QStringLiteral("tab %1px, label %2px")
                            .arg(tabs->tabRect(0).width())
                            .arg(metrics.horizontalAdvance(
                                QStringLiteral("NovuAI access")))));
}

// `.note` is a block of prose, not a strip.
//
// It was one control-height line with the rest elided, in the tone colour,
// with no glyph - so a warning that said what to do lost the half that said
// it, every note in the product read as a coloured sentence rather than as a
// tinted block, and the `<b>` the design puts at the front of every one of
// them was printed on the screen as markup.
void GalleryTest::aNoteWrapsCarriesAGlyphAndKeepsItsBoldLead()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    surface->resize(600, 400);

    const QString sentence =
        QStringLiteral("<b>1 order rejected by the exchange.</b> Rejections are kept so "
                       "the reason stays visible; they do not retry on their own, and "
                       "nothing here is sent again without being asked.");
    auto* note = new InfoBanner(sentence, Badge::Tone::Warning, surface);
    note->resize(520, note->heightForWidth(520));

    // The markup never reaches the screen, and the whole sentence - without
    // it - stays on the tooltip.
    QVERIFY(!note->toolTip().contains(QStringLiteral("<b>")));
    QVERIFY(note->toolTip().startsWith(QStringLiteral("1 order rejected")));

    // A note this long is more than one line at this width, and fewer lines
    // when it is given more room.
    const int narrow = note->heightForWidth(320);
    const int wide = note->heightForWidth(900);
    QVERIFY2(narrow > wide,
             qPrintable(QStringLiteral("320px:%1 900px:%2").arg(narrow).arg(wide)));

    // The glyph follows the tone: a warning carries the warning mark, and an
    // informational note the info one.
    QCOMPARE(note->glyph(), Glyph::None);  // unset means "from the tone"
    note->setGlyph(Glyph::Shield);
    QCOMPARE(note->glyph(), Glyph::Shield);

    // Width is still a preference and never a measurement: the regression
    // aBannerNeverSizesItselfToItsMessage exists for is not reintroduced by
    // letting the height follow the text.
    QCOMPARE(note->sizePolicy().horizontalPolicy(), QSizePolicy::Ignored);
}

// `td.num`, `td.mono` and `td.act` are classes the sheet writes, and every
// page in the product was expressing them by putting a styled QLabel in the
// cell - a column's typography reimplemented once per page, in a widget that
// cannot wrap to a width the table has not decided yet.
void GalleryTest::aColumnStatesItsOwnFaceInkAndAlignment()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* table = new TableView(surface);
    using Role = TableView::ColumnRole;
    table->setColumns({{QStringLiteral("Name"), Role::Text},
                       {QStringLiteral("P&L"), Role::Numeric},
                       {QStringLiteral("Since"), Role::Mono}});
    table->setRowCount(1);
    table->setText(0, 0, QStringLiteral("Breakout Hunter"));
    table->setText(0, 1, QStringLiteral("-77.50"));
    table->setText(0, 2, QStringLiteral("2026-09-13 06:01"));

    const ThemeProfile& profile = themeFor(table);

    // Text keeps the table's own face and stays left.
    QVERIFY(!table->item(0, 0)->data(Qt::FontRole).isValid());
    QVERIFY(table->item(0, 0)->textAlignment() & Qt::AlignLeft);

    // `td.num{font-family:'JetBrains Mono';text-align:right}` — the mono face
    // at the body size, because `.num` states no size of its own.
    const QFont numeric = table->item(0, 1)->data(Qt::FontRole).value<QFont>();
    QCOMPARE(numeric.family(), profile.type.monoFamily);
    QVERIFY(table->item(0, 1)->textAlignment() & Qt::AlignRight);

    // `td.mono{font-size:11.5px;color:var(--text3)}` — a step down, and muted.
    const QFont mono = table->item(0, 2)->data(Qt::FontRole).value<QFont>();
    QCOMPARE(mono.family(), profile.type.monoFamily);
    QCOMPARE(mono.pixelSize(), profile.type.xs);
    QVERIFY(mono.pixelSize() < numeric.pixelSize());
    QCOMPARE(table->item(0, 2)->foreground().color(), profile.colors.textDisabled);
    QCOMPARE(table->item(0, 1)->foreground().color(), profile.colors.textSecondary);
}

// A table inside a card is markup with a height, not a viewport: it is as tall
// as its rows and the *page* scrolls. Asking QAbstractScrollArea what it wants
// gets a fixed 256x192 viewport, which is why every page in the product had to
// compute a minimum height by hand from a row height it could not see.
void GalleryTest::aTableIsAsTallAsItsRows()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* table = new TableView(surface);
    using Role = TableView::ColumnRole;
    table->setColumns({{QStringLiteral("Name"), Role::Text},
                       {QStringLiteral("Value"), Role::Numeric}});
    table->setRowCount(3);
    for (int row = 0; row < 3; ++row) {
        table->setText(row, 0, QStringLiteral("Row %1").arg(row));
        table->setText(row, 1, QStringLiteral("%1.00").arg(row));
    }
    table->relayoutColumns();

    int rows = 0;
    for (int row = 0; row < table->rowCount(); ++row) {
        rows += table->rowHeight(row);
    }
    const int header = table->horizontalHeader()->height();
    QCOMPARE(table->sizeHint().height(), header + rows + 2 * table->frameWidth());

    // Neither bar exists in the design, and a table that claims every spare
    // pixel turns a three-row card into a panel the height of the window.
    QCOMPARE(table->verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
    QCOMPARE(table->sizePolicy().verticalPolicy(), QSizePolicy::Preferred);

    // `.tbl-wrap.cap{max-height:440px;overflow-y:auto}` is the one exception,
    // and asking for it is what brings the bar back.
    table->setHeightCap(40);
    QCOMPARE(table->sizeHint().height(), 40);
    QCOMPARE(table->verticalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
}

// A caption that folds is a caption every application gets for free.
//
// It used to be opt-in, and of the four modules sharing this rail exactly one
// remembered to opt in -- which is how one shared component came to behave
// four different ways in one window. `renderRail()` wraps every group in a
// `.sb-block` whose head is a toggle, in every module, so the default is the
// design's and an application that wants a fixed caption says so.
void GalleryTest::aKeyedCaptionFoldsWithoutBeingAskedTo()
{
    NavList nav;
    nav.addSection(QStringLiteral("Workspaces"), QStringLiteral("sec.ws"));
    nav.addItem(QStringLiteral("a"), QStringLiteral("A"));

    QVERIFY(nav.isExpanded(QStringLiteral("sec.ws")));
    const int open = nav.sizeHint().height();
    nav.setSectionExpanded(QStringLiteral("sec.ws"), false);
    QVERIFY(nav.sizeHint().height() < open);
    QVERIFY(!nav.isRowVisible(QStringLiteral("a")));

    // An unkeyed caption cannot be addressed, so it cannot fold either: there
    // is nothing for a click to toggle and nothing for a caller to restore.
    NavList anonymous;
    anonymous.addSection(QStringLiteral("Workspaces"));
    anonymous.addItem(QStringLiteral("a"), QStringLiteral("A"));
    const int before = anonymous.sizeHint().height();
    anonymous.setSectionExpanded(QString(), false);
    QCOMPARE(anonymous.sizeHint().height(), before);
}

// The list owns the fold, not the four applications that draw trees in it.
//
// Every one of them used to rebuild the whole rail from a set of open keys on
// each click, and each got it slightly different. The rows under a twisty are
// the list's to show and hide, and the signal still fires first so a caller
// that really does fetch on demand can still fill them.
void GalleryTest::shuttingARowHidesTheRowsUnderIt()
{
    NavList nav;
    nav.addItem(QStringLiteral("ws"), QStringLiteral("Novusion"));
    nav.setExpandable(QStringLiteral("ws"), true);
    nav.setExpanded(QStringLiteral("ws"), true);
    nav.addChild(QStringLiteral("ws.docs"), QStringLiteral("Documents"));
    nav.addChild(QStringLiteral("ws.mail"), QStringLiteral("Mail"));
    nav.addItem(QStringLiteral("other"), QStringLiteral("Archive"));

    QVERIFY(nav.isRowVisible(QStringLiteral("ws.docs")));
    const int open = nav.sizeHint().height();

    nav.setExpanded(QStringLiteral("ws"), false);
    QVERIFY(!nav.isRowVisible(QStringLiteral("ws.docs")));
    QVERIFY(!nav.isRowVisible(QStringLiteral("ws.mail")));
    // The row after the group is not part of it and never moves out of view.
    QVERIFY(nav.isRowVisible(QStringLiteral("other")));
    QVERIFY(nav.sizeHint().height() < open);

    nav.setExpanded(QStringLiteral("ws"), true);
    QCOMPARE(nav.sizeHint().height(), open);

    // A child added while its parent is shut arrives shut. A caller streaming
    // rows in cannot be expected to check the state of the row above each one.
    // (Rows are appended, so the parent has to be the last one for the child
    // to land under it -- which is exactly how a rail is filled.)
    NavList streamed;
    streamed.addItem(QStringLiteral("ws"), QStringLiteral("Novusion"));
    streamed.setExpandable(QStringLiteral("ws"), true);
    streamed.addChild(QStringLiteral("ws.vault"), QStringLiteral("Vault"));
    QVERIFY(!streamed.isRowVisible(QStringLiteral("ws.vault")));
    streamed.setExpanded(QStringLiteral("ws"), true);
    QVERIFY(streamed.isRowVisible(QStringLiteral("ws.vault")));
}

// Three things can hide a row and they compose rather than overwrite: the
// caller's filter, the caption above it, and the twisty above that. Folding a
// section used to rewrite every row's visibility on its way past, which put
// filtered rows back on screen and took expanded state with it.
void GalleryTest::theThreeWaysToHideARowDoNotOverwriteEachOther()
{
    NavList nav;
    nav.addSection(QStringLiteral("Workspaces"), QStringLiteral("sec"));
    nav.addItem(QStringLiteral("ws"), QStringLiteral("Novusion"));
    nav.setExpandable(QStringLiteral("ws"), true);
    nav.setExpanded(QStringLiteral("ws"), true);
    nav.addChild(QStringLiteral("ws.docs"), QStringLiteral("Documents"));
    nav.addItem(QStringLiteral("hidden"), QStringLiteral("Filtered out"));
    nav.setRowVisible(QStringLiteral("hidden"), false);

    QVERIFY(!nav.isRowVisible(QStringLiteral("hidden")));

    // Fold and unfold the caption over all of it.
    nav.setSectionExpanded(QStringLiteral("sec"), false);
    QVERIFY(!nav.isRowVisible(QStringLiteral("ws")));
    QVERIFY(!nav.isRowVisible(QStringLiteral("ws.docs")));
    nav.setSectionExpanded(QStringLiteral("sec"), true);

    // The filter survived the caption.
    QVERIFY(!nav.isRowVisible(QStringLiteral("hidden")));
    // And so did the twisty: the group is still open, not reset by the fold.
    QVERIFY(nav.isExpanded(QStringLiteral("ws")));
    QVERIFY(nav.isRowVisible(QStringLiteral("ws.docs")));

    // A shut group inside a caption stays shut when the caption reopens.
    nav.setExpanded(QStringLiteral("ws"), false);
    nav.setSectionExpanded(QStringLiteral("sec"), false);
    nav.setSectionExpanded(QStringLiteral("sec"), true);
    QVERIFY(nav.isRowVisible(QStringLiteral("ws")));
    QVERIFY(!nav.isRowVisible(QStringLiteral("ws.docs")));
}

// Collapsing the rail and folding a section are different states of different
// things, and the design says so outright: coming back from the icon rail must
// find the tree exactly as it was left.
void GalleryTest::railCollapseAndSectionDisclosureAreSeparateStates()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* rail = new Sidebar(surface);
    rail->setIdentity(Glyph::Vault, QStringLiteral("Vault"), QStringLiteral("NovuVault"));

    NavList* nav = rail->nav();
    nav->addSection(QStringLiteral("Workspaces"), QStringLiteral("sec"));
    nav->addItem(QStringLiteral("ws"), QStringLiteral("Novusion"), Glyph::Folder);
    nav->setExpandable(QStringLiteral("ws"), true);
    nav->setExpanded(QStringLiteral("ws"), true);
    nav->addChild(QStringLiteral("ws.docs"), QStringLiteral("Documents"));
    nav->setCurrentKey(QStringLiteral("ws.docs"));

    nav->setSectionExpanded(QStringLiteral("sec"), false);
    QVERIFY(!nav->isExpanded(QStringLiteral("sec")));

    rail->setCollapsed(true);
    QVERIFY(rail->isCollapsed());
    QVERIFY(nav->isCollapsed());
    // The rail narrowing says nothing about what is folded inside it.
    QVERIFY(!nav->isExpanded(QStringLiteral("sec")));
    QVERIFY(nav->isExpanded(QStringLiteral("ws")));
    QCOMPARE(nav->currentKey(), QStringLiteral("ws.docs"));

    rail->setCollapsed(false);
    QVERIFY(!nav->isCollapsed());
    QVERIFY(!nav->isExpanded(QStringLiteral("sec")));
    QCOMPARE(nav->currentKey(), QStringLiteral("ws.docs"));

    nav->setSectionExpanded(QStringLiteral("sec"), true);
    QVERIFY(nav->isRowVisible(QStringLiteral("ws.docs")));
}

// An application refreshes its rail by clearing it and building it again -
// that is what a count changing or a workspace being renamed costs. A caption
// that forgets it was folded on every refresh is a caption nobody can fold,
// and every application would have to save and restore the same set of keys.
void GalleryTest::aFoldedCaptionSurvivesTheRailBeingRebuilt()
{
    NavList nav;
    const auto fill = [&nav] {
        nav.addSection(QStringLiteral("Workspaces"), QStringLiteral("sec.ws"));
        nav.addItem(QStringLiteral("a"), QStringLiteral("A"));
        nav.addSection(QStringLiteral("System"), QStringLiteral("sec.sys"));
        nav.addItem(QStringLiteral("b"), QStringLiteral("B"));
    };
    fill();
    nav.setSectionExpanded(QStringLiteral("sec.ws"), false);

    nav.clear();
    fill();

    QVERIFY(!nav.isExpanded(QStringLiteral("sec.ws")));
    QVERIFY(!nav.isRowVisible(QStringLiteral("a")));
    // And the one that was never folded comes back open.
    QVERIFY(nav.isExpanded(QStringLiteral("sec.sys")));
    QVERIFY(nav.isRowVisible(QStringLiteral("b")));

    // Reopening it is remembered too, or the rail would be stuck shut.
    nav.setSectionExpanded(QStringLiteral("sec.ws"), true);
    nav.clear();
    fill();
    QVERIFY(nav.isExpanded(QStringLiteral("sec.ws")));
    QVERIFY(nav.isRowVisible(QStringLiteral("a")));
}

// `<button class="sb-bhead" data-block-toggle>` — the whole head folds the
// block. Aiming for the 16px chevron was this port's addition, and it made a
// caption look like a control that mostly did not respond. The `+` is
// hit-tested first, because creating something must not collapse the block it
// is created in.
void GalleryTest::thewholeCaptionFoldsItsBlockButThePlusDoesNot()
{
    QWidget* surface = makeSurface(QStringLiteral("novuspace"));
    auto* nav = new NavList(surface);
    nav->addSection(QStringLiteral("Workspaces"), QStringLiteral("sec"));
    nav->setSectionAction(QStringLiteral("sec"), Glyph::Plus, QStringLiteral("New"));
    nav->addItem(QStringLiteral("a"), QStringLiteral("A"));
    nav->setFixedWidth(238);
    nav->resize(nav->sizeHint());

    QSignalSpy folded(nav, &NavList::expandToggled);
    QSignalSpy added(nav, &NavList::sectionActionTriggered);

    // The words, nowhere near the chevron.
    const QRect caption = nav->rowRect(0);
    QTest::mouseClick(nav, Qt::LeftButton, Qt::NoModifier,
                      QPoint(caption.center().x(), caption.center().y()));
    QCOMPARE(folded.count(), 1);
    QCOMPARE(folded.at(0).at(1).toBool(), false);
    QVERIFY(!nav->isRowVisible(QStringLiteral("a")));
    QCOMPARE(added.count(), 0);

    // The `+`, which is inside the same head and must not fold it.
    const QRect plus = nav->sectionActionRect(0);
    QVERIFY(!plus.isNull());
    QTest::mouseClick(nav, Qt::LeftButton, Qt::NoModifier, plus.center());
    QCOMPARE(added.count(), 1);
    QCOMPARE(folded.count(), 1);
}

// `.navbtn.child .dot` beside every child, or `.tkids{border-left}` down the
// level. Two designs, one list -- so the profile says which.
void GalleryTest::aNestedRowShowsItsLevelTheWayTheProfileSays()
{
    ThemeProfile dotted = ThemeManager::instance().profile(QStringLiteral("novuspace"));
    QCOMPARE(dotted.controls.navChildMark, NavChildMark::Dot);

    ThemeProfile guided = dotted;
    guided.id = QStringLiteral("test-guide");
    guided.controls.navChildMark = NavChildMark::Guide;
    ThemeManager::instance().registerProfile(guided);
    QCOMPARE(ThemeManager::instance().profile(guided.id).controls.navChildMark,
             NavChildMark::Guide);
}

// The design draws a select as a button that opens a menu. A combo box brings
// a system list, a focus frame and a field shape instead.
void GalleryTest::aSelectIsAButtonThatOpensItsMenu()
{
    SelectButton select;
    QCOMPARE(select.focusPolicy(), Qt::NoFocus);
    QVERIFY(select.menu() != nullptr);

    select.setOptions({QStringLiteral("Anthropic"), QStringLiteral("OpenAI"),
                       QStringLiteral("Google")});
    // The first option is in force until something says otherwise, and the
    // menu is marked accordingly -- opening it shows the answer, not only the
    // list.
    QCOMPARE(select.currentKey(), QStringLiteral("Anthropic"));
    QVERIFY(select.menu()->isChecked(QStringLiteral("Anthropic")));

    QSignalSpy changed(&select, &SelectButton::currentChanged);
    select.setCurrentKey(QStringLiteral("OpenAI"));
    QCOMPARE(select.currentKey(), QStringLiteral("OpenAI"));
    QVERIFY(select.menu()->isChecked(QStringLiteral("OpenAI")));
    QVERIFY(!select.menu()->isChecked(QStringLiteral("Anthropic")));
    // Setting it is not choosing it: only the menu reports a choice.
    QCOMPARE(changed.count(), 0);

    // What it carries is paid for in width.
    const int bare = select.sizeHint().width();
    select.setGlyph(Glyph::Agent);
    QVERIFY(select.sizeHint().width() > bare);
    const int withGlyph = select.sizeHint().width();
    select.setStatus(StatusDot::Tone::Success, true);
    QVERIFY(select.sizeHint().width() > withGlyph);

    // `.select` is 26px and `.select.xs` is 23px, both from the profile.
    select.setControlSize(ControlSize::Small);
    QCOMPARE(select.height(), themeFor(&select).controlHeight(ControlSize::Small));
}

// `.card` sits on the page behind a hairline; `.card.raised` sits on a panel,
// takes the panel shade and drops the outline, because an outline against a
// surface one step away from it reads as a seam.
void GalleryTest::aRaisedCardKeepsItsOutlineOnAPanel()
{
    QWidget* surface = makeSurface(QStringLiteral("novuai"));
    const ThemeProfile& profile = themeFor(surface);

    auto* plain = new Card(QStringLiteral("Plain"), surface);
    auto* raised = new Card(QStringLiteral("Raised"), surface);
    raised->setSurface(Card::Surface::Raised);

    QCOMPARE(plain->surface(), Card::Surface::Plain);
    QCOMPARE(raised->surface(), Card::Surface::Raised);

    plain->resize(200, 90);
    raised->resize(200, 90);
    const QImage plainShot = plain->grab().toImage();
    const QImage raisedShot = raised->grab().toImage();

    // The middle of each is its own ground.
    QCOMPARE(QColor(plainShot.pixel(100, 60)), profile.colors.panelBg);
    QCOMPARE(QColor(raisedShot.pixel(100, 60)), profile.colors.panelBgAlt);

    // Surface changes do not remove the inherited border: sample the edge
    // away from the rounded corners.
    const QColor plainEdge(plainShot.pixel(0, 45));
    const QColor raisedEdge(raisedShot.pixel(0, 45));
    QCOMPARE(plainEdge, raisedEdge);
}

// `.seg button{flex:1}` -- two faces of one panel are two halves of one
// control, not a short word beside a long one.
void GalleryTest::segmentsCanShareTheTrayBetweenThem()
{
    Segmented seg({QStringLiteral("Tasks"), QStringLiteral("Changes and more")});
    QVERIFY(!seg.fills());
    seg.resize(300, seg.sizeHint().height());

    const int narrow = seg.itemRect(0).width();
    const int wide = seg.itemRect(1).width();
    QVERIFY(wide > narrow);

    seg.setFill(true);
    QVERIFY(seg.fills());
    // Even, and adding up to the tray exactly -- a rounded share left a pixel
    // at the trailing edge that read as a misdrawn border.
    const int a = seg.itemRect(0).width();
    const int b = seg.itemRect(1).width();
    QVERIFY(qAbs(a - b) <= 1);
    // Together they fill the tray: the second ends where the first began plus
    // both shares and the gap between them, with nothing left over.
    QCOMPARE(seg.itemRect(1).right() - seg.itemRect(0).left() + 1,
             a + b + (seg.itemRect(1).left() - seg.itemRect(0).right() - 1));
}

// The Novusion family mark: one drawing, worn by every product in its own
// accent. Not a plate with a glyph on it.
void GalleryTest::theFamilyMarkIsOneDrawingInEveryAccent()
{
    for (const QString& id : ThemeCatalog::builtinProfileIds()) {
        QWidget* surface = makeSurface(id);
        auto* mark = new AppMark(Glyph::None, surface);
        mark->setShape(AppMark::Shape::Gem);
        mark->setSideOverride(48);
        QCOMPARE(mark->shape(), AppMark::Shape::Gem);

        const QImage shot = mark->grab().toImage();
        // The right face is the accent itself; the left is a shade of it and
        // the top a tint, so all three are the same hue.
        const QColor right(shot.pixel(30, 24));
        const QColor accent = themeFor(surface).colors.accent;
        QVERIFY2(qAbs(right.hue() - accent.hue()) <= 4,
                 qPrintable(QStringLiteral("%1: %2 vs %3")
                                .arg(id, right.name(), accent.name())));
        // A gem carries no plate, so the corner is whatever is behind it.
        QVERIFY(QColor(shot.pixel(1, 1)).alpha() == 0
                || QColor(shot.pixel(1, 1)) != accent);
    }
}

#include "tst_gallery.moc"
