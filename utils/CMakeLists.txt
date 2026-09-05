
# utils — shared interface for Result<T> and small helpers
# cmake_minimum_required rationale: INTERFACE library requires 3.0+
cmake_minimum_required(VERSION 3.14)
project(utils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(utils INTERFACE)
target_include_directories(utils INTERFACE include/)
target_compile_features(utils INTERFACE cxx_std_11)
set_target_properties(utils PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
