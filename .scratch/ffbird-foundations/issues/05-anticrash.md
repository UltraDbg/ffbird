---
id: 05-anticrash
title: "anticrash: host signal handler with backtrace + demangle + dladdr"
blockedBy: [01-build-env, 02-logger]
blocks: []
readyForAgent: true
---

# 05-anticrash — anticrash: host signal handler with backtrace + demangle + dladdr

## Summary

Implement anticrash/include/anticrash/handler.h + src/handler.cpp sigaction for SEGV/ABRT/FPE/BUS/ILL, backtrace 25 + backtrace_symbols + __cxa_demangle + dladdr, detached hung guard thread, _Exit, uninstall restores.

## Seam

anticrash/handler.h install/uninstall — death tests observe log file content.

## Acceptance Criteria

1. install(path)->bool stores old sigaction 6 signals, sets handler, hasCrashed atomic<bool> guard, launch detached thread sleep 1s -> _Exit if hung.
2. handler dumps to log file (ofstream) + stdout: backtrace + demangled + dladdr fallback for '[' symbols, 1000-word stack walk.
3. uninstall() restores old handlers.
4. GTest death test: fork child install + raise(SIGSEGV) -> log contains "Backtrace", uninstall test.
5. Links logger optionally, CMAKE_DL_LIBS for dladdr, Linux guard.

## Blocked By

01-build-env, 02-logger

## Blocks

(leaf — no dependents)

## Spec Trace

Derived from `docs/spec/ffbird-foundations.md` — foundations first rewrite (C++11, .h only, small modules, Linux-only, Result vocab).

## Ready For Agent

Apply `ready-for-agent` triage label. No extra triage needed.

