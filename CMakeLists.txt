cmake_minimum_required(VERSION 3.22)
project(stopwatch)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-lncurses")

add_executable(stopwatch main.cpp)
