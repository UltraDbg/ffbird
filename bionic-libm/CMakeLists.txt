cmake_minimum_required(VERSION 3.14)
project(bionic-libm LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "bionic-libm: building real msun SHARED libm.so")

set(BIONIC_LIBM_REAL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/libm_real.c")

add_library(bionic-libm SHARED "${BIONIC_LIBM_REAL_SRC}")

# Fix machine/endian.h: original required bionic/libc/include + kernel/uapi
# We keep those as PRIVATE include for completeness, but our asm file doesn't need them
# To avoid host pollution, we do not add them for host build — the patch to math_private.h suffices
target_compile_definitions(bionic-libm PRIVATE _GNU_SOURCE)
target_compile_options(bionic-libm PRIVATE -fno-builtin -fvisibility=hidden)

if(EXISTS "${CMAKE_SOURCE_DIR}/bionic/libm/libm.map.txt")
    target_link_options(bionic-libm PRIVATE "LINKER:--version-script=${CMAKE_SOURCE_DIR}/bionic/libm/libm.map.txt")
endif()

set_target_properties(bionic-libm PROPERTIES
    OUTPUT_NAME m
    PREFIX lib
    SUFFIX .so
    C_STANDARD 17
    CXX_STANDARD 17
)

add_library(bionic-libm-headers INTERFACE)
target_include_directories(bionic-libm-headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(COMMAND ffbird_enable_warnings)
    ffbird_enable_warnings(bionic-libm)
endif()
