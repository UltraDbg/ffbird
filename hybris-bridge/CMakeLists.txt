cmake_minimum_required(VERSION 3.14)
project(hybris-bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(hybris-bridge STATIC include/hybris_bridge/bridge.h src/bridge.cpp)
target_include_directories(hybris-bridge PUBLIC include/)
target_link_libraries(hybris-bridge PUBLIC utils ulinker logger)
find_library(DL_LIB dl)
if(DL_LIB)
    target_link_libraries(hybris-bridge PUBLIC ${DL_LIB})
endif()
set_target_properties(hybris-bridge PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

if(COMMAND ffbird_enable_warnings)
    ffbird_enable_warnings(hybris-bridge)
endif()

if(FFBIRD_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
