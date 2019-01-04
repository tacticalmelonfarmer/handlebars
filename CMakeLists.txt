cmake_minimum_required(VERSION 3.10)

project(handlebars LANGUAGES CXX)

find_package(Threads)

add_executable(logger example/logger/main.cpp)
target_compile_features(logger PUBLIC cxx_std_17)
target_include_directories(logger PUBLIC include)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_link_libraries(logger stdc++fs)
endif()

add_executable(arithmetic example/arithmetic/main.cpp)
target_compile_features(arithmetic PUBLIC cxx_std_17)
target_include_directories(arithmetic PUBLIC include)

add_executable(repl_multithread example/repl_multithread/main.cpp)
target_compile_features(repl_multithread PUBLIC cxx_std_17)
target_include_directories(repl_multithread PUBLIC include)
target_link_libraries(repl_multithread Threads::Threads)