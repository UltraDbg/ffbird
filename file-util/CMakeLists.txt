# file-util — cmake_minimum_required rationale: add_library, target_include_directories(PUBLIC/PRIVATE).
cmake_minimum_required(VERSION 3.14)
project(file-util LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(file-util STATIC
    include/file-util/file_util.h
    include/file-util/env_path_util.h
    src/file_util.cpp
    src/env_path_util.cpp
)
target_include_directories(file-util PUBLIC include/)
set_target_properties(file-util PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

if(TARGET logger)
    target_link_libraries(file-util PUBLIC logger)
    target_compile_definitions(file-util PRIVATE HAVE_LOGGER)
endif()

if(COMMAND ffbird_enable_warnings)
    ffbird_enable_warnings(file-util)
endif()

if(FFBIRD_BUILD_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()
