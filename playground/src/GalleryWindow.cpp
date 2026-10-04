#include "novu/playground/GalleryWindow.h"

#include "novu/playground/ComponentRegistry.h"
#include "novu/playground/DevWatcher.h"
#include "novu/playground/ExampleLayout.h"
#include "novu/playground/Paths.h"
#include "novu/playground/SampleData.h"
#include "novu/playground/ThemeCatalog.h"

#include "novu/ui/Badge.h"
#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Feedback.h"
#include "novu/ui/Fields.h"
#include "novu/ui/Layout.h"
#include "novu/ui/Icon.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/ThemeScope.h"
#include "novu/ui/Tile.h"

#include <QComboBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>

using namespace novu::ui;

namespace novu::playground {

namespace {

constexpr int kNavWidth = 260;
constexpr int kBuildLogHeight = 120;

Badge::Tone toneFor(BuildState state)
{
    switch (state) {
    case BuildState::Ready:     return Badge::Tone::Success;
    case BuildState::Stale:     return Badge::Tone::Warning;
    case BuildState::Building:  return Badge::Tone::Info;
    case BuildState::Reloading: return Badge::Tone::Info;
    case BuildState::Failed:    return Badge::Tone::Danger;
    }
    return Badge::Tone::Neutral;
}

} // namespace

GalleryWindow::GalleryWindow(ComponentRegistry& registry, SampleData& data,
                             ThemeCatalog& themes, QWidget* parent)
    : AppWindow(parent)
    , m_registry(registry)
    , m_data(data)
    , m_themes(themes)
    , m_build(new BuildBridge(this))
    , m_watcher(new DevWatcher(this))
{
    // Through the shell, so the caption in the bar and the title the taskbar
    // and Alt-Tab show are one statement rather than two.
    m_chromeProfileId = ThemeManager::instance().defaultProfileId();
    m_profileId = m_chromeProfileId;

    setTitle(QStringLiteral("NovuUI Component Gallery"));
    // Its own icon set, at the profile's own icon size, in the profile's own
    // title ink. The gallery has no art of its own and should not acquire any:
    // whatever it shows in its bar is one more thing the library is claiming
    // to be able to draw.
    const ThemeProfile& chrome = ThemeManager::instance().profile(m_chromeProfileId);
    setIcon(glyphIcon(Glyph::Overview, chrome.window.iconSize, chrome.window.titleText));
    shell()->setContentSize(QSize(1440, 940));

    buildChrome();
    // Labelled for whichever Theme the gallery opens on, before anything is
    // shown.
    refreshApplicationFilter();
    rebuildNavigation();

    const QVector<ComponentSpec> specs = m_registry.all();
    if (!specs.isEmpty()) {
        selectComponent(specs.first().id);
    }

    connect(m_build, &BuildBridge::stateChanged, this, &GalleryWindow::onBuildStateChanged);
    connect(m_build, &BuildBridge::logAppended, this, [this](const QString& chunk) {
        m_buildLog->setText(m_buildLog->text() + chunk);
        if (auto* bar = m_buildLogArea->verticalScrollBar()) {
            bar->setValue(bar->maximum());
        }
    });

    // Sources need a build; data does not. Keeping the two apart is the whole
    // honesty of this tool.
    connect(m_watcher, &DevWatcher::sourcesChanged, this, &GalleryWindow::onSourcesChanged);
    connect(m_watcher, &DevWatcher::dataChanged, this, &GalleryWindow::onDataChanged);
    m_watcher->watchSources({paths::repoRoot() + QStringLiteral("/src"),
                             paths::repoRoot() + QStringLiteral("/include/novu/ui")});
    m_watcher->watchData({paths::sampleDataDir(), paths::themeDir()});

    showDataProblems();
    setStatus(BuildState::Ready, QString());
}

// ── chrome ────────────────────────────────────────────────────────────────

void GalleryWindow::buildChrome()
{
    auto* root = new QWidget(this);
    setContentWidget(root);

    // The chrome is a scope of its own, on the default profile. Everything
    // below is therefore drawn by NovuUI, styled by NovuUI, and unaffected by
    // whichever profile the developer selects for the preview.
    applyStyleSheet(root, m_chromeProfileId);

    auto* columns = new QHBoxLayout(root);
    columns->setContentsMargins(0, 0, 0, 0);
    columns->setSpacing(0);

    // ── navigation rail ──────────────────────────────────────────────────
    auto* rail = new QWidget(root);
    rail->setFixedWidth(kNavWidth);
    auto* railColumn = new QVBoxLayout(rail);
    const ThemeProfile& profile = themeFor(root);
    const int pad = profile.space(SpacingRole::CardPadding);
    railColumn->setContentsMargins(pad, pad, pad / 2, pad);
    railColumn->setSpacing(profile.space(SpacingRole::FormRowGap));

    railColumn->addWidget(label(QStringLiteral("NovuUI"), rail, TypeRole::PageTitle));
    railColumn->addWidget(label(QStringLiteral("Component gallery"), rail, TypeRole::Caption,
                                Ink::Muted));

    m_search = lineEdit(QStringLiteral("Search components"), rail);
    m_search->setClearButtonEnabled(true);
    connect(m_search, &QLineEdit::textChanged, this, &GalleryWindow::onSearchChanged);
    railColumn->addWidget(m_search);

    // Under the search box because it does the same job by other means:
    // search when you know the name, category when you do not. "All" first,
    // so the gallery still opens on everything.
    QStringList categories{QStringLiteral("All categories")};
    for (Category c : {Category::Foundation, Category::TitleBar, Category::Navigation,
                       Category::Footer, Category::Input, Category::DataDisplay,
                       Category::Feedback, Category::Help}) {
        categories << categoryLabel(c);
    }
    m_categoryBox = comboBox(categories, rail);
    connect(m_categoryBox, &QComboBox::currentIndexChanged, this,
            &GalleryWindow::onCategoryChanged);
    railColumn->addWidget(m_categoryBox);

    m_navScroll = new QScrollArea(rail);
    m_navScroll->setObjectName(QLatin1String(name::scrollArea));
    m_navScroll->setWidgetResizable(true);
    m_navScroll->setFrameShape(QFrame::NoFrame);
    m_navScroll->viewport()->setAutoFillBackground(false);
    m_navHost = new QWidget(m_navScroll);
    m_navHost->setAutoFillBackground(false);
    m_navLayout = new QVBoxLayout(m_navHost);
    m_navLayout->setContentsMargins(0, 0, 0, 0);
    m_navLayout->setSpacing(profile.space(SpacingRole::FormRowGap) / 2);
    m_navScroll->setWidget(m_navHost);
    railColumn->addWidget(m_navScroll, 1);

    columns->addWidget(rail);

    // ── the working column ───────────────────────────────────────────────
    auto* work = new QWidget(root);
    auto* workColumn = new QVBoxLayout(work);
    workColumn->setContentsMargins(pad / 2, pad, pad, pad);
    workColumn->setSpacing(profile.space(SpacingRole::CardGap));

    // header: title, metadata, and every action
    auto* header = new QWidget(work);
    auto* headerColumn = new QVBoxLayout(header);
    headerColumn->setContentsMargins(0, 0, 0, 0);
    headerColumn->setSpacing(profile.space(SpacingRole::FormRowGap) / 2);

    m_title = label(QStringLiteral("NovuUI"), header, TypeRole::PageTitle);
    headerColumn->addWidget(m_title);
    m_subtitle = label(QString(), header, TypeRole::Body, Ink::Secondary);
    m_subtitle->setWordWrap(true);
    headerColumn->addWidget(m_subtitle);
    m_meta = label(QString(), header, TypeRole::Caption, Ink::Muted);
    m_meta->setWordWrap(true);
    headerColumn->addWidget(m_meta);

    QHBoxLayout* actions = toolbar(headerColumn);

    actions->addWidget(label(QStringLiteral("Scenario"), header, TypeRole::Label, Ink::Muted));
    m_scenarioBox = comboBox({}, header, 190);
    connect(m_scenarioBox, &QComboBox::currentIndexChanged, this,
            &GalleryWindow::onScenarioChanged);
    actions->addWidget(m_scenarioBox);

    actions->addWidget(label(QStringLiteral("Theme"), header, TypeRole::Label, Ink::Muted));
    m_profileBox = comboBox({}, header, 220);
    connect(m_profileBox, &QComboBox::currentIndexChanged, this,
            &GalleryWindow::onProfileChanged);
    actions->addWidget(m_profileBox);

    // Visibility only, and no second dropdown: the Theme above already names
    // the application, so this is the whole control.
    m_appFilter = checkBox(QStringLiteral("Designed for this app only"), header);
    connect(m_appFilter, &QCheckBox::toggled, this,
            &GalleryWindow::onApplicationFilterToggled);
    actions->addWidget(m_appFilter);

    actions->addStretch(1);

    m_sourceButton = ghostButton(QStringLiteral("Open source"), header);
    connect(m_sourceButton, &QPushButton::clicked, this, &GalleryWindow::onOpenSource);
    actions->addWidget(m_sourceButton);

    m_rebuildButton = secondaryButton(QStringLiteral("Rebuild && reload"), header);
    m_rebuildButton->setToolTip(m_build->canBuild()
                                    ? m_build->buildCommandDescription()
                                    : QStringLiteral("No configured build tree at %1")
                                          .arg(paths::buildDir()));
    m_rebuildButton->setEnabled(m_build->canBuild());
    connect(m_rebuildButton, &QPushButton::clicked, this, &GalleryWindow::onRebuild);
    actions->addWidget(m_rebuildButton);

    // The build status gets its own line under the controls rather than a
    // place in the toolbar row. It is the one item here whose width is not
    // known in advance - "Rebuild needed" grows a list of every file that
    // changed - and sharing a row with the controls meant that list pushed
    // Theme and Scenario across the window whenever it got long.
    //
    // Left-aligned, because it is a statement about the state of the preview
    // and reads from the same edge as everything else in the header.
    QHBoxLayout* statusRow = toolbar(headerColumn);
    m_status = new Badge(QStringLiteral("Ready"), Badge::Tone::Success, header);
    statusRow->addWidget(m_status);
    statusRow->addStretch(1);

    workColumn->addWidget(header);

    // A problem strip, hidden until there is one.
    m_problem = new InfoBanner(QString(), Badge::Tone::Danger, work);
    m_problem->hide();
    workColumn->addWidget(m_problem);

    m_buildLogArea = new QScrollArea(work);
    m_buildLogArea->setObjectName(QLatin1String(name::scrollArea));
    m_buildLogArea->setWidgetResizable(true);
    m_buildLogArea->setFrameShape(QFrame::NoFrame);
    m_buildLogArea->setFixedHeight(kBuildLogHeight);
    m_buildLog = label(QString(), m_buildLogArea, TypeRole::Mono, Ink::Muted);
    m_buildLog->setFont(roleMonoFont(m_buildLogArea, TypeRole::Mono));
    m_buildLog->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    // Compiler and linker output is one unbroken line per diagnostic. Without
    // this the label's own width becomes the scroll area's minimum, and the
    // minimum becomes the window's.
    m_buildLog->setWordWrap(true);
    m_buildLog->setMinimumWidth(0);
    m_buildLog->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_buildLogArea->setWidget(m_buildLog);
    m_buildLogArea->hide();
    workColumn->addWidget(m_buildLogArea);

    // ── the preview surface ──────────────────────────────────────────────
    // Its own scope and its own composed stylesheet. This is the only widget
    // in the window whose profile changes.
    m_surface = new QWidget(work);
    m_surface->setAutoFillBackground(true);
    m_surfaceLayout = new QVBoxLayout(m_surface);
    m_surfaceLayout->setContentsMargins(0, 0, 0, 0);
    m_surfaceLayout->setSpacing(0);
    applyStyleSheet(m_surface, m_profileId);

    auto* previewScroll = new QScrollArea(work);
    previewScroll->setObjectName(QLatin1String(name::scrollArea));
    previewScroll->setWidgetResizable(true);
    previewScroll->setFrameShape(QFrame::NoFrame);
    previewScroll->setWidget(m_surface);
    workColumn->addWidget(previewScroll, 1);

    columns->addWidget(work, 1);

    refreshProfileSelector();
}

void GalleryWindow::rebuildNavigation()
{
    // Rebuilt rather than filtered in place: the entries are Tiles whose text
    // is fixed at construction, and a nav of twenty widgets is not worth an
    // incremental diff.
    m_navTiles.clear();
    while (QLayoutItem* item = m_navLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    const QString query = m_search ? m_search->text() : QString();
    // The registry does the filtering; the gallery only says which application
    // is in view. No membership rule lives in this file.
    const QString application = filterApplicationId();
    QVector<ComponentSpec> specs = m_registry.search(query, application);

    // Index 0 is "All categories", so the enum starts one along.
    const int pick = m_categoryBox ? m_categoryBox->currentIndex() : 0;
    const bool oneCategory = pick > 0;
    if (oneCategory) {
        const auto wanted = static_cast<Category>(pick - 1);
        QVector<ComponentSpec> kept;
        kept.reserve(specs.size());
        for (const ComponentSpec& spec : specs) {
            if (spec.category == wanted) {
                kept.append(spec);
            }
        }
        specs = kept;
    }

    if (specs.isEmpty()) {
        const QString message =
            application.isEmpty()
                ? QStringLiteral("No component matches \"%1\".").arg(query)
                : QStringLiteral("No component recorded for %1%2.")
                      .arg(ThemeCatalog::displayNameFor(application),
                           query.isEmpty() ? QString()
                                           : QStringLiteral(" matching \"%1\"").arg(query));
        m_navLayout->addWidget(label(message, m_navHost, TypeRole::Caption, Ink::Muted));
        m_navLayout->addStretch(1);
        return;
    }

    Category current = specs.first().category;
    bool first = true;
    for (const ComponentSpec& spec : specs) {
        if (!oneCategory && (first || spec.category != current)) {
            m_navLayout->addWidget(caption(categoryLabel(spec.category), m_navHost));
            current = spec.category;
            first = false;
        }
        // Tile, not a bespoke nav button: it is NovuUI's own "choose one of
        // several" control, and using it here means a regression in it shows
        // up the moment the gallery opens.
        auto* tile = new Tile(spec.name, categoryLabel(spec.category), m_navHost,
                              spec.id == m_componentId);
        tile->setProperty("componentId", spec.id);
        const QString id = spec.id;
        connect(tile, &QPushButton::clicked, this, [this, id]() { selectComponent(id); });
        m_navLayout->addWidget(tile);
        m_navTiles.append(tile);
    }
    m_navLayout->addStretch(1);
}

// ── selection ─────────────────────────────────────────────────────────────

void GalleryWindow::selectComponent(const QString& id)
{
    const ComponentSpec* spec = m_registry.find(id);
    if (!spec) {
        return;
    }
    m_componentId = id;

    for (Tile* tile : m_navTiles) {
        tile->setChecked(tile->property("componentId").toString() == id);
    }

    m_title->setText(spec->name);
    m_subtitle->setText(spec->description);

    QStringList meta;
    meta << QStringLiteral("%1 · %2").arg(categoryLabel(spec->category), spec->id);
    if (!spec->variants.isEmpty()) {
        meta << QStringLiteral("variants: %1").arg(spec->variants.join(QStringLiteral(", ")));
    }
    if (!spec->states.isEmpty()) {
        meta << QStringLiteral("states: %1").arg(spec->states.join(QStringLiteral(", ")));
    }
    // Adoption and intent, never merged into one line. The gallery is what
    // people quote, and a catalogue that overstates adoption is worse than one
    // that says nothing.
    meta << (spec->usedBy.isEmpty()
                 ? QStringLiteral("currently used by: nothing yet — built, not adopted")
                 : QStringLiteral("currently used by: %1")
                       .arg(spec->usedBy.join(QStringLiteral(", "))));
    if (!spec->designedFor.isEmpty()) {
        meta << QStringLiteral("designed for: %1")
                    .arg(spec->designedFor.join(QStringLiteral(", ")));
    }
    meta << QStringLiteral("%1 · %2").arg(spec->headerPath, spec->sourcePath);
    m_meta->setText(meta.join(QStringLiteral("\n")));

    m_sourceButton->setEnabled(paths::existsInRepo(spec->sourcePath));

    refreshScenarioSelector(*spec);
    repopulatePreview();
}

void GalleryWindow::refreshScenarioSelector(const ComponentSpec& spec)
{
    m_updatingSelectors = true;
    m_scenarioBox->clear();

    const QVector<Scenario> supported = spec.supportedScenarios();
    int index = 0;
    for (int i = 0; i < supported.size(); ++i) {
        const Scenario scenario = supported.at(i);
        m_scenarioBox->addItem(scenarioLabel(scenario), scenarioKey(scenario));
        m_scenarioBox->setItemData(i, scenarioDescription(scenario), Qt::ToolTipRole);
        if (scenario == m_scenario) {
            index = i;
        }
    }
    // A component that does not implement the current scenario falls back to
    // its first, rather than showing a page for a situation it does not have.
    m_scenario = supported.value(index, Scenario::Normal);
    m_scenarioBox->setCurrentIndex(index);
    m_updatingSelectors = false;
}

void GalleryWindow::refreshProfileSelector()
{
    m_updatingSelectors = true;
    m_profileBox->clear();
    const QStringList ids = ThemeCatalog::availableProfileIds();
    for (int i = 0; i < ids.size(); ++i) {
        const QString id = ids.at(i);
        m_profileBox->addItem(ThemeCatalog::displayNameFor(id), id);
        if (id == m_profileId) {
            m_profileBox->setCurrentIndex(i);
        }
    }
    if (m_profileBox->currentIndex() < 0 && !ids.isEmpty()) {
        m_profileId = ids.first();
        m_profileBox->setCurrentIndex(0);
    }
    m_updatingSelectors = false;
}

void GalleryWindow::repopulatePreview()
{
    const ComponentSpec* spec = m_registry.find(m_componentId);
    if (!spec) {
        return;
    }

    if (m_page) {
        m_surfaceLayout->removeWidget(m_page);
        m_page->deleteLater();
        m_page = nullptr;
    }

    PreviewContext ctx;
    ctx.parent = m_surface;
    ctx.scenario = m_scenario;
    ctx.data = &m_data;
    ctx.profileId = m_profileId;

    QWidget* page = spec->build ? spec->build(ctx) : nullptr;
    if (!page) {
        // Reported rather than tolerated: a page that returns nothing is a
        // registry entry that lies about being previewable.
        page = new QWidget(m_surface);
        auto* column = previewPage(page);
        column->addWidget(new InfoBanner(
            QStringLiteral("\"%1\" built no widget.").arg(spec->id),
            Badge::Tone::Danger, page));
        column->addStretch(1);
    }
    m_page = page;
    m_surfaceLayout->addWidget(m_page);
}

// ── slots ─────────────────────────────────────────────────────────────────

void GalleryWindow::onSearchChanged(const QString&)
{
    rebuildNavigation();
}

void GalleryWindow::onCategoryChanged(int)
{
    rebuildNavigation();
}

void GalleryWindow::onScenarioChanged(int index)
{
    if (m_updatingSelectors || index < 0) {
        return;
    }
    m_scenario = scenarioFromKey(m_scenarioBox->itemData(index).toString());
    repopulatePreview();
}

void GalleryWindow::onProfileChanged(int index)
{
    if (m_updatingSelectors || index < 0) {
        return;
    }
    m_profileId = m_profileBox->itemData(index).toString();

    // Both halves, together: the scope so themeFor() inside the surface
    // resolves to this profile, and the composed sheet so the styled widgets
    // agree with the self-painted ones. applyStyleSheet() does both.
    applyStyleSheet(m_surface, m_profileId);

    // The filter's context is the Theme, so changing the Theme changes what the
    // navigation shows - immediately, without touching the filter itself.
    refreshApplicationFilter();
    if (m_appFilter && m_appFilter->isChecked()) {
        rebuildNavigation();
    }

    // Rebuilt as well as repainted. A ThemeChangeEvent is enough for a
    // self-painted component, but NovuUI components also read geometry tokens
    // at construction — control heights, paddings, fonts — and a repaint
    // would leave those at the old profile's values.
    repopulatePreview();
}

QString GalleryWindow::filterApplicationId() const
{
    if (!m_appFilter || !m_appFilter->isChecked()) {
        return QString();
    }
    // Only a profile that names a product can narrow anything. Midnight Blue is
    // an appearance a user picks, not an application, so with it selected the
    // filter has nothing to filter by and the gallery stays whole.
    return novu::ui::isApplicationProfile(m_profileId) ? m_profileId : QString();
}

void GalleryWindow::refreshApplicationFilter()
{
    if (!m_appFilter) {
        return;
    }
    const bool isApplication = novu::ui::isApplicationProfile(m_profileId);
    m_appFilter->setEnabled(isApplication);
    m_appFilter->setText(isApplication
                             ? QStringLiteral("Designed for %1 only")
                                   .arg(ThemeCatalog::displayNameFor(m_profileId))
                             : QStringLiteral("Designed for this app only"));
    m_appFilter->setToolTip(
        isApplication
            ? QStringLiteral("Show only the components %1 is meant to use. That is "
                             "applicability, not adoption - each component also states "
                             "whether %1 consumes it today, and during extraction almost "
                             "none of them do.")
                  .arg(ThemeCatalog::displayNameFor(m_profileId))
            : QStringLiteral("%1 is a selectable theme, not an application, so there is "
                             "nothing to filter by.")
                  .arg(ThemeCatalog::displayNameFor(m_profileId)));
}

void GalleryWindow::onApplicationFilterToggled(bool on)
{
    Q_UNUSED(on);
    if (m_updatingSelectors) {
        return;
    }
    // Visibility only: the navigation is rebuilt and nothing else. The selected
    // component keeps its scenario and its profile, and if the filter hides it
    // the preview is left alone rather than jumping to another component -
    // hiding an entry is not the same as choosing a different one.
    rebuildNavigation();
}

void GalleryWindow::onOpenSource()
{
    const ComponentSpec* spec = m_registry.find(m_componentId);
    if (!spec) {
        return;
    }
    if (!paths::openInEditor(spec->sourcePath)) {
        m_problem->setTone(Badge::Tone::Warning);
        m_problem->setText(QStringLiteral("Could not open %1. Set NOVUUI_EDITOR to choose an "
                                          "editor.")
                               .arg(paths::absoluteInRepo(spec->sourcePath)));
        m_problem->show();
    }
}

void GalleryWindow::onRebuild()
{
    m_build->setRestoreArguments(restoreArguments());
    m_buildLog->clear();
    m_buildLogArea->show();
    m_build->rebuild();
}

void GalleryWindow::onSourcesChanged(const QStringList& paths)
{
    QStringList names;
    for (const QString& path : paths) {
        names.append(QFileInfo(path).fileName());
    }
    // Marked, not built. An automatic compiler on every keystroke is how a
    // development tool becomes something you turn off.
    m_build->markStale(QStringLiteral("%1 changed — the preview is the previous binary")
                           .arg(names.join(QStringLiteral(", "))));
}

void GalleryWindow::onDataChanged(const QStringList& changed)
{
    m_data.reload();
    m_themes.reload();
    refreshProfileSelector();

    // No build, no restart. Re-registering a profile has already repainted
    // every live widget; rebuilding the page picks up new sample rows too.
    repopulatePreview();
    showDataProblems();

    if (m_build->state() != BuildState::Failed && m_build->state() != BuildState::Stale) {
        QStringList names;
        for (const QString& path : changed) {
            names.append(QFileInfo(path).fileName());
        }
        setStatus(BuildState::Ready,
                  QStringLiteral("reloaded %1").arg(names.join(QStringLiteral(", "))));
    }
}

void GalleryWindow::onBuildStateChanged(BuildState state, const QString& detail)
{
    setStatus(state, detail);

    if (state == BuildState::Failed) {
        m_problem->setTone(Badge::Tone::Danger);
        m_problem->setText(detail.section(QLatin1Char('\n'), 0, 0));
        m_problem->show();
        m_buildLogArea->show();
        // The preview is deliberately untouched. What is on screen is the
        // last render that compiled, which is the thing being compared
        // against.
    } else if (state == BuildState::Ready) {
        showDataProblems();
    }

    m_rebuildButton->setEnabled(m_build->canBuild() && state != BuildState::Building
                                && state != BuildState::Reloading);
}

void GalleryWindow::setStatus(BuildState state, const QString& detail)
{
    m_status->setTone(toneFor(state));
    m_status->setText(detail.isEmpty()
                          ? buildStateLabel(state)
                          : QStringLiteral("%1 — %2").arg(buildStateLabel(state),
                                                          detail.section(QLatin1Char('\n'), 0, 0)));
    m_status->setToolTip(detail);
}

void GalleryWindow::showDataProblems()
{
    QStringList problems = m_data.errors();
    problems += m_themes.errors();
    if (problems.isEmpty()) {
        m_problem->hide();
        return;
    }
    m_problem->setTone(Badge::Tone::Warning);
    m_problem->setText(problems.first());
    m_problem->setToolTip(problems.join(QStringLiteral("\n")));
    m_problem->show();
}

// ── restore ───────────────────────────────────────────────────────────────

QStringList GalleryWindow::restoreArguments() const
{
    return {QStringLiteral("--component"), m_componentId,
            QStringLiteral("--scenario"),  scenarioKey(m_scenario),
            QStringLiteral("--profile"),   m_profileId};
}

void GalleryWindow::restore(const QString& componentId, const QString& scenario,
                            const QString& profileId)
{
    if (!profileId.isEmpty() && ThemeManager::instance().hasProfile(profileId)) {
        m_profileId = profileId;
        applyStyleSheet(m_surface, m_profileId);
        refreshProfileSelector();
    }
    if (!scenario.isEmpty()) {
        m_scenario = scenarioFromKey(scenario);
    }
    if (!componentId.isEmpty() && m_registry.find(componentId)) {
        selectComponent(componentId);
    } else {
        repopulatePreview();
    }
}

} // namespace novu::playground
