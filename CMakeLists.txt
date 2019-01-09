cmake_minimum_required(VERSION 3.10)

project(handlebars LANGUAGES CXX)

add_library(handlebars INTERFACE)
target_compile_features(handlebars INTERFACE cxx_std_17)
target_include_directories(handlebars INTERFACE include)

add_executable(logger example/logger/main.cpp)
target_link_libraries(logger handlebars)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_link_libraries(logger stdc++fs)
endif()

add_executable(arithmetic example/arithmetic/main.cpp)
target_link_libraries(arithmetic handlebars)

add_executable(repl example/repl/main.cpp)
target_link_libraries(repl handlebars)