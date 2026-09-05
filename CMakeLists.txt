# ffbird — top-level CMake
# cmake_minimum_required rationale: uses target_include_directories(INTERFACE), option(),
# find_package(Threads), enable_testing(), string(TIMESTAMP) — all available since 3.5.
# We require 3.14 for CMakePresets.json support (version 3) and modern policy behavior.
cmake_minimum_required(VERSION 3.14)
project(ffbird LANGUAGES C CXX ASM)
message(STATUS "FFBIRD - The Fast Native Engine")

# --- Linux-only gate ---
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "ffbird requires Linux — detected ${CMAKE_SYSTEM_NAME}")
endif()

# Global C++ settings
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories: libraries -> /lib, binaries -> /bin, tests -> /tests, android natives -> /lib_android
# Per-config variants for multi-config generators (e.g. Visual Studio, Ninja Multi-Config)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin)

# For the current development environment, we are guaranteed to be on a x64 machine
set(IS64_BIT FALSE)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(IS64_BIT TRUE)
endif()

# Under the assumption that null dereferences cannot occur, certain
# null pointer checks are optimized. However, given our unusual memory
# layouts, we disable it
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-delete-null-pointer-checks")
string(TIMESTAMP BUILD_TIMESTAMP "%Y%m%d-%H%M%S" UTC)
message(STATUS "Build timestamp: ${BUILD_TIMESTAMP}")

# Ensure output directories exist (bin may stay empty if no runtime targets)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib_android)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

##########################################
# CMake modules
##########################################
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
include(Options)
include(Deps)
include(CWarnings)

##########################################
# Git hash
##########################################
find_program(GIT_EXEC git)
function(git_commit_hash DIR OUT_VAL)
    if(GIT_EXEC)
        execute_process(
            COMMAND ${GIT_EXEC} log -1 --format=%h
            WORKING_DIRECTORY ${DIR}
            OUTPUT_VARIABLE OUTPUT
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE RESULT
        )
        if(RESULT EQUAL 0 AND NOT OUTPUT STREQUAL "")
            set(${OUT_VAL} ${OUTPUT} PARENT_SCOPE)
        else()
            # fallback to rev-parse
            execute_process(
                COMMAND ${GIT_EXEC} rev-parse --short HEAD
                WORKING_DIRECTORY ${DIR}
                OUTPUT_VARIABLE OUTPUT2
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE RESULT2
            )
            if(RESULT2 EQUAL 0 AND NOT OUTPUT2 STREQUAL "")
                set(${OUT_VAL} ${OUTPUT2} PARENT_SCOPE)
            else()
                set(${OUT_VAL} "unknown" PARENT_SCOPE)
            endif()
        endif()
    else()
        set(${OUT_VAL} "unknown" PARENT_SCOPE)
    endif()
endfunction()

git_commit_hash(${CMAKE_SOURCE_DIR} MANIFEST_GIT_COMMIT_HASH)
message(STATUS "Git commit hash: ${MANIFEST_GIT_COMMIT_HASH}")

##########################################
# Testing
##########################################
if(FFBIRD_BUILD_TESTS)
    enable_testing()
    include(CTest)
endif()

##########################################
# Subdirectories — foundations
##########################################
add_subdirectory(utils)
add_subdirectory(logger)
add_subdirectory(file-util)
add_subdirectory(argparser)
add_subdirectory(anticrash)
add_subdirectory(bionic)
add_subdirectory(hybris)
add_subdirectory(ulinker)
add_subdirectory(hybris-bridge)
add_subdirectory(bionic-libm)
add_subdirectory(bionic-libc)
add_subdirectory(bionic-libdl)
add_subdirectory(shims/liblog-shim)

# Stubs — each proves module layout on Linux
if(EXISTS "${CMAKE_SOURCE_DIR}/platform-linux/CMakeLists.txt")
    add_subdirectory(platform-linux)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/runtime-linux/CMakeLists.txt")
    add_subdirectory(runtime-linux)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/libc-shim/CMakeLists.txt")
    add_subdirectory(libc-shim)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/libgles-shim/CMakeLists.txt")
    add_subdirectory(libgles-shim)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/libegl-shim/CMakeLists.txt")
    add_subdirectory(libegl-shim)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/game/CMakeLists.txt")
    add_subdirectory(game)
endif()

# Natives — separate NDK project, OFF by default
if(FFBIRD_BUILD_NATIVES)
    if(NOT DEFINED ANDROID_NDK)
        if(DEFINED ENV{ANDROID_NDK})
            set(ANDROID_NDK $ENV{ANDROID_NDK})
        else()
            set(ANDROID_NDK "/home/clickpaw/Android/Sdk/ndk/30.0.16138531")
        endif()
    endif()
    if(EXISTS "${ANDROID_NDK}/build/cmake/android.toolchain.cmake")
        message(STATUS "Building natives with NDK: ${ANDROID_NDK}")
        # Build natives as ExternalProject or via add_subdirectory with toolchain would require re-configure.
        # We use ExternalProject_Add to keep host and NDK toolchains separate.
        include(ExternalProject)
        ExternalProject_Add(natives_print_test_arm64
            SOURCE_DIR "${CMAKE_SOURCE_DIR}/natives"
            BINARY_DIR "${CMAKE_BINARY_DIR}/natives_build_arm64"
            CMAKE_ARGS
                -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake
                -DANDROID_ABI=arm64-v8a
                -DANDROID_PLATFORM=android-21
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/lib_android/arm64-v8a
                -DCMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG=${CMAKE_BINARY_DIR}/lib_android/arm64-v8a
                -DCMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE=${CMAKE_BINARY_DIR}/lib_android/arm64-v8a
                -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/lib_android/arm64-v8a
                -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/lib_android/arm64-v8a
            INSTALL_COMMAND ""
            BUILD_ALWAYS TRUE
        )
        ExternalProject_Add(natives_print_test_x86_64
            SOURCE_DIR "${CMAKE_SOURCE_DIR}/natives"
            BINARY_DIR "${CMAKE_BINARY_DIR}/natives_build_x86_64"
            CMAKE_ARGS
                -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake
                -DANDROID_ABI=x86_64
                -DANDROID_PLATFORM=android-21
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/lib_android/x86_64
                -DCMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG=${CMAKE_BINARY_DIR}/lib_android/x86_64
                -DCMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE=${CMAKE_BINARY_DIR}/lib_android/x86_64
                -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/lib_android/x86_64
                -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/lib_android/x86_64
            INSTALL_COMMAND ""
            BUILD_ALWAYS TRUE
        )
        # Ensure built as part of default 'all' target
        add_custom_target(natives_all ALL DEPENDS natives_print_test_arm64 natives_print_test_x86_64)
        # Host inspection: also copy x86_64 libs to lib_android root for ctest
        add_custom_command(TARGET natives_all POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lib_android
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/lib_android/x86_64 ${CMAKE_BINARY_DIR}/lib_android
            COMMENT "Copy x86_64 natives to lib_android for host inspection"
        )
    else()
        message(WARNING "FFBIRD_BUILD_NATIVES=ON but ANDROID_NDK not found at ${ANDROID_NDK} — skipping natives")
    endif()
endif()
