# argparser — header-only argument parser
# cmake_minimum_required rationale: INTERFACE library requires 3.0+.
cmake_minimum_required(VERSION 3.14)
project(argparser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(argparser INTERFACE)
target_include_directories(argparser INTERFACE include/)
target_compile_features(argparser INTERFACE cxx_std_11)
# argparser uses logger::Result, so depend on logger if available
if(TARGET logger)
    target_link_libraries(argparser INTERFACE logger)
endif()

if(FFBIRD_BUILD_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()
