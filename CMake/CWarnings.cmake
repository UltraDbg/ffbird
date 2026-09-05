# CWarnings.cmake — warning flags helper
# Requires CMake 3.0+ (function, target_compile_options).

function(ffbird_enable_warnings target)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    )
    # GCC extras
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PRIVATE
            -Wmisleading-indentation
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
        )
    endif()
    if(FFBIRD_WERROR)
        target_compile_options(${target} PRIVATE -Werror)
    endif()
endfunction()
