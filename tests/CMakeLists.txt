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

# An elided rail line must stay readable: the full host and detail in the
# tooltip once either is cut. A truncated transport error was misread as a
# module incompatibility because nothing else carried the rest of it.
add_executable(tst_connection_strip_tooltip tst_connection_strip_tooltip.cpp)
target_link_libraries(tst_connection_strip_tooltip PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME connection_strip_tooltip COMMAND tst_connection_strip_tooltip)
set_tests_properties(connection_strip_tooltip PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen TIMEOUT 60)

# Renaming a rail row in place: Enter submits, Escape abandons, focus leaving
# submits, a refusal keeps the editor open on Enter and abandons on focus-out,
# and the editor survives the rail being rebuilt from data.
add_executable(tst_navlist_inline_edit tst_navlist_inline_edit.cpp)
target_link_libraries(tst_navlist_inline_edit PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME navlist_inline_edit COMMAND tst_navlist_inline_edit)
set_tests_properties(navlist_inline_edit PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen TIMEOUT 60)

# A browser's tab strip: renaming in place by NavList's rules, a per-tab menu
# request, scrolling past the width floor with hit-testing and reordering that
# follow the scroll, and the per-tab speaker mark.
add_executable(tst_tabbar_browser_workflow tst_tabbar_browser_workflow.cpp)
target_link_libraries(tst_tabbar_browser_workflow PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME tabbar_browser_workflow COMMAND tst_tabbar_browser_workflow)
set_tests_properties(tabbar_browser_workflow PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen TIMEOUT 60)

# Rail rows as drop targets: only declared rows accept, only the declared
# payload type, a drop reports row and payload, and a rebuild keeps them.
add_executable(tst_navlist_drop tst_navlist_drop.cpp)
target_link_libraries(tst_navlist_drop PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME navlist_drop COMMAND tst_navlist_drop)
set_tests_properties(navlist_drop PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen TIMEOUT 60)

# Prominent rail captions, and the fold and indent they have to live with.
add_executable(tst_navlist_sections tst_navlist_sections.cpp)
target_link_libraries(tst_navlist_sections PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME navlist_sections COMMAND tst_navlist_sections)
set_tests_properties(navlist_sections PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen TIMEOUT 60)

# Tree multi-selection and drag & drop: Ctrl+A through a filter, Escape, clear()
# and the context-menu selection rule; a drag carries the selection in display
# order, a drop is refused onto the dragged rows and their subtrees, reported
# and never performed by the tree, and a hovered collapsed parent opens.
add_executable(tst_treeview_selection_drop tst_treeview_selection_drop.cpp)
target_link_libraries(tst_treeview_selection_drop PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME treeview_selection_drop COMMAND tst_treeview_selection_drop)
set_tests_properties(treeview_selection_drop PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen TIMEOUT 60)

# The running status turns, and its one timer stops once nothing spins.
add_executable(tst_treeview_status_spinner tst_treeview_status_spinner.cpp)
target_link_libraries(tst_treeview_status_spinner PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME treeview_status_spinner COMMAND tst_treeview_status_spinner)
set_tests_properties(treeview_status_spinner PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen TIMEOUT 60)

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

# The global switch: off silences every hover path - the branded card, plain
# QWidget::setToolTip, Qt::ToolTipRole and a component's own handler - and on
# brings each back.
add_executable(tst_tooltip_gate tst_tooltip_gate.cpp)
target_link_libraries(tst_tooltip_gate PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME tooltip_gate COMMAND tst_tooltip_gate)
set_tests_properties(tooltip_gate PROPERTIES TIMEOUT 60)

add_executable(tst_card_interaction tst_card_interaction.cpp)
target_link_libraries(tst_card_interaction PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME card_interaction COMMAND tst_card_interaction)
set_tests_properties(card_interaction PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

add_executable(tst_widget_editor tst_widget_editor.cpp)
target_link_libraries(tst_widget_editor PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME widget_editor COMMAND tst_widget_editor)
set_tests_properties(widget_editor PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

# A row is as tall as its control, not as tall as its control's size hint.
# QPlainTextEdit over-asks by design, so a Description field of 64 pixels was
# sitting at the top of a row twice that height and the surplus read as a gap.
# A switch row is as tall as the hint under it. Three of them with wrapping
# hints drew over each other in NovuAI's Share form, and a label painted over
# a switch eats the press - so the switch could not be turned on, which reads
# as "the feature is not wired up" rather than as a layout fault.
add_executable(tst_toggle_row_height tst_toggle_row_height.cpp)
target_link_libraries(tst_toggle_row_height PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME toggle_row_height COMMAND tst_toggle_row_height)
set_tests_properties(toggle_row_height PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

add_executable(tst_toggle_click_state tst_toggle_click_state.cpp)
target_link_libraries(tst_toggle_click_state PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME toggle_click_state COMMAND tst_toggle_click_state)
set_tests_properties(toggle_click_state PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

add_executable(tst_small_button_pill tst_small_button_pill.cpp)
target_link_libraries(tst_small_button_pill PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME small_button_pill COMMAND tst_small_button_pill)
set_tests_properties(small_button_pill PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

add_executable(tst_setting_row_height tst_setting_row_height.cpp)
target_link_libraries(tst_setting_row_height PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME setting_row_height COMMAND tst_setting_row_height)
set_tests_properties(setting_row_height PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

# The strip's leading and trailing readings grow towards each other and are
# painted, so when they met they were drawn on top of one another.
add_executable(tst_status_bar_crowding tst_status_bar_crowding.cpp)
target_link_libraries(tst_status_bar_crowding PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME status_bar_crowding COMMAND tst_status_bar_crowding)
set_tests_properties(status_bar_crowding PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

# Fill divides the surplus, never the content: four segments in a narrow rail
# were each given less than their label needed and the labels overlapped.
add_executable(tst_segmented_fill tst_segmented_fill.cpp)
target_link_libraries(tst_segmented_fill PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME segmented_fill COMMAND tst_segmented_fill)
set_tests_properties(segmented_fill PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

# A select is as wide as its widest option; a menu that stays open marks what
# is chosen. Both were composer bugs: "Sh..." and invisible multi-select.
add_executable(tst_select_and_set_menu tst_select_and_set_menu.cpp)
target_link_libraries(tst_select_and_set_menu PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME select_and_set_menu COMMAND tst_select_and_set_menu)
set_tests_properties(select_and_set_menu PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

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

# The shared file picker's favourites and remembered folder: the store, the
# gestures that reach it, and the window that puts the two together. Redirects
# the user-scope INI to a temporary directory, so it never touches the real one.
add_executable(tst_file_picker_places tst_file_picker_places.cpp)
target_link_libraries(tst_file_picker_places PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME file_picker_places COMMAND tst_file_picker_places)
set_tests_properties(file_picker_places PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)

# A submenu opens beside its menu, and a message dialog is as large as its
# message: two surfaces that used to hide their own content.
add_executable(tst_menu_dialog_fit tst_menu_dialog_fit.cpp)
target_link_libraries(tst_menu_dialog_fit PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME menu_dialog_fit COMMAND tst_menu_dialog_fit)
set_tests_properties(menu_dialog_fit PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

# A control as a menu row (the colour row under "Marker"): placed, sized at
# its width, clickable, cleared with the rows.
add_executable(tst_popover_control tst_popover_control.cpp)
target_link_libraries(tst_popover_control PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME popover_control COMMAND tst_popover_control)
set_tests_properties(popover_control PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen TIMEOUT 60)

# Colour markers before the icon, and a row filter that hides and never removes.
add_executable(tst_treeview_marker_filter tst_treeview_marker_filter.cpp)
target_link_libraries(tst_treeview_marker_filter PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME treeview_marker_filter COMMAND tst_treeview_marker_filter)
set_tests_properties(treeview_marker_filter PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen TIMEOUT 60)

# The same toast is not shown twice.
add_executable(tst_toast_dedupe tst_toast_dedupe.cpp)
target_link_libraries(tst_toast_dedupe PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME toast_dedupe COMMAND tst_toast_dedupe)
set_tests_properties(toast_dedupe PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen TIMEOUT 60)

# A plain setToolTip() and an item's ToolTipRole are drawn as the card.
add_executable(tst_tooltip_plain tst_tooltip_plain.cpp)
target_link_libraries(tst_tooltip_plain PRIVATE Novu::UI Qt6::Test Qt6::Widgets)
add_test(NAME tooltip_plain COMMAND tst_tooltip_plain)
set_tests_properties(tooltip_plain PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen TIMEOUT 60)
