cmake_minimum_required(VERSION 3.14)
project(ulinker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ulinker STATIC include/ulinker/ulinker.h src/ulinker.cpp)
target_include_directories(ulinker PUBLIC include/)
target_link_libraries(ulinker PUBLIC utils)
target_link_libraries(ulinker PRIVATE logger)

if(TARGET bionic-linker)
    target_link_libraries(ulinker PRIVATE bionic-linker)
    target_compile_definitions(ulinker PRIVATE ULINKER_HAS_BIONIC=1)
endif()

find_library(DL_LIB dl)
if(DL_LIB)
    target_link_libraries(ulinker PUBLIC ${DL_LIB})
endif()
set_target_properties(ulinker PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

if(COMMAND ffbird_enable_warnings)
    ffbird_enable_warnings(ulinker)
endif()

if(FFBIRD_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
