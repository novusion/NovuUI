cmake_minimum_required(VERSION 3.21)

project(NovuUI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_library(NovuUI STATIC
    include/novu/ui/Theme.h
    include/novu/ui/Badge.h
    include/novu/ui/Button.h
    include/novu/ui/TableView.h
    src/Theme.cpp
    src/Badge.cpp
    src/Button.cpp
    src/TableView.cpp
)

add_library(Novu::UI ALIAS NovuUI)

target_include_directories(NovuUI PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(NovuUI PUBLIC Qt6::Widgets)
