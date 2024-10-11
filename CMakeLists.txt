cmake_minimum_required(VERSION 3.20)

project(cmm)

set(CMAKE_CXX_STANDARD 23)

add_definitions(-DDEBUG)

add_executable(cmm src/main.cpp src/parser.cpp)