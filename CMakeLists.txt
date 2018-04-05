cmake_minimum_required(VERSION 3.7)

project(handlebars LANGUAGES CXX)

find_package(Threads)

set(HEADERS ${CMAKE_SOURCE_DIR}/include)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(demo1 example/demo1/demo1.cpp)
target_compile_features(demo1 PUBLIC cxx_std_17)
target_include_directories(demo1 PUBLIC ${HEADERS})