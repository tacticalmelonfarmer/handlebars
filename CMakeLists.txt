cmake_minimum_required(VERSION 3.7)

project(events LANGUAGES CXX)

set(BUILD_TREE ${CMAKE_BINARY_DIR}/build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BUILD_TREE}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BUILD_TREE}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILD_TREE}/bin)

add_library(dispatcher STATIC include/dispatcher.hpp)
set_target_properties(dispatcher PROPERTIES LINKER_LANGUAGE CXX)