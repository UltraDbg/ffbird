cmake_minimum_required(VERSION 3.14)
project(hybris LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "hybris: building hybris-common STATIC with WANT_LINKER_N")

add_library(hybris-common STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/hybris_shim.cpp"
)

target_include_directories(hybris-common PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/hybris/include"
)

target_compile_definitions(hybris-common PUBLIC WANT_LINKER_N _GNU_SOURCE)
target_compile_options(hybris-common PUBLIC -fvisibility=hidden)

find_library(DL_LIB dl)
if(DL_LIB)
    target_link_libraries(hybris-common PUBLIC ${DL_LIB})
endif()
find_package(Threads REQUIRED)
target_link_libraries(hybris-common PUBLIC Threads::Threads)

if(COMMAND ffbird_enable_warnings)
    ffbird_enable_warnings(hybris-common)
endif()
