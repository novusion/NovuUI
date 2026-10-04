#pragma once

#include "novu/playground/BuildBridge.h"
#include "novu/playground/ComponentSpec.h"
#include "novu/playground/Scenario.h"

#include "novu/ui/Fields.h"

#include "novu/ui/WindowShell.h"
#include <QString>

class QComboBox;
class QLabel;
class QLineEdit;
class QScrollArea;
class QVBoxLayout;

namespace novu::ui {
class Badge;
class Button;
class InfoBanner;
class Tile;
} // namespace novu::ui

namespace novu::playground {

class ComponentRegistry;
class DevWatcher;
class SampleData;
class ThemeCatalog;

// The gallery shell: navigation, search, scenario and profile selectors,
// build status, and one preview surface.
//
// The chrome is built from NovuUI and nothing else — the navigation entries
// are Tiles, the search box is lineEdit(), the actions are Buttons, the
// status is a Badge, the failure notice is an InfoBanner. That is not
// decoration: a tool built from the library it previews finds the library's
// bugs first.
//
// The preview surface is a separate ThemeScope from the chrome. The developer
// switches the profile of what they are inspecting; the frame around it stays
// put, so there is always an unchanged reference on screen. Switching also
// rebuilds the page, because NovuUI components read geometry tokens at
// construction as well as at paint.
// It wears NovuUI's own frame rather than the platform's.
//
// A component gallery that asks the operating system to draw its title bar is
// the one window in the product that does not use the thing it is advertising -
// and the title bar is a component like any other, so not showing it here means
// it is the only one nobody looks at. AppWindow is used exactly the way an
// application uses it: a title, an icon and a content widget.
class GalleryWindow : public novu::ui::AppWindow
{
    Q_OBJECT

public:
    GalleryWindow(ComponentRegistry& registry, SampleData& data,
                  ThemeCatalog& themes, QWidget* parent = nullptr);

    // Restores a selection after a rebuild-and-restart. Unknown values are
    // ignored rather than refused.
    void restore(const QString& componentId, const QString& scenarioKey,
                 const QString& profileId);

    QString currentComponentId() const { return m_componentId; }
    Scenario currentScenario() const { return m_scenario; }
    QString currentProfileId() const { return m_profileId; }

    // The command line that would reopen the gallery exactly here.
    QStringList restoreArguments() const;

private slots:
    void onSearchChanged(const QString& query);
    void onCategoryChanged(int index);
    void onScenarioChanged(int index);
    void onApplicationFilterToggled(bool on);
    void onProfileChanged(int index);
    void onOpenSource();
    void onRebuild();
    void onSourcesChanged(const QStringList& paths);
    void onDataChanged(const QStringList& paths);
    void onBuildStateChanged(BuildState state, const QString& detail);

private:
    void buildChrome();
    void rebuildNavigation();
    // The application the selected Theme stands for, or empty when the Theme is
    // a selectable appearance rather than a product. Empty means the filter has
    // nothing to filter by.
    QString filterApplicationId() const;
    void refreshApplicationFilter();
    void selectComponent(const QString& id);
    void repopulatePreview();
    void refreshScenarioSelector(const ComponentSpec& spec);
    void refreshProfileSelector();
    void showDataProblems();
    void setStatus(BuildState state, const QString& detail);

    ComponentRegistry& m_registry;
    SampleData& m_data;
    ThemeCatalog& m_themes;
    BuildBridge* m_build = nullptr;
    DevWatcher* m_watcher = nullptr;

    // chrome
    QLineEdit* m_search = nullptr;
    QWidget* m_navHost = nullptr;
    QVBoxLayout* m_navLayout = nullptr;
    QScrollArea* m_navScroll = nullptr;
    // Narrowing the rail to one category. Seventy components in one scroll
    // is a list you read rather than a list you navigate, and the captions
    // that separate them only help once you already know which caption you
    // are looking for.
    QComboBox* m_categoryBox = nullptr;
    QComboBox* m_scenarioBox = nullptr;
    QComboBox* m_profileBox = nullptr;
    // One control, not a second application dropdown: the selected Theme
    // already says which application is in view, so the filter only has to be
    // on or off.
    novu::ui::CheckBox* m_appFilter = nullptr;
    novu::ui::Badge* m_status = nullptr;
    novu::ui::Button* m_rebuildButton = nullptr;
    novu::ui::Button* m_sourceButton = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_subtitle = nullptr;
    QLabel* m_meta = nullptr;
    novu::ui::InfoBanner* m_problem = nullptr;
    QLabel* m_buildLog = nullptr;
    QScrollArea* m_buildLogArea = nullptr;

    // preview
    QWidget* m_surface = nullptr;
    QVBoxLayout* m_surfaceLayout = nullptr;
    QWidget* m_page = nullptr;

    QList<novu::ui::Tile*> m_navTiles;
    QString m_componentId;
    Scenario m_scenario = Scenario::Normal;
    QString m_profileId;
    // The chrome's own profile. Fixed, so the frame is a constant while the
    // thing being inspected changes.
    QString m_chromeProfileId;
    bool m_updatingSelectors = false;
};

} // namespace novu::playground
