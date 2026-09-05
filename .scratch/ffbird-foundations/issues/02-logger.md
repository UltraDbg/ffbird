---
id: 02-logger
title: "logger: thread-safe Level + log + file sink, Result<T> shared"
blockedBy: [01-build-env]
blocks: [03-file-util, 05-anticrash]
readyForAgent: true
---

# 02-logger — logger: thread-safe Level + log + file sink, Result<T> shared

## Summary

Implement logger/include/logger/log.h (.h only, C++11, #ifndef guards, Logger instance + global accessor, mutex, ofstream file_, Level enum, to_string, setMinLevel/setLogFile). logger/include/logger/result.h defines Result<T> and Result<void>.

## Seam

logger/log.h public API — tests link logger::logger and observe files/return values.

## Acceptance Criteria

1. Logger is instance-based with optional Logger::global(); not a hard singleton — tests can isolate.
2. logger::Result<T> {bool ok; T value; string error; static success/failure} + Result<void> specialization, C++11, never throws.
3. log() uses vsnprintf 4096 + strftime localtime_r + mutex lock_guard + fflush; file sink creates parent dirs, appends.
4. GTest: level filter hides DEBUG when min INFO, setLogFile creates a/b/log.txt, 2 threads x1000 no interleaving (line count 2000), to_string(Level).
5. Header is .h only, clang-format/tidy clean.

## Blocked By

01-build-env

## Blocks

03-file-util, 05-anticrash

## Spec Trace

Derived from `docs/spec/ffbird-foundations.md` — foundations first rewrite (C++11, .h only, small modules, Linux-only, Result vocab).

## Ready For Agent

Apply `ready-for-agent` triage label. No extra triage needed.

