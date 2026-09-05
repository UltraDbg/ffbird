# ffbird — Stronger Base Plan (foundations → runtime)

> Status: draft — step-by-step build plan derived from `docs/research/ffbird_cool.md`.
> Reference: `ffbird_cool` (working runtime), `ffbird_old`.
> Goal: rebuild `ffbird` as more organized, more powerful base, from foundations upward. Each phase is shippable and testable; no phase jumps ahead.

---

## Principles (stronger base)

1. **One error model eventually**: `ffbird_cool` has `Result<T>` + `std::expected<T,IOError>`. New base picks `std::expected` (C++23) for new code, keeps `Result` only as compat shim until migration done. No new `Result` API.
2. **Small libs, clear deps**: skeleton's split (`logger`, `file-util`, `argparser`, `anticrash`, `client`) is kept, but mapped cleanly to `ImHelper` lessons: `logger` has no deps; `file-util` may link `logger`; `argparser` header-only; `anticrash` links nothing (or `logger` optionally). No cycles.
3. **ABI fidelity**: Android structs are width-exact (`static_assert`). Keep host vs Bionic duplication explicit; don't include NDK headers in host builds.
4. **TDD + presets from day 1**: `debug`/`release`/`ci` presets, `-Werror` in CI, `ctest` wired per phase.
5. **No proprietary blobs in repo**: `libflappybird.so` + `Assets/` fetched at runtime or via `XDG` — not committed.

---

## Phase 0 — Toolkit & Build (done skeleton, now harden)

**Goal**: build is green on empty skeleton, presets + warnings + deps work.

- [x] `CMakeLists.txt` top-level (C++23, `enable_language C CXX ASM`, `-fno-delete-null-pointer-checks`) — exists but needs cleanup (git hash function is broken: `execute_process COMMAND ${GIT_EXEC} WORKING_DIRECTORY` missing args).
- [ ] `CMake/` modules: `Options.cmake`, `Deps.cmake` (SDL3/OpenGL/Threads), `CWarnings.cmake` (`flbird_enable_warnings`) — port from `ffbird_cool/CMake/` (Source: `CMake/*.cmake`).
- [ ] `CMakePresets.json` (`debug`/`release`/`ci`) — port from `cool`.
- [ ] `.clang-format` / `.clang-tidy` — copy from `cool`.
- [ ] Wire `compile_commands.json` symlink and `ctest` skeleton.
- [ ] Fix `add_subdirectory` ordering: `logger` → `file-util` → `argparser` → `anticrash` → `client` already correct; add `ext` placeholder.

**Exit**: `cmake --preset debug && cmake --build --preset debug` succeeds with no source (libs build as INTERFACE/empty).

References: `ffbird_cool/CMakeLists.txt`, `CMake/*.cmake`, `CMakePresets.json`.

---

## Phase 1 — `logger` (ImHelper::Log)

**Goal**: thread-safe logging with levels, colors, file output, `source_location`.

Port from `ffbird_cool`:
- Headers: `Include/ImHelper/Log.hpp` → new `logger/include/logger/log.h` (keep `ImHelper::Logger` or rename to `ffbird::Logger`; decide ADR).
- Impl: `Source/ImHelper/Log.cpp` → `logger/src/log.cpp`.
- Error model: keep `LogLevel` enum + `LogLevelToString`.

**Tasks**
1. Create `logger/include/logger/log.h` — `Logger` singleton, `SetMinLevel`, `SetConsoleOutput/ColorOutput`, `SetLogFile`, `DetectColorSupport`, `Log`/`LogFmt`, helpers `LogDebug/Info/Warning/Error` + macros `LOG_DEBUG` etc. (Source: `Include/ImHelper/Log.hpp`).
2. Create `logger/src/log.cpp` — mutex, timestamp via `std::format`, ANSI colors, `ofstream` file stream (Source: `Source/ImHelper/Log.cpp`).
3. Add `Result` compat header if kept: `logger/include/logger/result.h` or shared `include/result.h`.
4. Tests: `logger/tests/` with GTest — level filter, file output, thread safety (2 threads logging 1000 entries).
5. Install rules + alias `logger::logger`.

**Exit**: `ctest -R logger` passes; `log` shim can later link this.

References: `Include/ImHelper/Log.hpp`, `Source/ImHelper/Log.cpp`, `Include/ImHelper/Result.hpp`.

---

## Phase 2 — `file-util` (ImHelper::Io)

**Goal**: file I/O with base-path, `std::expected` errors, cross-platform paths.

Port from `ffbird_cool`:
- Headers: `Include/ImHelper/Io.hpp` → `file-util/include/file-util/file-util.h` (+ `envpath-util.h` already referenced in `file-util/CMakeLists.txt`).
- Impl: `Source/ImHelper/Io.cpp` → `file-util/src/file-util.cpp` + `envpath-util.cpp`.

**Tasks**
1. `enum IOError` + `IOErrorToString`, `using Result<T>=expected<T,IOError>`, `ByteBuffer` (Source: `Include/ImHelper/Io.hpp`).
2. `class FileSystem` singleton: `SetBasePath`, `GetBasePath`, `ResolvePath`, `Exists/IsFile/IsDirectory`, `GetFileSize`, `CreateDirectories`, `GetExecutablePath()`, `runtime_data_dir()` (XDG). (Source: `Include/ImHelper/Io.hpp`, `Source/ImHelper/Io.cpp`).
3. Free fns: `ReadBytes/ReadString/ReadBytesLimited/ReadBytesRange/WriteBytes/WriteString`, `InitializeWithExecutablePath`, `ResolvePath` helper for assets. Dedupe candidate-path logic here for later reuse (see Phase 5/6).
4. Keep independence: `file-util` links `logger` only if `HAVE_LOGGER` (already in `file-util/CMakeLists.txt`) — log on errors optionally.
5. Tests: read/write round-trip, base-path resolution, missing file → `FileNotFound`, `ReadBytesRange` edge, XDG detection.

**Exit**: `ctest -R file-util` passes; `AAssetManager` stub can use this.

References: `Include/ImHelper/Io.hpp`, `Source/ImHelper/Io.cpp`.

---

## Phase 3 — `argparser` (new, header-only)

**Goal**: replace inline arg parsing in `cool/Source/main.cpp` with reusable parser.

**Tasks**
1. `argparser/include/argparser/argparser.h` — header-only, C++23, `std::expected` errors, supports `--help`, `--data-dir <dir>`, `--log-file <file>`, `--verbose`, unknown-arg error. API sketch:
   ```cpp
   struct Args { string dataDir; string logFile; bool verbose=false; bool help=false; };
   expected<Args, string> parse(int argc, char** argv);
   string help_text(string_view prog);
   ```
2. Tests: known flags, missing values, unknown arg, `--help`.

**Exit**: `ctest -R argparser` passes; `client/main.cpp` will use it.

References: `ffbird_cool/Source/main.cpp` arg loop (lines 20-45).

---

## Phase 4 — `anticrash` (new)

**Goal**: host crash handler (signals) that logs stacktrace, does not depend on Bionic.

**Tasks**
1. `anticrash/include/anticrash/anticrash.h` — `install()`, `uninstall()`, `set_log_file()`.
2. `anticrash/src/anticrash.cpp` — `sigaction` for `SIGSEGV/SIGABRT/SIGILL/SIGFPE/SIGBUS`, `backtrace` + `backtrace_symbols_fd`, writes to `Logger` if present (weak link), async-signal-safe path.
3. Optional: Linux `sigaltstack`.
4. Tests: manual trigger in Debug (`raise(SIGSEGV)` in test binary, check log file contains `backtrace`).

**Exit**: crash handler installs without breaking other signals; proven by manual test.

References: no counterpart in `cool` — greenfield.

---

## Phase 5 — `Platform` (Linux/SDL3)

**Goal**: abstract OS/windowing for later `ANativeWindow` bridging.

Create `platform/` (or keep `client` subdir? decide ADR).

**Tasks**
1. `include/platform/platform.h` — `namespace Platform { Result<void> Initialize(); void Shutdown() noexcept; string Name(); bool IsInitialized() noexcept; SDL_Window* GetWindow() noexcept; }` (Source: `Include/Platform/Platform.hpp`).
2. `src/linux/platform_linux.cpp` — `SDL_Init`, `SDL_CreateWindow(720,1280, OPENGL|RESIZABLE)`, `SDL_ShowWindow`, store `SDL_Window*` singleton (Source: `Source/Platform/Linux/PlatformLinux.cpp` — use `_old` windowing variant, not `_cool` minimal).
3. `CMakeLists.txt` — optional SDL3 (`find_package(SDL3 CONFIG)` fallback `pkg-config`, stub if not found — keep `_cool` pattern).
4. Tests: `Initialize` twice → failure, `Shutdown` idempotent, `GetWindow` non-null after init (if SDL available, else stub).

**Exit**: `Platform::Initialize()` works headless (stub) and with SDL3 (window).

References: `Include/Platform/Platform.hpp`, `Source/Platform/Linux/PlatformLinux.cpp`, `Source/Platform/CMakeLists.txt`, `CMake/Deps.cmake`.

---

## Phase 6 — `Runtime` (NativeLoader + Runtime)

**Goal**: load `libflappybird.so` reliably.

Create `runtime/` lib.

**Tasks**
1. `include/runtime/nativeloader.h` — `class NativeLibrary { Load, Unload, IsLoaded, Path, Handle, Symbol }` move-only, `Result<void>` / `Result<void*>` (Source: `Include/Runtime/NativeLoader.hpp`).
2. `src/nativeloader.cpp` — `dlopen(RTLD_NOW)`, `dlsym` + `dlerror` checks, `std::format` errors (Source: `Source/Runtime/NativeLoader.cpp`).
3. `include/runtime/runtime.h` — `struct RuntimeConfig { string nativelib, data_dir; }`, `Result<void> Initialize`, `Shutdown`, `IsInitialized`, `Version` (Source: `Include/Runtime/Runtime.hpp`).
4. `src/runtime.cpp` — checks `data_dir`, `Exists`, `Platform::Initialize` if needed, loads lib, mutex `s_initialized` (Source: `Source/Runtime/Runtime.cpp`).
5. Tests: load non-existent → failure, `Symbol` empty name → failure, double `Initialize` → failure, `IsInitialized` gate.

**Exit**: can `dlopen` a dummy `.so` in tests.

References: `Include/Runtime/NativeLoader.hpp`, `Source/Runtime/NativeLoader.cpp`, `Include/Runtime/Runtime.hpp`, `Source/Runtime/Runtime.cpp`, `Source/Runtime/CMakeLists.txt`.

---

## Phase 7 — `Runtime/Android` Compat (host side)

**Goal**: host structs that `libflappybird.so` will consume via `ANativeActivity_onCreate`.

Create `runtime/android/` (or `runtime/compat/`).

**Tasks** (order matters, ABI exact):
1. `AConfiguration` — 64 bytes, `static_assert`, `CreateConfiguration` (calloc), `CopyConfigurationFromAssetManager` (en_US, 320dpi, sdk 30) (Source: `Include/Runtime/Android/Compat/AConfiguration.hpp` + cpp).
2. `AAssetManager` + `AAsset` + `AAssetDir` — `CreateAssetManager(basePath)`, `OpenAsset` with 4 candidate paths + `Io::ResolvePath`, `ReadBytes`, `CloseAsset`, `GetBuffer/Length/Remaining`, `Seek/Read`, `OpenAssetDir` (Source: `Include/Runtime/Android/Compat/AAssetManager.hpp`).
3. `ANativeWindow` — `CreateNativeWindow(w,h,hostWindow)` fallback to `Platform::GetWindow()`, refcount (Source: `ANativeWindow`).
4. `ANativeActivity` — `ANativeActivityCallbacks` 16 ptrs + `ANativeActivity`, `CreateActivity` (calloc callbacks, strdup paths), `DestroyActivity` (Source: `ANativeActivity.hpp/cpp`).
5. `ALooper` — `PrepareLooper`, `Add/RemoveLooperFd`, `PollLooperOnce` via `poll()` (Source: `ALooper.hpp/cpp`).
6. `AInputQueue` + `AInputEvent` — `CreateInputQueue`, `AttachToLooper`, `Has/Get/Finish`, Motion/Key getters (Source: `AInputQueue.hpp/cpp`).
7. `NativeAppGlue` — `android_poll_source`, `android_app` 0x130 + enums `APP_CMD_*` (Source: `NativeAppGlue.hpp`).

**Exit**: each struct size `static_assert` passes; unit tests for asset open/close, looper poll with pipe fd.

References: `Include/Runtime/Android/Compat/*.hpp`, `Source/Runtime/Android/Compat/*.cpp`.

---

## Phase 8 — Shims (Bionic)

**Goal**: provide Bionic `DT_NEEDED` libs so hybris linker resolves `libflappybird.so`.

Create `runtime/shims/` with helper `add_bionic_shim`.

**Tasks** (Source: `Source/Runtime/Shims/CMakeLists.txt` + each `Lib*/`):
1. Helper `add_bionic_shim` — `SHARED`, `OUTPUT_NAME`, `SOVERSION ""`, `VISIBILITY hidden`, `version-script`, `RPATH $ORIGIN`.
2. `log` shim — `__android_log_print/vprint/buf_print` → `Logger` (Source: `LibLog/LibLog.cpp/.version`).
3. `android` shim — `AAssetManager_open`, `AConfiguration_*`, `ALooper_*`, `ANativeWindow_*` etc. (Source: `LibAndroid/LibAndroid.cpp/.version` — 1504B version script).
4. `EGL` shim — forward to host `libEGL.so.1` via `dlopen` once, each `egl*` via `dlsym` (Source: `LibEGL/LibEGL.cpp` — X11 `Display` sharing comment).
5. `GLESv2` shim — forward to `libGLESv2.so.2` (Source: `LibGLESv2/LibGLESv2.cpp` 7567B, `.version` 939B).
6. `OpenSLES` shim — stubs `SL_IID_*` + `slCreateEngine` → `SUCCESS` (Source: `LibOpenSLES/`).
7. `c`/`m`/`dl` shims — `libc.so.6`/`libm.so.6`/`libdl.so.2` forwarding with `__asm__(".symver ...")` (Source: `LibC/`, `LibM/`, `LibDL/`).
8. Custom target `shims` depends on all.

**Exit**: `readelf --dyn-syms build/lib/liblog.so | grep __android_log_print` shows exported; `ldd` on dummy Bionic binary resolves.

References: `Source/Runtime/Shims/CMakeLists.txt`, `Lib*/*`.

---

## Phase 9 — `Game` bridge

**Goal**: drive `libflappybird.so` lifecycle P0→P1.

Create `game/` lib (or `client/game/`).

**Tasks**
1. `include/game/asset_loader.h` — `LoadAssetBytes/String(basePath, filename)` with same 4-candidate logic via `file-util` (Source: `Include/Game/AssetLoader.hpp`, `Source/Game/AssetLoader.cpp`).
2. `include/game/game.h` — `struct GameConfig {dataDir, nativeLibPath, windowWidth, windowHeight, hostWindow}`, `class FlappyBirdGame { Initialize, Shutdown, IsInitialized, GetApp/Activity/Window, CreateWindow, DestroyWindow, GainFocus/LostFocus, PollAndProcess, IsGameInitialized, SetMainLoopCallback }` (Source: `Include/Game/Game.hpp`).
3. `src/game.cpp` — load lib, `Symbol("ANativeActivity_onCreate")`, `CreateAssetManager`, `CreateActivity`, call `onCreate`, wait 50×20ms for `activity->instance`, fallback `g_App` symbol, `CreateWindow` → `onNativeWindowCreated` or `pendingWindow`, `onStart/onResume`, `IsGameInitialized` via `g_Initialized` symbol (Source: `Source/Game/Game.cpp`).

**Exit**: `Game::Initialize` reaches `[P0] android_app created` against real `libflappybird.so` (if present) or mock `.so` in tests.

References: `Include/Game/Game.hpp`, `Source/Game/Game.cpp`, `Source/Game/CMakeLists.txt`.

---

## Phase 10 — `client` entrypoint

**Goal**: `flbird` executable wiring everything.

**Tasks**
1. `client/src/main.cpp` — arg parsing via `argparser`, `Logger::DetectColorSupport`, resolve data dir (`exeDir/Assets` → `XDG` → `InitializeWithExecutablePath`), `Platform::Initialize`, resolve `libPath = dataDir/libflappybird.so`, load+ `ANativeActivity_onCreate` direct (or via `Game` class — choose one, ADR), window creation, main loop with `ALooper_pollOnce` + input pump (Source: `ffbird_cool/Source/main.cpp` 7920B).
2. `client/CMakeLists.txt` — `add_executable(flbird main.cpp)` links `Game::Game`, `Runtime::Runtime`, `Platform::Platform`, `logger`, `file-util`, `argparser`, `Threads`, `SDL3`, `OpenGL`, `jnivm` family if needed (`libjnivm` vendored in `ext/`).
3. Post-build `copy_to_root` to `${CMAKE_SOURCE_DIR}/flbird` (Source: `Source/CMakeLists.txt` copy target).
4. Install rules: bin to `CMAKE_INSTALL_BINDIR`, Assets to `DATADIR/flbird` optional.

**Exit**: `./build/debug/flbird --help` prints usage; `./build/debug/flbird --data-dir Assets` logs `[P0]` to running.

References: `ffbird_cool/Source/main.cpp`, `Source/CMakeLists.txt`.

---

## Phase 11 — Polish & packaging

**Goal**: quality bar for stronger base.

- [ ] Tests aggregated: `ctest --preset debug`, CI `ci` preset with `-Werror`.
- [ ] `ext/libjnivm` vendored with `CXX_STANDARD 14` + `-w` (Source: top-level `CMakeLists.txt` `libjnivm` handling).
- [ ] `Assets/` handling: not committed, fetched via `runtime_data_dir()`, `Assets/libflappybird.so` lookup fallback.
- [ ] Docs: `README.md` with build table, layout diagram, helper examples (Source: `ffbird_cool/Readme.md`).
- [ ] Install: `ImHelperTargets` export, `GNUInstallDirs`.

---

## Dependency edges (for ticket ordering)

```
Phase0 → Phase1 (logger)
Phase1 → Phase2 (file-util needs logger optionally)
Phase1+2 → Phase3 (argparser independent but benefits from Logger)
Phase1 → Phase4 (anticrash links logger optionally)
Phase1+2 → Phase5 (Platform links logger)
Phase5 → Phase6 (Runtime needs Platform)
Phase6 → Phase7 (Compat needs Runtime/Io)
Phase7 → Phase8 (Shims need Compat ABI + Io/Log)
Phase7 → Phase9 (Game needs Compat + NativeLoader)
Phase9 → Phase10 (client needs Game + Platform + Runtime)
Phase10 → Phase11
```

---

## Risks / ADRs to record

- **Error model**: ADR — `Result<T>` vs `std::expected` — choose.
- **Module names**: `logger`+`file-util` vs `ImHelper` single lib — keep split or reunify? Skeleton's split is opportunity for stronger seam but needs shared `Result` header (`Include/Result.hpp` duplication in `cool`).
- **Platform windowing**: `GetWindow()` presence — ADR to guarantee.
- **Host vs Shim duplication**: keep duplicated struct defs or extract `abi/` header shared between `runtime/android` and `runtime/shims` — tradeoff NDK clash vs drift.
- **libjnivm**: needed? `cool` links `jnivm fake-jni baron` even though `libflappybird.so` is not Java — check if required at runtime or only for future Minecraft parity.

---

## Next action

1. Commit this plan + research: `git add docs/research/ffbird_cool.md docs/plan/Plan.md && git commit`
2. Run `grill-with-docs` to sharpen naming (e.g., `logger` vs `ImHelper::Log`, `file-util` paths) and record ADRs.
3. `to-spec` → `to-tickets` per phase above, each ticket declaring blocking edges.

