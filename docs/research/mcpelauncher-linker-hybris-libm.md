# Research: mcpelauncher linker + hybris + MinecraftUtils::libm — universal runtime with recent AOSP Bionic

> Date: 2026-09-05 (redo, corrects previous hallucination)
> Source: `/tmp/libhybris` (clone `https://github.com/libhybris/libhybris` at `7079712`, branch `master`), `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker` (`public_include/mcpelauncher/linker.h`, `src/linker.cpp`, `CMakeLists.txt`, `bionic/linker/linker.cpp`, `linker_soinfo.cpp`, `linker_relocate.cpp`, `linker_namespaces.cpp`, `bionic/libm/Android.bp`, `libm.map.txt`, `builtins.cpp`, `signbit.cpp`, `bionic/libc/libc.map.txt`, `bionic/libdl/libdl.map.txt`, `bionic/apex/ld.config.txt`), `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core` (`include/mcpelauncher/minecraft_utils.h`, `src/minecraft_utils.cpp`, `include/mcpelauncher/hybris_utils.h`, `src/hybris_utils.cpp`, `src/hybris_android_log_hook.cpp`), `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-client/src/main.cpp`
> Purpose: answer — clone libhybris, check what mcpelauncher kept vs removed, and how to implement hybris correctly for a **bigger universal runtime** that bundles a **real recent AOSP Bionic** (no invented shimming policy, no Flappy Bird size constraint). Fix previous research that hallucinated a shimming demand.
> Method: `git clone --depth 1 https://github.com/libhybris/libhybris /tmp/libhybris`, read `/tmp/libhybris/hybris/common/*.c`, `/tmp/libhybris/hybris/common/n|mm|jb/*.cpp`, `/tmp/libhybris/hybris/egl/*`, `/tmp/libhybris/README` vs `mcpelauncher-linker/bionic/linker/*`; counted `bionic/libm` 216 msun sources / 298 symbols; grepped `MinecraftUtils::` in `main.cpp`; compared `HybrisUtils::loadLibrary` vs `loadLibraryOS`.

---

## 1. Overview — universal runtime goal

You are not building a Flappy Bird toy runtime. You want a **universal Linux runtime that can load arbitrary Android Bionic `.so` files** (not just `libminecraftpe.so`) by bundling a **real recent AOSP Bionic** (`bionic/linker` + `bionic/libc` + `bionic/libm` + `bionic/libdl`) and bridging host glibc gaps with **real hybris**, not ad-hoc shims that forward `sin/cos` to `libm.so.6`.

`mcpelauncher manifest` already does this — but in a minimal way for one game. Upstream **libhybris** does it in a general way for many drivers. Understanding the delta tells how to go universal.

---

## 2. What libhybris upstream actually is — primary source `/tmp/libhybris`

### 2.1 Structure

Source: `/tmp/libhybris` (`README.md`, `hybris/common`, `hybris/egl`, `hybris/media`, `hybris/camera`, `hybris/sf`, `hybris/hardware`, `compat/`).

```
libhybris/  Source: /tmp/libhybris/README.md
├─ hybris/common/          hooks.c, hooks_shm.c, wrappers.c, logging.c, sysconf.c, dso_handle_counters.cpp, legacy_properties/  Source: /tmp/libhybris/hybris/common/Makefile.am
│  ├─ jb/                  Android Jelly Bean linker (old)  Source: /tmp/libhybris/hybris/common/jb/
│  ├─ mm/                  Android Marshmallow linker       Source: /tmp/libhybris/hybris/common/mm/linker.cpp
│  ├─ n/                   Android Nougat linker            Source: /tmp/libhybris/hybris/common/n/linker.cpp
│  ├─ o/                   Android O linker
│  └─ q/                   Android Q linker
├─ hybris/egl/             _android_egl_dlsym, hybris_egl_interface, ws.c  Source: /tmp/libhybris/hybris/egl/egl.c
├─ hybris/glesv1,glesv2,gralloc,hwc2,hardware,media,camera,sf,ui,vibrator,wifi,libsync,properties  Source: /tmp/libhybris/hybris/
├─ compat/                 camera/media/surface_flinger/ui input compat layers
└─ utils/                  generate_wrapper_macros.py → hybris/include/hybris/common/binding.h
```

README states goal verbatim: “allow you to load drivers that link against the bionic c library inside processes whose native c library is e.g. glibc, musl … load graphics drivers, or any other driver that's sitting inside an Android ‘so’ file”. Source: `/tmp/libhybris/README.md`.

### 2.2 Core mechanism — hooks + linker + wrappers

Source: `/tmp/libhybris/hybris/common/hooks.c` (first 300 lines):

* `hooks.c` implements **~800 Bionic→glibc hooks**: `pthread_mutex/cond/rwlock`, `__android_pthread_cond_pulse` via `SYS_futex`, `hybris_check_android_shared_mutex` (detects Android static initializers `0xFFFF` low pointers vs glibc heap), `locale` (`hybris_locale`), `property` (`my_property_get/set/list`), `dso_handle_counters`, `sincos`, `strlcat/strlcpy`, etc. It exports a `struct _hook {name, func, debug_func}` table with macros `HOOK_DIRECT` / `HOOK_TO` / `HOOK_INDIRECT`.

* `wrappers.c` / `wrapper_code_generic_arm.c` generates **runtime ARM wrappers** via `mmap(RWX)` that stash `symbol, function, trace_callback, msg` and trampoline. This is for ARM `bl` patching, not just `dlsym` interposing. Source: `/tmp/libhybris/hybris/common/wrappers.c`.

* `hybris/common/n/linker.cpp` etc. are **vendored AOSP Bionic linkers per Android version**. `hybris/common/Makefile.am` selects via `if HAS_ANDROID_6_0_0 → mm, HAS_ANDROID_7_0_0 → n, HAS_ANDROID_8_0_0 → o, HAS_ANDROID_10_0_0 → q`. Each is a full `bionic/linker` tree, not a stub. Source: `/tmp/libhybris/hybris/common/Makefile.am`.

* `hybris/egl/egl.c`, `hybris/media/*`, `hybris/hardware/*`, `compat/surface_flinger/ui` etc. are **per-subsystem bridges** (EGL `ws.c`, Gralloc `GrallocUsageConversion`, media `decoding_service`, etc.) — each hardware vendor needs its own hybris glue.

Upstream libhybris is therefore **not just a linker** — it is a linker **plus** a large userspace translation layer for every Android HAL that Bionic binaries may call.

---

## 3. What mcpelauncher kept vs removed — primary source `mcpelauncher-linker` + `mcpelauncher-core`

### 3.1 Kept — the linker and minimal log

Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/CMakeLists.txt`:

```cmake
add_library(linker STATIC bionic/linker/rt.cpp bionic/linker/linker_gdb_support.cpp ... bionic/linker/linker.cpp bionic/linker/linker_soinfo.cpp bionic/linker/linker_relocate.cpp bionic/linker/linker_namespaces.cpp bionic/linker/linker_config.cpp ... public_include/mcpelauncher/linker.h src/linker.cpp bionic/libdl/libdl.cpp)
```

* **Kept:** the **recent AOSP Bionic linker** (API 29 era, `platform-tools-29.0.6-56-g081b55b1f` from `https://github.com/minecraft-linux/android_bionic` fork). Evidence: `bionic/.git` `git describe platform-tools-29.0.6-56-g081b55b1f`, `bionic/linker/linker.cpp` head contains `kDefaultLdPaths = {"/system/lib64","/odm/lib64","/vendor/lib64"}` + Apex paths.

* Also kept: `bionic/libc/dlfcn` via `src/linker.cpp` `solist_init()` + `load_library("libdl.so", get_dl_symbols())`, and `bionic/libm`/`bionic/libc` *as host-forwarding shims* (see §4), not as real built `bionic/libm` msun.

* Minimal hybris kept: `mcpelauncher-core/src/hybris_utils.cpp` (`HybrisUtils::loadLibrary`, `loadLibraryOS`, `stubSymbols`) + `src/hybris_android_log_hook.cpp` (`hookAndroidLog` → `linker::load_library("liblog.so", {__android_log_print …})`). That is **4 symbols** of `liblog.so`, not the full hybris.

### 3.2 Removed — almost everything else from `/tmp/libhybris`

Compared via `ls /tmp/libhybris/hybris` vs `ls mcpelauncher-manifest/mcpelauncher-linker` + `grep -r hybris mcpelauncher-client`:

* **Removed:** `hybris/camera`, `hybris/media`, `hybris/sf`, `hybris/hardware`, `hybris/hwc2`, `hybris/gralloc`, `hybris/vulkan`, `hybris/wifi`, `hybris/input`, `hybris/ui`, `hybris/libsync`, `hybris/properties` (full property service), `hybris/common/hooks_shm.h` shared-memory hooks, `hybris/common/wrapper_code_generic_arm.c` ARM trampolines, `compat/*` layers.

* **Removed linker version multiplexing:** upstream has `jb/mm/n/o/q` per-Android-version linkers; mcpelauncher vendors **single** `bionic/linker` (API 29) and ignores `HAS_ANDROID_*` selection.

* **Removed `HybrisUtils::loadLibrary` heavy path:** upstream `android_dlopen` goes through `hooks.c` `__hybris_get_hooked_symbol` (800 hooks) then AOSP linker. mcpelauncher's `HybrisUtils::loadLibrary(std::string path)` is thin: `linker::dlopen(PathHelper::findDataFile("libs/hybris/" + path))` — it expects prebuilt hybris `.so` under `libs/hybris/` but manifest provides **no** prebuilt hybris `.so` in repo; it is unused for the core game path (only `loadLibraryOS` host path is used). Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core/src/hybris_utils.cpp`.

* **Replaced `hooks.c` with `libc-shim`:** upstream 800 hooks in `hooks.c` are replaced by `libc-shim/` 16 C++ files (`shim::get_shimmed_symbols()`). `mcpelauncher-core/src/minecraft_utils.cpp` `getLibCSymbols()` iterates that shim instead of `hooks.c`. Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core/src/minecraft_utils.cpp` line 46.

Result: mcpelauncher is **not a hybris clone** — it is a **Bionic linker re-host** plus a **tiny host-forwarding shim** for `libm/libz/liblog`. It removed the general hybris HAL bridging because `libminecraftpe.so` needs only a subset.

---

## 4. MinecraftUtils::libm — why it exists, what it does

### 4.1 Code — primary source `mcpelauncher-core/src/minecraft_utils.cpp` lines 55-59

```cpp
void* MinecraftUtils::loadLibM() {
#ifdef __APPLE__   // Source: .../minecraft_utils.cpp line 55
    void* libmLib = HybrisUtils::loadLibraryOS("libm.so","libm.dylib",libm_symbols, {{"sincos",(void*)__sincos},{"sincosf",(void*)__sincosf}});
#elif defined(__FreeBSD__)
    void* libmLib = HybrisUtils::loadLibraryOS("libm.so","libm.so",libm_symbols);
#else
    void* libmLib = HybrisUtils::loadLibraryOS("libm.so","libm.so.6",libm_symbols);
#endif
    if(!libmLib) throw std::runtime_error("Failed to load libm");
    return libmLib;
}
```

`libm_symbols` is a null-terminated `const char*[]` of **298** Bionic `libm` symbols imported from `minecraft/imported/libm_symbols.h` (generated from `bionic/libm/libm.map.txt` `LIBC {global: 298; local: *;}`). Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libm/libm.map.txt` (`wc -l 309`, `grep introduced` 115 arch-gated).

### 4.2 Hybris bridge for libm — `HybrisUtils::loadLibraryOS`

Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core/src/hybris_utils.cpp`:

```cpp
void* HybrisUtils::loadLibraryOS(const char *name, const std::string &path, const char** symbols, unordered_map<string,void*> syms) {
    void* handle = dlopen(path.c_str(), RTLD_LAZY); // host dlopen
    Log::trace("Loaded OS library %s", path.c_str());
    for (int i=0; symbols[i]!=nullptr; ++i)
        if (void* p = dlsym(handle, symbols[i])) syms[symbols[i]] = p; // host dlsym
    linker::load_library(name, syms); // publish into Bionic namespace as Bionic SONAME
    return handle;
}
```

It **does not build real Bionic `libm` msun** — it host-`dlopen`s `libm.so.6` (glibc), `dlsym`s each Bionic name, and **publishes the host pointer under the Bionic SONAME** `libm.so` into the Bionic linker namespace. `DT_NEEDED libm.so` then resolves to host glibc math.

Bionic real `libm` (`bionic/libm/Android.bp` 216 `upstream-freebsd/lib/msun/src/*.c` + `builtins.cpp` `__builtin_fabs` + `signbit.cpp` `__signbit` + arch `x86_64/*.S` `ceil.S` etc.) is **vendored but not built** as a Bionic `libm.so` in this path — it is only used for its `libm.map.txt` allowlist.

### 4.3 Why separate `libm` is needed (Bionic vs glibc)

Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libm/libm.map.txt` `LIBC {global: 298; ... introduced=21/23}` vs glibc `libm.so.6` (`GLIBC_2.2.5` versions, `sincos` GNU extension). Bionic `libm` is FreeBSD `msun`; glibc `libm` is different implementation. Without shim, `DT_NEEDED libm.so` would resolve to host `libm.so.6` with wrong `DT_VERNEED` (`LIBC` vs `GLIBC_2.2.5`) and missing `sincos` on macOS (hence Apple extra `{"sincos", __sincos}`).

### 4.4 Bootstrap order — primary source `mcpelauncher-client/src/main.cpp` grep `MinecraftUtils::`

```
workaroundLocaleBug()                      // setenv LC_ALL=C
  libC = getLibCSymbols()                  // line 396 — collects shim::get_shimmed_symbols()
  ThreadMover::hookLibC(libC)               // line 397 — early pthread hook before Bionic load
  linker::load_library("libc.so", libC)    // line 436 — injects libc shim as Bionic libc
  loadLibM()                                // line 437 — host libm.so.6 → Bionic libm.so
  setupHybris()                             // line 439 — loadLibraryOS libz.so.1, hookAndroidLog, load stubs libOpenSLES etc.
  update_LD_LIBRARY_PATH(game lib/<abi>)   // line 446
  setupGLES2Symbols(eglGetProcAddress)      // line 464/518
  loadMinecraftLib(...)                     // line 507/527/536 — retry loop
  getLibraryBase(handle) -> base            // line 543
```

This order is the **real hybris contract**: `libC` must be injected **before** any Bionic `DT_NEEDED` resolves, otherwise first Bionic load would see empty `libc.so`.

---

## 5. Strong points of each side

### mcpelauncher linker (recent AOSP vendored)

* **Faithful AOSP:** `bionic/linker` handles `RELA`, `TLS`, `IFUNC`, `GNU hash` Bloom, versioned symbols exactly as Android 10. Stock `libminecraftpe.so` links without `DT_NEEDED` patching. Source: `bionic/linker/linker_relocate.cpp`, `linker_soinfo.cpp`.

* **Minimal public API:** `linker.h` 8 inline wrappers + `init/load_library/relocate/get_library_base` — caller address explicit, no TLS hack.

* **Apex/ld.config aware:** `bionic/apex/ld.config.txt` + `linker_config.cpp` namespace isolation matches Android, not host glibc `ld-linux` search.

### libhybris upstream

* **General HAL bridging:** `hooks.c` 800 hooks + `wrappers.c` ARM trampolines + `hybris/egl/media/camera/sf/hardware` per-HAL glue handle vendor blobs that Bionic shim alone cannot (vendor `libGLESv2.so` needs host EGL `ws.c` mapping, not just symbol forwarding).

* **Version multiplexing:** `mm/n/o/q` linkers per Android version — same host process can run JB vs Q Bionic binaries.

* **Property/shared-memory hooks:** `legacy_properties/properties.c`, `hooks_shm.c` bridge `__system_property_get`.

---

## 6. Weak points — why mcpelauncher's hybris is not universal

* **Global solist, no isolation:** `bionic/linker/linker.cpp` globals `g_soinfo_allocator`, `g_default_namespace` — `solist_init()` is once-per-process; second `init()` corrupts. No `Result<void>` — `DL_ERR` + null. Hard to unit-test `libprint_test.so` in-process. Source: `bionic/linker/linker.cpp`.

* **Silent missing `dlsym`:** `HybrisUtils::loadLibraryOS` does `if(ptr) syms[sym]=ptr` and drops otherwise; later Bionic relocation treats it as weak-def or `cannot locate symbol` at load time, not at injection time. No missing list.

* **Repeated `dlopen` leaks:** `loadLibraryOS` `dlopen("libm.so.6")` per call never `dlclose`; `HybrisUtils::loadLibrary` return value only kept for `libm`.

* **30-file `libc-shim` maintenance:** upstream `libc-shim/` reimplements `pthread` `TGKILL`, `__cxa_*`, `strlcpy`, fortified `__*_chk` — fragile vs glibc 2.38 `__isoc23_*`.

* **No tests for Bionic seam:** `natives/print_test` is trivial `__android_log_print("hello")` host-built, not Bionic `libprint_test.so` with `DT_NEEDED liblog.so` loaded via `linker::load_library`.

* **Removed hybris HALs:** mcpelauncher removed `camera/media/sf/hardware` — universal runtime that loads arbitrary vendor `.so` will hit `dlopen("libcamera.so")` → `linker::dlerror` because no shim exists.

---

## 7. Personalized proposal — universal runtime that bundles recent AOSP Bionic + replicates hybris correctly (no Flappy Bird constraints)

### 7.1 Goal

For your universal runtime, **do not shim like mcpelauncher.** Bundle the **real recent AOSP Bionic** tree you already vendor (`mcpelauncher-linker/bionic` at `platform-tools-29.0.6-56-g081b55b1f`) as built artifacts, and replicate **full hybris** only for the translation that Bionic actually needs.

### 7.2 Module map — vendor, don't shim

```
ffbird/  (universal runtime, Linux-only, C++11 foundation already: utils/logger/file-util)
├─ utils/               Result<T> INTERFACE (already fixed, 09ac236)
├─ bionic-linker/       STATIC liblinker.a — build mcpelauncher-linker/bionic/linker/*.cpp verbatim
│                        + src/linker.cpp wrapper (solist_init, load_library, get_library_base)
│                        PUBLIC public_include/mcpelauncher/linker.h
├─ bionic-libc/         SHARED libBionicLibc.so — build bionic/libc/*.c + bionic/libc/bionic/*.cpp
│                        from real AOSP sources (not host forward), version script bionic/libc/libc.map.txt
├─ bionic-libm/         SHARED libBionicLibm.so — build bionic/libm 216 msun sources + builtins.cpp
│                        + signbit.cpp + fake_long_double.c + arch/x86_64/*.S, map bionic/libm/libm.map.txt (298 syms)
├─ bionic-libdl/        SHARED libBionicLibdl.so — build bionic/libdl/libdl.cpp, libdl.map.txt
├─ hybris-bridge/       STATIC — replicate /tmp/libhybris/hybris/common subset:
│                        hooks.c (pthread/tgkill/locale), wrappers.c (ARM trampoline, x86_64 no-op),
│                        properties/legacy_properties, dlfcn.c (android_dlopen → linker::dlopen)
│                        NOT full HALs camera/media/sf unless universal needs them — add per-HAL on demand
├─ runtime-linux/       NativeLoader that wraps linker::load_library → utils::Result<void*>, linker::init → Result
│                        (once_flag guard, DL_ERR → failure string)
├─ natives/print_test/  NDK Bionic libprint_test.so with DT_NEEDED liblog.so + libm.so (integration test)
└─ tests/bionic-*       CTest per-Bionic-lib, fork-per-test to isolate global solist
```

Why **real Bionic libm** not host `libm.so.6`: host `libm.so.6` is glibc `GLIBC_2.2.5` with `sincos` GNU extension differing from FreeBSD `msun` `__signbit`/`ceil.S` arch overrides. Bundling real msun makes `readelf --dyn-syms` of Bionic binary match built `libm.so` exactly; hybris then only translates `pthread`/`errno`/`TLS` where Bionic vs glibc actually diverge.

### 7.3 hybris — replicate, don't reinvent, keep only what universal needs

* **Keep from `/tmp/libhybris/hybris/common`:** `hooks.c` (pthread `MUTEX/COND/RWLOCK` `0xFFFF` low-pointer detection, `SYS_futex` pulse, `hybris_check_android_shared_*`), `wrappers.c` `create_wrapper` (`mmap RWX`, ARM thumb fixup), `dso_handle_counters.cpp`, `legacy_properties/properties.c` (system_property), `logging.c` (`HYBRIS_DEBUG_LOG`), `sysconf.c`.

* **Keep per-HAL only on demand:** start with `hybris/common` + `hybris/egl/ws.c` + `hybris/properties` — add `media/camera/sf` only when a Bionic `.so` `DT_NEEDED` hits them (revealed by `linker::dlerror` `cannot locate symbol`).

* **Do NOT keep:** full `compat/` camera/media/surface_flinger (old JB/MM compat), `mm/n/o/q` linker multiplexing — your recent AOSP Bionic at Q (`platform-tools-29.0.6`) covers universal target (API 21-29). Single `bionic/linker` from mcpelauncher.

* **Implement `HybrisBridge` API on top of `utils::Result`:**

```cpp
namespace hybris_bridge {
  utils::Result<void> init() noexcept; // once_flag, linker::init, property init
  utils::Result<void*> loadHostLib(const char* bionicSoname, const char* hostPath, const char** allowlist) noexcept;
  // allowlist = libm_symbols 298, not silent skip — return failure with missing list
  utils::Result<void> publishAndroidLog() noexcept; // hookAndroidLog → linker::load_library("liblog.so", {4 syms})
}
```

Return `Result::failure(dlerror + missing vector)` instead of silent `if(ptr) insert` or `throw runtime_error`.

### 7.4 Error model — `utils::Result` everywhere

All linker/hybris APIs return `utils::Result<T>` (already `utils/include/utils/result.h`), not `throw` or `DL_ERR`. `HybrisUtils::loadLibraryOS` today throws only for `loadLibM`; universal must not throw — `loadHostLib` collects `missing: vector<string>` and fails.

### 7.5 Version scripts — verbatim AOSP, not minimal shim

* `bionic-libm` → `bionic/libm/libm.map.txt` `LIBC {global: 298; local: *;}` verbatim (full 298 symbols, not a minimal 6-symbol stub).
* `bionic-libc` → `bionic/libc/libc.map.txt` (1762 lines) verbatim.
* `bionic-libdl` → `bionic/libdl/libdl.map.txt`.

Host-forward shim would need per-sym `__asm__(".symver sin,sin@LIBC")`; real Bionic build needs no `symver` — it *is* `LIBC`.

### 7.6 Testing seam — universal

* `natives/print_test` NDK `libprint_test.so` (`DT_NEEDED liblog.so`, `libm.so`) is first Bionic binary that exercises `bionic-linker` + `hybris-bridge` + `bionic-libm` end-to-end (not host `__android_log_print`).

* Each `tests/bionic-*` is **forked per test** (global `solist` isolation) — `ctest -j` still parallel, but each binary does `hybris_bridge::init(); loadLibrary("liblog.so"); loadLibrary("libprint_test.so"); dlsym; call`.

---

## 8. Risks and next decisions

* **Global solist** → Option A `once_flag` guard + fork-per-test (fast), Option B per-instance `android_namespace_t` (requires patching `bionic/linker/*.cpp` `solist_get_head` → instance field, drift from AOSP) — defer B until A insufficient.

* **Multilib `libm.so.6 → libm.so` fallback:** host fallback may pick 32-bit `libm.so` in cross builds; check `CMAKE_SIZEOF_VOID_P`. Not needed if you build real `bionic-libm` msun — fallback disappears.

* **UPSTREAM hybris HAL drift:** `/tmp/libhybris` has `mm/n/o/q` per-version hooks; your single Q linker may miss pre-Q `__aeabi_read_tp` etc. — add `jb/` hooks only if a Bionic binary needs them (detect via `readelf --notes` + `dlerror`).

* **GPL:** `bionic` (Apache-2.0) + `mcpelauncher-linker` (GPL-3.0) + `libhybris` (Apache-2.0) into `ffbird` (GPL-3.0) compatible, keep `LICENSE`/`NOTICE`.

---

## 9. Sources cited

* `/tmp/libhybris` (`README.md`, `hybris/common/Makefile.am`, `hybris/common/hooks.c`, `hybris/common/wrappers.c`, `hybris/common/n/linker.cpp`, `hybris/egl/egl.c`)
* `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/public_include/mcpelauncher/linker.h`
* `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/src/linker.cpp`
* `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/CMakeLists.txt`
* `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker.cpp`
* `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker_soinfo.cpp`
* `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libm/Android.bp`, `libm.map.txt` (309 lines, 298 globals), `builtins.cpp`, `signbit.cpp`, `fake_long_double.c`, `bionic/libm/x86_64/*.S`, `bionic/libc/libc.map.txt`, `bionic/libdl/libdl.map.txt`, `bionic/apex/ld.config.txt`, `bionic/apex/manifest.json`, `bionic/.git` (`git describe platform-tools-29.0.6-56-g081b55b1f`)
* `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core/include/mcpelauncher/minecraft_utils.h`, `src/minecraft_utils.cpp` (`loadLibM`, `getLibCSymbols`, `setupHybris`, `loadMinecraftLib`), `include/mcpelauncher/hybris_utils.h`, `src/hybris_utils.cpp`, `src/hybris_android_log_hook.cpp`
* `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-client/src/main.cpp` (bootstrap order)
* `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/minecraft-imported-symbols/include/minecraft/imported/libm_symbols.h`
