# ffbird — Stronger Base Plan (rewrite, foundations → runtime)

> Status: draft — step-by-step REWRITE plan. Reference is `docs/research/ffbird_cool.md` for *lessons*, not for copy.
> Source to learn from: `ffbird_cool` / `ffbird_old`. Source to build: `ffbird/` skeleton — `logger`, `file-util`, `argparser`, `anticrash`, `client`, `ext`.
> Goal: rewrite as stronger, more organized, more powerful base. Foundations first. No porting — every module is designed fresh, with its own interface at a clean seam.

---

## Rewrite stance

- **Not a port.** We do not copy `ffbird_cool/Source/ImHelper/Log.cpp` into `logger/src/log.cpp`. We *study* what it does (research doc), then design the new module's interface from scratch against the domain we want.
- **Research is input, not spec.** `docs/research/ffbird_cool.md` tells us what Flappy Bird's Bionic runtime *needs* (16 ANativeActivity callbacks, 64-byte AConfiguration, 0x130 android_app, 4-path asset resolve, version-script shims). The new design must *satisfy* those ABI truths but may organize them differently.
- **Prove each seam with tests before the next layer touches it.** A shim is not "done" when it compiles — it's done when `readelf --dyn-syms` + a host `dlopen` test proves the symbol is exported with the right version and forwards correctly.
- **One canonical error vocabulary.** `ffbird_cool` mixes `Result<T>` and `std::expected<T,IOError>`. Rewrite picks one: `std::expected` (C++23) for all new APIs. No new `Result<T>` type.

---

## Principles (stronger base)

1. **Deep modules, small interfaces.** Each lib hides a lot behind a few functions. If a header grows past ~80 lines, the seam is wrong.
2. **Deps point one way.** `logger` ← `file-util` ← `platform` ← `runtime` ← `compat` ← `shims` ← `game` ← `client`. No cycles. `argparser` and `anticrash` are leaf libs (no runtime dep).
3. **ABI fidelity is a test, not a comment.** Every Bionic ABI struct gets a `static_assert(sizeof == N)` + a compile-time test. Every shim symbol gets a `__asm__(".symver")` + an `nm -D` test.
4. **Determinism over cleverness.** Asset resolution has one implementation in `file-util` (`resolve_asset`) — not three copies. Callers call it.
5. **Presets + warnings from day 1.** `debug`/`release`/`ci (-Werror)` presets, `flbird_enable_warnings`, `ctest` wired per phase.

---

## Phase 0 — Build & conventions (skeleton harden)

**Goal**: empty skeleton builds green with conventions fixed.

- Fix top-level `CMakeLists.txt` git-hash helper (currently `execute_process(COMMAND ${GIT_EXEC} WORKING_DIRECTORY ...)` is broken — needs real `git rev-parse`).
- Create `CMake/` modules fresh (not copied): `Options.cmake` (`FFBIRD_BUILD_TESTS`, `FFBIRD_ENABLE_LTO`, `FFBIRD_WERROR`), `Deps.cmake` (SDL3 via `find_package` → `pkg-config` fallback, `Threads`, `OpenGL` optional), `CWarnings.cmake` (`flbird_enable_warnings` with `-Wall -Wextra -Wpedantic -Wshadow`, GCC extras). Design for new layout, inspired by `cool`.
- Create `CMakePresets.json` (`debug`/`release`/`ci`) and `compile_commands.json` symlink handling.
- Add `.clang-format` / `.clang-tidy` tuned for rewrite (C++23, 100 cols, `clang-tidy` checks we want from scratch).
- Keep `add_subdirectory` order `logger → file-util → argparser → anticrash → ext → client`.
- Wire `ctest` skeleton and `CTest` include.

**Exit**: `cmake --preset debug && cmake --build --preset debug` succeeds on empty libs.

Ref: research §§1,10 — layout & CMake lessons.

---

## Phase 1 — `logger` (rewrite)

**Goal**: thread-safe logging as a deep module. Interface designed for *clients*: game code, shims, crash handler.

*Lesson from cool* (research §3.3): cool's `ImHelper::Log` couples console + file + color + `source_location` in one singleton. Works, but hard to test (global mutex + `ofstream` inside). Rewrite favors injectability.

**Design sketch (not a copy)**:
```
logger/include/logger/logger.hpp
  enum class Level { Debug, Info, Warn, Error, None }
  struct Sink { virtual void write(Level, string_view, source_location) = 0 }
  class Logger { add_sink(sink), set_min_level, log(level, fmt, ...) }
  // Provide ConsoleSink + FileSink + NullSink; Logger is not a singleton — client creates one and shares via shared_ptr.
```
- If singleton is still desired for shim convenience, provide `Logger::global()` as thin accessor over an instance, not as the core.

**Tasks**
1. Design `logger/` public header + `Sink` abstraction. Decide ADR: singleton vs instance.
2. Implement `FileSink` (`filesystem::create_directories`, `ofstream::app`, flush per write), `ConsoleSink` (ANSI, `DetectColorSupport`), timestamp via `chrono` + `format`.
3. Tests: level filter, two-thread 1k writes no data race (`tsan`), file sink creates parent dirs, `DetectColorSupport` w/ `TERM`/`isatty` mock.

**Exit**: `ctest -R logger` passes; `Logger` can be linked by `file-util` optionally via `HAVE_LOGGER` without hard dep.

---

## Phase 2 — `file-util` (rewrite)

**Goal**: file I/O with base-path root, one asset resolver. No global `FileSystem` singleton with hidden state.

*Lesson from cool* (research §§3.2, 6): cool has `FileSystem` singleton + `std::expected` `IOError` + duplicated 4-path asset logic in 3 places.

**Design sketch**:
```
file-util/include/file-util/fs.hpp
  enum class IoError { NotFound, Denied, ... }
  using IoResult<T> = expected<T, IoError>
  class Fs { explicit Fs(path base); path resolve(path) const; IoResult<bytes> read_bytes(path) const; ... }
  // No singleton. Caller owns Fs. Provide free function resolve_asset(Fs&, string_view) -> path candidates.
```
- `envpath-util.h` in skeleton becomes `Fs::executable_dir()` + `xdg_data_dir()` helpers. Keep them pure functions, not members.

**Tasks**
1. Define `IoError` + `to_string`, `IoResult`, `Fs` class with `resolve` (prepend base if relative), `exists/is_file/is_dir`, `read_bytes/read_string/write_bytes`, `read_range`.
2. Implement `resolve_asset` (single place): `base/filename`, `base/assets/filename`, `filename`, `base/<stripped assets/>`.
3. Link `logger` optionally — log on `Denial` only if logger present, no hard dep.
4. Tests: `resolve` with absolute vs relative, missing → `NotFound`, round-trip write/read, asset candidates order, `executable_dir` via `/proc/self/exe` stub.

**Exit**: `file-util` can serve `AAssetManager` and `Game` without duplication.

---

## Phase 3 — `argparser` (rewrite, header-only)

**Goal**: small, testable arg parsing. Not the inline loop from `cool/Source/main.cpp`.

*Lesson*: cool parses `--data-dir`, `--log-file`, `--verbose`, `--help` inline. Rewrite makes it a leaf lib.

**Design sketch**:
```
argparser/include/argparser/argparser.hpp
  struct Args { path data_dir; path log_file; bool verbose=false; bool help=false; }
  expected<Args, string> parse(span<string_view> args)
  string help_text(string_view prog)
```

**Tasks**
1. Header-only C++23 parser, no dep except `expected`. Support helpers for typed flags.
2. Tests: known flags, missing value → error string, unknown flag → error, `--help` short-circuit.

---

## Phase 4 — `anticrash` (rewrite, greenfield)

**Goal**: host crash handler proving shims can survive faults. No counterpart in `cool` — greenfield.

**Design sketch**:
```
anticrash/include/anticrash/handler.hpp
  void install(path log_file); void uninstall();
```
- Uses `sigaction` + `sigaltstack` for `SIGSEGV/ABRT/ILL/FPE/BUS`, `backtrace` + `backtrace_symbols_fd` to file, async-signal-safe. Logger integration is optional (if logger available, also `log(Error, ...)` after signal returns via `write(2)` path).

**Tasks**
1. Implement `install` with `SA_SIGINFO`, store old handlers, write crash log atomically.
2. Tests: death test (`raise(SIGSEGV)` in child, check log contains `backtrace`), `uninstall` restores.

---

## Phase 5 — `platform` (rewrite, new `platform/` lib)

**Goal**: OS abstraction with explicit window ownership.

*Lesson from cool* (research §4): `_cool` minimal vs `_old` windowing drift + missing `GetWindow()` in header. Rewrite makes ownership explicit.

**Design sketch**:
```
platform/include/platform/window.hpp
  class Window { static expected<Window, string> create(int w,int h, string title); void show(); SDL_Window* native() const; }
platform/include/platform/platform.hpp
  expected<void,string> init(); void shutdown(); string name();
```

**Tasks**
1. `Platform` init: `SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMEPAD)`, `init` fails if already initialized (second call → error). `shutdown` idempotent.
2. `Window` owns `SDL_Window*`; creation is `Window::create(720,1280)`, not hidden global. `ANativeWindow` bridging will take `Window::native()`.
3. Stub path when SDL3 absent (no hard require at configure).
4. Tests: double init → error, `native()` non-null post-create, stub when SDL missing.

**Note**: skeleton has no `platform/` today — create it. `client` will depend on it.

---

## Phase 6 — `runtime` (rewrite, `runtime/` lib)

**Goal**: native lib loading as a deep module, no hidden `Platform::Initialize` coupling.

*Lesson from cool* (research §5): cool's `Runtime::Initialize` auto-calls `Platform::Initialize` + checks `data_dir` existence + loads lib. Mixes concerns. Rewrite separates: `Remaps checks?` → Caller decides.

**Design sketch**:
```
runtime/include/runtime/loader.hpp
  class Lib { static expected<Lib,string> open(path); void* sym(string_view) const; expected<void*, string> find(string_view) const; }
runtime/include/runtime/runtime.hpp
  struct Config { path data_dir; path lib_path; }
  expected<void,string> validate(Config) // checks existence, no side effects
```

**Tasks**
1. `Lib::open` uses `dlopen(RTLD_NOW)` + `dlerror` → `string` error via `format`; move-only; `find` returns `expected`.
2. `validate` pure function; loading is caller's job (keeps `runtime` independent of `Platform`).
3. Tests: open missing → error, `find("")` → error, `find` on missing symbol → error, move semantics.

---

## Phase 7 — `runtime/android` compat (rewrite, host side)

**Goal**: host structs that satisfy Bionic ABI truths, designed as thin value types + factory functions, not singletons.

*Lessons* (research §6): 7 structs, exact sizes (`AConfiguration 64`, `android_app 0x130`, callbacks 16 ptrs), 4-path asset, `poll` looper. Cool duplicates structs between host and shim to avoid NDK clash — keep duplication explicit.

**Order (rewrite from scratch, size assertions first)**:
1. `a_configuration.hpp` — `struct AConfig { ... }` 64B + `make_config()` + `from_asset_mgr`.
2. `a_asset.hpp` — `struct Asset { vector<uint8_t> bytes; size_t off; string name; }` + `AssetManager { Fs fs; }` + `open_asset(AssetManager, filename)` using `file-util::resolve_asset`.
3. `a_window.hpp` — `struct NativeWindow { int32_t w,h,fmt,refs; SDL_Window* host; }` + `make_window`.
4. `a_activity.hpp` — `struct Callbacks { 16 fn ptrs }` + `struct Activity { Callbacks* cb; ... instance; assetManager; }` + `make_activity`.
5. `a_looper.hpp` — `struct Looper { vector<pollfd> ... }` + `prepare/add/remove/poll`.
6. `a_input.hpp` — `InputQueue` + `InputEvent` stubs.
7. `app_glue.hpp` — `android_app` 304B + `APP_CMD_*`, `LOOPER_ID_*`.

**Tasks per file**: header with `static_assert`, factory (`make_*` / `create_*`), no global. Tests for `sizeof`, asset open/close round-trip against temp dir, looper poll with pipe fd, `android_app` size.

---

## Phase 8 — `shims` (rewrite, `runtime/shims/`)

**Goal**: Bionic `DT_NEEDED` libs with correct SONAMEs, designed as forwarding shims with `version-script` contract tested by `readelf`.

*Lesson* (research §7): `add_bionic_shim` pattern (SHARED, `PREFIX lib`, `SOVERSION ""`, `hidden`, `RPATH $ORIGIN`, `_GNU_SOURCE`) + 8 shims. Rewrite re-derives version scripts from `nm -D libflappybird.so` expected imports, not from copying `cool/Lib*.version`.

**Tasks**
1. Fresh `add_bionic_shim` helper in `runtime/shims/CMakeLists.txt`.
2. Each shim rewritten:
   - `log` → forwards `__android_log_print` to `logger::Logger` (global instance if chosen).
   - `android` → `AAssetManager_open` etc. using `file-util` resolver; keep structs local to avoid host header clash.
   - `egl`/`glesv2` → `dlopen` host `libEGL.so.1` / `libGLESv2.so.2` once via `call_once`, per-symbol `dlsym`.
   - `opensles` → stub `SL_IID_*` + `slCreateEngine` success.
   - `c/m/dl` → forward to host `libc.so.6`/`libm.so.6`/`libdl.so.2` with `.symver`.
3. For each: hand-written `LibXXX.version` listing only exported symbols; test via `readelf --dyn-syms` expects exactly those.
4. Aggregate `shims` target.

**Exit**: `nm -D build/shims/liblog.so | grep __android_log_print` shows `T`; `ldd` on dummy Bionic binary finds `libandroid.so`.

---

## Phase 9 — `game` bridge (rewrite, `game/` or `client/game/`)

**Goal**: drive `libflappybird.so` lifecycle P0→P1 with explicit state machine, not scattered `if (callbacks)`.

*Lesson* (research §8): cool's `FlappyBirdGame` loads `ANativeActivity_onCreate`, calls it, waits 50×20ms for `instance`, fallback `g_App`, then `onNativeWindowCreated`. Logic is correct but state is hidden bool.

**Design sketch**:
```
game/include/game/game.hpp
  enum class State { Unloaded, LibLoaded, Created, Windowed, Focused }
  struct Config { path data_dir; path lib; int winW, winH; SDL_Window* host; }
  class Game { expected<void,string> create(Config); expected<void,string> create_window(); void poll(int timeoutMs); State state() const; }
```

**Tasks**
1. `Fs` + `Loader::Lib` reuse; `Activity` via compat factory; call `ANativeActivity_onCreate`; wait with `chrono` for `instance`; fallback `sym("g_App")`.
2. Single `resolve_asset` call path.
3. Tests: state transitions, mock lib (build tiny `libmock.so` exporting `ANativeActivity_onCreate` that writes `instance`).

---

## Phase 10 — `client` entrypoint (rewrite)

**Goal**: thin `main.cpp` that composes leaves, no business logic.

**Design sketch** (~60 lines):
```
parse args → Fs base → Logger setup → Platform::init → Window::create → Runtime::validate → Game::create → Game::create_window → loop (poll + render) → shutdown
```

**Tasks**
1. Use `argparser::parse`, `file-util::Fs`, `platform::Window`, `runtime::Lib`, `game::Game`.
2. `CMakeLists.txt` links `logger file-util argparser platform runtime android_compat game shims`, plus `Threads`, `SDL3` if found, `OpenGL` optional; `copy_to_root` post-build.
3. Add `ext/` handling for optional `libjnivm` if still needed (decide via ADR — not required for Flappy Bird, may drop).

**Exit**: `./build/debug/flbird --help` → help text; `--data-dir Assets` → `[P0]` logs.

---

## Phase 11 — polish & quality

- `ctest --preset debug`, `ci` with `-Werror`, `tsan`/`asan` in debug.
- Benchmarks for `file-util` resolver if hot path.
- Docs: `README.md` build table + layout diagram (no copy from cool).
- `install` rules via `GNUInstallDirs`, export set.

---

## Edges

```
0 → 1
1 → 2 (optional dep)
1 → 3, 4
1,2 → 5
5 → 6
6 → 7
7 → 8 & 9 (8 and 9 can parallelize after 7)
7,8,9 → 10
10 → 11
```

---

## ADRs to record (during grill)

- Error model: `std::expected` only.
- Logger: instance vs singleton.
- Fs: value type with base path vs global.
- Module split: keep `logger`/`file-util` separate (skeleton) vs reunify?
- Window ownership: `Window` class vs global `Platform::GetWindow()`.
- `ext/libjnivm`: keep or drop for Flappy Bird.
- Shim struct duplication vs shared `abi/` header.

---

## Next action

1. Commit this rewritten plan.
2. `grill-with-docs` to lock ADRs above and write `CONTEXT.md`.
3. `to-spec` → `to-tickets` from this plan, each ticket declaring edges, starting at Phase 0.

