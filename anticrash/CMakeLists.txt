# anticrash — host crash handler
# cmake_minimum_required rationale: add_library STATIC, target_link_libraries with dl.
cmake_minimum_required(VERSION 3.14)
project(anticrash LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(anticrash STATIC
    include/anticrash/handler.h
    src/handler.cpp
)
target_include_directories(anticrash PUBLIC include/)
set_target_properties(anticrash PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
target_link_libraries(anticrash PUBLIC Threads::Threads)
# dladdr requires dl
find_library(DL_LIB dl)
if(DL_LIB)
    target_link_libraries(anticrash PRIVATE ${DL_LIB})
endif()

if(TARGET logger)
    target_link_libraries(anticrash PRIVATE logger)
endif()

if(COMMAND ffbird_enable_warnings)
    ffbird_enable_warnings(anticrash)
endif()

if(FFBIRD_BUILD_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()
