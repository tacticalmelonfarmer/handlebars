cmake_minimum_required(VERSION 3.7)

project(handlebars LANGUAGES CXX)

add_executable(demo1 example/demo1/demo1.cpp)
target_compile_features(demo1 PUBLIC cxx_std_17)
target_include_directories(demo1 PUBLIC include)

add_executable(demo2 example/demo2/demo2.cpp)
target_compile_features(demo2 PUBLIC cxx_std_17)
target_include_directories(demo2 PUBLIC include)

add_executable(demo3 example/demo3/demo3.cpp)
target_compile_features(demo3 PUBLIC cxx_std_17)
target_include_directories(demo3 PUBLIC include)

set_target_properties(demo1 demo2 demo3
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)