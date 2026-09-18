cmake_minimum_required(VERSION 3.21)
project(calcli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(calcli src/main.cpp)