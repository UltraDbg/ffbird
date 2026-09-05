# ffbird — Rewrite Plan (foundations first)

> Status: draft — rewrite, not port.
> Constraints: `.h` only, `C++11` mandatory, small modules (each does one job), no copy from `ffbird_cool` / `mcpelauncher-manifest` (module ideas only if explicitly allowed — see research docs for lessons, not for structure).
> Way of work: `natives/` are test fixtures built with Android NDK; `ffbird` itself is Linux-only and fails to compile on other OS. `client/` and `ext/` are gone — `ext/` is reserved for future external CMake packages (e.g. sdl3).
> Grill: 2 rounds + NDK discovery (`/home/clickpaw/Android/Sdk/ndk/30.0.16138531` r30-beta3, platform `android-37.0`, CMake 4.4.3, gcc 16.2.1 / clang 22.1.8).

---

## 1. What we are not doing yet

We stop before any Android/runtime/JVM work. No `ANativeActivity`, no `AAssetManager`, no `ALooper`, no EGL/GLES forwarding, no `libjnivm`/`hybris` linker. Those concepts are **undecided placeholders** — we will design them when we reach them, not copy `ffbird_cool` or `mcpelauncher-manifest`.

That means in this plan we only harden the foundations: build env, logging, file I/O, arg parsing, crash handling, and empty stubs that prove the module layout. The first milestone (load `libprint_test.so` and see `__android_log_print`) is tracked but **not chased at the cost of foundations** — it comes last in this document.

---

## 2. Module map (top-level dirs are the seam)

```
ffbird/
├── CMakeLists.txt            # thin, Linux-only gate, adds subdirs, calls natives/ separately
├── CMake/                    # Options.cmake, Deps.cmake, CWarnings.cmake  (cmake min picked per function used)
├── CMakePresets.json         # debug / release / ci (-Werror)
├── .clang-format / .clang-tidy  # defaults proposed by us (see Phase 0)
├── ext/README.md             # reserved for future external packages (e.g. sdl3) — not used now
├── natives/                  # separate CMake project, built with NDK toolchain (see Phase 6)
│   └── print_test/           # → libprint_test.so, one C file calling __android_log_print
├── logger/                   # 1 job: thread-safe logging
├── file-util/                # 1 job: file + env path helpers (FileUtil + EnvPathUtil)
├── argparser/                # 1 job: header-only arg parsing
├── anticrash/                # 1 job: host signal handler (thread-safe, stack trace)
├── platform-linux/           # stub at this stage — compiles only on Linux, else #error
├── runtime-linux/            # stub at this stage — same guard
├── libc-shim/                # stub per-shim naming convention (top-level, one per .so)
├── libgles-shim/             # stub (example name, pattern: lib*.so → lib*-shim/)
├── libegl-shim/              # stub
└── game/                     # one game impl stub (not Android-generic code) — empty now
```

**Rules**:

* Only `.h` files for headers (C++ classes allowed inside, with `#ifndef` guards or `#pragma once` + guards — we pick one standard and enforce via `clang-format`). No `.hpp` anywhere.
* `C++11` everywhere: `-std=c++11`, no `std::expected`, no `string_view`, no `optional`. Errors via `Result<T>` (see Phase 0).
* Each `CMakeLists.txt` is small, declares one `add_library` (`STATIC` or `INTERFACE`) and its direct `PUBLIC` include + `PRIVATE` link. Dependencies point one way: `logger` ← `file-util` ← `platform-linux` family (stubs for now). `argparser` and `anticrash` are leaves.
* Every module has `include/<module>/<header>.h` + `src/*.cpp` + optional `tests/` with GTest (`enable_testing`, `add_test`).
* Non-Linux → `#error "ffbird requires Linux"` in every public `.h` (or at least in top `CMakeLists.txt` check `if(NOT LINUX) message(FATAL_ERROR)` plus header guard).

---

## 3. Phase 0 — Build environment (must be green before any feature)

**Goal**: `cmake --preset debug && cmake --build --preset debug` + `ctest` works on empty stubs.

Tasks:

* Fix top-level `CMakeLists.txt`: remove `client/` `add_subdirectory`, keep `logger/file-util/argparser/anticrash/platform-linux/runtime-linux/libc-shim/.../game` + `add_subdirectory(ext)` placeholder + separate `add_subdirectory(natives)` guard (see Phase 6). Fix `git_commit_hash` (currently `execute_process(COMMAND ${GIT_EXEC} WORKING_DIRECTORY ...)` missing args — needs `git rev-parse --short HEAD` or `log -1 --format=%h`). Set `CMAKE_CXX_STANDARD 11`, `REQUIRED ON`, `EXTENSIONS OFF`, `POSITION_INDEPENDENT_CODE ON`, `EXPORT_COMPILE_COMMANDS ON`. Add `if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux") message(FATAL_ERROR "ffbird requires Linux")`.
* `CMake/min_version` rationale: pick `cmake_minimum_required(VERSION 3.8)` if we use `try_compile` with `CXX_STANDARD` etc., but we will state the exact version in the file header as "minimum 3.5 because we only use `add_library INTERFACE`, `target_include_directories`, `find_package`". We will not hard-code `3.0...4.0` range — pick the smallest that covers used commands (inspect after writing CMakeLists).
* Create `CMake/Options.cmake` (options: `FFBIRD_BUILD_TESTS ON/OFF`, `FFBIRD_WERROR ON in ci`), `CMake/Deps.cmake` (only `Threads REQUIRED` + optional `GTest` via `find_package(GTest)` for `tests/`), `CMake/CWarnings.cmake` (`flbird_enable_warnings(target)` with `-Wall -Wextra -Wpedantic -Wconversion -Wshadow` + GCC extras `-Wmisleading-indentation -Wduplicated-cond`), plus `check_cxx_source_compiles` for atomics if needed.
* `CMakePresets.json` `debug`/`release`/`ci`.
* `.clang-format` and `.clang-tidy` defaults — we propose: `BasedOnStyle: Google, ColumnLimit: 100, IndentWidth: 4, SortIncludes: true, AllowShortFunctionsOnASingleLine: None`; `clang-tidy: checks='clang-*,modernize-*,readability-*,performance-*,bugprone-*', WarningsAsErrors: '' in ci` — to be tweaked after first file lands.
* `ext/README.md`: "Reserved for external CMake packages (e.g. sdl3). Do not put project sources here."
* Wire `enable_testing()` + `include(CTest)` at top-level when `FFBIRD_BUILD_TESTS=ON`.

Exit: empty `logger` etc. build as `STATIC` with one `.cpp` or `INTERFACE`; `ctest` shows 0 tests but passes.

---

## 4. Phase 1 — `logger` (first real code, thread-safe)

**Job**: one place to write `tag: message` with level, optional file sink. Thread-safe, no global `printf` races.

Design (C++11, `.h` only):

* `logger/include/logger/log.h` — `#ifndef LOGGER_LOG_H / #define`, `enum class Level { TRACE, DEBUG, INFO, WARN, ERROR }`, `const char* to_string(Level)`, `class Logger { public: void setMinLevel(Level); void addSink(...) maybe; bool setLogFile(const std::string& path); void log(Level, const char* tag, const char* fmt, ...) // varargs or string overload; private: Level min_; std::mutex m_; std::ofstream file_; }` + free helpers `void log_debug(...)` or macro `LOG_INFO(tag, fmt, ...)`. Decision ADR: `Logger` is **instance-based** (caller owns `Logger logger;`) plus optional `Logger::global()` accessor for shims — not a hard singleton. This keeps tests isolated.
* `logger/src/log.cpp` — `vsnprintf` 4096B buffer, trim trailing `\r\n`, `strftime("%H:%M:%S", localtime_r)`, lock `mutex`, write to `stdout` + `file_` if open, `fflush`. Use `std::mutex` + `lock_guard`. Keep `DetectColorSupport()` if we add ANSI later, but for foundation just plain text — color is later optimization.
* `logger/include/logger/result.h` — defines `template<typename T> struct Result { bool ok; T value; std::string error; static Result success(T); static Result failure(std::string); }` and `Result<void>` specialization. This file is **shared** so `file-util` can reuse same `Result` type. C++11 clean: no `std::expected`, no `noexcept` beyond what C++11 allows, copies are explicit.
* Tests `logger/tests/` GTest: level filter (`setMinLevel`), concurrent 2×1000 logs (TSan if available), `setLogFile` creates parent dirs, `to_string`.

Deps: none. Provides `logger::logger` alias.

---

## 5. Phase 2 — `file-util` (FileUtil + EnvPathUtil, both in one lib)

**Job**: small file + path helpers, reused by every later module. Thread-safe where it touches the filesystem.

Design:

* `file-util/include/file-util/file_util.h` — `#ifndef FILE_UTIL_FILE_UTIL_H`, `class FileUtil { public: static std::string getParent(const std::string& path); static bool exists(const std::string& path); static bool isDirectory(const std::string& path); static void mkdirRecursive(const std::string& path); // throws runtime_error, like mcpelauncher but via Result wrapper if we choose static bool readFile(path, std::string& out); static Result<std::string> readFile(const std::string& path); static Result<void> writeFile(const std::string& path, const std::string& data); // you asked writeFile cool — yes }`
  * `readFile` returns `Result<std::string>` with `error` on `open`/`lseek`/`read` failure. Keep `access(F_OK)` + `stat` path similar to `mcpelauncher/file-util` but do not copy code — rewrite from `man open`.
  * `mkdirRecursive` throws or returns `Result<void>` — we pick `Result<void>` to stay consistent with `writeFile`.
* `file-util/include/file-util/env_path_util.h` — `class EnvPathUtil { public: static std::string getAppDir(); // readlink /proc/self/exe + dirname static std::string getWorkingDir(); // getcwd static std::string getHomeDir(); // getenv HOME or getpwuid_r static std::string getDataHome(); // XDG_DATA_HOME or ~/.local/share static bool findInPath(const std::string& what, std::string& out, const char* path=nullptr, const char* cwd=nullptr); }`
* `file-util/src/file_util.cpp` + `env_path_util.cpp` — implement with `access`, `stat`, `mkdir`, `open/read/lseek/close`. Behind `#ifdef HAVE_LOGGER` log errors via `Logger` if target exists (like `mcpelauncher/file-util/CMakeLists.txt` already does `if(TARGET logger) target_link_libraries(file-util logger) compile_def HAVE_LOGGER`). Keep that optional link.
* Thread safety: `readFile`/`writeFile` use local `fd`/`stat`, no shared state — thread-safe by construction. `mkdirRecursive` uses `stat` + `mkdir` loop — safe for concurrent callers (one will succeed, others see `exists`).
* Tests: `getParent` edge (`"/a/b/"`), `exists`/`isDirectory`, `mkdirRecursive` nested, `readFile` missing → `!ok`, `writeFile` round-trip, `getAppDir` non-empty, `getDataHome` respects `XDG_DATA_HOME`.

Deps: optionally `logger` (PRIVATE `HAVE_LOGGER`).

---

## 6. Phase 3 — `argparser` (header-only, `.h` only)

**Job**: parse `argc/argv` into typed values, no `printf` side effects in library (return `Result`).

Design (header-only, C++11):

* `argparser/include/argparser/arg.h` + `arg_parser.h` + `arg_list.h` — but all as `.h` (no `.hpp`). Provide:
  ```cpp
  // arg_list.h
  class ArgList { public: ArgList(int argc, const char** argv); std::string next(); const char* nextOrNull(); bool hasNext() const; };
  // arg_parser.h
  class ArgParser { public: bool parse(int argc, const char** argv); void addArg(const std::string& longName, const std::string& shortName, const std::string& desc, std::function<void(ArgList&)> handler); void printHelp() const; };
  // arg.h
  template<typename T> class Arg { public: Arg(ArgParser& p, const std::string& longName, const std::string& shortName, const std::string& desc, const T& def = T()); const T& get() const; operator const T&() const; private: T value_; };
  // free handleValue overloads for string/int/float/bool/vector
  ```
  Keep the `mcpelauncher/arg-parser` seam idea (template `arg<T>` + handler map) because you said header-only is better, but **do not assume** same short-name handling — we design our own help text format and error return. For rewrite we return `Result<void>` from `parse` instead of `bool + printf("Unknown argument")` — but keep `printHelp` for `--help`.
* Tests: `Arg<string>` with `--data-dir foo`, `Arg<bool>` with `true/on/yes`, missing value → `!ok`, unknown arg → error, `--help` triggers `printHelp`.

Deps: none (INTERFACE).

---

## 7. Phase 4 — `anticrash` (host crash handler)

**Job**: catch host `SIGSEGV/ABRT/FPE/BUS/ILL`, dump stack, never deadlock.

Design:

* `anticrash/include/anticrash/handler.h` — `#ifndef ANTICRASH_HANDLER_H`, `namespace anticrash { bool install(const std::string& logFile); void uninstall(); }`
* `anticrash/src/handler.cpp` — `sigaction` with `SA_SIGINFO` or plain `sa_handler`, `backtrace` 25 + `backtrace_symbols`, `abi::__cxa_demangle`, `linker::dladdr`-style fallback if we later have linker — for now just `dladdr` (host). Detached 1-second hung guard thread like `mcpelauncher-core/crash_handler.cpp`. `_Exit(signal)` after dump. Store old handlers to restore in `uninstall`. `hasCrashed` `std::atomic<bool>`.
* Thread safety: `install` is called once at startup; handler is async-signal-safe only via `write(2)` + `backtrace` (which is not strictly async-safe but matches upstream) — we document the trade-off.
* Tests: death test — fork child, `install`, `raise(SIGSEGV)`, check log contains `Backtrace`.

Deps: `logger` optional, `CMAKE_DL_LIBS` if we use `dladdr`.

---

## 8. Phase 5 — Stubs (every undecided module is an empty build + Linux guard)

**Goal**: prove the module layout without implementing runtime.

For each `platform-linux/`, `runtime-linux/`, `libc-shim/`, `libgles-shim/`, `libegl-shim/`, `game/`:

* `CMakeLists.txt` → `add_library(<name> STATIC src/stub.cpp) PUBLIC include/` with `if(NOT LINUX) message(FATAL_ERROR)` or header `#error "requires Linux"` (see Phase 5.1).
* `include/<module>/stub.h` — single header with `#ifndef ... / #error "platform-linux requires Linux"` + empty namespace + comment `// TODO: implemented in later phase`.
* `src/stub.cpp` — one empty translation unit so `add_library` is not `INTERFACE` where we want a real archive later.
* Example `platform-linux/include/platform_linux/platform.h`:
  ```cpp
  #ifndef PLATFORM_LINUX_PLATFORM_H
  #define PLATFORM_LINUX_PLATFORM_H
  #ifndef __linux__
  #error "platform-linux requires Linux"
  #endif
  namespace platform { // empty stub
  }
  #endif
  ```
* Same for `runtime-linux`, `libc-shim` etc. — top-level names stay `platform-linux`, `runtime-linux`, `libc-shim` (not `platform/linux`).

Tests: CTest checks that `cmake --build` succeeds and `nm -D` would later show symbols — for now just that the stub builds.

---

## 9. Phase 6 — `natives/print_test` (separate NDK project, called from top-level)

**Goal**: `ffbird` top CMake can trigger NDK build of `libprint_test.so` without coupling host build to NDK.

Tasks:

* `natives/CMakeLists.txt` (separate project) — `cmake_minimum_required(VERSION 3.8)` (because we use `ANDROID_ABI` vars), `project(natives)`, `add_library(print_test SHARED print_test/print_test.cpp)` with `target_link_libraries(print_test log)` (NDK `liblog`). Header: one `extern "C" void print_test_hello();` that does `__android_log_print(ANDROID_LOG_INFO, "print_test", "hello")`.
* `natives/print_test/print_test.cpp` — `#include <android/log.h>`, `extern "C" void print_test_hello() { __android_log_print(ANDROID_LOG_INFO, "print_test", "hello"); }`
* `natives/print_test/CMakeLists.txt` — delegates to parent or stands alone with `find_library(log-lib log)`.
* Top-level `ffbird/CMakeLists.txt` calls natives via:
  ```cmake
  option(FFBIRD_BUILD_NATIVES "Build natives with NDK" OFF)
  if(FFBIRD_BUILD_NATIVES)
    if(NOT DEFINED ANDROID_NDK)
      set(ANDROID_NDK "/home/clickpaw/Android/Sdk/ndk/30.0.16138531")
    endif()
    add_subdirectory(natives ${CMAKE_BINARY_DIR}/natives_build
      CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21)
  endif()
  ```
  Or simpler: `add_custom_target(natives_print_test COMMAND ${ANDROID_NDK}/.../cmake ...)` — we pick `ExternalProject_Add` if `ANDROID_ABI` varies.
* Host test later (not in this foundations plan) will `dlopen` the built `libprint_test.so` — but that test lives in `runtime-linux` Phase when `NativeLoader` exists. For now natives just need to **build**.

Discovery (fact): NDK is at `/home/clickpaw/Android/Sdk/ndk/30.0.16138531` (r30-beta3), `android.toolchain.cmake` at `build/cmake/android.toolchain.cmake`, `aarch64-linux-android21-clang` etc. in `toolchains/llvm/prebuilt/linux-x86_64/bin`, platform `android-37.0` available. We will default to `ANDROID_ABI arm64-v8a` + `ANDROID_PLATFORM android-21` (minimum for `__android_log_print`).

Exit: `cmake -DFFBIRD_BUILD_NATIVES=ON -DANDROID_ABI=arm64-v8a .. && cmake --build . --target print_test` produces `libprint_test.so` under `build/natives_build/`.

---

## 10. Testing, formatting, quality

* Every non-stub module has `tests/` with GTest (`find_package(GTest)` when `FFBIRD_BUILD_TESTS=ON`). Host tests run via `ctest --preset debug`.
* Natives have no GTest — they are validated by host `dlopen` test in a later milestone (not this plan).
* `ext/README.md` stays.
* CI preset `ci` sets `FFBIRD_WERROR=ON` → `flbird_enable_warnings` adds `-Werror`.
* No proprietary blobs in repo: `Assets/` and `APKs/` are `.gitignored`, natives are built from source.

---

## 11. Dependency edges (foundations only)

```
Phase0 (build) ─┬─→ logger ─┬─→ file-util ──→ anticrash? (optional log)
                ├─→ argparser (leaf)
                ├─→ anticrash (leaf)
                ├─→ ext (no code)
                └─→ stubs (platform-linux, runtime-linux, libc-shim, game) — all depend on Phase0 only
Phase0 ─→ natives/print_test (separate toolchain, no host dep)
```

Later phases (not this plan) will wire `platform-linux → runtime-linux → libc-shim → game → natives` when we design the loader.

---

## 12. Next action after this plan

1. `git add docs/plan/Plan.md` + `git commit` (this file).
2. Run `to-spec` on this plan if you want `spec.md`, else `to-tickets` directly — tickets under `.scratch/<feature>/issues/` or GitHub issues, starting at Phase 0 and following edges (lowest blocking count first).
3. Implement with TDD per ticket, then `code-review` (Standards: `.h` only, C++11, thread-safe; Spec: `Result<T>`, small modules, Linux-only guard).

