# Deps.cmake — dependencies for ffbird
# Requires CMake 3.5+ (find_package, Threads).

find_package(Threads REQUIRED)

if(FFBIRD_BUILD_TESTS)
    find_package(GTest QUIET)
    if(NOT GTEST_FOUND)
        # Try pkg-config fallback
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(GTEST gtest)
        endif()
    endif()
endif()
