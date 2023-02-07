cmake_minimum_required(VERSION 3.10)

project(Traceroute VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(traceroute src/main.cpp )

target_include_directories(traceroute PUBLIC "${CMAKE_SOURCE_DIR}/include")


find_package(Threads REQUIRED)
target_link_libraries(traceroute ${CMAKE_THREAD_LIBS_INIT})
add_compile_options(-g)