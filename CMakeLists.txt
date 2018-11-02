cmake_minimum_required(VERSION 3.7)

project(handlebars LANGUAGES CXX)

add_executable(demo1 example/demo1/demo1.cpp)
target_compile_features(demo1 PUBLIC cxx_std_17)
target_include_directories(demo1 PUBLIC include)

set_target_properties(demo1
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)