# libc-shim — stub
# cmake_minimum_required rationale: add_library STATIC, target_include_directories
cmake_minimum_required(VERSION 3.14)
project(libc-shim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(libc-shim STATIC src/stub.cpp)
target_include_directories(libc-shim PUBLIC include/)
set_target_properties(libc-shim PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF LINKER_LANGUAGE CXX)

if(COMMAND ffbird_enable_warnings)
    ffbird_enable_warnings(libc-shim)
endif()
