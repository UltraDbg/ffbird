cmake_minimum_required(VERSION 3.14)
find_package(GTest QUIET)
if(NOT GTEST_FOUND)
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(GTEST gtest)
  endif()
endif()

# Resolve GTest targets/libs
if(TARGET GTest::GTest)
  set(GT GTest::GTest)
  if(TARGET GTest::Main)
    set(GT_MAIN GTest::Main)
  else()
    find_library(GTEST_MAIN_LIB2 gtest_main)
    set(GT_MAIN ${GTEST_MAIN_LIB2})
  endif()
else()
  find_library(GTEST_LIB gtest)
  find_library(GTEST_MAIN_LIB gtest_main)
  if(GTEST_LIB AND GTEST_MAIN_LIB)
    set(GT ${GTEST_LIB})
    set(GT_MAIN ${GTEST_MAIN_LIB})
  else()
    message(WARNING "GTest not found — skipping ulinker tests")
    return()
  endif()
endif()

find_package(Threads REQUIRED)

add_executable(ulinker_tests test_ulinker.cpp)
target_link_libraries(ulinker_tests PRIVATE ulinker utils ${GT} Threads::Threads dl)
if(GT_MAIN)
  target_link_libraries(ulinker_tests PRIVATE ${GT_MAIN})
endif()
target_link_libraries(ulinker_tests PRIVATE pthread)
set_target_properties(ulinker_tests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
add_test(NAME ulinker_tests COMMAND ulinker_tests)

# Bionic stress metadata verification (NDK-generated libs) — empty stub that passes
add_executable(bionic_stress_meta_tests test_bionic_stress_meta.cpp)
target_link_libraries(bionic_stress_meta_tests PRIVATE ulinker utils ${GT} ${GT_MAIN} Threads::Threads dl)
set_target_properties(bionic_stress_meta_tests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
)
add_test(NAME bionic_stress_meta_tests COMMAND bionic_stress_meta_tests)

# bionic_stress_run — forked real load test
add_executable(bionic_stress_run_tests test_bionic_stress_run.cpp)
target_link_libraries(bionic_stress_run_tests PRIVATE ulinker hybris-bridge utils logger ${GT} ${GT_MAIN} Threads::Threads dl)
set_target_properties(bionic_stress_run_tests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
)
add_test(NAME bionic_stress_run_tests COMMAND bionic_stress_run_tests)
