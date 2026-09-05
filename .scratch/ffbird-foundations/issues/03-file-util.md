---
id: 03-file-util
title: "file-util: FileUtil + EnvPathUtil + writeFile, Result<void> whole"
blockedBy: [01-build-env, 02-logger]
blocks: []
readyForAgent: true
---

# 03-file-util — file-util: FileUtil + EnvPathUtil + writeFile, Result<void> whole

## Summary

Implement file-util/include/file-util/file_util.h + env_path_util.h + src/*, both in one lib file-util, optional HAVE_LOGGER link.

## Seam

file-util/file_util.h + env_path_util.h — observe files on disk + Result.ok.

## Acceptance Criteria

1. FileUtil {getParent, exists(access), isDirectory(stat/S_ISDIR), mkdirRecursive -> Result<void>, readFile -> Result<string>, writeFile -> Result<void>}. mkdirRecursive Result not throw (except documented).
2. EnvPathUtil {getAppDir(readlink /proc/self/exe + dirname), getWorkingDir(getcwd), getHomeDir(getenv HOME/getpwuid_r), getDataHome(XDG_DATA_HOME or ~/.local/share), findInPath(PATH parse + cwd prefix + X_OK)} with #ifndef __linux__ #error guard.
3. Thread-safe by construction (no shared mutable). HAVE_LOGGER optional: file-util/CMakeLinks logger PRIVATE when TARGET exists + compile_def HAVE_LOGGER.
4. GTest: getParent "/a/b/" == "/a", exists/isDirectory, mkdirRecursive nested, write+read round-trip, missing read -> !ok, getAppDir non-empty, getDataHome respects env, findInPath("sh").

## Blocked By

01-build-env, 02-logger

## Blocks

(leaf — no dependents)

## Spec Trace

Derived from `docs/spec/ffbird-foundations.md` — foundations first rewrite (C++11, .h only, small modules, Linux-only, Result vocab).

## Ready For Agent

Apply `ready-for-agent` triage label. No extra triage needed.

