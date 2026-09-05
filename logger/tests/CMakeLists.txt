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
    if(TARGET GTest::Main)
        set(GTEST_MAIN_TARGET GTest::Main)
    else()
        # fallback: try gtest_main via pkg-config
        find_library(GTEST_MAIN_LIB2 gtest_main)
        if(GTEST_MAIN_LIB2)
            set(GTEST_MAIN_TARGET ${GTEST_MAIN_LIB2})
        else()
            set(GTEST_MAIN_TARGET GTest::GTest)
        endif()
    endif()
else()
    # Try common lib names
    find_library(GTEST_LIB gtest)
    find_library(GTEST_MAIN_LIB gtest_main)
    if(GTEST_LIB AND GTEST_MAIN_LIB)
        add_executable(logger_tests test_logger.cpp)
        target_link_libraries(logger_tests PRIVATE logger ${GTEST_LIB} ${GTEST_MAIN_LIB} pthread)
        target_include_directories(logger_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
        add_test(NAME logger_tests COMMAND logger_tests)

# Tests binaries go to /tests
set_target_properties(logger_tests PROPERTIES
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
        message(WARNING "GTest not found — skipping logger tests")
        return()
    endif()
endif()

add_executable(logger_tests test_logger.cpp)
target_link_libraries(logger_tests PRIVATE logger ${GTEST_TARGET} ${GTEST_MAIN_TARGET} Threads::Threads)
# Ensure gtest_main provides main
if(TARGET GTest::Main)
    target_link_libraries(logger_tests PRIVATE GTest::Main)
endif()
add_test(NAME logger_tests COMMAND logger_tests)

# Tests binaries go to /tests
set_target_properties(logger_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

