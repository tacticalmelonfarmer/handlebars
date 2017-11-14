cmake_minimum_required(VERSION 3.7)

project(events LANGUAGES CXX)

find_package(Threads)

set(BUILD_TREE ${CMAKE_BINARY_DIR}/build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BUILD_TREE}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BUILD_TREE}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILD_TREE}/bin)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(dispatch_use example/dispatch_use.cpp)
target_link_libraries(dispatch_use ${CMAKE_THREAD_LIBS_INIT})
target_compile_features(dispatch_use PUBLIC cxx_std_17)

add_executable(handler_use example/handler_use.cpp)
target_compile_features(handler_use PUBLIC cxx_std_17)