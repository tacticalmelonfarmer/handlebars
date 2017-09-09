cmake_minimum_required(VERSION 3.7)
project(events CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

add_subdirectory(variant)
include_directories(variant/include)

add_library(dispatcher dispatcher.hpp)
set_target_properties(dispatcher PROPERTIES LINKER_LANGUAGE CXX)

add_executable(test1 ${PROJECT_SOURCE_DIR}/test1.cpp)
target_link_libraries(test1 dispatcher)
add_dependencies(test1 dispatcher)

add_executable(test2 ${PROJECT_SOURCE_DIR}/test2.cpp)
target_link_libraries(test2 dispatcher)
add_dependencies(test2 dispatcher)

add_executable(test3 ${PROJECT_SOURCE_DIR}/test3.cpp)
target_link_libraries(test3 dispatcher)
add_dependencies(test3 dispatcher)

add_executable(test4 ${PROJECT_SOURCE_DIR}/test4.cpp)
target_link_libraries(test4 dispatcher)
add_dependencies(test4 dispatcher)