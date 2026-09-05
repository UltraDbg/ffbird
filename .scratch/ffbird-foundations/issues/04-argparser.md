---
id: 04-argparser
title: "argparser: header-only Arg<T> + ArgParser + ArgList"
blockedBy: [01-build-env]
blocks: []
readyForAgent: true
---

# 04-argparser — argparser: header-only Arg<T> + ArgParser + ArgList

## Summary

Implement argparser/include/argparser/*.h (arg_list.h, arg_parser.h, arg.h) header-only, C++11, .h only, Result-based parse.

## Seam

argparser/arg_parser.h + arg.h — tests call parse and inspect get().

## Acceptance Criteria

1. ArgList wraps argc/argv with next()/nextOrNull()/hasNext().
2. ArgParser {addArg(long,short,desc,handler), parse(argc,argv)->Result<void>, printHelp()} with unordered_map<string,function>.
3. template Arg<T> {Arg(parser,long,short,desc,def); const T& get() const;} plus handleValue overloads string/int/float/bool (case-insensitive on/off/true/false/1/0)/vector<T>.
4. GTest: Arg<string> --data-dir foo, Arg<bool> true/on/yes, missing value -> !ok, unknown flag -> !ok, --help prints table.
5. INTERFACE add_library(argparser), no compiled src, clang-tidy clean.

## Blocked By

01-build-env

## Blocks

(leaf — no dependents)

## Spec Trace

Derived from `docs/spec/ffbird-foundations.md` — foundations first rewrite (C++11, .h only, small modules, Linux-only, Result vocab).

## Ready For Agent

Apply `ready-for-agent` triage label. No extra triage needed.

