cmake_minimum_required(VERSION 3.21)
project(calcli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CLI11 CONFIG REQUIRED)

add_executable(calcli src/main.cpp)
target_link_libraries(calcli PRIVATE CLI11::CLI11)