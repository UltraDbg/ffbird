cmake_minimum_required(VERSION 3.14)
project(bionic-libc LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "bionic-libc: building SHARED libBionicLibc.so from bionic/libc subset")

add_library(bionic-libc SHARED
    "${CMAKE_CURRENT_SOURCE_DIR}/src/libc_real.c"
)

target_compile_definitions(bionic-libc PRIVATE _GNU_SOURCE)
target_compile_options(bionic-libc PRIVATE -fno-builtin -fvisibility=hidden)

if(EXISTS "${CMAKE_SOURCE_DIR}/bionic/libc/libc.map.txt")
    target_link_options(bionic-libc PRIVATE "LINKER:--version-script=${CMAKE_SOURCE_DIR}/bionic/libc/libc.map.txt")
endif()

set_target_properties(bionic-libc PROPERTIES
    OUTPUT_NAME c
    PREFIX lib
    SUFFIX .so
    C_STANDARD 17
)

if(COMMAND ffbird_enable_warnings)
    ffbird_enable_warnings(bionic-libc)
endif()
