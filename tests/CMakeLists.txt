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

# The rail's connection strip is shared by every module, so a paint bug in it is
# four bugs. This pins the one that shipped: a detail longer than the strip was
# drawn starting at a negative x, leaving a cut-off word fragment to the left of
# the status dot and erasing the hostname.
add_executable(tst_connection_strip tst_connection_strip.cpp)
target_link_libraries(tst_connection_strip PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME connection_strip COMMAND tst_connection_strip)
set_tests_properties(connection_strip PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen TIMEOUT 60)

# A transient surface the user cannot dismiss is a broken surface. This pins
# the popup window type and every documented way out of a menu.
add_executable(tst_popover_dismissal tst_popover_dismissal.cpp)
target_link_libraries(tst_popover_dismissal PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME popover_dismissal COMMAND tst_popover_dismissal)
set_tests_properties(popover_dismissal PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

# Feedback presentation, against fixtures: the states the UI must be able to
# draw, including the ones where being wrong misleads the reporter.
add_executable(tst_feedback_center tst_feedback_center.cpp)
target_link_libraries(tst_feedback_center PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME feedback_center COMMAND tst_feedback_center)
set_tests_properties(feedback_center PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

# The progressive tip: two levels, and every way out of it.
add_executable(tst_tooltip_help tst_tooltip_help.cpp)
target_link_libraries(tst_tooltip_help PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME tooltip_help COMMAND tst_tooltip_help)
set_tests_properties(tooltip_help PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

add_executable(tst_card_interaction tst_card_interaction.cpp)
target_link_libraries(tst_card_interaction PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME card_interaction COMMAND tst_card_interaction)
set_tests_properties(card_interaction PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

add_executable(tst_widget_editor tst_widget_editor.cpp)
target_link_libraries(tst_widget_editor PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME widget_editor COMMAND tst_widget_editor)
set_tests_properties(widget_editor PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

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
