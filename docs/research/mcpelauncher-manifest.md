# Research: mcpelauncher-manifest — Primary Source Analysis

> Date: 2026-09-05
> Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest` (branch `ng`, upstream `https://github.com/minecraft-linux/mcpelauncher-manifest.git`)
> Purpose: understand the upstream manifest that `ffbird` (Flappy Bird) and `jetpackjoyride` borrow patterns from (hybris linker, Bionic shims, SDL/windowing, JNI). Inform the **rewrite** of `ffbird` — keep lessons, not copies.
> Method: read top-level `CMakeLists.txt` + `README.md` + `.gitmodules` + `ext/*.cmake`, then each submodule's `CMakeLists.txt` + key headers + `src/` entry points. All paths below are relative to the manifest root unless noted. `Source:` cites the file.

---

## 1. Overview — a manifest of 30 submodules

```
mcpelauncher-manifest/          Source: CMakeLists.txt (5672B), README.md, .gitmodules (2696B)
├── CMakeLists.txt              enables C/CXX/ASM, CXX17, -fno-delete-null-pointer-checks, 64-bit/ARM logic, m32 cross
├── cmake/FindPulseAudio.cmake
├── ext/                        cur l/sdl3/json/snmalloc/gamepad_mappings  Source: ext/*.cmake
├── android-support-headers/    NDK API stubs (INTERFACE)                  Source: android-support-headers/
├── arg-parser/                 header-only arg parsing (INTERFACE)          Source: arg-parser/
├── axml-parser/                binary AndroidManifest.xml parser            Source: axml-parser/
├── base64/                     base64 encode/decode                        Source: base64/
├── cll-telemetry/              CCL telemetry (event batch, http)            Source: cll-telemetry/
├── daemon-utils/               auto_shutdown_service (client+server)       Source: daemon-utils/
├── eglut/                      EGL+X11 window glue                          Source: eglut/
├── epoll-shim/                 kqueue→epoll for macOS                       Source: epoll-shim/
├── file-picker/                zenity/Cocoa picker                          Source: file-picker/
├── file-util/                  FileUtil + EnvPathUtil                       Source: file-util/
├── game-window/                GameWindow abstraction (EGLUT/GLFW/SDL3)     Source: game-window/
├── imgui/                      vendored imgui v1.62
├── libc-shim/                  30+ file Bionic→host libc shim               Source: libc-shim/
├── libjnivm/                   jnivm/fake-jni/baron (C++14, CXX standard lib) Source: libjnivm/
├── linux-gamepad/              linux gamepad mapping                        Source: linux-gamepad/
├── logger/                     Log::trace/debug/info/warn/error → printf    Source: logger/
├── mcpelauncher-apkinfo/       apk info extraction                          Source: mcpelauncher-apkinfo/
├── mcpelauncher-client/        80-file launcher client (JNI+fake_*+patches) Source: mcpelauncher-client/
├── mcpelauncher-common/        PathHelper, openssl_multithread               Source: mcpelauncher-common/
├── mcpelauncher-core/          hook/mod_loader/crash/hybris_utils           Source: mcpelauncher-core/
├── mcpelauncher-linker/        hybris Bionic linker (STATIC)                Source: mcpelauncher-linker/
├── mcpelauncher-*bin/          prebuilt natives (linux/mac)
├── mcpelauncher-errorwindow/   error window
├── mcpelauncher-webview/       xbox webview
├── minecraft-imported-symbols/ INTERFACE import of Minecraft symbols
├── msa-daemon-client/          MSA daemon IPC client                        Source: msa-daemon-client/
├── osx-elf-header/             mach-O→ELF translation
├── properties-parser/          Java .properties parser
├── sdl3/                       vendored SDL3 preview
├── simple-ipc/                 simpleipc RPC                                Source: simple-ipc/
└── LICENSE                     GPL-3.0 (35147B)
```

Branch `ng` is active (Source: `git status` → `Sur la branche ng`), `origin/ng` at `2af7c24 Branch Sync`. `git submodule status` shows all 30 submodules at `heads/master` pinned commits (e.g., `logger 6cd91de`, `file-util 47193f`, `arg-parser ba9f5e2`, `libjnivm f24b98c`, `sdl3 483e79b preview-3.1.6-403`). `libjnivm` shows local content-modified (dirty).

`.gitmodules` (Source: `.gitmodules`) maps 26 `../*.git` + 4 GitHub URLs (webview, libjnivm, errorwindow, sdl3/imgui/axml/apkinfo).

License is **GPL-3.0** — any code borrowed into `ffbird` (also GPL in `LICENSE`) stays GPL. Rewrite must still attribute if close to manifest's `logger`/`file-util` etc.

---

## 2. Build system — top-level orchestration

Source: `CMakeLists.txt` (5672B)

- `cmake_minimum_required 3.0...4.0`, `enable_language C CXX ASM`, `CXX_STANDARD 17`, `IS_64BIT` from `CMAKE_SIZEOF_VOID_P`, `BUILD_X86` m32 path (`-m32`, `i686-linux-gnu`), `IS_ARM_BUILD` detection, `TIMESTAMP BUILD_TIMESTAMP`, `git_commit_hash` via `git log -1 --format=%h` (vs `ffbird_cool` which used `git rev-parse` stub).
- Options: `BUILD_CLIENT ON`, `BUILD_UI ON` (needs Qt), `BUILD_TESTING OFF`, `ENABLE_DEV_PATHS ON` (`-DDEV_EXTRA_PATHS=natives+build/gamecontrollerdb`), `USE_OWN_CURL ON` → `ext/curl.cmake`, `USE_GAMECONTROLLERDB ON`, `USE_SNMALLOC OFF`, `USE_SDL3_AUDIO ON`.
- Conditional blocks:
  - `if BUILD_CLIENT` → include `game-window/BuildSettings.cmake`, pull `ext/sdl3.cmake` (vendored SDL3 STATIC, disables CAMERA/RENDER/DIALOG etc.), optionally `snmalloc`, then `eglut`+`linux-gamepad` if `GAMEWINDOW_SYSTEM==EGLUT` or `glfw`.
  - `if BUILD_WEBVIEW && BUILD_UI` → `mcpelauncher-webview`.
  - Always `add_subdirectory(logger, base64, file-util, properties-parser, arg-parser, mcpelauncher-linker, libc-shim, simple-ipc, daemon-utils/*, msa-daemon-client, file-picker, game-window, cll-telemetry, minecraft-imported-symbols, mcpelauncher-common, mcpelauncher-core, mcpelauncher-apkinfo, axml-parser)` when `BUILD_CLIENT`.
  - Finally `android-support-headers` (INTERFACE), `libjnivm` with `JNIVM_ENABLE_RETURN_NON_ZERO ON` + `JNIVM_FAKE_JNI_MINECRAFT_LINUX_COMPAT ON`, `mcpelauncher-client` (executable).
- `ext/` (Source: `ext/curl.cmake` 3KB ExternalProject curl 8.21→8.0.1 with openssl/websocket, `ext/json.cmake` FetchContent nlohmann_json 3.7.3 INTERFACE, `ext/sdl3.cmake` vendored vs system toggle, `ext/glfw.cmake`, `ext/snmalloc.cmake`, `ext/gamepad_mappings*.cmake` downloads `gamecontrollerdb.txt`).
- `cmake/FindPulseAudio.cmake` finds `pulse-simple`.
- No `CMakePresets.json` generation — presets would be added by `ffbird` rewrite (see `ffbird_cool/CMakePresets.json` `debug/release/ci`).

**Lesson for `ffbird` rewrite**: manifest's top-level is heavy (30 `add_subdirectory`, conditional `GAMEWINDOW_SYSTEM`, vendored vs system SDL). `ffbird` skeleton (`CMakeLists.txt` 1598B) is minimal (just `logger/file-util/argparser/anticrash/client`). Rewrite should keep top-level thin and push choices into leaf modules + `CMake/Options.cmake` / `Deps.cmake` like `ffbird_cool/CMake/` — not replicate manifest's monolith, but learn its dependency ordering: `logger` before `file-util` (conditional `HAVE_LOGGER`), `android-support-headers` before `libjnivm`/`client`, `game-window` before `client`.

---

## 3. Foundation leaves — small C++11 libs (C++17 project wrapper)

### 3.1 `logger` (Source: `logger/include/log.h`, `logger/src/log.cpp`, `logger/CMakeLists.txt`)

- `add_library(logger include/log.h src/log.cpp) PUBLIC include/` (Source: `logger/CMakeLists.txt` 174B, `CMAKE_VERSION 2.6...4.0.0`).
- Header (Source: `logger/include/log.h`):
  ```cpp
  enum class LogLevel { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR };
  class Log {
    static void vlog(LogLevel, const char* tag, const char* fmt, va_list);
    static void log(LogLevel, const char* tag, const char* fmt, ...);
    // macro LogFuncDef generates trace/debug/info/warn/error
    static const char* getLogLevelString(LogLevel);
  };
  ```
  No `Instance()`, no `source_location`, no file sink — **global static methods**, `printf` style, `__attribute__((format(printf,2,3)))`.
- Impl (Source: `logger/src/log.cpp`  ~60 lines): `vsnprintf` into `char buffer[4096]`, trim `\r\n`, `strftime("%H:%M:%S", localtime_r)`, `printf("%s %-5s [%s] %s\n", tbuf, levelStr, tag, buffer); fflush(stdout);` No mutex (not thread-safe beyond `printf` atomicity), no file, no color, no min-level filter (caller guards if needed). This is the simplest possible logger.
- Contrast with `ffbird_cool`'s `ImHelper::Log` (mutex, file, ANSI, `source_location`, `std::format`) and `ffbird` skeleton's empty `logger/` (0 sources). For rewrite, manifest's `logger` is a good **minimal seam** if `ffbird` wants printf compat (`libc-shim` and `client` expect `Log::info(tag, fmt, ...)`). But it lacks the features `ffbird_cool` proved useful (file logging for crash triage, levels).

### 3.2 `file-util` (Source: `file-util/include/FileUtil.h`, `file-util/include/EnvPathUtil.h`, `file-util/src/FileUtil.cpp`, `file-util/src/EnvPathUtil.cpp`, `file-util/CMakeLists.txt`)

- `add_library(file-util include/FileUtil.h src/FileUtil.cpp include/EnvPathUtil.h src/EnvPathUtil.cpp [MacOS.mm]) PUBLIC include/; if(APPLE) -framework Foundation; if(TARGET logger) link logger + -D HAVE_LOGGER` (Source: `file-util/CMakeLists.txt` 558B, `CXX_STANDARD 11`).
- `class FileUtil` (Source: `FileUtil.h`):
  ```cpp
  static string getParent(path);
  static bool exists(path); // access(F_OK)
  static bool isDirectory(path); // stat + S_ISDIR (stat64 on aarch64)
  static void mkdirRecursive(path); // throw runtime_error on exist-as-file or mkdir fail
  static bool readFile(path, string& out); // open(O_RDONLY), fstat, lseek SIZE, read loop, HAVE_LOGGER error log in NDEBUG guard
  ```
  No `writeFile`, no `readBytesRange`, no `basePath` — callers manage paths.
- `class EnvPathUtil` (Source: `EnvPathUtil.h/.cpp`):
  ```cpp
  static string getAppDir();   // readlink(/proc/self/exe) + dirname, or _NSGetExecutablePath + realpath on macOS
  static string getWorkingDir(); // getcwd
  static string getHomeDir();  // getenv(HOME) or getpwuid_r
  static string getDataHome(); // getenv(XDG_DATA_HOME) or getHomeDir()+/.local/share
  static bool findInPath(what,result,path,cwd); // parse PATH colon-separated, cwd-prefix if relative, access(X_OK)
  ```
  `EnvPathUtil::getAppDir` is the same pattern `ffbird_cool/Source/ImHelper/Io.cpp` uses for `GetExecutablePath`. `getDataHome` is the `runtime_data_dir()` analogue.
- Contrast with `ffbird_cool`'s `FileSystem` singleton + `std::expected` + `ReadBytes/WriteString` + `ResolvePath`. Manifest's version is C++11 `bool/string`, synchronous, no error codes beyond `bool`/`throw`. Simpler, but less composable.

### 3.3 `arg-parser` (Source: `arg-parser/include/argparser/arg.h`, `arg_parser.h`, `arg_list.h`, `CMakeLists.txt`)

- `add_library(argparser INTERFACE) include/` (Source: `arg-parser/CMakeLists.txt` 170B) — header-only, C++11.
- `class arg_list` (Source: `arg_list.h`): wraps `argc/argv` with `next()`, `next_or_null()`, `next_value_or_null()` (peek without consuming if next starts with `-`).
- `class arg_parser` (Source: `arg_parser.h` 1040B):
  ```cpp
  class arg_parser {
    unordered_map<string, handler> handlers; vector<help_entry> help_entries;
    bool parse(int argc, const char** argv); // returns false on help/unknown/invalid
    void add_arg(name, shortname, desc, handler);
    void print_help(); // "Program Help" + list
  };
  ```
  Handlers are `function<void(arg_list&)>` registered via `add_arg`.
- `class arg<T>` (Source: `arg.h` 1468B):
  ```cpp
  template<typename T> class arg {
    arg(arg_parser& p, string name, string shortname, string desc, T def=T());
    T const& get() const;
  };
  // specializations handle_value(T&, arg_list&) for string (next()), int (stoi), float (stof), bool (case-insensitive on/off/true/false/1/0), vector<T> (emplace_back)
  ```
  Usage: declare `arg<string> dataDir(parser, "--data-dir", "-d", "data dir", "")` then `parser.parse(argc,argv)` fills it. On unknown `-h/--help` prints help. This is the arg pattern `ffbird` skeleton's `argparser/` intends to host (currently empty).
- Contrast with `ffbird_cool` inline `for (i) if(arg=="--data-dir")` loop. Manifest's `arg_parser` is more structured, still printf-based (errors to stdout), no `expected`.

### 3.4 `properties-parser`, `base64`, `simple-ipc`, `daemon-utils`

- `properties-parser` (Source: `properties-parser/`): Java `.properties` parser (not deep-read, but `mcpelauncher-client` uses for `options.txt` soft).
- `base64` (Source: `base64/include/base64.h`, `base64/src/base64.cpp`): encode/decode.
- `simple-ipc` (Source: `simple-ipc/src/client|server|unix`): `simpleipc::connection`, JSON-RPC via Unix sockets (used by `mcpelauncher-client`'s `RpcCallbackServer` sending `minecraft://`/`file://` to `JniSupport` — see `mcpelauncher-client/src/main.cpp`).
- `daemon-utils` (Source: `daemon-utils/client/include/daemon_utils/daemon_launcher.h`, `server/include/.../auto_shutdown_service.h`): daemon launch + auto-shutdown service.

All are leaf libs with no SDL/Bionic dep.

---

## 4. `android-support-headers` — NDK stubs (INTERFACE)

Source: `android-support-headers/include/android/*.h`, `EGL/egl.h`, `KHR/khrplatform.h`, `android-support-headers/CMakeLists.txt` (212B `INTERFACE`)

- Vendors Android NDK headers as plain C headers, no build: `asset_manager.h` (AAssetManager/AAsset/AAssetDir, `AASSET_MODE_*` 0..3, `AAssetManager_open/openDir/read/seek/getBuffer/getLength/close`, `__RENAME_IF_FILE_OFFSET64`), `native_activity.h` (16-callback `ANativeActivityCallbacks` + `ANativeActivity` with `JavaVM* vm, JNIEnv* env, jobject clazz, internalDataPath/externalDataPath, sdkVersion, instance, assetManager, obbPath`), `input.h` (51KB, motion/key), `looper.h` (poll), `native_window.h` (ANativeWindow), `game_activity.h` etc.
- Used by `mcpelauncher-client` (`fake_*` headers) and `libjnivm` JNI. The upstream `android/` structs are the **ABI ground truth** that `ffbird_cool`'s `Include/Runtime/Android/Compat/*.hpp` replicates (and `ffbird_cool` explicitly avoids including to dodge Bionic/host clash — Source: `ffbird_cool/Include/Runtime/Android/Compat/ANativeActivity.hpp` comment "Do NOT include <android/native_activity.h> to avoid Bionic/host clash"). For `ffbird` rewrite, `android-support-headers` can be taken as the spec for sizing (`static_assert` 64B config, 304B app).

---

## 5. Windowing / platform

### 5.1 `game-window` (Source: `game-window/include/game_window.h`, `game-window/CMakeLists.txt` 1914B, `BuildSettings.cmake`, `src/window_sdl3.cpp` etc.)

- `BuildSettings.cmake` (Source: `game-window/BuildSettings.cmake`): `GAMEWINDOW_SYSTEM_DEFAULT EGLUT` (GLFW on Apple), `GAMEWINDOW_SYSTEM` cache string.
- `CMakeLists.txt`: `add_library(gamewindow ${GAMEWINDOW_SOURCES}) PUBLIC include/` + conditional `if(GAMEWINDOW_SYSTEM STREQUAL EGLUT) add eglut+linux-gamepad` elif `GLFW` elif `SDL3` → `SDL3::SDL3`. This is the **3-backend** abstraction.
- `include/game_window.h` (8080B): enums `GraphicsApi{OPENGL, OPENGL_ES2}`, `KeyAction`, `MouseButtonAction`, `GamepadButtonId/AxisId`, `FullscreenMode`; class `GameWindow` with pure virtuals `makeCurrent`, `setIcon`, `show`, `close`, `pollEvents`, `setCursorDisabled`, `getFullscreen`, `getWindowSize`, `swapBuffers`, `setSwapInterval`, `startTextInput` etc., plus `DrawCallback`, `WindowSizeCallback`, `MouseButtonCallback` etc. set via `setDrawCallback` etc.
- `src/window_sdl3.cpp` (25KB): `SDL3GameWindow` — `SDL_SetHint(TOUCH_MOUSE_EVENTS 0, APP_NAME Minecraft)`, `SDL_GL_SetAttribute` per `GraphicsApi` (`ES profile 3.0` vs `Core 3.2, doublebuffer, depth 24 stencil 8`), `SDL_CreateWindow(title,w,h, OPENGL|RESIZABLE|HIGH_PIXEL_DENSITY)` → throw `runtime_error(SDL_GetError())` on null, `SDL_GL_CreateContext`, `SDL_GL_MakeCurrent`, `SDL_StopTextInput`, `setRelativeScale` (`GetWindowSizeInPixels` vs `GetWindowSize` for HiDPI → `relativeScale`), X11 cursor hack (`SDL_X11Cursor->internal->cursor = NULL`), `makeCurrent`, `getWindowSize`, `show/close/pollEvents` (dispatch via `SDL_PollEvent`, gamepad mapping `getKeyGamePad`/`getAxisGamepad`).
- Subsystems: `joystick_manager.cpp`, `window_eglut.cpp` (eglut path), `window_glfw.cpp` analogous.

**Lesson**: manifest's `GameWindow` is deeper than `ffbird_cool`'s `Platform::Initialize` stub (which did `SDL_Init` + `SDL_CreateWindow 720x1280` and exposed no poll/input). Rewrite should consider adopting `game-window`'s callback model vs staying thin.

### 5.2 `eglut` (Source: `eglut/include/eglut.h`, `src/eglut.c` 14979B, `src/eglut_x11.c` 33531B)

- Mini GLUT on EGL+X11 (fork of mcpelauncher's eglut). Provides `eglutInit`, `eglutCreateWindow`, `eglutSwapBuffers`, X11 `Display*` management. Used when `linux-gamepad` is present.

### 5.3 `sdl3` + `linux-gamepad` + `imgui`

- `sdl3` is vendored SDL3 preview (403 commits behind upstream). `ext/sdl3.cmake` builds vendored STATIC with disabled CAMERA/RENDER/DIALOG etc.
- `linux-gamepad` wraps `/dev/input/event*`.
- `imgui` vendored for `mcpelauncher-client` UI (not in `game-window`).

---

## 6. Bionic runtime — the heavy part

### 6.1 `mcpelauncher-linker` (Source: `mcpelauncher-linker/CMakeLists.txt` 2397B, `src/linker.cpp`, `public_include/mcpelauncher/linker.h`, `bionic/linker/linker.cpp` etc.)

- `add_library(linker STATIC bionic/linker/*.cpp core/base/*.cpp bionic/libc/... public_include/mcpelauncher/linker.h src/linker.cpp)` links `z pthread`, includes `core/base/include core/liblog/include core/libcutils/include`, defs `PATH_MAX=256 _GNU_SOURCE`, `-include compat.h`.
- Public API (Source: `public_include/mcpelauncher/linker.h`):
  ```cpp
  namespace linker {
    void init(); // solist_init + load_library("libdl.so", get_dl_symbols) + g_ld_debug_verbosity
    void* load_library(name, unordered_map<string,void*> symbols);
    int unload_library(handle);
    void relocate(handle, symbols);
    size_t get_library_base(handle);
    void get_library_code_region(handle, base&, size&);
    void* dlopen(name,flags); // __loader_dlopen
    void* dlsym(handle,symbol);
    int dladdr(addr, info);
    int dlclose(handle);
    char* dlerror();
    int dl_iterate_phdr(cb,data);
    void update_LD_LIBRARY_PATH(p);
  }
  extern "C" __loader_dlopen/dlsym/dladdr/dlclose/dlerror/dl_iterate_phdr/android_dlopen_ext
  ```
- Impl (Source: `mcpelauncher-linker/src/linker.cpp`): `solist_init()` → `linker::load_library("libdl.so", get_dl_symbols())`; `load_library` does `soinfo::load_library` + `increment_ref_count`; `relocate` adds symbols.
- This is the **hybris Bionic linker** `ffbird_cool`'s top-level `CMakeLists.txt` references as optional `hybris::hybris`. `ffbird` rewrite's Phase 8 shims would either reuse this linker (full hybris) or ship minimal `c/m/dl` shims as `ffbird_cool` did.

### 6.2 `libc-shim` (Source: `libc-shim/CMakeLists.txt` 1633B, `src/common.cpp` 9KB+ plus 30 files: `pthreads.cpp`, `semaphore.cpp`, `network.cpp`, `dirent.cpp`, `cstdio.cpp`, `errno.cpp`, `ctype_data.cpp`, `stat.cpp`, `file_misc.cpp`, `sysconf.cpp`, `system_properties.cpp`, `sched.cpp`, `bionic/strlcpy.cpp`, `src/armhfrewrite.h`, `no-fortify.h` etc.)

- `add_library(libc-shim src/common.cpp src/pthreads.cpp ... ) PUBLIC include/ PRIVATE logger epoll-shim(Apple)` + `HAVE_ATOMICS_WITHOUT_LIB` check.
- `include/` is empty at top (headers are in `src/*.h` public via `target_include_directories PRIVATE`? Actually `src/` headers are used as shim API). Shim provides `shim::bionic::clock_type/mmap_flags/rlimit_resource`, `to_host_*` translators, `stack_chk_guard`, `handle_runtime_error`, `mmap/mremap/memalign/strlcpy/clock_gettime/prctl/sendfile/__*_chk` etc.
- `src/common.cpp` maps `bionic::clock_type→CLOCK_MONOTONIC/BOOTTIME`, `mmap_flags→MAP_*`, validates flags, sets `stack_chk_guard` via `getauxval(AT_RANDOM)`, `__cxa_atexit` stub.
- This is far heavier than `ffbird_cool`'s `LibC/LibC.cpp` (9KB, 10 symbols forwarded to `libc.so.6`). Manifest's `libc-shim` is the **real shim** for all Bionic libc differences, not just forwarding.

### 6.3 `epoll-shim` (Source: `epoll-shim/include/sys/epoll.h` 1403B, `src/epoll.c` 10KB, `CMakeLists.txt`)

- For macOS/FreeBSD where `kqueue` exists but `epoll` does not. Impl (Source: `epoll.c`): `epoll_create→kqueue`, `epoll_create1`, `kqueue_save_state/load_state` via `EVFILT_USER` storing key/val in kevent, `epoll_ctl(ADD/MOD/DEL)` translating `EPOLLIN/OUT/HUP/PRI/ET` to `EV_ADD/EV_ENABLE/EV_CLEAR`, storing user `data.ptr`. Tests in `test/epoll-test.c` 20KB.

### 6.4 `osx-elf-header` + `minecraft-imported-symbols` (INTERFACE)

- `osx-elf-header` translates Mach-O ELF for macOS hybris. `minecraft-imported-symbols` is INTERFACE import of Minecraft's `android_symbols.h` (JNI symbols).

---

## 7. Core — hooks, loader, crash, patches

Source: `mcpelauncher-core/CMakeLists.txt` 1867B, `include/mcpelauncher/*.h`, `src/*.cpp`

- `add_library(mcpelauncher-core include/mcpelauncher/hook.h mod_loader.h hybris_utils.h patch_utils.h crash_handler.h minecraft_utils.h minecraft_version.h fmod_utils.h src/hook.cpp mod_loader.cpp hybris_utils.cpp hybris_android_log_hook.cpp crash_handler.cpp patch_utils.cpp minecraft_version.cpp fmod_utils.cpp) PUBLIC mcpelauncher-common logger linker libc-shim minecraft-imported-symbols file-util jnivm DL_LIBS` + includes for linker `compat.h`.

#### HookManager (Source: `include/mcpelauncher/hook.h`, `src/hook.cpp`)

- `class HookManager` singleton `instance`, inner `LibInfo` per `dlopen` handle (parses `DT_STRTAB/SYMTAB/REL/JMPREL/PT_GNU_RELRO`), `HookedSymbol {original, firstHook→lastHook}`, `HookInstance {replacement, orig**}`. Parses ELF dynamic tables via `soinfo_from_handle(handle)->dynamic/base`, applies `R_GENERIC_ABSOLUTE/JUMP_SLOT/GLOB_DAT` relocations via `mprotect`-style (RELRO tracking). `addLibrary(removeLibrary)`, `createHook(lib, symbol, replacement, orig**)`, `applyHooks()` iterates `rel/relsz` + `pltrel`. This is the **PLT hook** system `ffbird` would need for intercepting e.g. `android_log`.

#### HybrisUtils (Source: `include/mcpelauncher/hybris_utils.h`, `src/hybris_utils.cpp`)

- `static bool loadLibrary(path)` → `linker::dlopen(PathHelper::findDataFile("libs/hybris/"+path),0)` + `Log::error` on fail.
- `static void* loadLibraryOS(name, path, symbols[], unordered_map)` → `dlopen(path)` host → collect `dlsym` → `linker::load_library(name, syms)`. This bridges host `libEGL.so.1`→ Bionic `libEGL.so`.
- `static void stubSymbols(name, symbols[], stubfunc)` → map each to stub → `linker::load_library`.
- `static void hookAndroidLog()`.

#### CrashHandler (Source: `include/mcpelauncher/crash_handler.h`, `src/crash_handler.cpp`)

- `static bool hasCrashed`; `handleSignal(sig, aptr)` → reset `sigaction` for `SEGV/ABRT/FPE/BUS/ILL` to `nullptr`, if `hasCrashed` return, launch detached thread sleep 1s → `_Exit(sig)` hung guard, `backtrace(array 25)` + `backtrace_symbols` + `abi::__cxa_demangle` + `linker::dladdr` fallback for `[` unknown, dump 1000 stack words via `pptr` walk + `linker::dladdr`.
- macOS x86_64 special `handle_fs_fault` for `0x64/0x65` FS prefix fault.
- `registerCrashHandler()` → `sigaction` with `SA_SIGINFO` on mac vs `sa_handler`, ignores `SIGTRAP` (debugbreak in 1.16.100.51+).

#### ModLoader, PatchUtils, MinecraftUtils etc.

- `ModLoader` loads `libs/native/` mods, `PatchUtils` applies ELF patches, `MinecraftVersion` parses version strings, `FmodUtils` handles FMOD stubs.

---

## 8. `mcpelauncher-client` — the 80-file orchestration

Source: `mcpelauncher-client/CMakeLists.txt` 6705B, `src/main.cpp` + 60 others

- `add_executable(mcpelauncher-client src/main.cpp window_callbacks.cpp xbox_live_helper.cpp splitscreen_patch.cpp strafe_sprint_patch.cpp fake_swappygl.cpp cll_upload_auth_step.cpp gl_core_patch.cpp hbui_patch.cpp jni/{jni_descriptors,java_types,main_activity,asset_manager,playfab,store,cert_manager,http_stub,package_source,jni_support,fmod,...} fake_looper.cpp fake_window.cpp fake_assetmanager.cpp fake_egl.cpp fake_inputqueue.cpp symbols.cpp core_patches.cpp thread_mover.cpp util.cpp settings.cpp)` links `logger properties-parser mcpelauncher-core gamewindow filepicker msa-daemon-client daemon-server-utils cll-telemetry argparser baron android-support-headers libc-shim mcpelauncher-apkinfo curl openssl`.
- `src/main.cpp` flow: `RpcCallbackServer` (simpleipc auto_shutdown_service handling `minecraft://`/`file://`), `LauncherOptions options`, `SmartStub` templated stub generator for `android_syms` via `Log::warn`, `getOptionsPath()` (`PathHelper::getPrimaryDataDirectory()+games/com.mojang/minecraftpe/options.txt`), `parseOptions(saveOptions)` via `properties::property_list`, full `main(argc,argv)` → `argparser` → `PathHelper` → `minecraft_utils` → `linker::init` → `HybrisUtils::loadLibrary` → `fake_*` setup → `GameWindow` creation → patches (`gl_core`, `hbui`, `strafe_sprint`, `splitscreen`, `texel_aa` on x86) → JNI env `main_activity` + `asset_manager` → `CorePatches` → loop.

Fake shims (Source: `src/fake_*.cpp`): `fake_looper.cpp`, `fake_window.cpp`, `fake_assetmanager.cpp`, `fake_egl.cpp`, `fake_inputqueue.cpp`, `fake_swappygl.cpp` — these are the host-side shims `ffbird_cool`'s `Source/Runtime/Shims/LibAndroid/LibEGL/LibGLESv2/LibLog` mirrors at Bionic level. Manifest's fakes live in client, not as version-script libs.

---

## 9. Other leaves

- `mcpelauncher-common` (Source: `mcpelauncher-common/include/mcpelauncher/path_helper.h`, `src/path_helper.cpp`): `class PathHelper { static PathInfo pathInfo; findAppDir()/findUserHome()/getWorkingDir()/findDataFile()}` — searches `appDir`, `dataHome`, `XDG_DATA_DIRS`, `DEV_EXTRA_PATHS` for libs. `getWorkingDir` + `findAppDir` via `/proc/self/exe` same as `EnvPathUtil`.
- `libjnivm` (Source: `libjnivm/README.md`): `jnivm/fake-jni/baron` — JNI VM for Minecraft's Java side, `JNIVM_FAKE_JNI_MINECRAFT_LINUX_COMPAT ON` for static promotion oddity, `CXX 14` boundary (like `ffbird_cool`).
- `cll-telemetry` (68 files): event batch/uploader with `event_batch.h`, `curl` http, `configuration_manager`.
- `file-picker` (Source: `file-picker/include/file_picker.h`): `class FilePicker { setTitle/setFileName/setMode/setFileNameFilters, bool show(), string getPickedFile()` — zenity vs Cocoa.
- `minecraft-imported-symbols`, `axml-parser`, `properties-parser`, `base64`, `simple-ipc`, `daemon-utils`, `msa-daemon-client`, `mcpelauncher-apkinfo`, `mcpelauncher-webview`, `mcpelauncher-errorwindow`: supporting.

---

## 10. What matters for `ffbird` rewrite (from foundations)

| `ffbird` skeleton | Manifest counterpart | Lesson |
|---|---|---|
| `logger/` (empty) | `logger/` (printf, LogLevel, vlog, Log::info(tag,fmt)) | Keep printf seam for libc-shim compat, but add optional file/level for `ffbird`'s needs. Rewrite picks wrapper that can forward to manifest's simple logger or to `ImHelper::Log`-style file sink. |
| `file-util/` (empty) | `file-util/` (FileUtil+EnvPathUtil, /proc/self/exe, XDG_DATA_HOME) | Reuse `getAppDir` pattern; decision ADR: value-type `Fs(base)` vs static `FileUtil`. Manifest's static is simpler, `ffbird_cool`'s base-path `Fs` is testable — rewrite favors `Fs` value type but keeps `getAppDir` free fn. |
| `argparser/` (empty) | `arg-parser/` (`arg_parser` + `arg<T>`) | Header-only template `arg<T>` with `arg_list` is a good seam for `client`'s `--data-dir --log-file` handling. Keep similar, but `ffbird` may use `expected<Args,string>` instead of `printf` errors. |
| `anticrash/` (empty) | `mcpelauncher-core/src/crash_handler.cpp` | Manifest's `backtrace`+`linker::dladdr`+hung guard thread is more robust than `ffbird_cool`'s none. Borrow signal set + demangle, adapt to `logger::Log` sink. |
| `ext/` (empty) | `ext/curl.cmake, json.cmake, sdl3.cmake` | Manifest's vendored SDL3 + curl ExternalProject is heavy. `ffbird` should keep `ext/` thin (maybe just `gamecontrollerdb` download), rely on system `SDL3` where possible like `ffbird_cool/CMake/Deps.cmake`. |
| (new) `platform/` | `game-window/` (3 backends) | Manifest proves SDL3 backend (window_sdl3.cpp 25KB) is viable with HiDPI `relativeScale` + cursor hack. Rewrite can start with SDL3 only, but design `GameWindow` interface to allow EGLUT fallback. |
| (new) `runtime/shims/` | `libc-shim/` (30 files) + `mcpelauncher-linker` | Manifest's `libc-shim` is the full Bionic→host translation (mmap, clock, prctl, arc4random). `ffbird_cool` had tiny `LibC/LibM` forwards. Rewrite must decide: full shim (linker) vs minimal forwarding. For Flappy Bird (simple .so) minimal may suffice; for Jetpack Joyride (heavier) full may be needed. |
| (new) `runtime/link` | `mcpelauncher-linker` (`linker::dlopen`) | Hybris linker is STATIC, needs `PATH_MAX 256`, `compat.h`, `bionic/libc/include`. If `ffbird` links hybris, its `libmcpelauncher-linker` must init before any Bionic lib. |

---

## 11. Risks / open questions

- `libc-shim` is 30 files, `mcpelauncher-core/hook.cpp` touches RELRO `mprotect` — risky to roll own. For `ffbird` rewrite, prefer reusing upstream's `libc-shim` + `linker` as submodules (like manifest does) rather than rewriting shims from scratch per `ffbird_cool`'s minimal forwarding.
- `game-window`'s `GAMEWINDOW_SYSTEM` choice affects `HybrisUtils::loadLibraryOS` for EGL. Keep X11 `Display` sharing invariant that `ffbird_cool` noted (Mesa EGL vs wl_display hash).
- `android-support-headers` are GPL headers — okay, but don't fork them; use upstream as spec like manifest.
- `libjnivm` is C++14 inside C++17/23 project — set target `CXX_STANDARD 14` per lib like `ffbird_cool` does.

---

## 12. Primary source index (full)

Top: `CMakeLists.txt`, `README.md`, `.gitmodules`, `cmake/FindPulseAudio.cmake`, `ext/*`
Leaves: `logger/include/log.h + src/log.cpp`, `file-util/include/FileUtil.h + EnvPathUtil.h + src/FileUtil.cpp + EnvPathUtil.cpp`, `arg-parser/include/argparser/*`, `android-support-headers/include/android/*`, `game-window/include/game_window.h + src/window_sdl3.cpp`, `eglut/include/eglut.h + src/eglut.c + src/eglut_x11.c`, `epoll-shim/include/sys/epoll.h + src/epoll.c`, `mcpelauncher-linker/public_include/mcpelauncher/linker.h + src/linker.cpp + bionic/linker/linker.cpp`, `libc-shim/src/common.cpp + common.h + CMakeLists.txt`, `mcpelauncher-core/include/mcpelauncher/hook.h + hybris_utils.h + crash_handler.h + src/hook.cpp + hybris_utils.cpp + crash_handler.cpp`, `mcpelauncher-common/include/mcpelauncher/path_helper.h + src/path_helper.cpp`, `mcpelauncher-client/src/main.cpp + CMakeLists.txt`, `file-picker/include/file_picker.h`, `libjnivm/README.md`, plus `base64, cll-telemetry, daemon-utils, simple-ipc, etc.`

All above read directly; `ffbird/docs/research/ffbird_cool.md` remains input for game-specific Bionic ABI (64B config, 0x130 app, 16 callbacks).

