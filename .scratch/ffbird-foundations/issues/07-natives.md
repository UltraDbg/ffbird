---
id: 07-natives
title: "natives/print_test: separate NDK CMake project libprint_test.so"
blockedBy: [01-build-env]
blocks: []
readyForAgent: true
---

# 07-natives — natives/print_test: separate NDK CMake project libprint_test.so

## Summary

Create natives/CMakeLists.txt + print_test/ that builds libprint_test.so with NDK android.toolchain.cmake, exported void print_test_hello() calling __android_log_print.

## Seam

artefact existence + nm -D — later host dlopen test belongs to future runtime-linux loader ticket, not this spec.

## Acceptance Criteria

1. natives/CMakeLists.txt project(natives), add_library(print_test SHARED print_test/print_test.cpp) linked to NDK log, one extern "C" void print_test_hello().
2. natives/print_test/print_test.cpp includes <android/log.h> and calls __android_log_print(ANDROID_LOG_INFO,"print_test","hello").
3. Top-level option FFBIRD_BUILD_NATIVES OFF by default; when ON and ANDROID_NDK set (default /home/clickpaw/Android/Sdk/ndk/30.0.16138531), add_custom_target or ExternalProject_Add builds arm64-v8a android-21 via toolchain build/cmake/android.toolchain.cmake.
4. When NDK present, cmake -DFFBIRD_BUILD_NATIVES=ON .. && cmake --build --target print_test produces libprint_test.so; nm -D shows print_test_hello and U __android_log_print. Host ctest without NDK still passes.

## Blocked By

01-build-env

## Blocks

(leaf — no dependents)

## Spec Trace

Derived from `docs/spec/ffbird-foundations.md` — foundations first rewrite (C++11, .h only, small modules, Linux-only, Result vocab).

## Ready For Agent

Apply `ready-for-agent` triage label. No extra triage needed.

