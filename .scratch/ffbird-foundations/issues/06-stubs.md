---
id: 06-stubs
title: "stubs: platform-linux, runtime-linux, libc-shim, libgles-shim, libegl-shim, game"
blockedBy: [01-build-env]
blocks: []
readyForAgent: true
---

# 06-stubs — stubs: platform-linux, runtime-linux, libc-shim, libgles-shim, libegl-shim, game

## Summary

Create top-level dirs platform-linux/, runtime-linux/, libc-shim/, libgles-shim/, libegl-shim/, game/ each as STATIC stub that builds and fails on non-Linux.

## Seam

Each stub's public header include — build proves seam.

## Acceptance Criteria

1. Each has CMakeLists.txt add_library(STUB STATIC src/stub.cpp) PUBLIC include/ and include/<mod>/stub.h with #ifndef guard + #ifndef __linux__ #error "requires Linux".
2. src/stub.cpp is one empty TU so archive is not INTERFACE where we later need real symbols.
3. Build smoke: cmake --build succeeds, headers includable on Linux.
4. No behaviour — out of scope for foundations (Android concepts remain undecided).

## Blocked By

01-build-env

## Blocks

(leaf — no dependents)

## Spec Trace

Derived from `docs/spec/ffbird-foundations.md` — foundations first rewrite (C++11, .h only, small modules, Linux-only, Result vocab).

## Ready For Agent

Apply `ready-for-agent` triage label. No extra triage needed.

