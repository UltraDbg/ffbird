# Research: ffbird_cool — Primary Source Analysis

> Date: 2026-09-05
> Source: `/home/clickpaw/dev/Android/ffbird_cool` (full working runtime)
> Sibling: `ffbird_old` (near-identical, minor lag)
> Purpose: inform rebuild of `ffbird` — stronger, more organized base, step-by-step foundations.

All paths below are relative to `ffbird_cool/` unless stated. Citations use `Source:` + file path.

---

## 1. Top-level layout

```
ffbird_cool/
├── CMakeLists.txt            Source: CMakeLists.txt
├── CMakePresets.json         Source: CMakePresets.json
├── CMake/ Options|Deps|CWarnings  Source: CMake/Options.cmake etc.
├── Include/                  Public headers (installed)
├── Source/                   Implementations + app entry
├── Lib/libjnivm              Vendored JNI VM (C++14) Source: Lib/libjnivm/
├── Assets/ libflappybird.so + sprites/audio  Source: Assets/
├── References/mcpelauncher-linux  Source: References/mcpelauncher-linux/
├── Readme.md                 Source: Readme.md
└── build/debug|release/      Source: build/
```

Top-level `CMakeLists.txt` (Source: `CMakeLists.txt`) sets:
- `CMAKE_CXX_STANDARD 23` globally, then drops to `14` only for `libjnivm`.
- `CMAKE_POSITION_INDEPENDENT_CODE ON`, `CMAKE_EXPORT_COMPILE_COMMANDS ON`.
- Defines `ImHelper` STATIC from `Source/ImHelper/Io.cpp` + `Log.cpp`.
- Adds `Source/Platform`, `Source/Runtime`, `Source/Game`.
- Adds vendored `Lib/libjnivm` with `-w` (suppress warnings).
- Adds `Source/` as `flbird` executable.
- Installs headers to `CMAKE_INSTALL_INCLUDEDIR` and binary to `bin`.

Presets (Source: `CMakePresets.json`): `debug`, `release`, `ci` (Werror).

---

## 2. Module map (dependency graph)

```
ImHelper (Io + Log + Result)         ← no deps, used everywhere
   ↑
Platform (Linux/SDL3)                ← depends ImHelper
   ↑
Runtime  (NativeLoader + Runtime)     ← depends ImHelper, Platform, dl
   ↑
Runtime/Android (Compat)              ← depends ImHelper
   ├─ ANativeActivity, ANativeWindow, AAssetManager, AConfiguration, ALooper, AInputQueue, NativeAppGlue
   ↑
Runtime/Shims (Bionic shims)          ← depends ImHelper, dl, X11
   ├─ log, android, EGL, GLESv2, OpenSLES, c, m, dl  (each is SHARED lib with version script)
   ↑
Game (FlappyBirdGame + AssetLoader)   ← depends Runtime, Platform, ImHelper, AndroidCompat
   ↑
flbird (main.cpp)                     ← links Game, Runtime, Platform, ImHelper, jnivm, SDL3, OpenGL
```

Source: `Source/CMakeLists.txt`, `Source/Runtime/CMakeLists.txt`, `Source/Runtime/Android/CMakeLists.txt`, `Source/Runtime/Shims/CMakeLists.txt`, `Source/Game/CMakeLists.txt`, `Source/Platform/CMakeLists.txt`.

**New `ffbird` skeleton mapping** (what exists today):
- `logger/`  ↔ `ImHelper/Log` (currently stub, no source)
- `file-util/` ↔ `ImHelper/Io` (stub)
- `argparser/` ↔ *new* — not in `ffbird_cool` (cool parses args inline in `main.cpp`)
- `anticrash/` ↔ *new* — not in `ffbird_cool` (cool has no crash handler)
- `client/` ↔ `Source/` + `Source/Game` (currently empty)
- `ext/` ↔ `Lib/` (currently empty)
- Missing in skeleton: `Platform/`, `Runtime/`, `Runtime/Shims/`, `Assets/`, `CMake/` helpers.

**Insight for stronger base**: keep `ImHelper` as one library or split into `logger` + `file-util` as skeleton does — but avoid duplication. `ffbird_cool` proves a single `ImHelper` is simpler. Skeleton's split will need `Result.hpp` shared between them (cool has `Include/Result.hpp` and `Include/ImHelper/Result.hpp` identical).

---

## 3. ImHelper — Foundation toolkit

### 3.1 Result<T>

Source: `Include/Result.hpp`, `Include/ImHelper/Result.hpp` (identical, 729B).

```cpp
template <typename T> struct Result { bool ok; T value; string error; static Result success(T); static Result failure(string); operator bool(); };
template <> struct Result<void> { bool ok; string error; ... };
```

Used by `Platform::Initialize()`, `Runtime::Initialize()`, `NativeLoader::Load/Symbol`. Lightweight, no `std::expected`.

### 3.2 Io (File I/O)

Source: `Include/ImHelper/Io.hpp` (7909B) + `Source/ImHelper/Io.cpp` (11038B).

- `class FileSystem` singleton: `SetBasePath`, `GetBasePath`, `ResolvePath`, `Exists`, `IsFile`, `GetFileSize`, `CreateDirectories`, `GetExecutablePath()`, `runtime_data_dir()`.
- Free functions: `ReadBytes`, `ReadString`, `ReadBytesLimited`, `ReadBytesRange`, `WriteBytes`, `WriteString`, `InitializeWithExecutablePath()`, `SetBasePath`, `ResolvePath`.
- Error model: `std::expected<T, IOError>` with `IOError` enum (FileNotFound, AccessDenied, etc.) and `IOErrorToString`. This is *different* from `Result<T>` — two error systems coexist.
- `runtime_data_dir()` checks `XDG_DATA_HOME`, `HOME/.local/share/flbird`.
- `GetExecutablePath()` via `/proc/self/exe` on Linux.
- Tests: `ReadBytes` resolves via `FileSystem::Instance().ResolvePath` which prepends base path if relative.

### 3.3 Log

Source: `Include/ImHelper/Log.hpp` (7150B) + `Source/ImHelper/Log.cpp` (5675B).

- `enum LogLevel {Debug,Info,Warning,Error,None}` + `LogLevelToString`.
- `class Logger` singleton: `SetMinLevel`, `SetConsoleOutput`, `SetColorOutput`, `SetLogFile`, `CloseLogFile`, `DetectColorSupport()`, `Log(level, msg, source_location)`, `LogFmt`.
- Thread-safe via `std::mutex` + `std::scoped_lock`; timestamp via `std::chrono` + `std::format`.
- File logging: opens `std::ofstream` with `std::filesystem::create_directories`; auto-flush per entry.
- Helpers: `LogDebug/Info/Warning/Error`, macros `LOG_DEBUG/INFO/WARN/ERROR` with `std::format`.
- Shims reuse: `LibLog` forwards `__android_log_print` → `Logger::Log`.

---

## 4. Platform

Source: `Include/Platform/Platform.hpp` (545B) + `Source/Platform/Linux/PlatformLinux.cpp` (1101B in `_cool`, 2117B in `_old` with X11/SDL window).

- Namespace `Platform { Result<void> Initialize(); void Shutdown() noexcept; string Name(); bool IsInitialized() noexcept; }`
- Linux impl: `SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMEPAD)`, `SDL_CreateWindow("flbird",720,1280, WINDOW_OPENGL|RESIZABLE)` shown in `_old` variant. `_cool` minimal version omits window creation — only `SDL_Init` + log.
- `GetWindow()` exists in `_old` (returns `SDL_Window*`) but not in `_cool` header — drift indicates refactor in progress. For shim `ANativeWindow` hostWindow bridging, `GetWindow()` is needed (see `Source/Runtime/Android/Compat/ANativeWindow.cpp` fallback).
- `Platform::Name()` returns `"Linux/SDL3"`.

**Dependency note** (Source: `Source/Platform/CMakeLists.txt`): SDL3 optional at configure — if not found, builds as stub (still provides `Name/Init`). Top-level `CMake/Deps.cmake` searches `SDL3 CONFIG` then `pkg-config sdl3`.

---

## 5. Runtime — NativeLoader + Runtime

### 5.1 NativeLoader

Source: `Include/Runtime/NativeLoader.hpp` (887B) + `Source/Runtime/NativeLoader.cpp` (2619B).

- `class NativeLibrary { Load(path), Unload(), IsLoaded(), Path(), Handle(), Symbol(name) }`
- `Load` uses `dlopen(path, RTLD_NOW)` + `dlerror` → `Result<void>::failure` with `std::format`.
- `Symbol` uses `dlsym` + `dlerror` check, returns `Result<void*>`.
- Move-only (deleted copy, explicit move).
- Used by `Runtime::Initialize` and `Game::Initialize` + `main.cpp` direct path.

### 5.2 Runtime

Source: `Include/Runtime/Runtime.hpp` (523B) + `Source/Runtime/Runtime.cpp` (2343B).

- `struct RuntimeConfig { string nativelib; string data_dir; }`
- `Result<void> Initialize(RuntimeConfig)`, `Shutdown()`, `IsInitialized()`, `Version()="0.0.1"`
- `Initialize` checks: `data_dir` non-empty, `Exists(data_dir)`, `nativelib` is file if non-empty, ensures `Platform::Initialize()` if not already, logs `"[Runtime] Initializing, Data Dir = {}"`, loads nativelib via `NativeLibrary::Load`. Guarded by `std::mutex s_mutex` + `bool s_initialized`.
- Top-level `Source/Runtime/CMakeLists.txt`: STATIC `Runtime` from `Runtime.cpp` + `NativeLoader.cpp`, links `ImHelper`, `Platform`, `AndroidCompat`.

---

## 6. Runtime/Android — Compat layer (host side)

Source: `Include/Runtime/Android/Compat/*.hpp` + `Source/Runtime/Android/Compat/*.cpp`, `Source/Runtime/Android/CMakeLists.txt` (982B).

Seven structs + helpers, each with host-side creation that mirrors NDK ABI but is opaque to game:

| Header | Struct | Key helpers | Source |
|--------|--------|-------------|--------|
| `AAssetManager.hpp` | `AAssetManager {string basePath}`, `AAsset {vector<uint8_t> data, length, offset, filename}`, `AAssetDir` | `CreateAssetManager`, `OpenAsset`, `ReadAsset`, `SeekAsset`, `OpenAssetDir` etc. | `Include/Runtime/Android/Compat/AAssetManager.hpp` (1454B), `Source/Runtime/Android/Compat/AAssetManager.cpp` (4572B) |
| `AConfiguration.hpp` | `AConfiguration` (exactly 64 bytes, `static_assert(sizeof==64)`) | `CreateConfiguration` (calloc 64), `CopyConfigurationFromAssetManager` (fills en_US, port, finger, 320dpi, sdk 30) | `Include/Runtime/Android/Compat/AConfiguration.hpp` (1084B) |
| `ANativeWindow.hpp` | `ANativeWindow {w=1080,h=1920,fmt,ref=1,hostWindow}` | `CreateNativeWindow`, `Acquire/Release`, `GetWidth/Height`, `SetBuffersGeometry` | `Include/Runtime/Android/Compat/ANativeWindow.hpp` (934B) |
| `ANativeActivity.hpp` | `ANativeActivityCallbacks` (16 fn ptrs, `static_assert 16*sizeof(void*)`), `ANativeActivity` | `CreateActivity` (calloc activity+callbacks, strdup paths, sdk 30), `DestroyActivity` | `Include/Runtime/Android/Compat/ANativeActivity.hpp` (2693B) |
| `ALooper.hpp` | `ALooper {vector<pollfd>, idents, events, callbacks, datas}` | `PrepareLooper`, `AddLooperFd`, `RemoveLooperFd`, `PollLooperOnce` (poll), `PollLooperAll` | `Include/Runtime/Android/Compat/ALooper.hpp` (903B) |
| `AInputQueue.hpp` | `AInputQueue {queue<AInputEvent*>, mutex, looper, ident}`, `AInputEvent` | `CreateInputQueue`, `AttachToLooper`, `HasInputEvents`, `GetInputEvent`, `FinishInputEvent`, Motion/Key getters | `Include/Runtime/Android/Compat/AInputQueue.hpp` (1944B) |
| `NativeAppGlue.hpp` | `android_poll_source`, `android_app` (0x130=304 bytes, `static_assert`), enums `LOOPER_ID_*`, `APP_CMD_*` (16 cmds) | `android_app_read_cmd`, `pre/post_exec_cmd`, `android_main` extern | `Include/Runtime/Android/Compat/NativeAppGlue.hpp` (2779B) |

**Asset resolution** (Source: `Source/Runtime/Android/Compat/AAssetManager.cpp`): tries `basePath/filename`, `basePath/assets/filename`, `filename`, and stripped `assets/` prefix; resolves via `Io::FileSystem::ResolvePath` or `std::filesystem::exists`; loads via `Io::ReadBytes`; logs sizes. `OpenAssetFileDescriptor` returns `-1` (forces fallback to buffer).

**ANativeWindow** fallback to `Platform::GetWindow()` if `hostWindow` null (Source: `Source/Runtime/Android/Compat/ANativeWindow.cpp`).

**Host vs Shim split**: `Source/Runtime/Android/Compat/*` is host-side (C++ directly called from `Game`/`main`). `Source/Runtime/Shims/LibAndroid/LibAndroid.cpp` (9039B) is Bionic-side (exported C symbols for `libflappybird.so` via hybris linker) — duplicates struct defs locally to avoid header clash. Same duality for `ALooper`, `AInputQueue`.

---

## 7. Runtime/Shims — Bionic shims (hybris)

Source: `Source/Runtime/Shims/CMakeLists.txt` (3930B) + 8 subdirs.

Helper `add_bionic_shim(NAME SOURCES)` (Source: `Source/Runtime/Shims/CMakeLists.txt`):
- `add_library(NAME SHARED SOURCES)` with `OUTPUT_NAME NAME`, `PREFIX lib`, `SUFFIX .so`, `SOVERSION ""`, `VISIBILITY hidden`, `RPATH $ORIGIN`, target def `_GNU_SOURCE`, include `Include/` + `Lib/libhybris/hybris/include`, C++23, version script `LibNAME.version`.

Eight shims:

| Target | File | Exported symbols (via `.version`) | Host forward | Source |
|--------|------|-----------------------------------|--------------|--------|
| `log` | `LibLog/LibLog.cpp` (2072B) | `__android_log_print`, `__android_log_vprint`, `__android_log_buf_print` | → `ImHelper::Logger` | `Source/Runtime/Shims/LibLog/` |
| `android` | `LibAndroid/LibAndroid.cpp` (9039B) | `AAssetManager_open/read/close`, `AConfiguration_*`, `ALooper_prepare/pollOnce`, `ANativeWindow_*`, etc. (see `LibAndroid.version` 1504B) | → `ImHelper::Io::FileSystem` + local stubs | `Source/Runtime/Shims/LibAndroid/` |
| `EGL` | `LibEGL/LibEGL.cpp` (3996B) | `eglGetDisplay`, `eglInitialize`, `eglChooseConfig`, `eglCreateWindowSurface`, `eglSwapBuffers`, etc. (`LibEGL.version` 352B) | `dlopen libEGL.so.1` + `dlsym` on demand via `std::once_flag` | `Source/Runtime/Shims/LibEGL/` |
| `GLESv2` | `LibGLESv2/LibGLESv2.cpp` (7567B) | ~40 GL calls (`glActiveTexture`, `glDrawArrays`, etc.) (`LibGLESv2.version` 939B) | `dlopen libGLESv2.so.2` | `Source/Runtime/Shims/LibGLESv2/` |
| `OpenSLES` | `LibOpenSLES/LibOpenSLES.cpp` (2643B) | `SL_IID_ENGINE/PLAY/SEEK/VOLUME/...` + `slCreateEngine` | stub (`SL_RESULT_SUCCESS`) | `Source/Runtime/Shims/LibOpenSLES/` |
| `c` (`bionic_c`) | `LibC/LibC.cpp` (9337B) | `calloc`, `free`, `fopen`, `fprintf`, `vsnprintf`, `pthread_*` wrappers | `dlopen libc.so.6` | `Source/Runtime/Shims/LibC/` |
| `m` (`bionic_m`) | `LibM/LibM.cpp` (1848B) | `sin`, `cos`, `pow`, `sinf`, `cosf`, `sincos` | `dlopen libm.so.6` | `Source/Runtime/Shims/LibM/` |
| `dl` (`bionic_dl`) | `LibDL/LibDL.cpp` (1616B) | `dlopen`, `dlsym`, `dlclose`, `dlerror`, `dladdr` | `dlopen libdl.so.2` | `Source/Runtime/Shims/LibDL/` |

Pattern: `ensure_host_*` → `dlopen("libX.so.Y")` once, then `load_host<T>("sym")` per call; each exported C symbol has `__asm__(".symver sym,sym@LIBC")` for Bionic version. All shims built with `-fvisibility=hidden` and explicit version scripts — only listed symbols exported (checked with `readelf --dyn-syms` expectation).

X11 nuance (Source: `Source/Runtime/Shims/LibEGL/LibEGL.cpp` comment in old): uses native X11 `XOpenDisplay`/`XCreateWindow` (like `mcpelauncher-linux` `libs/hybris`/`eglut`) so Mesa EGL and window share same `Display`, avoiding Wayland `wl_display` hash mismatch.

---

## 8. Game — FlappyBird bridge

Source: `Include/Game/Game.hpp` (2304B), `Source/Game/Game.cpp` (5899B), `Include/Game/AssetLoader.hpp` (429B), `Source/Game/AssetLoader.cpp` (973B), `Source/Game/CMakeLists.txt` (595B).

- `struct GameConfig { string dataDir, nativeLibPath; int windowWidth=720, windowHeight=1280; void* hostWindow=nullptr; }`
- `class FlappyBirdGame { Initialize(outError), Shutdown(), IsInitialized(), GetApp/Activity/Window, CreateWindow, DestroyWindow, GainFocus/LostFocus, PollAndProcess, IsGameInitialized(), SetMainLoopCallback }`
- `Initialize` steps (Source: `Source/Game/Game.cpp`): load lib via `NativeLibrary::Load`, `Symbol("ANativeActivity_onCreate")`, `CreateAssetManager(dataDir)`, `CreateActivity(...)`, call `onCreate_(activity,nullptr,0)`, wait up to 50×20ms for `activity->instance` (= `android_app*`), fallback via `Symbol("g_App")` deref, set `initialized_=true`. **Stops after onCreate** — does not auto-create window (caller must call `CreateWindow()`), so `__android_log_print("Creating: %p")` visible without `Init@0xc390`.
- `CreateWindow`: `CreateNativeWindow` → `callbacks->onNativeWindowCreated` if present, else `app->pendingWindow = window` + thread will pick up `APP_CMD_INIT_WINDOW`; also calls `onStart`/`onResume`.
- `AssetLoader`: `LoadAssetBytes(basePath, filename)` tries `basePath/filename`, `basePath/assets/filename`, `filename`, stripped `assets/` variant via `ImHelper::Io::ReadBytes`; `LoadAssetString` wraps bytes→string. Used by future `InitGame` texture path.
- Linkage (Source: `Source/Game/CMakeLists.txt`): `Game STATIC` links `Runtime::Runtime`, `Runtime::AndroidCompat`, `Platform::Platform`, `ImHelper::ImHelper`.

---

## 9. Entrypoint — main.cpp

Source: `Source/main.cpp` (7920B), `Source/CMakeLists.txt` (1904B).

- Parses `--help`, `--verbose`, `--log-file`, `--data-dir`; `Logger::DetectColorSupport()`, `SetMinLevel(Debug)`.
- Resolves data dir: `GetBasePath` if empty → try `exeDir/Assets`, then `runtime_data_dir()` (XDG), then `InitializeWithExecutablePath()`.
- `Platform::Initialize()` → `Name()`.
- Resolves `libPath = dataDir/libflappybird.so` fallback `Assets/libflappybird.so`; loads via `NativeLoader::NativeLibrary`, dlsym `ANativeActivity_onCreate`, creates `AAssetManager` + `ANativeActivity` via Compat, calls `onCreate`. Logs `[P0]` steps with `std::cout` + `LOG_INFO`. Mirrors decompiled `ANativeActivity_onCreate` at `0x921c` noted in comment.
- `Source/CMakeLists.txt`: `add_executable(flbird main.cpp)` links `Game::Game`, `Runtime::Runtime`, `Platform::Platform`, `ImHelper::ImHelper`, `hybris::hybris` if present, `SDL3::SDL3`, `OpenGL::GL`, `jnivm fake-jni baron`, copies binary to `${CMAKE_SOURCE_DIR}/flbird` post-build via `copy_to_root ALL`.

---

## 10. Helpers — CMake modules + third-party

Source: `CMake/Options.cmake` (552B): `FLBIRD_BUILD_TESTS`, `FLBIRD_ENABLE_LTO`, `FLBIRD_ENABLE_SANITIZERS`.
Source: `CMake/Deps.cmake` (1318B): `PkgConfig`, `SDL3 CONFIG` → `pkg-config sdl3` fallback, `OpenGL`, `Threads REQUIRED`, `GTest` if tests on.
Source: `CMake/CWarnings.cmake` (1387B): `flbird_enable_warnings(target)` (`-Wall -Wextra -Wpedantic -Wconversion -Wshadow`), alias `jjride_enable_warnings`.
Source: `Lib/libjnivm/` (vendored, `Lib/libjnivm/README.md`): fake-jni + jnivm + baron, built as C++14 with `-w`.
Source: `References/mcpelauncher-linux/` (14678B `CMakeLists.txt`): reference for `hybris` linker, `eglut`, `epoll`, `hook.cpp`, `appplatform.cpp` — not built but guidance for shim X11/EGL design.

---

## 11. Key observations for stronger base

1. **Two error systems**: `Result<T>` (own) vs `std::expected<T,IOError>` (Io). Unify or keep distinct — current code uses both; plan should decide. Stronger base would pick one (e.g., `std::expected` everywhere, `Result` deprecated).

2. **ImHelper vs split logger/file-util**: skeleton splits `Io` and `Log` into separate libs. `ffbird_cool` keeps them together — easier linkage for shims (`log` shim only needs `Log`, `android` shim needs `Io`). Split is viable if `Result` header is shared and both link minimal deps (`logger` no deps, `file-util` depends on `logger` iff `HAVE_LOGGER` — see `file-util/CMakeLists.txt`).

3. **SDL3 window ownership**: `_cool` Platform does not create window; `_old` does and exposes `GetWindow()`. The `Game` shim fallback `CreateNativeWindow` expects `Platform::GetWindow()`. Stronger base should make window ownership explicit (Platform owns SDL window, Game owns ANativeWindow wrapping it).

4. **Bionic shim duplication**: Compat headers (host) and shim cpp (Bionic) duplicate struct definitions to avoid NDK clash. This is intentional (comment in `ANativeActivity.hpp` "Do NOT include <android/native_activity.h>"). Keep but extract shared ABI header if possible.

5. **No tests yet**: `FLBIRD_BUILD_TESTS` option exists but no test target. Stronger base should add GTest + `ctest` early (even for `Io`/`Log`).

6. **Asset path handling duplicated**: `AAssetManager.cpp`, `Game/AssetLoader.cpp`, `LibAndroid.cpp` all duplicate candidate-path logic. Dedupe into `Io::ResolveAssetPath`.

7. **Version scripts are critical**: each shim's `.version` controls exported symbols; hybris linker resolves `DT_NEEDED` by SONAME (`liblog.so` etc.). Must keep `add_bionic_shim` pattern.

8. **No crash handler**: skeleton's `anticrash/` is new — no counterpart in `cool`. Plan should prioritize after shims.

---

## 12. File inventory (full, for traceability)

All files under `ffbird_cool` excluding `build/` (see `find` dump in analysis). Core primary sources listed above; full `Assets/` sprites/audio, `Lib/libjnivm/include|src/`, `References/` omitted for brevity but captured in tree snapshot.

