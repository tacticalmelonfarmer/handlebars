cmake_minimum_required(VERSION 3.10)

project(handlebars LANGUAGES CXX)

add_executable(function tests/function/main.cpp)
target_compile_features(function PUBLIC cxx_std_17)
target_include_directories(function PUBLIC include)

add_executable(global_api example/global_api/main.cpp)
target_compile_features(global_api PUBLIC cxx_std_17)
target_include_directories(global_api PUBLIC include)

add_executable(minimal_api example/minimal_api/main.cpp)
target_compile_features(minimal_api PUBLIC cxx_std_17)
target_include_directories(minimal_api PUBLIC include)

set_target_properties(function global_api minimal_api
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)