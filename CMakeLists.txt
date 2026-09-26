cmake_minimum_required(VERSION 3.21)

project(NovuUI VERSION 0.1.0 LANGUAGES CXX)

# UTF-8 sources; see the note in the consuming project. Without this a literal
# in a QStringLiteral is converted from the wrong code page at compile time.
if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_library(NovuUI STATIC
    include/novu/ui/Theme.h
    include/novu/ui/ThemeProfile.h
    include/novu/ui/ThemeManager.h
    include/novu/ui/ThemeScope.h
    include/novu/ui/Card.h
    include/novu/ui/CommandPalette.h
    include/novu/ui/Layout.h
    include/novu/ui/Avatar.h
    include/novu/ui/Badge.h
    include/novu/ui/Icon.h
    include/novu/ui/Motion.h
    include/novu/ui/IconButton.h
    include/novu/ui/Segmented.h
    include/novu/ui/SelectionBar.h
    include/novu/ui/WidgetEditor.h
    src/WidgetEditor.cpp
    include/novu/ui/Separator.h
    include/novu/ui/Settings.h
    include/novu/ui/StatusDot.h
    include/novu/ui/StyleSpec.h
    include/novu/ui/Toggle.h
    include/novu/ui/BottomPanel.h
    include/novu/ui/Button.h
    include/novu/ui/TableView.h
    include/novu/ui/Style.h
    include/novu/ui/StyleSheet.h
    include/novu/ui/Text.h
    include/novu/ui/AutoGridLayout.h
    include/novu/ui/FlowLayout.h
    include/novu/ui/DropTarget.h
    include/novu/ui/Swatch.h
    include/novu/ui/TabBar.h
    include/novu/ui/TabStrip.h
    include/novu/ui/Chip.h
    include/novu/ui/Field.h
    include/novu/ui/Fields.h
    include/novu/ui/FloatingPanel.h
    include/novu/ui/Form.h
    include/novu/ui/StatCard.h
    include/novu/ui/Feedback.h
    include/novu/ui/FeedbackCenter.h
    include/novu/ui/PopupSurface.h
    include/novu/ui/FeedbackForm.h
    include/novu/ui/GateCard.h
    include/novu/ui/HelpOverlay.h
    include/novu/ui/AddressBar.h
    include/novu/ui/AppMark.h
    include/novu/ui/DiffView.h
    include/novu/ui/Disclosure.h
    include/novu/ui/Drawer.h
    include/novu/ui/FilePicker.h
    include/novu/ui/ItemList.h
    include/novu/ui/ItemStrip.h
    include/novu/ui/Keys.h
    include/novu/ui/LogView.h
    include/novu/ui/MonoLineView.h
    include/novu/ui/ModuleBar.h
    include/novu/ui/NavList.h
    include/novu/ui/PageHeader.h
    include/novu/ui/Popover.h
    include/novu/ui/SettingRow.h
    include/novu/ui/Sidebar.h
    include/novu/ui/Slider.h
    include/novu/ui/Splitter.h
    include/novu/ui/SplashScreen.h
    include/novu/ui/StatusBar.h
    include/novu/ui/Tooltip.h
    include/novu/ui/Tile.h
    include/novu/ui/Toast.h
    include/novu/ui/TreeView.h
    include/novu/ui/VirtualListView.h
    include/novu/ui/TitleBar.h
    include/novu/ui/WindowShell.h
    src/Theme.cpp
    src/ThemeProfile.cpp
    src/AppProfiles.cpp
    src/ThemeManager.cpp
    src/ThemeScope.cpp
    src/Card.cpp
    src/CommandPalette.cpp
    src/Layout.cpp
    src/Avatar.cpp
    src/Badge.cpp
    src/GlyphTable.inc
    src/Icon.cpp
    src/Motion.cpp
    src/IconButton.cpp
    src/Segmented.cpp
    src/SelectionBar.cpp
    src/Separator.cpp
    src/Settings.cpp
    src/StatusDot.cpp
    src/StyleSpec.cpp
    src/Toggle.cpp
    src/BottomPanel.cpp
    src/Button.cpp
    src/TableView.cpp
    src/Style.cpp
    src/StyleSheet.cpp
    src/Text.cpp
    src/AutoGridLayout.cpp
    src/FlowLayout.cpp
    src/DropTarget.cpp
    src/Swatch.cpp
    src/TabBar.cpp
    src/TabStrip.cpp
    src/Chip.cpp
    src/Field.cpp
    src/Fields.cpp
    src/FloatingPanel.cpp
    src/Form.cpp
    src/StatCard.cpp
    src/Feedback.cpp
    src/FeedbackCenter.cpp
    src/PopupSurface.cpp
    src/FeedbackForm.cpp
    src/GateCard.cpp
    src/HelpOverlay.cpp
    src/AddressBar.cpp
    src/AppMark.cpp
    src/DiffView.cpp
    src/Disclosure.cpp
    src/Drawer.cpp
    src/FilePicker.cpp
    src/ItemList.cpp
    src/ItemStrip.cpp
    src/Keys.cpp
    src/LogView.cpp
    src/MonoLineView.cpp
    src/ModuleBar.cpp
    src/NavList.cpp
    src/PageHeader.cpp
    src/Popover.cpp
    src/SettingRow.cpp
    src/Sidebar.cpp
    src/Slider.cpp
    src/Splitter.cpp
    src/SplashScreen.cpp
    src/StatusBar.cpp
    src/Tooltip.cpp
    src/Tile.cpp
    src/Toast.cpp
    src/TreeView.cpp
    src/VirtualListView.cpp
    src/TitleBar.cpp
    src/WindowShell.cpp
)

add_library(Novu::UI ALIAS NovuUI)

target_include_directories(NovuUI PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(NovuUI PUBLIC Qt6::Widgets)

# The window shell keeps the native frame alive under its own chrome: DWM owns
# the drop shadow, the corner radius and the border colour, so the frame stays
# a real Windows window while nothing native is drawn. PRIVATE - nothing in the
# public headers exposes a Windows type.
if(WIN32)
    target_link_libraries(NovuUI PRIVATE dwmapi)
endif()

# ── Development-only targets ────────────────────────────────────────────────
#
# The component gallery and the tests are development tools. They must never
# reach an application build: NovuAdmin and NovuSpace consume this repository
# with add_subdirectory(), so the default is derived from whether NovuUI is
# the project actually being built rather than from a flag a consumer has to
# remember to switch off.
#
# Nothing below is reachable from the NovuUI target. `Novu::UI` links no
# playground source, carries no sample data, and gains no include directory,
# so a production link is byte-for-byte what it was before this existed.
option(NOVUUI_BUILD_PLAYGROUND
    "Build the NovuUI component gallery (development only)" ${PROJECT_IS_TOP_LEVEL})
option(NOVUUI_BUILD_TESTS
    "Build the NovuUI development tests (development only)" ${PROJECT_IS_TOP_LEVEL})

if(NOVUUI_BUILD_PLAYGROUND)
    add_subdirectory(playground)
endif()

if(NOVUUI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
