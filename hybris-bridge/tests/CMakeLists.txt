cmake_minimum_required(VERSION 3.14)
find_package(GTest QUIET)
if(NOT GTEST_FOUND)
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(GTEST gtest)
  endif()
endif()
if(TARGET GTest::GTest)
  set(GT GTest::GTest)
else()
  find_library(GTEST_LIB gtest)
  find_library(GTEST_MAIN_LIB gtest_main)
  if(GTEST_LIB AND GTEST_MAIN_LIB)
    add_executable(hybris_bridge_tests test_bridge.cpp)
    target_link_libraries(hybris_bridge_tests PRIVATE hybris-bridge ulinker utils ${GTEST_LIB} ${GTEST_MAIN_LIB} pthread dl)
    set_target_properties(hybris_bridge_tests PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
      RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
      RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
      RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
      RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
      ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )
    add_test(NAME hybris_bridge_tests COMMAND hybris_bridge_tests)
    return()
  else()
    message(WARNING "GTest not found — skipping hybris-bridge tests")
    return()
  endif()
endif()
if(TARGET GTest::Main)
  set(GT_MAIN GTest::Main)
else()
  find_library(GTEST_MAIN_LIB2 gtest_main)
  set(GT_MAIN ${GTEST_MAIN_LIB2})
endif()
add_executable(hybris_bridge_tests test_bridge.cpp)
target_link_libraries(hybris_bridge_tests PRIVATE hybris-bridge ulinker utils ${GT} Threads::Threads dl)
if(GT_MAIN)
  target_link_libraries(hybris_bridge_tests PRIVATE ${GT_MAIN})
endif()
set_target_properties(hybris_bridge_tests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
add_test(NAME hybris_bridge_tests COMMAND hybris_bridge_tests)
