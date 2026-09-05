cmake_minimum_required(VERSION 3.14)
project(bionic-libdl LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "bionic-libdl: building SHARED libBionicLibdl.so from bionic/libdl/libdl.cpp")

add_library(bionic-libdl SHARED
    "${CMAKE_CURRENT_SOURCE_DIR}/src/libdl_real.cpp"
)

target_compile_definitions(bionic-libdl PRIVATE _GNU_SOURCE)
target_compile_options(bionic-libdl PRIVATE -fvisibility=hidden)

if(EXISTS "${CMAKE_SOURCE_DIR}/bionic/libdl/libdl.map.txt")
    target_link_options(bionic-libdl PRIVATE "LINKER:--version-script=${CMAKE_SOURCE_DIR}/bionic/libdl/libdl.map.txt")
endif()

set_target_properties(bionic-libdl PROPERTIES
    OUTPUT_NAME BionicLibdl
    PREFIX lib
    SUFFIX .so
)

if(COMMAND ffbird_enable_warnings)
    ffbird_enable_warnings(bionic-libdl)
endif()
