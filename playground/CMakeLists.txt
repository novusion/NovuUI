# NovuUI Component Gallery — development only.
#
# Reached exclusively through NOVUUI_BUILD_PLAYGROUND in the parent
# CMakeLists, which defaults to PROJECT_IS_TOP_LEVEL. An application that
# add_subdirectory()s NovuUI never configures this file.
#
# Two targets on purpose:
#   NovuUIPlaygroundLib   registry, sample data, theme catalog, pages, window
#   NovuUIPlayground      the executable — main() and nothing else
# The tests link the library. A gallery whose only entry point is a GUI
# executable cannot be tested.

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_library(NovuUIPlaygroundLib STATIC
    include/novu/playground/BuildBridge.h
    include/novu/playground/ComponentRegistry.h
    include/novu/playground/ComponentSpec.h
    include/novu/playground/DevWatcher.h
    include/novu/playground/ExampleLayout.h
    include/novu/playground/GalleryWindow.h
    include/novu/playground/Paths.h
    include/novu/playground/SampleData.h
    include/novu/playground/Scenario.h
    include/novu/playground/ShadowRun.h
    include/novu/playground/ThemeCatalog.h
    src/BuildBridge.cpp
    src/ComponentRegistry.cpp
    src/ComponentSpec.cpp
    src/Components.cpp
    src/DevWatcher.cpp
    src/ExampleLayout.cpp
    src/GalleryWindow.cpp
    src/Paths.cpp
    src/SampleData.cpp
    src/Scenario.cpp
    src/ShadowRun.cpp
    src/ThemeCatalog.cpp
    src/pages/AdminPage.cpp
    src/pages/ApplicationRulesPage.cpp
    src/pages/BadgePage.cpp
    src/pages/ChromePage.cpp
    src/pages/DiffViewPage.cpp
    src/pages/DisclosurePage.cpp
    src/pages/KeysPage.cpp
    src/pages/LoadingPage.cpp
    src/pages/LogViewPage.cpp
    src/pages/DropTargetPage.cpp
    src/pages/MotionPage.cpp
    src/pages/OverlayPage.cpp
    src/pages/PopoverPage.cpp
    src/pages/SettingRowPage.cpp
    src/pages/SettingsPage.cpp
    src/pages/SidebarPage.cpp
    src/pages/TooltipPage.cpp
    src/pages/TreeViewPage.cpp
    src/pages/FilePickerPage.cpp
    src/pages/SplashScreenPage.cpp
    src/pages/WorkspacePage.cpp
    src/pages/BottomPanelPage.cpp
    src/pages/ButtonPage.cpp
    src/pages/CardPage.cpp
    src/pages/ChipPage.cpp
    src/pages/FeedbackFormPage.cpp
    src/pages/FeedbackPage.cpp
    src/pages/FieldPage.cpp
    src/pages/FieldsPage.cpp
    src/pages/FlowLayoutPage.cpp
    src/pages/FormPage.cpp
    src/pages/HelpOverlayPage.cpp
    src/pages/IconButtonPage.cpp
    src/pages/IconPage.cpp
    src/pages/IdentityPage.cpp
    src/pages/LayoutPage.cpp
    src/pages/Pages.h
    src/pages/SeparatorPage.cpp
    src/pages/ShellPage.cpp
    src/pages/StatCardPage.cpp
    src/pages/StatusDotPage.cpp
    src/pages/TabStripPage.cpp
    src/pages/TogglePage.cpp
    src/pages/ItemListPage.cpp
    src/pages/TableViewPage.cpp
    src/pages/ThemeRolesPage.cpp
    src/pages/ThemeStylingPage.cpp
    src/pages/TilePage.cpp
    src/pages/TypographyPage.cpp
    src/pages/WindowShellPage.cpp
)

target_include_directories(NovuUIPlaygroundLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(NovuUIPlaygroundLib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# The playground consumes NovuUI exactly as an application does: the public
# target, no source-level shortcut. What renders here is what ships.
target_link_libraries(NovuUIPlaygroundLib PUBLIC Novu::UI Qt6::Widgets)

# Development paths, baked in at configure time so a freshly built playground
# runs with no environment set up. Every one of them can be overridden by an
# environment variable at runtime — see Paths.cpp.
target_compile_definitions(NovuUIPlaygroundLib PUBLIC
    NOVUUI_PLAYGROUND_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    NOVUUI_PLAYGROUND_REPO_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
    NOVUUI_PLAYGROUND_BUILD_DIR="${CMAKE_BINARY_DIR}"
    NOVUUI_PLAYGROUND_CMAKE="${CMAKE_COMMAND}"
    NOVUUI_PLAYGROUND_CONFIG="$<CONFIG>"
)

add_executable(NovuUIPlayground WIN32 src/main.cpp)
target_link_libraries(NovuUIPlayground PRIVATE NovuUIPlaygroundLib)

# A console subsystem build is often what a developer actually wants while
# chasing a warning; WIN32 hides the console. -DNOVUUI_PLAYGROUND_CONSOLE=ON
# gives it back without touching this file.
option(NOVUUI_PLAYGROUND_CONSOLE "Build the gallery as a console application" OFF)
if(NOVUUI_PLAYGROUND_CONSOLE AND WIN32)
    set_target_properties(NovuUIPlayground PROPERTIES WIN32_EXECUTABLE OFF)
endif()

# Sample data is never installed and never copied next to a binary. It is read
# from the source tree, which is also what makes editing a JSON file show up
# in a running gallery without a rebuild.
