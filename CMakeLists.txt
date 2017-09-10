cmake_minimum_required(VERSION 3.7)
project(events CXX)

#set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} --std=c++1z)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Release)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

add_executable(test1 ${PROJECT_SOURCE_DIR}/test1.cpp)

add_executable(test2 ${PROJECT_SOURCE_DIR}/test2.cpp)

add_executable(test3 ${PROJECT_SOURCE_DIR}/test3.cpp)

add_executable(test4 ${PROJECT_SOURCE_DIR}/test4.cpp)

add_executable(test5 ${PROJECT_SOURCE_DIR}/test5.cpp)