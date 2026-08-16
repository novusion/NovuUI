# Development tests. Reached only through NOVUUI_BUILD_TESTS, which defaults
# to PROJECT_IS_TOP_LEVEL — an application consuming NovuUI never configures
# them.

if(NOT TARGET NovuUIPlaygroundLib)
    message(STATUS "NovuUI tests need the playground library; "
                   "enable NOVUUI_BUILD_PLAYGROUND. Skipping.")
    return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Test Widgets)

add_executable(tst_gallery tst_gallery.cpp)
target_link_libraries(tst_gallery PRIVATE NovuUIPlaygroundLib Qt6::Test Qt6::Widgets)

# The tests instantiate every registered component, so they need a platform
# plugin. Offscreen means they run the same in CI as on a desk.
add_test(NAME gallery COMMAND tst_gallery)
set_tests_properties(gallery PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 300
)
