cmake_minimum_required(VERSION 3.7)

project(events)

set(BUILD_TREE ${CMAKE_BINARY_DIR}/build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BUILD_TREE}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BUILD_TREE}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILD_TREE}/bin)

add_library(variant STATIC include/variant.hpp include/typelist.hpp)
set_target_properties(variant PROPERTIES LINKER_LANGUAGE CXX)

add_executable(test1 tests/test1.cpp)
target_include_directories(test1 PRIVATE include)
target_link_libraries(test1 PRIVATE variant)

add_executable(test2 tests/test2.cpp)
target_include_directories(test2 PRIVATE include)
target_link_libraries(test2 PRIVATE variant)

if(MSVC)
    add_compile_options("/W4")
    target_compile_options(test1 PRIVATE "/std:c++17")
    target_compile_options(test2 PRIVATE "/std:c++17")
elseif(CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options("-Wall -Wextra -Wpedantic")
    target_compile_options(test1 PRIVATE "-std=c++1z")
    target_compile_options(test2 PRIVATE "-std=c++1z")
endif()