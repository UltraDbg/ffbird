cmake_minimum_required(VERSION 3.14)
project(log-shim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(log-shim SHARED src/log_shim.cpp)
target_include_directories(log-shim PUBLIC include/)
target_link_libraries(log-shim PUBLIC utils logger)
set_target_properties(log-shim PROPERTIES
    OUTPUT_NAME log
    PREFIX lib
    SUFFIX .so
    CXX_VISIBILITY_PRESET hidden
    C_VISIBILITY_PRESET hidden
)
target_link_options(log-shim PRIVATE "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/LibLog.version")

if(COMMAND ffbird_enable_warnings)
    ffbird_enable_warnings(log-shim)
endif()
