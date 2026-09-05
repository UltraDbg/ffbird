##########################################
############## FFBIRD ####################
##########################################
cmake_minimum_required(VERSION 3.0 ... 4.0)
message(STATUS "FFBIRD - The Fast Native Engine")

enable_language(C CXX ASM)

# For the current developement environnement, we are guaranteed
# to be on a x64 machine
set(IS64_BIT FALSE)
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(IS64_BIT TRUE)
endif()

# Under the assumption that null dereferences cannot occur, certain
# null pointer checks are optimized. However, given our unusual memory
# layouts, we disable it
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-delete-null-pointer-checks")
string(TIMESTAMP BUILD_TIMESTAMP "%Y%m%d-%H%M%S" UTC)
message(STATUS "Build timestamp: ${BUILD_TIMESTAMP}")

##########################################
##############   GIT  ####################
##########################################
find_program(GIT_EXEC git)
function(git_commit_hash DIR OUT_VAL)
  if (GIT_EXEC)
    execute_process(
      COMMAND ${GIT_EXEC}
      WORKING_DIRECTORY ${DIR}
      OUTPUT_VARIABLE OUTPUT
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE RESULT
    )

    if (RESULT EQUAL 0)
      set(${OUT_VAL} ${OUTPUT} PARENT_SCOPE)
    else()
      set(${OUT_VAL} "unknown" PARENT_SCOPE)
    endif()
  else()
    set(${OUT_VAL} "" PARENT_SCOPE)
  endif()
endfunction()

git_commit_hash(${CMAKE_SOURCE_DIR} MANIFEST_GIT_COMMIT_HASH)

add_subdirectory(logger)
add_subdirectory(file-util)
add_subdirectory(argparser)
add_subdirectory(anticrash)

# The client
add_subdirectory(client)