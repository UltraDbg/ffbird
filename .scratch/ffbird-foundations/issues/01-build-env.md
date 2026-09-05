---
id: 01-build-env
title: "Foundations: build env, presets, Linux gate, Result vocabulary, formatting"
blockedBy: []
blocks: [02-logger, 03-file-util, 04-argparser, 05-anticrash, 06-stubs, 07-natives]
readyForAgent: true
---

# 01-build-env — Foundations: build env, presets, Linux gate, Result vocabulary, formatting

## Summary

Harden top CMake + CMake/ + presets + clang config + ext placeholder so empty modules build green.

## Seam

cmake --preset debug (build seam) — no source seam, just configure+build+ctest.

## Acceptance Criteria

1. CMakeLists.txt has Linux-only gate (message(FATAL_ERROR) if not Linux) plus header #error guard proposal.
2. CMAKE_CXX_STANDARD 11 REQUIRED, correct git_commit_hash (git log -1 --format=%h or rev-parse), string(TIMESTAMP).
3. CMake/Options.cmake (FFBIRD_BUILD_TESTS, FFBIRD_WERROR), Deps.cmake (Threads, GTest optional), CWarnings.cmake (flbird_enable_warnings) exist.
4. CMakePresets.json debug/release/ci (-Werror in ci).
5. .clang-format (Google 100 cols) + .clang-tidy in root.
6. ext/README.md reserves ext for sdl3.
7. enable_testing()/CTest wired, cmake --preset debug && cmake --build --preset debug && ctest passes on empty stubs.
8. cmake_minimum_required set to smallest covering used commands, documented in header.

## Blocked By

(none — can be grabbed immediately after spec)

## Blocks

02-logger, 03-file-util, 04-argparser, 05-anticrash, 06-stubs, 07-natives

## Spec Trace

Derived from `docs/spec/ffbird-foundations.md` — foundations first rewrite (C++11, .h only, small modules, Linux-only, Result vocab).

## Ready For Agent

Apply `ready-for-agent` triage label. No extra triage needed.

