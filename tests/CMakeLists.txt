# Development tests. Reached only through NOVUUI_BUILD_TESTS, which defaults
# to PROJECT_IS_TOP_LEVEL — an application consuming NovuUI never configures
# them.

find_package(Qt6 REQUIRED COMPONENTS Test Widgets)

# The window chrome test needs NovuUI and nothing else. Declared before the
# playground guard below so it still runs in a configuration that builds the
# tests without the gallery - the chrome is production code, and a test of it
# must not depend on a development tool.
add_executable(tst_window_chrome tst_window_chrome.cpp)
target_link_libraries(tst_window_chrome PRIVATE Novu::UI Qt6::Test Qt6::Widgets)

add_test(NAME window_chrome COMMAND tst_window_chrome)
set_tests_properties(window_chrome PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 120
)

if(NOT TARGET NovuUIPlaygroundLib)
    message(STATUS "NovuUI tests need the playground library; "
                   "enable NOVUUI_BUILD_PLAYGROUND. Skipping.")
    return()
endif()

add_executable(tst_gallery tst_gallery.cpp)
target_link_libraries(tst_gallery PRIVATE NovuUIPlaygroundLib Qt6::Test Qt6::Widgets)

# The tests instantiate every registered component, so they need a platform
# plugin. Offscreen means they run the same in CI as on a desk.
add_test(NAME gallery COMMAND tst_gallery)
set_tests_properties(gallery PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 300
)
