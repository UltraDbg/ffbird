# natives — separate CMake project for Android NDK test fixture
# cmake_minimum_required rationale: uses add_library SHARED, find_library
cmake_minimum_required(VERSION 3.14)
project(natives LANGUAGES C CXX)

# This project is intended to be built with Android NDK toolchain:
#   cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake
#         -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 ..

add_library(print_test SHARED print_test/print_test.cpp)

target_include_directories(print_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link against NDK log library
find_library(ANDROID_LOG_LIB log)
if(ANDROID_LOG_LIB)
    target_link_libraries(print_test PRIVATE ${ANDROID_LOG_LIB})
else()
    # Fallback: try log directly (NDK provides it via toolchain)
    target_link_libraries(print_test PRIVATE log)
endif()

set_target_properties(print_test PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
