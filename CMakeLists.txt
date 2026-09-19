cmake_minimum_required(VERSION 3.21)
project(calcli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(CLI11 CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(calcli_core src/parser.cpp src/event.cpp)
target_include_directories(calcli_core PUBLIC include)
target_compile_features(calcli_core PUBLIC cxx_std_20)

add_executable(calcli src/main.cpp)
target_link_libraries(calcli PRIVATE CLI11::CLI11 calcli_core)
target_link_libraries(calcli_core PUBLIC CURL::libcurl nlohmann_json::nlohmann_json)

enable_testing()
add_subdirectory(tests)