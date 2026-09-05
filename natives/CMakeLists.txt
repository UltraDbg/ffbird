# natives — separate CMake project for Android NDK test fixture
# cmake_minimum_required rationale: uses add_library SHARED, find_library
cmake_minimum_required(VERSION 3.14)
project(natives LANGUAGES C CXX)

# Output for android natives -> /lib_android
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib_android)
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib_android)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib_android)
endif()
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib_android)

# This project is intended to be built with Android NDK toolchain:
#   cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake
#         -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 ..

add_library(print_test SHARED print_test/print_test.cpp)
target_include_directories(print_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
find_library(ANDROID_LOG_LIB log)
if(ANDROID_LOG_LIB)
    target_link_libraries(print_test PRIVATE ${ANDROID_LOG_LIB})
else()
    target_link_libraries(print_test PRIVATE log)
endif()
set_target_properties(print_test PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

add_library(m_probe SHARED libm_probe/libm_probe.cpp)
target_include_directories(m_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(m_probe PROPERTIES OUTPUT_NAME "m_probe" PREFIX "lib" SUFFIX ".so")
find_library(ANDROID_LOG_LIB2 log)
if(ANDROID_LOG_LIB2)
    target_link_libraries(m_probe PRIVATE ${ANDROID_LOG_LIB2} m)
else()
    target_link_libraries(m_probe PRIVATE log m)
endif()
set_target_properties(m_probe PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

add_library(bionic_stress SHARED bionic_stress/bionic_stress.cpp)
target_include_directories(bionic_stress PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(bionic_stress PROPERTIES OUTPUT_NAME "bionic_stress" PREFIX "lib" SUFFIX ".so")
find_library(ANDROID_LOG_LIB3 log)
if(ANDROID_LOG_LIB3)
    target_link_libraries(bionic_stress PRIVATE ${ANDROID_LOG_LIB3} m log)
else()
    target_link_libraries(bionic_stress PRIVATE log m)
endif()
target_link_libraries(bionic_stress PRIVATE log m)
set_target_properties(bionic_stress PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
