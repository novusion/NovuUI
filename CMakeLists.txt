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

# ── STATIC by default, SHARED when a host asks for it ──────────────────────
#
# Every application that consumes this library links it statically, and that is
# fine for an application: one binary, one copy of the state below.
#
# It stops being fine the moment a process loads a plugin that also draws with
# NovuUI. ThemeManager::instance() is a function-local static, so a statically
# linked copy inside a plugin is a SECOND ThemeManager: the host registers its
# profiles - the accent, the spacing roles, the semantic colours - into its own,
# the plugin's widgets resolve themeFor() against the plugin's, and the plugin
# draws with this library's built-in defaults instead of the application's
# configured theme. ThemeScope's resolution is the same story.
#
# So a host that loads UI plugins asks for SHARED and gets one runtime for the
# whole process. The default stays STATIC, deliberately: NovuHub, NovuExplorer,
# NovuSource and NovuAdmin consume this library today and must keep building and
# deploying exactly as they do, without a DLL beside them.
option(NOVUUI_BUILD_SHARED
    "Build NovuUI as a shared library, so a process that loads UI plugins has one NovuUI runtime"
    OFF)

if(NOVUUI_BUILD_SHARED)
    set(_novuui_link_kind SHARED)
else()
    set(_novuui_link_kind STATIC)
endif()

add_library(NovuUI ${_novuui_link_kind}
    include/novu/ui/Theme.h
    include/novu/ui/ThemeProfile.h
    include/novu/ui/ThemeManager.h
    include/novu/ui/ThemeScope.h
    include/novu/ui/Card.h
    include/novu/ui/ChatCard.h
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
    include/novu/ui/HelpCenter.h
    include/novu/ui/HelpOverlay.h
    include/novu/ui/AddressBar.h
    include/novu/ui/AppMark.h
    include/novu/ui/DiffView.h
    include/novu/ui/Disclosure.h
    include/novu/ui/Drawer.h
    include/novu/ui/FilePicker.h
    include/novu/ui/FilePickerDialog.h
    include/novu/ui/ItemList.h
    include/novu/ui/ItemStrip.h
    include/novu/ui/Keys.h
    include/novu/ui/LogView.h
    include/novu/ui/MonoLineView.h
    include/novu/ui/MarkdownDocumentView.h
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
    src/NovuAiProfile.cpp
    src/Novusion.cpp
    src/ThemeManager.cpp
    src/ThemeScope.cpp
    src/Card.cpp
    src/ChatCard.cpp
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
    src/HelpCenter.cpp
    src/ReleaseNotes.cpp
    src/HelpOverlay.cpp
    src/AddressBar.cpp
    src/AppMark.cpp
    src/DiffView.cpp
    src/Disclosure.cpp
    src/Drawer.cpp
    src/FilePicker.cpp
    src/FilePickerDialog.cpp
    src/ItemList.cpp
    src/ItemStrip.cpp
    src/Keys.cpp
    src/LogView.cpp
    src/MonoLineView.cpp
    src/MarkdownDocumentView.cpp
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

if(NOVUUI_BUILD_SHARED AND WIN32)
    # Every symbol, rather than ninety headers annotated with an export macro.
    #
    # This is the mechanism CMake provides for exactly this conversion, and it
    # is sufficient here because nothing in the public API is an exported DATA
    # symbol: the only static members in the headers are `static constexpr int`,
    # which are compile-time, and the process-wide state is in function-local
    # statics inside the .cpp files - which is what has to live in the DLL and
    # now does. Annotating the headers instead would churn every public class in
    # a library four other applications are building against.
    set_target_properties(NovuUI PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        # A consumer links the import library and loads the DLL beside its own
        # executable; the host's deploy step is what puts it there.
        RUNTIME_OUTPUT_NAME NovuUI)
    # WINDOWS_EXPORT_ALL_SYMBOLS carries functions and not data, and every
    # Q_OBJECT class has a data symbol: moc's `staticMetaObject`. So the classes
    # are annotated with NOVUUI_API as well - see include/novu/ui/Export.h - and
    # these two definitions are what switch that macro between exporting here and
    # importing in a consumer.
    target_compile_definitions(NovuUI
        PUBLIC NOVUUI_SHARED
        PRIVATE NOVUUI_BUILDING)
    # LNK4197, silenced for a structural reason and nothing else.
    #
    # The two export mechanisms above overlap on purpose: WINDOWS_EXPORT_ALL_SYMBOLS
    # writes every symbol into a generated .def file, and the annotated classes
    # are ALSO marked dllexport so their moc data symbol comes out. A class
    # member therefore appears twice - once from the .def, once from the
    # declspec - and the linker says so, 255 times.
    #
    # It is the same symbol at the same address either way; the warning means
    # "exported more than once", not "exported wrongly". The alternative is
    # annotating a hundred free functions by hand to drop the .def, which is a
    # far larger and more fragile change to a library four other applications
    # build against. Only this one warning number is suppressed.
    target_link_options(NovuUI PRIVATE /IGNORE:4197)
endif()

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
