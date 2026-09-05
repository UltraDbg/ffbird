cmake_minimum_required(VERSION)

set(CMAKE_CXX_STANDARD 11)

project(file-util LANGUAGES CXX)

add_library(file-util include/file-util/file-util.h src/file-util.cpp include/file-util/envpath-util.h src/envpath-util.cpp)

target_include_directories(file-util PUBLIC include/)

if (TARGET logger)
    target_link_libraries(file-util logger)
    target_compile_definitions(file-util PRIVATE HAVE_LOGGER)
endif()