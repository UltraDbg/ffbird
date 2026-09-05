# Options.cmake — ffbird build options
# cmake_minimum_required rationale: uses `option()` (since 2.8) and simple variables.

option(FFBIRD_BUILD_TESTS "Build tests (ctest + GTest)" OFF)
option(FFBIRD_WERROR "Treat warnings as errors" OFF)
option(FFBIRD_BUILD_NATIVES "Build natives with NDK (requires ANDROID_NDK)" OFF)
