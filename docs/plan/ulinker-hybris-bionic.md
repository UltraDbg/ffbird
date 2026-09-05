# ffbird — ulinker + hybris + recent Bionic plan (universal runtime)

> Status: draft — for `plan/ulinker-hybris-bionic` branch. Builds on `ffbird-foundations` (utils/logger/file-util on `main` at `bd50851`+`d6f12ab`). Corrects previous hallucination: no Flappy Bird shim limit — this is a **universal runtime** that bundles a **real recent AOSP Bionic** and replicates **real hybris** cleanly.
> Sources: `https://android.googlesource.com/platform/bionic` (to-be-vendored `bionic/` at latest `master`/`aosp-main`), `https://github.com/libhybris/libhybris` at `7079712` (`/tmp/libhybris` clone), `mcpelauncher-manifest/mcpelauncher-linker` (`public_include/mcpelauncher/linker.h`, `src/linker.cpp`, `bionic/linker/*`) and `mcpelauncher-core` (`minecraft_utils.h/.cpp` `loadLibM`/`setupHybris`, `hybris_utils.h/.cpp`) as comparison. See `docs/research/mcpelauncher-linker-hybris-libm.md` (281 lines, 0 `ffbird_cool`) for kept vs removed analysis.
> Constraints carried forward: `utils::Result<T>` (`utils/include/utils/result.h`), `C++11` for ffbird modules (bionic itself is C17/C++17 — its **build** is C17, but **ffbird wrappers** stay C++11), `.h`-only for ffbird public headers, small single-job modules, Linux-only gate, `lib`/`bin`/`tests`/`lib_android` output layout.

---

## 1. What we are building and why not a shim

Previous research and the accidental `ffbird_cool` reference implied a minimal host-forwarding shim (e.g. `LibM.cpp` → `dlopen("libm.so.6")` + 6 symbols). You clarified: you want the opposite — **replicate hybris** and **bundle a real recent AOSP Bionic** (`bionic/linker` + `bionic/libc` + `bionic/libm` + `bionic/libdl`) as built artifacts, so arbitrary `DT_NEEDED libm.so` resolves to the **vendored `msun` 298-symbol `LIBC` `libm.so`**, not glibc `libm.so.6` with `GLIBC_2.2.5` versions.

**Universal runtime needs:**
- Any Bionic `DT_NEEDED` (not just `liblog`/`libm`) must resolve without host `ld-linux` interference.
- `TLS`, `IFUNC`, `RELA`, `GNU hash` Bloom, versioned symbols, namespaces (`ld.config` Apex) must be faithful AOSP — only real `bionic/linker` gives that.
- `pthread`/`tgkill`/`errno`/`TLS` differences must be bridged by **real hybris `hooks.c`/`wrappers.c`**, not 16-file `libc-shim`.

Hence three pillars: **(a) vendor latest Bionic, (b) write clean `ulinker` (our own improved linker wrapper like `mcpelauncher-linker` but cleaner), (c) vendor latest libhybris and clean it to the subset a universal runtime actually needs** — all wired so own shims can still be plugged via an explicit allowlist.

---

## 2. Module map (proposed, seams are top-level dirs)

```
ffbird/
├── CMakeLists.txt            # + utils, bionic-*, hybris-*, ulinker, shims
├── CMake/ Options.cmake, Deps.cmake, CWarnings.cmake
├── CMakePresets.json
├── utils/                    # already: Result<T> INTERFACE (09ac236)
├── logger/ file-util/ argparser/ anticrash/   # foundations, already on main
├── platform-linux/ runtime-linux/ libc-shim/ libegl-shim/ libgles-shim/ game/  # stubs today, evolve
├── bionic/                   # VENDORED real recent AOSP Bionic (read-only, see §3)
│   ├── linker/               #   bionic/linker/*.cpp/h (recent master)
│   ├── libc/                 #   bionic/libc/*.c + map (`libc.map.txt` ~1760 lines)
│   ├── libm/                 #   bionic/libm 216 msun + builtins/signbit/fake_long_double + arch/*.S, `libm.map.txt` 298
│   ├── libdl/                #   bionic/libdl/*.cpp, `libdl.map.txt`
│   └── apex/                 #   bionic/apex/ld.config.txt + manifest.json
├── hybris/                   # VENDORED cleaned latest libhybris (read-only, see §4)
│   ├── common/               #   hooks.c, hooks_shm.c, wrappers.c, logging.c, sysconf.c, dso_handle_counters.cpp
│   ├── common/n/             #   keep N (Q) linker version only; drop jb/mm/o version multiplex
│   └── egl/                  #   keep egl/ws.c only initially; add media/camera/sf on demand
├── ulinker/                  # NEW — our clean linker wrapper (like mcpelauncher-linker but cleaner) — §5
│   ├── include/ulinker/ulinker.h        # public API: ulinker::init/load/relocate/base (Result)
│   ├── src/ulinker.cpp                  # solist_init wrapper, get_library_base, etc.
│   └── tests/                           # CTest, fork-per-test
├── hybris-bridge/            # NEW — cleaned hybris glue for universal use — §6
│   ├── include/hybris_bridge/bridge.h   # hybris_bridge::init/loadHostLib/publishLog
│   └── src/bridge.cpp                   # wraps HybrisUtils::loadLibraryOS but with Result + missing list
├── bionic-libm/  bionic-libc/  bionic-libdl/  # OPTIONAL built Bionic shims if not building full bionic/ directly
│                                # Alternatively build bionic/ directly as above; keep these as reference for version scripts
├── shims/                        # YOUR OWN shims — explicit allowlist with version scripts, see §7
│   ├── liblog-shim/  libandroid-shim/  libGLES-shim/  # each SHARED, `LINKER:--version-script` per AOSP map
│   └── README.md                 # how to add a shim: add_bionic_shim(NAME, SRCS, VERSION)
├── natives/print_test/           # NDK Bionic libprint_test.so (DT_NEEDED liblog.so + libm.so)
└── ext/  # reserved
```

**Rules kept:** `.h`-only for ffbird public headers, `C++11` for ffbird-side code (`ulinker`/`hybris-bridge`/`shims` wrappers), `bionic/` itself builds as its native `C17/C++17` (its own `Android.bp` logic, but we drive it via CMake `add_subdirectory(bionic/linker)` equivalent or via `ExternalProject` if toolchain clash). Each `CMakeLists.txt` small, direct `PUBLIC` include + `PRIVATE` link; `utils::Result` everywhere, no `throw`.

---

## 3. Vendor latest Bionic — how

### 3.1 Where from, what version

* Upstream: `https://android.googlesource.com/platform/bionic` — clone `aosp-main` (latest `master`). Current `mcpelauncher-linker` pins `platform-tools-29.0.6-56-g081b55b1f` (API 29 Q, 2020) from fork `minecraft-linux/android_bionic`. For universal runtime, move to **latest `aosp-main`** (API 34+ / 35) — `git clone https://android.googlesource.com/platform/bionic bionic` + pin via `git describe` (e.g. `aosp-main-2026-09-05-gXXXXXXX`) and document in `bionic/README.FFBIRD.md`.

* Alternatives: `https://android.googlesource.com/platform/bionic` is the source of truth; `minecraft-linux/android_bionic` is the mcpelauncher-stabilized fork — use the former directly for latest, keep the latter as fallback reference.

### 3.2 What to vendor (read-only)

Copy `bionic/` as a **read-only vendor** (like `ext/` but top-level `bionic/`). Keep `.git` as `bionic/.git` for `git log --oneline -5` reproducibility, but do **not** modify files in `bionic/` — wrappers live outside.

Keep:
- `bionic/linker/` (all `*.cpp`/`*.h`: `linker.cpp`, `linker_soinfo.cpp`, `linker_relocate.cpp`, `linker_namespaces.cpp`, `linker_config.cpp`, `linker_phdr.cpp`, `linker_block_allocator.cpp`, etc.)
- `bionic/libc/` (`libc.map.txt`, `version_script.txt`, `arch-*` `string/*.S`)
- `bionic/libm/` (`Android.bp` 216 msun sources, `libm.map.txt` 298, `builtins.cpp`, `signbit.cpp`, `fake_long_double.c`, `arch/x86_64/*.S`)
- `bionic/libdl/` (`libdl.cpp`, `libdl.map.txt`)
- `bionic/apex/` (`ld.config.txt`, `manifest.json`)
- `bionic/libc/include/android/dlext.h` and private headers needed by linker

Do **not** vendor `bionic/tests`, `benchmarks`, `tools/versioner` beyond reference.

### 3.3 How to track updates

* Script `scripts/vendor-bionic.sh`: `git clone --depth 1 https://android.googlesource.com/platform/bionic /tmp/bionic-latest && rsync -a --delete /tmp/bionic-latest/ bionic/ --exclude .git && git -C bionic rev-parse --short HEAD > bionic/.aosp_rev`
* CI `cron` that runs the script and opens a PR when `bionic/.aosp_rev` changes — same as `mcpelauncher-manifest` pins submodules at `heads/master` commits.

---

## 4. Vendor latest libhybris — cleaned

### 4.1 Where from

* `https://github.com/libhybris/libhybris` (`/tmp/libhybris` at `7079712` already cloned). Keep as `hybris/` read-only vendor, like `bionic/`.

### 4.2 What to keep vs remove — from comparison in research

**Keep (core translation that universal needs):**

* `hybris/common/hooks.c` (~800 hooks: `pthread_mutex/cond/rwlock` low-pointer `0xFFFF` shm detection, `SYS_futex` pulse, `locale`, `sincos`, `strlcat/strlcpy`, `property` `my_property_get/set/list`, `dso_handle_counters`) — this is the **real** Bionic→glibc translation that `libc-shim` 16 files only partially duplicate.
* `hybris/common/wrappers.c` + `hybris/common/wrapper_code_generic_arm.c` — runtime `mmap(RWX)` ARM trampolines for `bl` patching.
* `hybris/common/dso_handle_counters.cpp`, `hybris/common/logging.c`, `hybris/common/sysconf.c`, `hybris/common/native_handle.c`, `hybris/common/legacy_properties/` (`properties.c`/`cache.c`) — property bridge `__system_property_get`.
* `hybris/common/n/` (or `q/` depending on target API) — single linker version matching your **latest Bionic API level** (do not keep `jb/mm/o/q` multiplexing; pick `n` or `q` that matches `bionic` API 34).

**Keep on demand (add when a Bionic .so needs it):**

* `hybris/egl` (`egl.c`, `ws.c`, `hybris_egl_interface`) — needed if Bionic lib needs host EGL.
* `hybris/properties`, `hybris/libsync`, `hybris/gralloc` — add when `dlerror` says missing `libsync.so` etc.

**Remove (not needed for universal headless/Bionic lib load):**

* `hybris/camera`, `hybris/media` (`decoding_service`, `codec.cpp`), `hybris/sf` (surfaceflinger), `hybris/hardware` (hwcomposer), `hybris/hwc2`, `hybris/input`, `hybris/ui`, `hybris/vibrator`, `hybris/wifi`, `hybris/opencl`, `hybris/libnfc`, `hybris/vulkan` — these are HALs for Halium/SailfishOS to run Android vendor blobs; a universal Bionic runtime loading `libminecraftpe.so`/`libprint_test.so` does not need them.
* `compat/camera`, `compat/media`, `compat/surface_flinger`, `compat/ui` — old Android compat shims.

Implementation: `hybris/CMakeLists.txt` that `add_subdirectory(common)` with `target_compile_definitions(WANT_LINKER_Q)` (or `N`) and `add_library(hybris-common STATIC hooks.c wrappers.c ...)` — then `hybris-bridge/` links that, exposing a clean API.

---

## 5. ulinker — our own improved linker (like mcpelauncher-linker but cleaner)

### 5.1 Why a wrapper is needed

`bionic/linker` is a **library-internal** linker — its public `__loader_*` symbols are meant for the loader, not for a host process to call `dlopen` on Bionic files. `mcpelauncher-linker` adds `public_include/mcpelauncher/linker.h` (`__loader_dlopen(..., nullptr)` inline wrappers) + `src/linker.cpp` (`solist_init` + `soinfo::load_library` + `increment_ref_count` + `get_library_base`). We replicate **that idea but cleaner**.

### 5.2 Public API (clean, `utils::Result`, `once_flag`)

`ulinker/include/ulinker/ulinker.h`:

```cpp
#pragma once
#include "utils/result.h"
#include <unordered_map>
#include <string>
namespace ulinker {
  utils::Result<void> init() noexcept; // once_flag, solist_init, preload libdl.so shim, LD verbosity
  utils::Result<void*> loadLibrary(const char* soname,
      const std::unordered_map<std::string,void*>& extraSymbols) noexcept; // soinfo::load_library + refcount
  utils::Result<void> relocate(void* handle,
      const std::unordered_map<std::string,void*>& extra) noexcept; // add_symbols
  utils::Result<size_t> getLibraryBase(void* handle) noexcept; // soinfo->base
  utils::Result<void> getLibraryCodeRegion(void* handle, size_t& base, size_t& size) noexcept;
  utils::Result<void> updateLdLibraryPath(const char* path) noexcept;
  const char* dlError() noexcept; // last linker error, thread_local if possible
}
```

No `throw`, no `DL_ERR` global alone — every failure carries `error` string (`dlerror()` + context). `init()` is idempotent: `static std::once_flag once;` + `if (already) return failure("already initialized")` vs re-`solist_init`.

### 5.3 Implementation sketch

`ulinker/src/ulinker.cpp`:

* Include `../bionic/linker/linker_soinfo.h`, `linker_debug.h` (private Bionic headers — `target_include_directories(ulinker PRIVATE bionic/ bionic/linker/core/base/include ...)` + `-include compat.h` + `PATH_MAX=256 _GNU_SOURCE` as `mcpelauncher-linker/CMakeLists.txt` does).
* `init()` reads `getenv("ULINKER_VERBOSITY")` → `g_ld_debug_verbosity`, calls `solist_init()`, then `loadLibrary("libdl.so", getDlSymbols())` (where `getDlSymbols()` is the 6 `libdl` Bionic symbols from `bionic/libdl/libdl.cpp`).
* `loadLibrary` wraps `soinfo::load_library(name, symbols)` + `increment_ref_count`.
* `getLibraryBase` wraps `soinfo_from_handle(handle)->base`.
* All `extern "C" void __loader_assert` aborts on internal inconsistency — keep but log via `logger`.

Build:

```cmake
add_library(ulinker STATIC
  ${BIONIC_LINKER_SRCS}   # bionic/linker/*.cpp list from bionic/linker/Android.bp
  src/ulinker.cpp
)
target_link_libraries(ulinker PUBLIC utils z pthread)
target_include_directories(ulinker PRIVATE bionic/ bionic/linker/include core/base/include ...)
target_compile_definitions(ulinker PRIVATE PATH_MAX=256 _GNU_SOURCE)
target_compile_options(ulinker PRIVATE -include compat.h)
target_include_directories(ulinker PUBLIC include)
set_target_properties(ulinker PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
```

`CXX17` for `bionic/` itself (AOSP needs it), but public `ulinker.h` is `C++11` clean (`Result`).

### 5.4 Isolation fix (global solist)

* **Short term:** `ulinker::init()` `once_flag` + fork-per-test `CTest` isolation (each `tests/ulinker_*` binary does `fork()` or is own process — `solist` is pristine). This is what mcpelauncher lacks (silent second `init` corruption).
* **Long term (if needed):** per-instance `LinkerInstance{ android_namespace_t ns; soinfo* head; }` threading `solist_get_head` → `instance.head` — requires patching `bionic/linker/*.cpp` search-replace; defer until fork-per-test proves insufficient.

---

## 6. hybris-bridge — cleaned hybris glue for universal use

### 6.1 API

`hybris-bridge/include/hybris_bridge/bridge.h`:

```cpp
#pragma once
#include "utils/result.h"
#include <string>
#include <unordered_map>
namespace hybris_bridge {
  utils::Result<void> init() noexcept; // init hybris common + properties + wrappers
  utils::Result<void*> loadHostLibrary(const char* bionicSoname, const char* hostPath,
      const char** bionicAllowlist, std::unordered_map<std::string,void*> extra = {}) noexcept;
  // allowlist = libm_symbols 298, not silent — returns failure with missing vector if dlsym fails
  utils::Result<void> publishAndroidLog() noexcept; // hookAndroidLog → ulinker::loadLibrary("liblog.so", 4 syms)
  utils::Result<void> stubSymbols(const char* bionicSoname, const char** syms, void* stub) noexcept;
}
```

`loadHostLibrary` is the new `HybrisUtils::loadLibraryOS` but with `Result` and missing-list.

### 6.2 Implementation

* Wraps `hybris/common/hooks.c` initialization (`hybris_init` → `_android_linker_init` with `get_hooked_symbol` callback) + `dlopen(hostPath)` + `dlsym` loop:

```cpp
utils::Result<void*> loadHostLibrary(...) noexcept {
  void* handle = dlopen(hostPath, RTLD_LAZY);
  if (!handle) return failure(dlerror());
  unordered_map<string,void*> syms = extra;
  vector<string> missing;
  for (int i=0; symbols[i]; ++i) {
    if (void* p = dlsym(handle, symbols[i])) syms[symbols[i]] = p;
    else missing.push_back(symbols[i]);
  }
  if (!missing.empty()) return failure("missing host syms: " + join(missing));
  auto r = ulinker::loadLibrary(bionicSoname, syms);
  if (!r.ok) return r;
  return handle; // host handle kept (leaked) for lifetime, or store in vector for dlclose on shutdown
}
```

* `publishAndroidLog` does `ulinker::loadLibrary("liblog.so", {__android_log_print → Log::vlog})` as `mcpelauncher-core/src/hybris_android_log_hook.cpp` does, but via `logger`.

Build: `add_library(hybris-bridge STATIC src/bridge.cpp)` + `target_link_libraries(hybris-bridge PUBLIC ulinker utils logger hybris-common)` where `hybris-common` is the cleaned `hybris/common` `STATIC`.

---

## 7. Shims — your own shims, explicit allowlist, version scripts

Your universal runtime still needs **your own shims** for Bionic libs that are **not** real Bionic (e.g. `libandroid.so` `AAssetManager`, `libEGL.so` host forward). Pattern (like `ffbird_cool` `add_bionic_shim` but without Flappy Bird limit):

```cmake
function(add_bionic_shim NAME SOURCES VERSION_FILE)
  add_library(${NAME} SHARED ${SOURCES})
  set_target_properties(${NAME} PROPERTIES OUTPUT_NAME "${NAME}" PREFIX "lib" SUFFIX ".so"
    CXX_VISIBILITY_PRESET hidden C_VISIBILITY_PRESET hidden)
  target_link_options(${NAME} PRIVATE "LINKER:--version-script=${VERSION_FILE}")
  install(TARGETS ${NAME} LIBRARY DESTINATION lib)
endfunction()
# example: add_bionic_shim(log LibLog/LibLog.cpp LibLog.version)
```

Version script is **verbatim** `LIBC`/`LIBLOG`:

```
LIBC { global: sin; cos; pow; sincos; ... 298; local: *; }; // from bionic/libm/libm.map.txt
```

Keep `__asm__(".symver sin,sin@LIBC")` per export so Bionic `DT_VERNEED LIBC` resolves. This is where you bridge **your own shims** — not libhybris, not Bionic.

---

## 8. Phases (dependency order, tracer bullet)

**Phase 0 — vendor + ulinker skeleton (1 ticket, blocks all)**

* `scripts/vendor-bionic.sh` + `scripts/vendor-hybris.sh` (clone latest, rsync, record `git describe` in `bionic/.aosp_rev` + `hybris/.hybris_rev`).
* `bionic/CMakeLists.txt` stub that `add_library(bionic-linker STATIC bionic/linker/*.cpp)` builds as `CXX17` (verify `cmake --build` on empty).
* `ulinker/` skeleton: `ulinker.h` with 6 `Result` APIs + `src/ulinker.cpp` `once_flag` + `tests/ulinker_smoke.cpp` (`ulinker::init(); loadLibrary("liblog.so", {})`).

**Phase 1 — hybris-bridge cleaned (1 ticket, after 0)**

* `hybris/CMakeLists.txt` that builds `hybris/common` `hooks.c/wrappers.c` as `hybris-common` `STATIC` (no `jb/mm/o/q` multiplex, just `n`/`q` one).
* `hybris-bridge/` `bridge.h` + `loadHostLibrary` + `publishAndroidLog` + `tests/hybris_bridge_loadlibm.cpp` (host `libm.so.6` → Bionic `libm.so` via allowlist 6-sym minimal, assert not silent).

**Phase 2 — real Bionic libm/libc/libdl (3 tickets, parallel after 1)**

* `bionic-libm/` built from `bionic/libm` 216 msun + `builtins.cpp` etc., `libm.map.txt` verbatim, `arch/x86_64/*.S`. Test `nm -D` 298.
* `bionic-libc/` from `bionic/libc` (subset needed for `print_test`, then expand).
* `bionic-libdl/` from `bionic/libdl`.

Alternatively, build `bionic/` directly as `bionic-lib*` — keep as either top `bionic/` or `bionic-lib*/`.

**Phase 3 — integration: `natives/print_test` as Bionic lib (1 ticket, after 2)**

* Build `natives/print_test` via NDK `android.toolchain.cmake` as **Bionic** `libprint_test.so` (`DT_NEEDED liblog.so` + `libm.so`).
* Integration test `tests/print_test_bionic.cpp`: fork, `ulinker::init(); hybris_bridge::publishAndroidLog(); ulinker::loadLibrary("libm.so", bionic-libm map); ulinker::loadLibrary("libprint_test.so", {}); dlsym(print_test_hello); call; assert `logger` file has `Backtrace`-like line.

**Phase 4 — your own shims (per-shim tickets, after 3)**

* `shims/liblog-shim/` (`__android_log_print` → `logger`), `shims/libandroid-shim/` (`AAssetManager`), etc., each with own `.version` and `add_bionic_shim`. Test per shim via `ulinker::loadLibrary`.

---

## 9. Testing methodology — NDK/SDK generated libs for real Bionic vs host

This is how we prove `ulinker` + `hybris-bridge` + real `bionic-libm` work without guessing. Every `DT_NEEDED` is a test artifact built with the **same NDK/SDK the runtime will see in the wild**, not a host `gcc` stub.

### 9.1 Toolchain — which NDK/SDK and how it is invoked

* **System NDK:** `/home/clickpaw/Android/Sdk/ndk/30.0.16138531` (`r30-beta3`, `android.toolchain.cmake` at `build/cmake/android.toolchain.cmake`, prebuilts `toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android21-clang` etc., platform `android-37.0` available). Top-level `CMakeLists.txt` already respects `ANDROID_NDK` env override and defaults to that path — keep it. Source: `mcpelauncher-linker` and `ffbird` foundations `FFBIRD_BUILD_NATIVES` `ExternalProject_Add` with `CMAKE_TOOLCHAIN_FILE`, `ANDROID_ABI=arm64-v8a`, `ANDROID_PLATFORM=android-21`. Keep default `arm64-v8a` + `android-21` (minimum for `__android_log_print`) and add `x86_64` for host `qemu` runs.

* **Invocation for natives:** same as `natives/print_test` today:

```cmake
# top-level CMakeLists.txt — natives as isolated toolchain build
include(ExternalProject)
ExternalProject_Add(natives_print_test
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/natives"
  BINARY_DIR "${CMAKE_BINARY_DIR}/natives_build"
  CMAKE_ARGS
    -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake
    -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/lib_android
  INSTALL_COMMAND "" BUILD_ALWAYS TRUE)
add_custom_target(natives_all ALL DEPENDS natives_print_test)
```

`natives/CMakeLists.txt` is a **separate CMake project** built with that toolchain. It must set `CMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/lib_android` when `ANDROID_ABI` is set, so `build/debug/lib_android/libprint_test.so` is the artifact, not `natives_build/libprint_test.so`. Already done for `print_test` — reuse for every Bionic test lib below. Do **not** use `ndk-build` (`Android.mk`) — keep CMake-only to stay testable via `compile_commands.json`.

* **SDK tools for verification (not building):** `llvm-readelf`, `llvm-nm`, `llvm-objdump` from the same NDK prebuilt (`toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf`) + host `readelf --dyn-syms --verneed --verdef -d` + `nm -D`. Use them in tests to assert `DT_NEEDED`, `DT_VERNEED LIBC`, `DT_SONAME`, and `T print_test_hello` / `U __android_log_print`.

### 9.2 What we generate with NDK vs what we bundle from real Bionic

* **Bundled from real AOSP Bionic (host-built, `C17/C++17`):** `bionic/linker` `STATIC`, `bionic/libm` 216 `msun` + `libm.map.txt` 298, `bionic/libc` `libc.map.txt`, `bionic/libdl` — these are built **once** via host `gcc 16.2.1` with `bionic/` sources, not via NDK. They are the `LIBC` `DT_SONAME libm.so` that Bionic binaries see.

* **Generated via NDK (Bionic ABI fixtures):** every `natives/<name>/` is a **Bionic** `SHARED` built with `android.toolchain.cmake` (`arm64-v8a` + `x86_64` both where feasible) that exercises a specific `DT_NEEDED` edge:

```
natives/
├── print_test/          exists: libprint_test.so → DT_NEEDED liblog.so, libm.so — calls __android_log_print("hello") + sin(0)
├── libm_probe/          NEW: libm_probe.so → DT_NEEDED libm.so — calls acos/sincos/hypot/erf/fma/fe* to force 20+ libm symbols
├── libc_probe/          NEW: liblibc_probe.so → DT_NEEDED libc.so — calls pthread_mutex, open/read, __cxa_atexit, strlcpy
├── libdl_probe/         NEW: libdl_probe.so → DT_NEEDED libdl.so — calls dlopen/dlsym/dladdr/dlclose
├── tls_probe/           NEW: libtls_probe.so → thread_local + __tls_get_addr (TLS relocs)
└── version_probe/       NEW: libversion_probe.so → DT_VERNEED LIBC with introduced=21/23 symbols (cabs, acoshl)
```

Each has `print_test.cpp` style single `extern "C" void probe();` and a `.version` is **not** needed for the probe itself — it *consumes* the Bionic `libm.so` `LIBC` version, proving our `bionic-libm` `libm.map.txt` is correct.

### 9.3 How probes are written (one file per edge, `__android_log_print`-style)

Same minimal pattern as `print_test`:

```cpp
// natives/libm_probe/libm_probe.cpp
#include <math.h>
#include <android/log.h>
extern "C" void libm_probe() {
  __android_log_print(ANDROID_LOG_INFO, "libm_probe", "sin=%f", sin(0.5));
  volatile double a = acos(0.5), b = sincos(0); (void)a; (void)b;
}
```

`CMakeLists.txt` for the `natives/` project:

```cmake
add_library(m_probe SHARED libm_probe/libm_probe.cpp)
target_link_libraries(m_probe PRIVATE log m) # Bionic DT_NEEDED, not host
set_target_properties(m_probe PROPERTIES OUTPUT_NAME "m_probe" PREFIX "lib" SUFFIX ".so")
```

Top-level `ExternalProject_Add` builds all `m_*_probe` libs together into `build/debug/lib_android/` — they share the same `lib_android` output so `ulinker::updateLdLibraryPath` sees them together.

### 9.4 Host vs Bionic load — what each test proves

| Test binary (CTest, single process) | How it loads | What it proves | Failure → what to fix |
|---|---|---|---|
| `tests/host_dlopen_print` | host `dlopen("lib/liblog.so")` → `dlsym(__android_log_print)` | Host shim still works if someone host-loads (not Bionic) | Shim `.so` not in `lib/` |
| `tests/bionic_load_log` | `ulinker::init()` → `ulinker::loadLibrary("liblog.so", log_symbols)` → `ulinker::dlsym(handle, "__android_log_print")` | `bionic-linker` solist + hybris publish | `cannot locate symbol` → allowlist missing |
| `tests/bionic_load_print` | `ulinker::init()` → `loadLibrary("liblog.so")` → `loadLibrary("libprint_test.so", {})` → `dlsym(print_test_hello)` → call → assert log file contains `hello` | Full Bionic `libprint_test.so` with `DT_NEEDED liblog.so` resolves via Bionic namespace, not host | `DT_NEEDED liblog.so not found` → `updateLdLibraryPath` or `lib_android` not in search |
| `tests/bionic_load_libm_probe` | `ulinker::init()` → `loadLibrary("libm.so", bionic-libm map)` → `loadLibrary("libm_probe.so")` → `dlsym(libm_probe)` → call | Real Bionic `libm.so` 298-sym `LIBC` satisfies `DT_NEEDED libm.so` without host `libm.so.6` `GLIBC_2.2.5` | `DT_VERNEED LIBC not found` → `bionic-libm` version script wrong; `U sincos` → `builtins.cpp` missing |
| `tests/bionic_load_tls` | same → `libtls_probe.so` | `RELA` + `TLS` relocs (`R_AARCH64_TLS_TPREL`) handled by `bionic/linker/linker_relocate.cpp` | `TLS reloc failed` → `bionic/linker` TLS support missing |
| `tests/readelf_verneed` | host `llvm-readelf --verneed lib_android/libm_probe.so` | NDK-built probe really has `Verneed: LIBC` `introduced=21` symbols | Probe built with wrong `ANDROID_PLATFORM` |

All `bionic_*` tests are **fork-per-test** or **separate CTest binaries** because `bionic/linker` `g_soinfo_allocator`/`g_default_namespace` is process-global (`solist_init()` once). `ulinker::init()` guards with `std::once_flag` and returns `failure("already initialized")` on second call; tests either `fork()` before `init()` or are separate `add_test` binaries so `solist` is pristine. `ctest -j` still works.

### 9.5 SDK extras — APK / lib extraction for universal coverage

For universal runtime, also test with a **real APK-extracted `libminecraftpe.so` slice** (not just `print_test`):

* Use SDK `build-tools/*/aapt` or `unzip -p app.apk lib/arm64-v8a/libminecraftpe.so > /tmp/libmcpe.so` then `readelf -d /tmp/libmcpe.so | grep NEEDED` to enumerate `DT_NEEDED` set. Feed that set to `hybris-bridge` allowlist generator.

* Keep a **golden `readelf` snapshot** `tests/data/libmcpe.readelf` (committed) and assert `nm -D libBionicLibm.so` covers every `UND` libm symbol from that snapshot — catches drift when Bionic `libm.map.txt` adds `introduced=23` symbols.

* Optional: `qemu-aarch64` + `adb` + `emulator` not needed for linker tests — host `x86_64` Bionic cross-build (`ANDROID_ABI=x86_64`) runs under host `qemu` via `binfmt` if needed, but `arm64-v8a` Bionic `libm_probe.so` can be `readelf`-inspected without execution.

### 9.6 Output layout ties to tests

Already fixed in `ffbird-foundations` + `feat/output-dirs` (`084a3b1`): host libraries → `/lib`, executables → `/bin`, test binaries → `/tests`, android Bionic natives → `/lib_android` (`build/debug/lib_android/libprint_test.so` `T print_test_hello` `U __android_log_print`). Keep this layout — `bionic-libm` `SHARED` also goes to `lib/` (host-visible Bionic libs like `lib` vs `lib_android` for NDK Bionic fixtures are distinct: `lib/libBionicLibm.so` is host-built real Bionic `libm.so`; `lib_android/libm_probe.so` is NDK-built probe that *consumes* it).

---

## 10. How we intend to code it properly


* **Small modules, one job:** `bionic-linker` (link only), `hybris-bridge` (translate only), `bionic-libm` (math only), `ulinker` (public wrapper). No `utils` creep.
* **Headers `.h`-only, ffbird wrappers `C++11`:** Bionic itself is `C17/C++17` — its `STATIC` build uses `CXX17`, but public `ulinker.h`/`hybris_bridge/bridge.h` are `C++11` + `utils::Result` so `file-util`/`runtime-linux` can stay `C++11`.
* **`Result` everywhere:** `ulinker::loadLibrary` → `Result<void*>`, `hybris_bridge::loadHostLibrary` → `Result<void*>` with missing list, never `throw` nor silent `if(ptr)`.
* **`once_flag` not `bool`:** `ulinker::init` and per-shim `ensureHost` use `std::call_once` (not `static bool host_once` race).
* **Version scripts verbatim:** copy `bionic/libm/libm.map.txt` `LIBC {global: …; local: *;}` — no hand-maintained 6-sym stub.
* **Tests fork:** each `tests/ulinker_*` is fork-per-test (global `solist` isolation) or own CTest binary; `ctest -j` still parallel.
* **Linux-only gate:** top `CMakeLists.txt` `FATAL_ERROR` if not Linux, each public `.h` `#error`.
* **No copy:** ideas from `mcpelauncher-linker`/`libhybris` but **no copy** of `mcpelauncher_core` shim code — rewrite from `man dlopen` + `bionic/README.md`.

---

## 10. Immediate next steps

1. `git checkout -b plan/ulinker-hybris-bionic` (already on it) — commit this plan as `docs/plan/ulinker-hybris-bionic.md`.
2. `scripts/vendor-bionic.sh` + `scripts/vendor-hybris.sh` — vendor latest, record revs, open PR that only adds `bionic/` + `hybris/` read-only (no code yet).
3. Ticket per phase above in `.scratch/ulinker-hybris-bionic/issues/` (or GitHub issues) with blocking edges `Phase0 → Phase1 → Phase2 → Phase3 → Phase4` as tracer bullets.
4. Start `ulinker` skeleton with `init/loadLibrary` → `Result` + `once_flag` (Phase 0 ticket).

---

## 11. Risks

* `solist` global → fork-per-test tax; mitigate with `once_flag` + CTest parallelism.
* `bionic` C17 vs ffbird C++11 ABI: keep `ulinker` wrapper `extern "C"` where needed.
* `hybris` `hooks.c` 800 may need `libgcc`/`libatomic` on newer Glibc — test against `gcc 16.2.1` as in `mcpelauncher-linker`.
* `libhybris` GPL vs Bionic Apache-2.0 vs ffbird GPL-3.0: keep `LICENSE` attribution.
