cmake_minimum_required(VERSION 3.14)
project(bionic-libm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Placeholder — real Bionic libm (msun 298) is vendored in bionic/libm
# Building real msun requires Android build system (Android.bp + Bionic headers)
# For now, keep stub that provides version script path for verification
# Future: integrate via bionic/CMakeLists.txt that builds bionic_m_real SHARED
if(EXISTS "${CMAKE_SOURCE_DIR}/bionic/libm/libm.map.txt")
    message(STATUS "bionic-libm: Bionic vendored (recent AOSP) found at bionic/libm — stub for now, real build via bionic/CMakeLists.txt")
endif()

add_library(bionic-libm STATIC include/bionic_libm/libm.h src/stub.cpp)
target_include_directories(bionic-libm PUBLIC include/)
target_link_libraries(bionic-libm PUBLIC utils)
set_target_properties(bionic-libm PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

if(COMMAND ffbird_enable_warnings)
    ffbird_enable_warnings(bionic-libm)
endif()
