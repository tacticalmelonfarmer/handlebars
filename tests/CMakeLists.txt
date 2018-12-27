cmake_minimum_required(VERSION 3.10)

project(handlebars-tests LANGUAGES CXX)

add_executable(function-tests function/main.cpp)
target_compile_features(function-tests PUBLIC cxx_std_17)
target_include_directories(function-tests PUBLIC ../include)

set_target_properties(function-tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
