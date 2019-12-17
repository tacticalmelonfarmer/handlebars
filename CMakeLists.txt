cmake_minimum_required(VERSION 3.10)

project(handlebars)

add_subdirectory(ext/callable)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
add_library(libhandlebars INTERFACE)
target_compile_features(libhandlebars INTERFACE cxx_std_17)
target_include_directories(libhandlebars INTERFACE include)
target_link_libraries(libhandlebars INTERFACE libcallable Threads::Threads)

add_executable(arithmetic examples/easy/arithmetic/main.cpp)
target_link_libraries(arithmetic libhandlebars)
### fast version
add_executable(fast_arithmetic examples/fast/arithmetic/main.cpp)
target_link_libraries(fast_arithmetic libhandlebars)

add_executable(edgewise examples/easy/edgewise/main.cpp)
target_link_libraries(edgewise libhandlebars)

add_executable(logger examples/easy/logger/main.cpp)
target_link_libraries(logger libhandlebars)

add_executable(lvalues examples/easy/lvalues/main.cpp)
target_link_libraries(lvalues libhandlebars)

add_executable(ordered examples/easy/ordered/main.cpp)
target_link_libraries(ordered libhandlebars)

add_executable(repl examples/easy/repl/main.cpp)
target_link_libraries(repl libhandlebars)

add_executable(rvalues examples/easy/rvalues/main.cpp)
target_link_libraries(rvalues libhandlebars)