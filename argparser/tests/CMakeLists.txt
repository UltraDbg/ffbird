cmake_minimum_required(VERSION 3.14)
find_package(GTest QUIET)
if(NOT GTEST_FOUND)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(GTEST gtest)
        if(GTEST_FOUND)
            add_library(GTest::GTest INTERFACE IMPORTED)
            target_include_directories(GTest::GTest INTERFACE ${GTEST_INCLUDE_DIRS})
            target_link_libraries(GTest::GTest INTERFACE ${GTEST_LIBRARIES})
            find_library(GTEST_MAIN_LIB gtest_main HINTS ${GTEST_LIBRARY_DIRS})
            if(GTEST_MAIN_LIB)
                add_library(GTest::Main INTERFACE IMPORTED)
                target_link_libraries(GTest::Main INTERFACE ${GTEST_MAIN_LIB})
            endif()
        endif()
    endif()
endif()

if(TARGET GTest::GTest)
    set(GTEST_TARGET GTest::GTest)
else()
    find_library(GTEST_LIB gtest)
    find_library(GTEST_MAIN_LIB gtest_main)
    if(GTEST_LIB AND GTEST_MAIN_LIB)
        add_executable(argparser_tests test_argparser.cpp)
        target_link_libraries(argparser_tests PRIVATE argparser logger ${GTEST_LIB} ${GTEST_MAIN_LIB} pthread)
        add_test(NAME argparser_tests COMMAND argparser_tests)

# Tests binaries go to /tests
set_target_properties(argparser_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

        return()
    else()
        message(WARNING "GTest not found — skipping argparser tests")
        return()
    endif()
endif()

if(TARGET GTest::Main)
    set(GTEST_MAIN_TARGET GTest::Main)
else()
    find_library(GTEST_MAIN_LIB2 gtest_main)
    set(GTEST_MAIN_TARGET ${GTEST_MAIN_LIB2})
endif()

add_executable(argparser_tests test_argparser.cpp)
target_link_libraries(argparser_tests PRIVATE argparser logger ${GTEST_TARGET} Threads::Threads)
if(GTEST_MAIN_TARGET)
    target_link_libraries(argparser_tests PRIVATE ${GTEST_MAIN_TARGET})
endif()
add_test(NAME argparser_tests COMMAND argparser_tests)

# Tests binaries go to /tests
set_target_properties(argparser_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

