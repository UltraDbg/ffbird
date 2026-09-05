cmake_minimum_required(VERSION 3.14)
project(bionic LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "bionic: building bionic-linker STATIC from bionic/linker")

add_library(bionic-linker STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/linker_compat.cpp"
)

target_include_directories(bionic-linker PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(bionic-linker PRIVATE _GNU_SOURCE PATH_MAX=256)
# Do NOT force include bionic headers for host build — keep host STL clean
target_compile_options(bionic-linker PRIVATE -fvisibility=hidden)
set_target_properties(bionic-linker PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

find_library(Z_LIB z)
if(Z_LIB)
    target_link_libraries(bionic-linker PUBLIC ${Z_LIB})
endif()
find_package(Threads REQUIRED)
target_link_libraries(bionic-linker PUBLIC Threads::Threads)

if(COMMAND ffbird_enable_warnings)
    ffbird_enable_warnings(bionic-linker)
endif()
