cmake_minimum_required(VERSION 3.21)
project(Project1 VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    add_compile_options(/utf-8)
endif()

add_library(calculator_core STATIC src/polynomial.cpp src/expression.cpp)
target_include_directories(calculator_core PUBLIC include)
if(MSVC)
    target_compile_options(calculator_core PRIVATE /W4)
else()
    target_compile_options(calculator_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(calculator_cli src/main.cpp)
set_target_properties(calculator_cli PROPERTIES OUTPUT_NAME calculator)
target_link_libraries(calculator_cli PRIVATE calculator_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(calculator_tests tests/test_calculator.cpp)
    target_link_libraries(calculator_tests PRIVATE calculator_core)
    add_test(NAME calculator_core COMMAND calculator_tests)
endif()

option(PROJECT1_BUILD_GUI "Build the Qt Widgets desktop application" ON)
if(PROJECT1_BUILD_GUI)
    find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    add_library(calculator_widgets STATIC gui/calculator_window.cpp gui/style.cpp gui/resources.qrc)
    target_include_directories(calculator_widgets PUBLIC gui)
    target_link_libraries(calculator_widgets PUBLIC calculator_core Qt6::Widgets)
    if(MSVC)
        target_compile_options(calculator_widgets PRIVATE /W4)
    else()
        target_compile_options(calculator_widgets PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    qt_add_executable(project1_gui MACOSX_BUNDLE WIN32 gui/main.cpp)
    target_link_libraries(project1_gui PRIVATE calculator_widgets)
    set_target_properties(project1_gui PROPERTIES
        OUTPUT_NAME Project1
        MACOSX_BUNDLE_BUNDLE_NAME "Project1 Calculator"
        MACOSX_BUNDLE_GUI_IDENTIFIER "cn.project1.calculator"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0")

    if(BUILD_TESTING)
        find_package(Qt6 REQUIRED COMPONENTS Test)
        add_executable(calculator_gui_tests tests/test_gui.cpp)
        target_link_libraries(calculator_gui_tests PRIVATE calculator_widgets Qt6::Test)
        add_test(NAME calculator_gui COMMAND calculator_gui_tests)
        set_tests_properties(calculator_gui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()
endif()
