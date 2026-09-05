cmake_minimum_required(VERSION 3.14)
project(bionic-libm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Placeholder — real Bionic libm will be SHARED from bionic/libm/*.c
add_library(bionic-libm STATIC include/bionic_libm/libm.h src/stub.cpp)
target_include_directories(bionic-libm PUBLIC include/)
target_link_libraries(bionic-libm PUBLIC utils)
set_target_properties(bionic-libm PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

if(COMMAND ffbird_enable_warnings)
    ffbird_enable_warnings(bionic-libm)
endif()
