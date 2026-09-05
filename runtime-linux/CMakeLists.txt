cmake_minimum_required(VERSION 3.14)
project(runtime-linux LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(runtime-linux STATIC src/stub.cpp)
target_include_directories(runtime-linux PUBLIC include/)
target_link_libraries(runtime-linux PUBLIC utils ulinker hybris-bridge logger)
set_target_properties(runtime-linux PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF LINKER_LANGUAGE CXX)

if(COMMAND ffbird_enable_warnings)
    ffbird_enable_warnings(runtime-linux)
endif()
