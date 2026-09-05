## Problem Statement

`ffbird` is being rewritten as a stronger, more organized base for a native Linux compatibility runtime (started from Flappy Bird, in the style of jjride & mcpelauncher). The current skeleton (`logger/`, `file-util/`, `argparser/`, `anticrash/`, `client/`, `ext/`) is empty: headers are placeholders, no implementation, no tests, no Linux guard, no `C++11`-clean `Result` vocabulary, and no `natives/` test fixtures. The previous rewrite plan was rejected because it assumed porting `ffbird_cool`/`mcpelauncher-manifest` code and structure.

The user wants a **novel foundations-first approach**: produce tiny, single-job modules that compile only on Linux, use only `.h` headers, stay strictly `C++11`, and prove the layout with an Android NDK side-project `natives/print_test` (`libprint_test.so` calling `__android_log_print`). The first milestone (loading `libprint_test.so` and seeing its print) must not be chased at the cost of proper foundations — the foundations have to be shippable, testable, and thread-safe before any `platform/runtime/shims/game` or Android generic code is designed.

Without this, every later layer (shims, loader, game) would inherit an inconsistent build env, a split error model, and copy-pasted seams from upstream.

## Solution

Rewrite `ffbird` from the foundations upward under four hard constraints: only `.h` headers, `C++11` mandatory, one job per module, no copy from `ffbird_cool`/`mcpelauncher-manifest` (ideas only if explicitly allowed). Keep `ext/` as a reserved empty directory for future external CMake packages. Remove `client/` as a top-level concept (it will return later as `game/` wiring).

Build `logger`, `file-util`, `argparser`, `anticrash` as the first real modules with thread-safe, `Result<T>`-based APIs. Provide empty, Linux-guarded stubs for `platform-linux`, `runtime-linux`, `libc-shim`, `libgles-shim`, `libegl-shim`, `game` so the module layout is proven by `cmake --build` without implementing Android behaviour. Provide `natives/` as a **separate CMake project** built with the system NDK (`/home/clickpaw/Android/Sdk/ndk/30.0.16138531` r30-beta3, platform `android-37.0`) that produces `libprint_test.so`. Harden the build env (`CMake/`, `CMakePresets.json`, `.clang-format`/`.clang-tidy`, `GTest`, `ctest`, Linux-only gate, correct `cmake_minimum_required` per function used).

## User Stories

1. As a contributor, I want `ffbird` to fail to configure on non-Linux with a clear error, so that I never accidentally build a Linux-only runtime on the wrong OS.
2. As a contributor, I want the top-level build to accept `-DFFBIRD_BUILD_TESTS=ON` and run `ctest`, so that foundations stay green on every commit.
3. As a contributor, I want a `debug`/`release`/`ci` preset that sets the same `CXX_STANDARD 11` and warning flags, so that local and CI builds match.
4. As a contributor, I want `clang-format` and `clang-tidy` configs checked in, so that `.h`-only headers stay consistently formatted without debate.
5. As a contributor, I want `ext/` to exist as `ext/README.md` saying it is reserved, so that future `sdl3` vendoring has a known home and no one puts project sources there.
6. As a `logger` consumer, I want to include one `.h` and call `Logger::log(Level, tag, fmt, ...)` with a level filter, so that I can emit diagnostics without touching global state.
7. As a `logger` consumer, I want to create an instance (`Logger logger;`) and optionally use a global accessor, so that tests can isolate logging.
8. As a `logger` consumer, I want `setLogFile(path)` to create parent dirs and append, so that host crashes can be inspected from a file.
9. As a `logger` consumer, I want the logger to be thread-safe, so that two threads logging 1000 lines each never interleave a single entry.
10. As a `file-util` consumer, I want `FileUtil::readFile(path) -> Result<string>` and `writeFile(path, data) -> Result<void>`, so that I can handle `NOT_FOUND` vs `IO_ERROR` without exceptions.
11. As a `file-util` consumer, I want `exists`/`isDirectory`/`getParent`/`mkdirRecursive` as small, tested helpers, so that bootstrap code does not shell out to `std::filesystem` (C++17).
12. As a `file-util` consumer, I want `EnvPathUtil::getAppDir()` via `readlink(/proc/self/exe)`, `getWorkingDir`, `getHomeDir`, `getDataHome` (XDG), and `findInPath`, so that data-dir resolution works without hard-coding `Assets/`.
13. As a `file-util` consumer, I want `Result<T>` as the single error vocabulary across `file-util` and `logger::Result`, so that I learn one pattern for `C++11`.
14. As an `argparser` consumer, I want a header-only parser where `Arg<string> dataDir(parser, "--data-dir", "-d", "desc")` registers itself, so that adding a flag is one line.
15. As an `argparser` consumer, I want `parse(argc, argv) -> Result<void>` with an error string on unknown flag or missing value, so that `main` can print `help` deterministically.
16. As an `argparser` consumer, I want `--help`/`-h` to print a sorted help table, so that milestones can document themselves.
17. As an operator, I want `anticrash` to `install(logFile)` and catch `SIGSEGV/ABRT/FPE/BUS/ILL`, dump `backtrace` + `__cxa_demangle` + `dladdr`, then `_Exit`, so that a host crash leaves a file like `mcpelauncher-core/crash_handler`.
18. As an operator, I want `anticrash::uninstall()` to restore old handlers, so that tests can isolate signal state.
19. As a `platform-linux` consumer, I want the module to build as a stub that includes successfully on Linux and `#error`s on other OS, so that the layout is proven before any SDL choice.
20. As a `runtime-linux` consumer, I want the same stub guarantee as `platform-linux`, so that the host `dlopen` seam has a known home without committing to `NativeLoader` semantics yet.
21. As a `libc-shim`/`libgles-shim`/`libegl-shim` consumer, I want one top-level dir per shim (`libc-shim`, not `shims/libc`) that builds as a stub `STATIC` lib, so that future `add_bionic_shim` work has a convention.
22. As a `game` consumer, I want a single `game/` dir for the one game impl (not Android-generic code), stubbed now, so that game-specific bridges do not leak into `runtime-linux`.
23. As a tester, I want `natives/print_test` to build `libprint_test.so` with the system NDK `android.toolchain.cmake` for `arm64-v8a` + `android-21`, so that the milestone artifact is a real Bionic `.so` calling `__android_log_print`.
24. As a tester, I want the top-level build to **not** require the NDK for host tests — `FFBIRD_BUILD_NATIVES=OFF` is the default and host `ctest` still passes, so that contributors without NDK can work on `logger`/`file-util`.
25. As a CI maintainer, I want `FFBIRD_BUILD_NATIVES=ON` to be testable on a machine with NDK at `ANDROID_NDK=/home/clickpaw/Android/Sdk/ndk/30.0.16138531` (or env override), so that nightly verifies `libprint_test.so` still builds.
26. As a contributor, I want every public `.h` to use `#ifndef` guards (not just `#pragma once`) and be checked by `clang-tidy`, so that the `.h`-only rule is enforced.
27. As a contributor, I want `Result<T>` to never throw and to carry a copyable `std::string error`, so that `C++11` code can handle errors without `try/catch`.
28. As a contributor, I want `file-util` to optionally link `logger` via `HAVE_LOGGER` without a hard dependency, so that it can emit debug on `readFile` failure when present but still build alone.
29. As a contributor, I want the `git_commit_hash` helper in the top-level CMake to actually call `git rev-parse --short HEAD` (or `log -1 --format=%h`) with correct `WORKING_DIRECTORY`/`OUTPUT_STRIP`, so that `BUILD_TIMESTAMP` and hash are not `unknown`.

## Implementation Decisions

* **Modules kept vs replaced**: Keep `logger`, `file-util`, `argparser`, `anticrash` as real foundations (each gets `include/<module>/<header>.h` + `src/*.cpp` + `tests/`). Remove `client/` (its future is `game/` wiring). Keep `ext/` as reserved empty dir with `README`. Add `platform-linux`, `runtime-linux`, `libc-shim`, `libgles-shim`, `libegl-shim`, `game`, `natives/print_test` as stubs now — their implementations are out of scope for this spec.

* **Header rule**: Only `.h` everywhere. C++ class definitions are allowed inside `.h` (e.g., `class Logger`, `class FileUtil`). Convention is `#ifndef <MODULE>_<FILE>_H / #define ... #endif // header guard` plus optional `#pragma once` for editors; enforced by `clang-format` and `clang-tidy` `llvm-header-guard`.

* **Language level**: `CMAKE_CXX_STANDARD 11`, `REQUIRED ON`, `EXTENSIONS OFF` in every `CMakeLists.txt` plus top-level. No `std::expected`, `string_view`, `optional`, `filesystem`; use `std::string`, `vector`, `map`/`unordered_map`, `std::mutex`, `std::atomic`, `backtrace`/`dladdr`.

* **Error vocabulary**: One `Result<T>` template (and `Result<void>` specialization) lives in `logger` but is logically shared. Shape for `C++11`:
  * `bool ok; T value; std::string error; static Result success(T v); static Result failure(std::string e);` for non-void, and `bool ok; std::string error;` for `void`. No exceptions thrown by `Result` itself; callers check `ok`. `writeFile`/`readFile`/`parse` return `Result`. This keeps `file-util` and later `runtime-linux` on one model.

* **Build seam**: Top-level `CMakeLists.txt` is thin — Linux gate (`if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux") message(FATAL_ERROR "ffbird requires Linux")`), `enable_language(C CXX ASM)`, `find_program(GIT_EXEC)`, corrected `git_commit_hash`, `string(TIMESTAMP)`, `add_subdirectory(logger)`, `file-util`, `argparser`, `anticrash`, `platform-linux`, `runtime-linux`, `libc-shim`, `libgles-shim`, `libegl-shim`, `game`, plus `ext` placeholder and conditional `natives` via `ExternalProject_Add` or `add_subdirectory` with toolchain.

* **`CMake/` modules**: `Options.cmake` (`FFBIRD_BUILD_TESTS`, `FFBIRD_WERROR`), `Deps.cmake` (`Threads REQUIRED`, `GTest` optional), `CWarnings.cmake` (`flbird_enable_warnings(target)` with `-Wall -Wextra -Wpedantic -Wconversion -Wshadow` and GCC extras). `cmake_minimum_required` is set to the smallest version that covers actually used commands (e.g., `3.5` for `target_include_directories` with `INTERFACE`); the file header comments the rationale rather than using a `3.0...4.0` range.

* **`logger` seam (highest useful)**: Public `logger/log.h` is the only seam; tests assert observable behaviour: level filter, `setLogFile` creates dirs + appends, thread-safety (2 threads × 1000), `to_string(Level)`. Implementation uses `std::mutex` + `lock_guard` + `vsnprintf` 4096B + `strftime` + `ofstream`.

* **`file-util` seam**: Public `file-util/file_util.h` and `env_path_util.h` are the seams. Behaviour: `getParent` trims trailing slashes, `exists` via `access(F_OK)`, `isDirectory` via `stat`/`S_ISDIR`, `mkdirRecursive` returns `Result<void>` (or throws only if we decide via ADR — default is `Result`), `readFile`/`writeFile` use `open`/`lseek`/`read`/`write` loops. `EnvPathUtil::getAppDir` uses `readlink("/proc/self/exe")` + `strrchr` dirname, fallback for mac `#ifdef` that `#error`s per Linux-only rule. Optional `HAVE_LOGGER` link.

* **`argparser` seam**: Header-only, highest seam is `ArgParser::parse` + `Arg<T>::get()`. Behaviour: `Arg<T>` registers handler in constructor via `ArgParser::addArg`; `parse` iterates `ArgList` (wraps `argc/argv`), looks up `unordered_map<string, handler>`, throws `invalid_argument` internally which `parse` converts to `Result<void>::failure`. `printHelp` gathers `help_entry{name, shortname, desc}`. Header-only means no `src/` build artifact.

* **`anticrash` seam**: Public `anticrash/handler.h` with `install(path) -> bool` + `uninstall()`. Behaviour: `install` stores old `sigaction` for six signals, `sigaction` with `sa_sigaction` or `sa_handler` depending on platform, `backtrace` 25 + `backtrace_symbols` + `__cxa_demangle` + `dladdr` + detached 1s hung guard thread, `_Exit`. `uninstall` restores. Signal handler itself is minimal async-signal work via `write(2)`.

* **Stub modules seam**: Each stub's public header is the seam: it must be includable on Linux and `#error` otherwise. Build proves `add_library(STUB STATIC src/stub.cpp)` works. No behaviour to test beyond include + build.

* **`natives/print_test` seam**: Public C export `extern "C" void print_test_hello();` The seam is that `dlopen` + `dlsym("print_test_hello")` on the host (later, when `runtime-linux` implements a loader) would resolve — for now the seam is the artefact exists. Implementation: one `print_test.cpp` calling `__android_log_print(ANDROID_LOG_INFO, "print_test", "hello")`, linked to NDK `log`.

* **`ext/` seam**: `ext/README.md` is the only file; its presence is the contract that future `sdl3` vendoring goes under `ext/`.

* **Formatting/lint seam**: `.clang-format` (`BasedOnStyle: Google`, `ColumnLimit: 100`, `IndentWidth: 4`, `SortIncludes: true`) and `.clang-tidy` (`checks: clang-*,modernize-*,readability-*,performance-*,bugprone-*`) — both in repo root.

* **Git hash seam**: `git_commit_hash(DIR, OUT)` calls `git log -1 --format=%h` (or `rev-parse --short HEAD`) with `WORKING_DIRECTORY` and `OUTPUT_STRIP_TRAILING_WHITESPACE` + `RESULT_VARIABLE` check, else `unknown`.

## Testing Decisions

What makes a good test here: assert external behaviour through public headers, not private helpers. One include path is the seam; tests link against the built library (or header-only) and observe return values, files created, or death.

* **Which modules are tested now**: `logger`, `file-util`, `argparser`, `anticrash`, plus a build-smoke for every stub (`platform-linux`, `runtime-linux`, `libc-shim`, `libgles-shim`, `libegl-shim`, `game`) that just checks the library builds and its header is includable. `natives/print_test` is tested by "does `libprint_test.so` exist and `nm -D` shows `print_test_hello` / `__android_log_print` reference" when `FFBIRD_BUILD_NATIVES=ON`.

* **Prior art in this codebase**: `mcpelauncher-manifest` has `cll-telemetry/test`, `epoll-shim/test/epoll-test.c`, and `libjnivm`/`simple-ipc` tests — all GTest/`gtest` death-test style. `ffbird` will follow that: GTest 1.18 (`/usr/include/gtest`, `pkg-config gtest` 1.18) via `find_package(GTest)` when `FFBIRD_BUILD_TESTS=ON`, `enable_testing()`, `add_test(NAME ...)`.

* **Concrete test intents**:
  * `logger`: level filter (`setMinLevel(INFO)` hides `DEBUG`), `setLogFile` creates `a/b/log.txt`, 2-thread 1000-write no interleaving (check line count 2000), `to_string`.
  * `file-util`: `getParent("/a/b/") == "/a"`, `exists`/`isDirectory`, `mkdirRecursive("a/b/c")`, `writeFile`+`readFile` round-trip, missing `readFile("/nope") -> !ok`, `getAppDir()` non-empty, `getDataHome` respects `XDG_DATA_HOME`, `findInPath("sh")`.
  * `argparser`: `Arg<string>`, `Arg<int>`, `Arg<bool>` parsing `true/on/yes`, missing value → `!ok`, unknown flag → `!ok`, `--help` prints table.
  * `anticrash`: death test `install` + `raise(SIGSEGV)` → log contains `Backtrace`, `uninstall` restores (second `raise` does not log twice).

* **Build tests**: `cmake --preset debug -DFFBIRD_BUILD_TESTS=ON && cmake --build --preset debug && ctest --preset debug` is the gate; `ci` preset adds `-Werror` via `flbird_enable_warnings`.

## Out of Scope

* Any `platform` windowing (`SDL3`, `GLFW`, `eglut`, HiDPI `relativeScale`, cursor hacks) — stub only.
* Any `runtime` loader (`dlopen`/`dlsym`/`dlclose` `Result` wrapper, `get_library_base`) — stub only.
* Any `libc-shim`/`libgles-shim`/`libegl-shim` Bionic forwarding (`mmap_flags→MAP_*`, `clock_type→CLOCK_MONOTONIC`, `__android_log_print` forwarding, `libEGL.so.1` `dlopen`) — stubs only.
* Any `game/` bridge (Flappy Bird `libflappybird.so` lifecycle, `Asset` 4-path resolve is `file-util` future, not now).
* Any `android-support-headers` vendoring or `libjnivm`/`hybris` linker reuse — undecided placeholders.
* Any `ANativeActivity` 16 callbacks, `AConfiguration` 64B, `android_app` 0x130, `ALooper` `poll`, `AInputQueue`, `ANativeWindow` — not designed yet (see `docs/research/*` for lessons, deliberately not carried into this spec).
* Any `Assets/` or `APKs/` bundling, `lib/*.so` commit, or host `dlopen` test of `libprint_test.so` (that test belongs to a future `runtime-linux` loader ticket, not this foundations spec).
* Any `client/` executable — removed.

## Further Notes

* Research inputs: `docs/research/ffbird_cool.md` (Bionic ABI sizes, 4-path asset, version-script shims) and `docs/research/mcpelauncher-manifest.md` (30 submodules, `logger` printf, `FileUtil`/`EnvPathUtil`, `arg-parser` `arg<T>` template, `mcpelauncher-linker` hybris, `game-window` 3-backend, `crash_handler` backtrace). Both are **lessons, not specs** per grill.
* System NDK is `/home/clickpaw/Android/Sdk/ndk/30.0.16138531` (`android.toolchain.cmake` at `build/cmake/android.toolchain.cmake`, prebuilts `aarch64-linux-android21-clang` etc., platform `android-37.0`). Top-level should allow `ANDROID_NDK` env override and not require NDK for host builds.
* Current skeleton still has `client/CMakeLists.txt` 0B and old `CMakeLists.txt` `enable_language(C CXX ASM)` with `IS64_BIT` + broken `git_commit_hash` — this spec expects those to be fixed in Phase 0 work (see Implementation Decisions).
* Grill history: Round 1 (vision/boundaries) + Round 2 (foundations vs runtime, `Result`, stub semantics) + NDK discovery — recorded in `docs/plan/Plan.md` grill header.

