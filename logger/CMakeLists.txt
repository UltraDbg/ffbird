# logger — cmake_minimum_required rationale: add_library(STUB), target_include_directories, find_package(Threads/GTest).
cmake_minimum_required(VERSION 3.14)
project(logger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(logger STATIC include/logger/log.h include/logger/result.h src/log.cpp)
target_include_directories(logger PUBLIC include/)
target_link_libraries(logger PUBLIC Threads::Threads)
# Result<T> lives in utils; logger re-exports it for backward compat
if(TARGET utils)
    target_link_libraries(logger PUBLIC utils)
endif()
set_target_properties(logger PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

# Warnings
if(COMMAND ffbird_enable_warnings)
    ffbird_enable_warnings(logger)
endif()

if(FFBIRD_BUILD_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()
