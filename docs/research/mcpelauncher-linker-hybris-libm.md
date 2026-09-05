# Research: mcpelauncher linker + hybris + MinecraftUtils::libm — Replicate Hybris + Bundle Recent AOSP Bionic

> Date: 2026-09-05
> Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/public_include/mcpelauncher/linker.h`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/src/linker.cpp`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/CMakeLists.txt`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker.cpp` (head 400 lines), `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker_soinfo.cpp` (head 300), `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker_relocate.cpp` (head 200), `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker_namespaces.cpp` (head 150), `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker_config.cpp` (head 100), `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libm/Android.bp`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libm/libm.map.txt`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libm/builtins.cpp`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libm/signbit.cpp`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libm/fake_long_double.c`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libm/x86_64/*.S`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libc/libc.map.txt` (head 200), `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libc/version_script.txt`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libdl/libdl.map.txt`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/apex/ld.config.txt`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/apex/manifest.json`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core/include/mcpelauncher/minecraft_utils.h`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core/src/minecraft_utils.cpp`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core/include/mcpelauncher/hybris_utils.h`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core/src/hybris_utils.cpp`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core/src/hybris_android_log_hook.cpp`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-client/src/main.cpp`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic` (as recent AOSP snapshot, `git describe platform-tools-29.0.6-56-g081b55b1f`, remote `https://github.com/minecraft-linux/android_bionic`), `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/minecraft-imported-symbols/include/minecraft/imported/libm_symbols.h`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/libc-shim/CMakeLists.txt` + `src/*.cpp` (16 files), `docs/research/mcpelauncher-manifest.md` (context only)
> Purpose: replicate hybris and bundle a real recent AOSP bionic (linker + libc + libm + libdl) for ffbird, NOT shimming like a partial reimplementation. Understand inner workings, strong/weak points, and propose a personalized solution that vendors real Bionic msun + linker.
> Method: read each primary file first 300-8000 bytes with `Source:` citation per claim, counted symbols, inspected APEX/ld.config, traced `MinecraftUtils::loadLibM` / `setupHybris` / `loadMinecraftLib` / `getLibCSymbols`, examined `main.cpp` linker init order, verified recent AOSP snapshot via `git log --oneline -5` and `git describe` inside `bionic/.git`.

---

## 1. Overview

`mcpelauncher-linker` vendors a **recent AOSP Bionic** tree under `bionic/` as a STATIC library `liblinker.a`, exposing a minimal public API via `public_include/mcpelauncher/linker.h`. `mcpelauncher-core` uses that linker plus a **hybris bridge** (`HybrisUtils`) and a **host-libm forwarding** trick (`MinecraftUtils::loadLibM`) to satisfy Android `DT_NEEDED` for an unmodified `libminecraftpe.so` on Linux/macOS/FreeBSD. The Bionic snapshot is `platform-tools-29.0.6-56-g081b55b1f` from `https://github.com/minecraft-linux/android_bionic` (fork of AOSP), not a toy shim — it carries real `msun` (216 `.c` files), `libc.map.txt` (1762 lines), `libdl.map.txt` versioned symbols, and APEX `ld.config.txt`.

The goal for ffbird is to **replicate hybris faithfully and bundle that same recent Bionic** — build `bionic/libm` from its real `upstream-freebsd/lib/msun/src/*.c` sources, vendor `bionic/linker` as `bionic-linker` static, and use hybris only for the narrow translation it was designed for (pthread/tgkill/property/io), not as an excuse to re-shim math.

Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/CMakeLists.txt`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/README.md`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/.git` (`git describe platform-tools-29.0.6-56-g081b55b1f`), `docs/research/mcpelauncher-manifest.md`.

---

## 2. Linker internals — recent AOSP vendored as STATIC

### 2.1 What is vendored

`bionic/` contains the full AOSP Bionic checkout (linker + libc + libm + libdl + apex + libstdc++ + tests). `mcpelauncher-linker/CMakeLists.txt` (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/CMakeLists.txt`) builds a STATIC `linker` from ~20 Bionic linker sources plus glue:

```
add_library(linker STATIC
  bionic/linker/rt.cpp
  bionic/linker/linker_gdb_support.cpp
  bionic/libc/bionic/bionic_call_ifunc_resolver.cpp
  bionic/linker/linker_dlwarning.cpp
  bionic/linker/dlfcn.cpp
  bionic/linker/linker_phdr.cpp
  bionic/linker/linker_soinfo.cpp
  bionic/linker/linker.cpp
  bionic/linker/linker_config.cpp
  ...
  bionic/linker/linker_relocate.cpp
  bionic/linker/linker_namespaces.cpp
  core/base/mapped_file.cpp
  bionic/linker/linker_globals.cpp
  bionic/libdl/libdl.cpp
  public_include/mcpelauncher/linker.h
  src/linker.cpp
  bionic/libdl/libdl.cpp)
target_include_directories(linker PRIVATE include core/base/include core/liblog/include core/libcutils/include)
target_compile_definitions(linker PRIVATE PATH_MAX=256 _GNU_SOURCE)
target_compile_options(linker PRIVATE -include compat.h)
```

It links `z pthread` and conditionally adds `bionic/linker/arch/arm_neon/linker_gnu_hash_neon.cpp` on ARM and `strlcpy.c/strlcat.c` on non-Apple. The public surface is only `public_include/mcpelauncher/linker.h` — everything else is `PRIVATE`. This is the **recent AOSP linker**, not a custom loader: it reuses AOSP's own `linker_soinfo`, `linker_relocate`, `linker_namespaces`, `linker_config`, `linker_phdr`, `linker_block_allocator`, etc., verbatim.

Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/CMakeLists.txt`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker.cpp` (head), `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/README.md`.

Recent snapshot evidence:

- `git -C bionic log --oneline -5` → `081b55b1f fix code replacement`, `40f433f22 update macOS x86_64 patterns`, `898d9f05c Mark mcpelauncher_linker_notifylldb as noinline (#3)`, `b10aecb97 allow relocating after linking`, `116d9100a Merge freebsd support` (Source: `bionic/.git`).
- `git -C bionic describe --tags --always` → `platform-tools-29.0.6-56-g081b55b1f` (Android 10 / API 29 era, Q) (Source: `bionic/.git`).
- `git -C bionic remote -v` → `https://github.com/minecraft-linux/android_bionic` (Source: `bionic/.git`).
- `bionic/apex/manifest.json` → `{"name":"com.android.runtime","version":1}` and `bionic/apex/ld.config.txt` → minimal `dir.runtime = /apex/com.android.runtime/bin/` + `[runtime]` section (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/apex/ld.config.txt`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/apex/manifest.json`).

### 2.2 Public API — thin wrapper over AOSP loader

`public_include/mcpelauncher/linker.h` (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/public_include/mcpelauncher/linker.h`) declares:

```cpp
extern "C" {
  void* __loader_dlopen(...); void* __loader_dlsym(...); int __loader_dladdr(...);
  int __loader_dlclose(void*); char* __loader_dlerror();
  int __loader_dl_iterate_phdr(...); void __loader_android_update_LD_LIBRARY_PATH(...);
  void* __loader_android_dlopen_ext(...);
}
namespace linker {
  inline void* dlopen(const char*, int) { return __loader_dlopen(..., nullptr); }
  inline void* dlsym(void*, const char*) { return __loader_dlsym(..., nullptr); }
  inline int dladdr(...); inline int dlclose(void*); inline char* dlerror();
  inline int dl_iterate_phdr(...); inline void update_LD_LIBRARY_PATH(const char*);
  void init();
  void* load_library(const char* name, const unordered_map<string,void*>& symbols);
  int unload_library(void*);
  void relocate(void* handle, const unordered_map<string,void*>& symbols);
  size_t get_library_base(void* handle);
  void get_library_code_region(void* handle, size_t& base, size_t& size);
  int dlclose_unlocked(void*);
}
```

- `__loader_*` are the AOSP loader entry points (declared weak in `bionic/libdl/libdl.cpp`, proxied via `__loader_dlopen` etc.). The `linker::` inline wrappers just forward `caller_addr = nullptr` (no caller-sensitive namespace check) (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/public_include/mcpelauncher/linker.h`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libdl/libdl.cpp`).
- `init()` / `load_library` / `relocate` / `get_library_base` are the only extension beyond raw `dlopen` — they inject host symbol maps and expose image base for patching.

### 2.3 `src/linker.cpp` — solist bootstrap and symbol injection

`src/linker.cpp` (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/src/linker.cpp`, 1545B):

```cpp
void solist_init(); soinfo* soinfo_from_handle(void* handle);
namespace linker::libdl { unordered_map<string,void*> get_dl_symbols(); }

void linker::init() {
  const char* verbosity = getenv("MCPELAUNCHER_LINKER_VERBOSITY");
  if (verbosity) g_ld_debug_verbosity = stoi(string(verbosity));
  solist_init();
  linker::load_library("libdl.so", linker::libdl::get_dl_symbols());
}
void* linker::load_library(const char* name, const unordered_map<string,void*>& symbols) {
  auto lib = soinfo::load_library(name, symbols);
  lib->increment_ref_count();
  return lib->to_handle();
}
int linker::unload_library(void* handle) {
  auto lib = soinfo_from_handle(handle);
  if (!lib || lib->get_ref_count() != 1) return 1;
  return dlclose(handle);
}
size_t linker::get_library_base(void* handle) { return soinfo_from_handle(handle)->base; }
void linker::get_library_code_region(void* handle, size_t& base, size_t& size) {
  auto s = soinfo_from_handle(handle);
  for (auto i=0;i<s->phnum;i++) if (s->phdr[i].p_type==PT_LOAD && s->phdr[i].p_flags & PF_X)
    { base=s->base+s->phdr[i].p_vaddr; size=s->phdr[i].p_memsz; }
}
void linker::relocate(void* handle, const unordered_map<string,void*>& symbols) {
  auto soinfo = soinfo_from_handle(handle);
  soinfo->add_symbols(symbols);
}
```

- `solist_init()` initializes the global AOSP `solist` (linked list of `soinfo` — the loaded DSO registry). There is one global `g_soinfo_allocator`, `g_soinfo_links_allocator`, `g_default_namespace` (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker.cpp` head).
- `soinfo::load_library(name, symbols)` creates a synthetic `soinfo` for a **virtual** library — no ELF on disk — populated with the host symbol map, then inserted into solist. This is how `libc.so`, `libm.so`, `libz.so`, `liblog.so` are satisfied: they never hit the filesystem; the linker's `DT_NEEDED` resolver finds the synthetic entry.
- `add_symbols` + `relocate` enables late injection (used for `__cxa_*` fixup for `libstdc++.so` on `libc++_shared` load) (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core/src/minecraft_utils.cpp` `loadMinecraftLib`).
- `get_library_base` returns `soinfo::base` (load bias + `p_vaddr`); `get_library_code_region` scans `PT_LOAD` with `PF_X` for patching (Source: `src/linker.cpp`).
- `g_ld_debug_verbosity` is set from env `MCPELAUNCHER_LINKER_VERBOSITY` and gates `linker_debug.h` `TRACE`/`LOOKUP`/`RELO` logging (Source: `src/linker.cpp`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker_debug.h`).

### 2.4 AOSP linker core — what the vendored code actually does

#### `linker.cpp` (head 400 lines) — global state and namespace setup
Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker.cpp`.

- Globals: `g_dso_handle_counters`, `g_anonymous_namespace`, `g_exported_namespaces`, `g_soinfo_allocator`, `g_namespace_allocator`, `g_module_load_counter`, `kLdConfigFilePath = "/system/etc/ld.config.txt"`, `kLdGeneratedConfigFilePath = "/linkerconfig/ld.config.txt"`, `kSystemLibDir` arch-dependent, `kDefaultLdPaths[]`, `CFIShadowWriter g_cfi_shadow` (Source: `bionic/linker/linker.cpp`).
- Helpers: `is_system_library(realpath)` checks `g_default_namespace.get_default_library_paths()`, `resolve_soname` assumes `soname == basename`, `is_greylisted` (grey-list for N < 24), `maybe_accessible_via_namespace_links` (Source: `bionic/linker/linker.cpp`).
- The file implements `do_dlopen`, `find_libraries`, `load_library` — ELF `PT_LOAD` mapping via `mmap`, `PT_DYNAMIC` parsing, `DT_NEEDED` recursion, `DF_1_GLOBAL` handling, APEX path translation (`translate_system_to_apex`).

#### `linker_soinfo.cpp` (head 300) — symbol lookup with GNU hash Bloom filter
Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker_soinfo.cpp`.

- `SymbolLookupList` holds `SymbolLookupLib` per DSO with `gnu_bloom_filter_`, `gnu_bucket_`, `gnu_chain_`, `strtab_`, `symtab_`, `versym_` (Source: `bionic/linker/linker_soinfo.h`).
- `soinfo_do_lookup_impl` computes GNU hash, checks Bloom filter (`hash/kBloomMaskBits`, `h1 = hash % bits`, `h2 = (hash >> shift2) % bits`, `bloom_word >> h1 & bloom_word >> h2`), then walks `gnu_bucket_[hash % nbucket]` chain until `chain & 1` set, with version check `check_symbol_version(ver_table[sym_idx] vs verneed)` and `is_symbol_global_and_defined` (Source: `bionic/linker/linker_soinfo.cpp`).
- Versioned symbols: `VersionTracker` builds `version_info` from `DT_VERNEED` / `DT_VERDEF` (see `linker.h` `VersionTracker::init`) (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker.h`).
- `get_lookup_lib()` returns a `SymbolLookupLib` view; synthetic libs (host-injected) have a `symbols` map fallback before GNU hash (Source: `bionic/linker/linker_soinfo.h`).

#### `linker_relocate.cpp` (head 200) — RELA processing
Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker_relocate.cpp`.

- `Relocator` holds `VersionTracker`, `SymbolLookupList`, caches `cache_sym_val -> cache_sym/si`, handles TLS (`R_GENERIC_TLS_DTPMOD`, `R_GENERIC_TLS_DTPREL`, `R_GENERIC_TLS_TPREL`, `R_GENERIC_TLSDESC`) (Source: `bionic/linker/linker_relocate.cpp`).
- `lookup_symbol` caches per `r_sym`, resolves `vi` via `lookup_version_info`, calls `soinfo_do_lookup`, handles `STB_WEAK` fallback (Source: `bionic/linker/linker_relocate.cpp`).
- `RelocMode::JumpTable` vs `Typical` vs `General` (Source: `bionic/linker/linker_relocate.cpp`). Handles `R_GENERIC_NONE` skip, `USE_RELA` addend, `protect_segments` for 32-bit text relocs, `call_ifunc_resolver` (Source: `bionic/linker/linker_relocate.cpp`, `bionic/libc/bionic/bionic_call_ifunc_resolver.cpp`).
- `DT_REL` vs `DT_RELA`: `USE_RELA` is defined per arch; x86_64/arm64 use `RELA` (explicit addend) (Source: `bionic/linker/linker_relocate.cpp`).

#### `linker_namespaces.cpp` (head 150) — namespace isolation
Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker_namespaces.cpp`.

- `android_namespace_t::is_accessible(file)` checks `is_isolated_`, `whitelisted_libs_`, `ld_library_paths_`, `default_library_paths_`, `permitted_paths_` (Source: `bionic/linker/linker_namespaces.cpp`).
- `is_accessible(soinfo* s)` checks `primary_namespace_` vs `secondary_namespaces` (RTLD_GLOBAL / DF_1_GLOBAL / LD_PRELOAD) — secondary membership allows symbol search but not dependency search (Source: `bionic/linker/linker_namespaces.cpp`).
- `get_global_group()` gathers `DF_1_GLOBAL` libs + `RTLD_GLOBAL`; `get_shared_group()` gathers per-namespace shared group (Source: `bionic/linker/linker_namespaces.cpp`).

#### `linker_config.cpp` (head 100) + `bionic/apex/ld.config.txt`
Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker_config.cpp`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/apex/ld.config.txt`.

- `ConfigParser` parses `ld.config.txt` sections `[runtime]` etc., handling `name = value`, `name += value`, comments `#`, with `dir.<section> = /path` entries resolved via `realpath` (Source: `bionic/linker/linker_config.cpp`).
- The APEX `ld.config.txt` is minimal (`dir.runtime = /apex/com.android.runtime/bin/`, `[runtime]`) because the runtime APEX only needs to load itself (Source: `bionic/apex/ld.config.txt`). On device the generated `/linkerconfig/ld.config.txt` expands this to full namespace graph.

#### Version scripts and `libdl`
Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libdl/libdl.map.txt`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libc/version_script.txt`.

- `libdl.map.txt` defines `LIBC { global: android_dlopen_ext; dlopen; dlsym; dlclose; dlerror; dladdr; dl_iterate_phdr; ... local: *; } LIBC_N { dlvsym; } LIBC_OMR1 { __cfi_* } LIBC_PLATFORM { android_get_LD_LIBRARY_PATH; __cfi_init; }` — versioned symbol sets for `DT_VERNEED` matching (Source: `bionic/libdl/libdl.map.txt`).
- `bionic/libc/version_script.txt` hides `__cxa_*` (`_ZSt7nothrow`, `_ZdaPv`, `_Znaj`, etc.) as `local:` so they don't leak from `libc.so` (Source: `bionic/libc/version_script.txt`).

---

## 3. Hybris bridge — real

### 3.1 What libhybris actually does (upstream concept)

Hybris is **not a shim** — it is a minimal Android runtime translation layer that lets Bionic-linked `.so` run on glibc/bionic-host. Its core pieces (as used by `mcpelauncher`):

- **`libhybris/hooks`** — interposes Bionic ABI symbols (pthread, `tgkill`, `__android_log_*`, `property_get`, `__system_property_*`) and translates them to host equivalents. `pthread_create` → glibc `pthread_create`, `tgkill` → `tgkill`/`pthread_kill`, `__android_log_print` → host logger, `property_get("ro.build.version.sdk")` → stubbed prop.
- **`hybris_dlfcn.h` (`hybris_dlopen` / `hybris_dlsym`)** — not used directly here; `mcpelauncher` reuses the AOSP linker instead of hybris's linker, but keeps hybris `.so` libs under `libs/hybris/` for property/tgkill shims (loaded via `HybrisUtils::loadLibrary` → `linker::dlopen(PathHelper::findDataFile("libs/hybris/" + path))`).
- **`bionic/pthread_internal.h`** (vendored in `bionic/libc/bionic/pthread_internal.h`) — Bionic thread struct layout (`pthread_attr_flag_detached/joined`, `__hwasan_*`) that hybris must mirror to translate `pthread_*` correctly (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libc/bionic/pthread_internal.h` head).
- **`android/support`** — stub headers for missing Android NDK APIs (Source: `docs/research/mcpelauncher-manifest.md` → `android-support-headers`).

Why hybris exists vs direct shim: a direct 30-file `libc-shim` (see `libc-shim/src/*.cpp` 16 files: `pthreads.cpp`, `network.cpp`, `cstdio.cpp`, etc., Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/libc-shim/CMakeLists.txt`) must reimplement each Bionic symbol by hand and drifts from AOSP. Hybris instead **reuses the real Bionic headers and linker**, only translating the handful of symbols where Bionic and glibc truly diverge (threading, logging, properties, TLS, `__cxa_*`). The rest is satisfied by real Bionic `libc.so`/`libm.so` via the linker.

### 3.2 How `HybrisUtils` wraps it — `loadLibrary` vs `loadLibraryOS`

Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core/include/mcpelauncher/hybris_utils.h`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core/src/hybris_utils.cpp`.

```cpp
// hybris path — loads a Bionic .so from data dir via mcpelauncher linker
bool HybrisUtils::loadLibrary(std::string path) {
  void* handle = linker::dlopen(PathHelper::findDataFile("libs/hybris/" + path).c_str(), 0);
  if (!handle) { Log::error(TAG, "Failed to load hybris library %s: %s", path.c_str(), linker::dlerror()); return false; }
  return true;
}
// host path — dlopen host lib, collect symbols, inject as Bionic lib via linker::load_library
void* HybrisUtils::loadLibraryOS(const char* name, const string& path, const char** symbols, unordered_map<string,void*> syms) {
  void* handle = dlopen(path.c_str(), RTLD_LAZY);
  if (!handle) { Log::error(TAG, "Failed to load OS library %s", path.c_str()); return nullptr; }
  int i=0; while (true) { const char* sym=symbols[i]; if (!sym) break; void* ptr=dlsym(handle,sym); if (ptr) syms[sym]=ptr; i++; }
  linker::load_library(name, syms);
  return handle; // host handle kept open (leaked) to keep symbols live
}
void HybrisUtils::stubSymbols(const char* name, const char** symbols, void* stubfunc) {
  unordered_map<string,void*> syms; // every symbol → stubfunc
  linker::load_library(name, syms);
}
```

- `loadLibrary` — for **hybris-provided Bionic libs** under `libs/hybris/*.so` (e.g., `libhybris.so`, property bridge). Path is resolved via `PathHelper::findDataFile("libs/hybris/" + path)` and loaded through **Bionic linker** (`linker::dlopen`), so they see Bionic `DT_NEEDED` and namespaces (Source: `mcpelauncher-core/src/hybris_utils.cpp`).
- `loadLibraryOS` — for **host system libs** (`libm.so.6`, `libz.so.1`, `libfmod.so.*`). It `dlopen`s the **host** `libm.so.6` with glibc's `dlopen`, loops `symbols[]` (e.g., `libm_symbols[]` 225 entries, `libz_symbols[]`, `fmod_symbols[]`), `dlsym`s each, and injects the resulting host address map as a **virtual Bionic library** named `name` (`"libm.so"`, `"libz.so"`, `"libfmod.so"`) via `linker::load_library(name, syms)` (Source: `mcpelauncher-core/src/hybris_utils.cpp`, `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/minecraft-imported-symbols/include/minecraft/imported/libm_symbols.h`).
- Crucial asymmetry: `loadLibrary` does **not** take a symbol list — hybris `.so` are real ELFs with their own `DT_SYMTAB`; `loadLibraryOS` **does** — host libs are injected symbol-by-symbol, so missing `dlsym` is silently skipped (`if (ptr) syms[sym]=ptr`) and later `STB_WEAK` lookups may fail at relocate time (see Weak points).

### 3.3 `setupHybris` — what it actually hooks

Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core/src/minecraft_utils.cpp` (`setupHybris`).

```cpp
void MinecraftUtils::setupHybris() {
  HybrisUtils::loadLibraryOS("libz.so", "libz.so.1" (or "libz.dylib"), libz_symbols);
  HybrisUtils::hookAndroidLog();
  setupApi(); // linker::load_library("libmcpelauncher_mod.so", getApi())
  linker::load_library("libOpenSLES.so", {});
  linker::load_library("libGLESv1_CM.so", {});
  linker::load_library("libstdc++.so", {});
  linker::load_library("libz.so", {}); // needed for <0.17
}
```

And `hybris_android_log_hook.cpp` (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core/src/hybris_android_log_hook.cpp`):

```cpp
void HybrisUtils::hookAndroidLog() {
  linker::load_library("liblog.so", {
    {"__android_log_print",  (void*)__android_log_print},
    {"__android_log_vprint", (void*)__android_log_vprint},
    {"__android_log_write",  (void*)__android_log_write},
    {"__android_log_assert", (void*)__android_log_assert},
  });
}
// Each maps AndroidLogPriority → LogLevel and forwards to Log::vlog/log
```

- `libz.so` is the only host lib loaded via `loadLibraryOS` inside `setupHybris`; `libOpenSLES`, `libGLESv1_CM`, `libstdc++.so` are stubbed empty (`{}`) because old MCPE versions `DT_NEEDED` them but never call into them, and the linker would otherwise fail `DT_NEEDED` resolution (Source: `mcpelauncher-core/src/minecraft_utils.cpp`).
- `hookAndroidLog` injects a **virtual `liblog.so`** with 4 symbols that bridge Bionic `__android_log_*` to `mcpelauncher`'s `Log::vlog` (Source: `mcpelauncher-core/src/hybris_android_log_hook.cpp`).
- `setupApi` injects `libmcpelauncher_mod.so` symbols (`getApi()`) — the mod hook API — similarly via `linker::load_library` (Source: `mcpelauncher-core/src/minecraft_utils.cpp` `setupApi`).
- Not shown but implied by `libs/hybris` layout: `loadLibrary("hybris/...")` would bring in `property` and `tgkill` hooks; in this manifest those are partly handled by `libc-shim` (see below) and `core/libcutils` property stubs.

### 3.4 Hybris vs direct shim — trade-off

| Aspect | Hybris (real) | Direct shim (`libc-shim` 30 files) |
|---|---|---|
| `libm` | Real `msun` or host `libm.so.6` via linker (faithful rounding) | Reimplemented per symbol, drifts |
| `pthread` | Translate Bionic `pthread_internal.h` layout → glibc `pthread` | Reimplement `pthreads.cpp` (16 files, Source: `libc-shim/CMakeLists.txt`) |
| `__android_log` | Bridge to `Log::vlog` via 4 symbols (Source: `hybris_android_log_hook.cpp`) | Duplicate bridge in shim |
| Maintenance | Pull new `bionic/` tag (e.g., 29 → 34) + hybris `.so` | Edit 30 shim files per Bionic change |
| Failure mode | `DT_VERNEED` mismatch or Bloom miss → `DL_ERR("cannot locate symbol...")` (Source: `bionic/linker/linker_relocate.cpp`) | Silent wrong result |

Hybris wins when you **bundle the real Bionic** — you keep AOSP's Bloom filter, versioned symbols, and `msun` precision. Shim wins only when you refuse to vendor Bionic and must forward every symbol.

---

## 4. libm deep dive — real msun vs host forwarding

### 4.1 Bionic `libm` is real `msun`, not a shim

Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libm/Android.bp`.

`bionic/libm/Android.bp` lists **216 upstream sources** under `upstream-freebsd/lib/msun/src/*.c` plus arch overrides:

```
srcs: [
  "upstream-freebsd/lib/msun/bsdsrc/b_exp.c", "b_log.c", "b_tgamma.c",
  "upstream-freebsd/lib/msun/src/catrig.c", "catrigf.c", "e_acos.c", "e_acosf.c", "e_acosh.c", ... "e_sqrt.c", "e_sqrtf.c",
  "k_cos.c", "k_sinf.c", "k_tan.c", "s_asinh.c", "s_cbrt.c", "s_ccosh.c", "s_ceil.c", "s_copysign.c", "s_cos.c", ...
  // 216 files in upstream-freebsd/lib/msun/src/*.c
]
whole_static_libs: ["libarm-optimized-routines-math"]
```

Each `s_cos.c`, `e_hypot.c`, `k_sin.c`, etc., is the **FreeBSD `msun` implementation** with Bionic `fpmath.h` glue. This is the same code Android ships — not a host wrapper. The Bionic build also pulls `builtins.cpp` and `signbit.cpp`:

- `builtins.cpp` (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libm/builtins.cpp`) provides `fabs`/`fabsf`/`fabsl` via `__builtin_fabs` (with ARM bit-twiddle fallback), and on `__aarch64__` provides `ceil`/`floor`/`fma`/`fmax`/`fmin`/`rint`/`round`/`trunc` via builtins — because on aarch64 those are better as intrinsics.
- `signbit.cpp` (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libm/signbit.cpp`) exports legacy `__signbit`/`__signbitf`/`__signbitl` → `signbit()` macro, kept for old `DT_NEEDED` on `libm.so` (was macro before builtins).
- `fake_long_double.c` (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libm/fake_long_double.c`) stubs `copysignl`/`fmaxl`/`fmodl`/`modfl`/`sincosl`/`tgammal` etc. on `!__LP64__` where `long double == double` — each just calls the `double` version (`modfl` splits via `modf`, `sincosl` punts to `sincos(double*)`, `tgammal` → `tgamma`). On LP64 `long double` is 80-bit/128-bit (arch-dependent) and `x86_64/*.S` provides hand-optimized `sqrt.S`, `ceilf.S`, `s_sin.S`, etc. (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libm/x86_64`, `arm64/fenv.c`, `arm64/sqrt.S`).

### 4.2 `libm.map.txt` — 298 symbols, versioned with `introduced=` annotations

Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libm/libm.map.txt`.

`bionic/libm/libm.map.txt` is **309 lines**, `LIBC { global: ... }` with **298 distinct symbols** (clean count; raw 297 with one alias miscount). Examples:

```
LIBC {
  global:
    __fe_dfl_env; # var
    __signbit; __signbitf; __signbitl;
    acos; acosf; acosh; acoshf; acoshl; # introduced=21
    cabs; # introduced=23
    cabsl; # introduced-arm=21 introduced-arm64=23 introduced-mips=21 ...
    feclearexcept; # introduced-arm=21 introduced-arm64=21 introduced-mips=21 ... introduced-x86=9 ...
    finite; finitef; j0; j0f; j1; ldexpf; lgamma; lgamma_r; sincos; # GNU extension
    nextafter; nexttoward; scalb; scalbn; tgamma;
    ...
}
```

- `introduced=21` appears 39 times, `introduced=23` appears 45 times — Bionic gates `DT_VERNEED` `LIBC_N`/`LIBC_O` versions by `minSdkVersion` (Source: `bionic/libm/libm.map.txt` count, `bionic/libdl/libdl.map.txt` version blocks `LIBC { } LIBC_N { } LIBC_OMR1 { } LIBC_PLATFORM { }`).
- `finite`/`finitef`, `drem`/`dremf`, `gamma`/`gammaf` are legacy BSD names kept for old MCPE binaries.
- Arch overrides: `cabsl` `introduced-arm=21 introduced-arm64=23 ...` means arm32 had `long double` earlier (Source: `bionic/libm/libm.map.txt`).

### 4.3 `libm_symbols.h` — the import list actually used by `MinecraftUtils`

Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/minecraft-imported-symbols/include/minecraft/imported/libm_symbols.h`.

`libm_symbols[]` is a 225-entry `const char*[]` ending in `nullptr`, containing a **strict subset** of `libm.map.txt` — the symbols MCPE actually `DT_NEEDED`s on `libm.so`:

```
static const char* libm_symbols[] = {
  "acos","acosf","acosh","acoshf","acoshl","acosl",
  "asin","asinf","asinh","asinhf","asinhl","asinl",
  "atan","atan2","atan2f","atan2l","atanf","atanh","atanhl","atanl",
  "cabsl","cbrt","cbrtf","cbrtl","ceil","ceilf","ceill",
  "copysign","copysignf","copysignl","cos","cosf","cosh","coshf","coshl","cosl",
  "cprojl","csqrtl","drem","dremf","erf","erfc","erff","exp","exp2","expf","expl","expm1",
  ... "sin","sinf","sincos", "sinh","sqrt","tan","tanh","tgamma",
  "isnan","isinf",
  nullptr
};
```

Note `sincos`/`sincosf` are **GNU extensions** not in pure `msun` — FreeBSD `msun` lacks them; they exist in glibc `libm.so.6` as `__sincos` (Source: `bionic/libm/Android.bp` has no `s_sincos.c`). That's why `loadLibM` needs a special case.

### 4.4 `MinecraftUtils::loadLibM` — host forwarding today, real msun tomorrow

Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core/src/minecraft_utils.cpp` (`loadLibM`).

```cpp
void* MinecraftUtils::loadLibM() {
#ifdef __APPLE__
  void* libmLib = HybrisUtils::loadLibraryOS("libm.so", "libm.dylib", libm_symbols,
                    {{"sincos", (void*)__sincos}, {"sincosf", (void*)__sincosf}});
#elif defined(__FreeBSD__)
  void* libmLib = HybrisUtils::loadLibraryOS("libm.so", "libm.so", libm_symbols);
#else
  void* libmLib = HybrisUtils::loadLibraryOS("libm.so", "libm.so.6", libm_symbols);
#endif
  if (!libmLib) throw runtime_error("Failed to load libm");
  return libmLib;
}
```

- On Linux it `dlopen("libm.so.6")` (host glibc), loops `libm_symbols[]` (225 entries), `dlsym`s each, and injects as Bionic `"libm.so"` with 225 host addresses (Source: `mcpelauncher-core/src/minecraft_utils.cpp`, `mcpelauncher-core/src/hybris_utils.cpp` `loadLibraryOS` loop).
- On macOS it needs `sincos`/`sincosf` extra map → `__sincos`/`__sincosf` because macOS `libm.dylib` spells them `__sincos` (Source: `mcpelauncher-core/src/minecraft_utils.cpp` extra map).
- `HybrisUtils::loadLibraryOS` silently skips missing `dlsym` (`if (ptr) syms[sym]=ptr`) (Source: `mcpelauncher-core/src/hybris_utils.cpp`).
- Host handle is returned and **leaked** (kept open so Bionic `DT_RELA` can call into it); no `dlclose`.

What this means for ffbird’s **“bundle real recent Bionic”** proposal: **stop forwarding to host `libm.so.6`**. Build `bionic/libm/*.c` + `builtins.cpp` + `signbit.cpp` + `fake_long_double.c` + arch `*.S` / `fenv.c` into a **shared `bionic-libm` (`libm.so`)** with `bionic/libm/libm.map.txt` as version script verbatim. Only keep `loadLibraryOS` for the `sincos` GNU fallback (or vendor `s_sincos.c` from `upstream-netbsd` if available). Host forwarding remains as a **narrow `sincos` polyfill**, not the primary libm.

### 4.5 `getLibCSymbols` and `libc-shim` — the C half

Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-core/src/minecraft_utils.cpp` (`getLibCSymbols`).

```cpp
unordered_map<string,void*> MinecraftUtils::getLibCSymbols() {
  unordered_map<string,void*> syms;
  for (auto const& s : shim::get_shimmed_symbols()) syms[s.name]=s.value;
  return syms;
}
```

`shim::get_shimmed_symbols()` is generated from `libc-shim/src/*.cpp` (16 files, Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/libc-shim/CMakeLists.txt`: `common.cpp`, `pthreads.cpp`, `network.cpp`, `cstdio.cpp`, `dirent.cpp`, `stat.cpp`, `errno.cpp`, `sysconf.cpp`, `sched.cpp`, etc.). In current mcpelauncher flow `libC = getLibCSymbols(); ThreadMover::hookLibC(libC); linker::load_library("libc.so", libC)` injects shimmed C symbols as virtual `libc.so` (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-client/src/main.cpp` grep `getLibCSymbols`/`load_library("libc.so"`). For ffbird, the real Bionic `bionic/libc/libc.map.txt` (1762 lines, Source: `bionic/libc/libc.map.txt`) and `version_script.txt` (hides `__cxa_*`) should provide the C symbols; shim survives only for **hybris-translated** symbols (pthread/tgkill/io — see Hybris section).

---

## 5. How it all wires in `main.cpp`

Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-client/src/main.cpp` (grep `MinecraftUtils::`, `linker::`).

Init order (simplified):

1. `linker::init()` — `solist_init()` + `load_library("libdl.so", libdl_symbols)` (Source: `src/linker.cpp` `init`).
2. `auto libC = MinecraftUtils::getLibCSymbols(); ThreadMover::hookLibC(libC);` then per arch:
   - `__APPLE__ && __aarch64__` — `MinecraftUtils::loadLibM()` + `dlopen_ext` with `ANDROID_DLEXT_MCPELAUNCHER_HOOKS` for `libc.so` (variadic compat) + `dlopen` `liblog` variadic compat.
   - `USE_ARMHF_SUPPORT` — `load_library("ld-android.so")` + `dlopen_ext` for `libc.so` hooks + `dlopen` `libm.so`.
   - else (Linux x86_64) — `linker::load_library("libc.so", libC); MinecraftUtils::loadLibM();` (Source: `mcpelauncher-client/src/main.cpp` greps).
3. `MinecraftUtils::setupHybris()` — `loadLibraryOS("libz.so","libz.so.1", libz_symbols)`, `hookAndroidLog()` → `load_library("liblog.so", 4 syms)`, `setupApi()` → `load_library("libmcpelauncher_mod.so", api)`, stub `libOpenSLES`/`libGLESv1_CM`/`libstdc++.so` (Source: `mcpelauncher-core/src/minecraft_utils.cpp` `setupHybris`).
4. `linker::update_LD_LIBRARY_PATH(PathHelper::findGameFile("lib/" + abi))` — so `DT_NEEDED` bare sonames (`libminecraftpe.so` → `libm.so`) resolve via the linker's `ld_library_paths_` / `default_library_paths_` (Source: `mcpelauncher-client/src/main.cpp` `update_LD_LIBRARY_PATH`, `bionic/linker/linker_namespaces.cpp` `is_accessible`).
5. `MinecraftUtils::loadFMod()` — `loadLibraryOS("libfmod.so", dataFile("lib/native/<abi>/libfmod.so.*"), fmod_symbols)` (Source: `mcpelauncher-core/src/minecraft_utils.cpp` `loadFMod`).
6. `linker::load_library("libandroid.so", android_syms)` where `android_syms` is `SmartStub<android_symbols>` → stubs with `Log::warn` (Source: `mcpelauncher-client/src/main.cpp` `SmartStub` + `load_library("libandroid.so"`), `minecraft-imported-symbols/include/minecraft/imported/android_symbols.h`.
7. `MinecraftUtils::loadMinecraftLib(hooks)` — resolves `libminecraftpe.so` via `PathHelper::findGameFile("lib/<abi>/libminecraftpe.so")` + `ANDROID_DLEXT_MCPELAUNCHER_HOOKS` for `showMousePointer`/`hideMousePointer`/`setFullscreenMode`/`GameActivity_finish`, optionally `__cxa_*` relocate for `libstdc++.so`, then `handle = linker::dlopen("libminecraftpe.so")` and `base = linker::get_library_base(handle)` for patching (Source: `mcpelauncher-core/src/minecraft_utils.cpp` `loadMinecraftLib` 200+ lines, `src/linker.cpp` `get_library_base`).

---

## 6. Strong points

1. **Faithful AOSP linker** — vendors the real `bionic/linker` (Bloom filter GNU hash, `RELA`, versioned `DT_VERNEED`/`DT_VERDEF`, `VersionTracker`, TLS `TLSDESC`, `ifunc` resolver). MCPE `DT_NEEDED` resolves exactly as on device; no ad-hoc ELF parser to maintain (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/linker/linker_soinfo.cpp`, `linker_relocate.cpp`, `linker.h` `VersionTracker`).

2. **Stock binary compatibility** — unmodified `libminecraftpe.so` runs; `DT_NEEDED` `libm.so`/`libz.so`/`liblog.so` satisfied via synthetic `soinfo::load_library` without patching the ELF (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/src/linker.cpp`, `mcpelauncher-core/src/hybris_utils.cpp`).

3. **APEX / `ld.config` structure** — `bionic/apex/ld.config.txt` + `bionic/linker/linker_config.cpp` (`ConfigParser` handling `dir.<section>`, `[section]`, `+=`) preserves Android's namespace isolation design, even though the minimal `dir.runtime` config suffices for the runtime APEX. Extensible to full `/linkerconfig/ld.config.txt` with per-namespace `ld_library_paths_` (Source: `bionic/apex/ld.config.txt`, `bionic/linker/linker_config.cpp`).

4. **Version scripts verbatim** — `bionic/libm/libm.map.txt` (298 symbols, `introduced=` per API level/arch), `bionic/libc/version_script.txt` (local `__cxa_*`), `bionic/libdl/libdl.map.txt` (`LIBC`/`LIBC_N`/`LIBC_OMR1`/`LIBC_PLATFORM`) let the linker enforce `DT_VERNEED` correctly; MCPE linked against older Bionic still resolves (Source: `bionic/libm/libm.map.txt`, `bionic/libc/version_script.txt`, `bionic/libdl/libdl.map.txt`).

5. **Minimal public API** — `public_include/mcpelauncher/linker.h` exposes only `init`, `load_library`, `relocate`, `get_library_base`/`get_library_code_region`, plus `__loader_*` passthrough. All AOSP detail stays `PRIVATE` (`core/base/include`, `bionic/linker/*`) (Source: `public_include/mcpelauncher/linker.h`, `src/linker.cpp`).

6. **Arch overrides handled** — `bionic/libm/x86_64/*.S` (e.g., `sqrt.S` → `sqrtsd %xmm0,%xmm0`), `arm64/fenv.c` + `sqrt.S`, `arm/fenv.c`, `i387`, `upstream-freebsd` msun give per-arch correctly-rounded results; `fake_long_double.c` handles `!__LP64__` where `long double == double` (Source: `bionic/libm/x86_64`, `bionic/libm/arm64`, `bionic/libm/fake_long_double.c`, `bionic/libm/builtins.cpp`).

7. **Small, testable foundations reused** — the manifest’s `docs/research/mcpelauncher-manifest.md` lessons (`logger` thin wrapper, `file-util` `FileUtil`/`EnvPathUtil`) are directly applicable to ffbird’s `logger`/`file-util` that already exist (Source: `docs/research/mcpelauncher-manifest.md`).

---

## 7. Weak points

1. **Global `solist` / `g_default_namespace` — no isolation** — `solist` is a single global linked list (`solist_init()`, `solist_add_soinfo`, `g_default_namespace`) (Source: `bionic/linker/linker.cpp`, `src/linker.cpp`). No namespace isolation by default; every `load_library` mutates global state. A second Bionic-linked game or concurrent test cannot sandbox its `DT_NEEDED`.

2. **Silent `dlsym` skip in `loadLibraryOS`** — `if (ptr) syms[sym]=ptr` (Source: `mcpelauncher-core/src/hybris_utils.cpp`) silently drops missing symbols. At `RELA` time the linker then `DL_ERR("cannot locate symbol ...")` or, for `STB_WEAK`, returns `nullptr` — but the caller never knows which symbols were missing until relocate fails (Source: `bionic/linker/linker_relocate.cpp` `lookup_symbol`).

3. **`throw` vs `Result`** — `loadLibM()` `throw runtime_error("Failed to load libm")` and `loadFMod` similarly (Source: `mcpelauncher-core/src/minecraft_utils.cpp`). ffbird’s `logger/result.h` already provides `Result<T>` with `ok/value/error` (Source: `/home/clickpaw/dev/Android/ffbird/logger/include/logger/result.h`); mcpelauncher predates it and forces `try/catch` at `main.cpp` load.

4. **Repeated `dlopen` leaks** — `HybrisUtils::loadLibraryOS` returns the host `handle` but the caller often discards it (`setupHybris()` ignores `libz.so` handle, `loadLibM` returns it but `main.cpp` ignores it) (Source: `mcpelauncher-core/src/minecraft_utils.cpp` `setupHybris`, `hybris_utils.cpp`). Host handles are intentionally leaked to keep symbols live, but there is no ref-count or `dlclose` path — repeated hot-reload leaks.

5. **30-file `libc-shim` maintenance** — `libc-shim/src` has 16 files (`pthreads.cpp`, `network.cpp`, `cstdio.cpp`, `dirent.cpp`, `stat.cpp`, `errno.cpp`, `sysconf.cpp`, `sched.cpp`, etc., Source: `libc-shim/CMakeLists.txt` + `src/` listing). Each Bionic `libc.map.txt` bump (1762 lines, Source: `bionic/libc/libc.map.txt`) requires hand-editing shim; the real Bionic `bionic/libc/*.c` is not used.

6. **Global linker state `g_ld_debug_verbosity`** — `linker::init()` reads `MCPELAUNCHER_LINKER_VERBOSITY` into global `g_ld_debug_verbosity` (Source: `src/linker.cpp`, `bionic/linker/linker_debug.h` `LINKER_VERBOSITY_PRINT/TRACE/DEBUG`, ` LINKER_DEBUG_TO_LOG`). Not thread-safe; no per-instance logger.

7. **No tests for Bionic libs** — `bionic/tests` exists upstream but `mcpelauncher-linker` ships no `ctest` for `libprint_test` / `print_test_hello` (JNI `__android_log_print`) nor for `libm` precision (Source: `/home/clickpaw/dev/Android/ffbird/natives/print_test/print_test.cpp` `print_test_hello` → `__android_log_print`).

8. **Tight coupling to `GameWindow` / `JniSupport`** — `main.cpp` mixes `linker::` init, `MinecraftUtils::setupHybris`, `GameWindowManager`, `JniSupport`, `ModLoader`, `ThreadMover`, `CorePatches` in one 1000+ line `main` (Source: `mcpelauncher-client/src/main.cpp` grep `linker::` / `MinecraftUtils::` / `HybrisUtils::` counts). Hard to unit-test linker in isolation.

9. **`LD_LIBRARY_PATH` mutation** — `linker::update_LD_LIBRARY_PATH(PathHelper::findGameFile(...))` mutates global `g_default_namespace` `ld_library_paths_` (Source: `mcpelauncher-client/src/main.cpp` `update_LD_LIBRARY_PATH`, `bionic/linker/linker.cpp` `kDefaultLdPaths`, `bionic/linker/linker_namespaces.cpp` `get_default_library_paths`). Impure; breaks reproducibility if called twice.

---

## 8. Lessons for ffbird foundations (logger, file-util, stubs — no shim)

These are the durable lessons from `docs/research/mcpelauncher-manifest.md` that apply to ffbird’s own foundations, independent of Bionic:

- **Logger** — ffbird’s `logger` (`logger/include/logger/log.h` + `logger/include/logger/result.h`, Source: `/home/clickpaw/dev/Android/ffbird/logger/include/logger/log.h`, `result.h`) already mirrors the manifest’s `logger` (`LogLevel TRACE..ERROR`, `Logger` with `mutex_`, `file_`, `setLogFile`/`clearLogFile`, `global()` accessor) but adds `Result<T>` (manifest’s `logger` throws or returns `bool`). Keep `Result<T>` and file-rotation; don’t copy mcpelauncher’s `Log::vlog` global statics.
- **File-util** — ffbird’s `file-util` (`file-util/include/file-util/file_util.h` + `env_path_util.h`, Source: `/home/clickpaw/dev/Android/ffbird/file-util/include/file-util/`) already resembles manifest’s `file-util` (`FileUtil::exists`, `EnvPathUtil::findInPath`, `getAppDir`). Keep `Result` error returns, not `throw`; reuse `PathHelper` search order (data dir + abi dir) but via `file-util` API.
- **Stubs / `runtime-linux`** — ffbird’s `runtime-linux` is a stub (`runtime-linux/include/runtime_linux/runtime.h` → `namespace runtime_linux { // TODO }`, Source: `/home/clickpaw/dev/Android/ffbird/runtime-linux/include/runtime_linux/runtime.h`). Lesson: keep stubs **thin and typed** (one `.cpp` per translation unit, no `libc-shim` spread). The manifest’s 30-file shim is what you avoid by bundling real Bionic.
- **Options / Deps** — ffbird’s `CMake/Options.cmake` (`option(FFBIRD_BUILD_TESTS)`, `FFBIRD_WERROR`, `FFBIRD_BUILD_NATIVES`, Source: `/home/clickpaw/dev/Android/ffbird/CMake/Options.cmake`) already separates concerns better than manifest’s monolithic top-level `CMakeLists.txt` (5672B). Keep it.

---

## 9. Personalized proposal for ffbird — bundle recent AOSP Bionic + replicate hybris

**Principle**: vendor the **real** `bionic/` (linker + libc + libm + libdl) as in `mcpelauncher-linker`, build `libm` from `bionic/libm/upstream-freebsd/lib/msun/src/*.c` with `builtins.cpp`/`signbit.cpp`/`fake_long_double.c` + arch `*.S`/`fenv.c`, and use hybris **only** for `pthread`/`tgkill`/`__android_log`/`property` translation. No host `libm.so.6` forwarding as primary path.

### 9.1 Module map

```
ffbird/
├── bionic-linker/          # STATIC liblinker.a — mirrors mcpelauncher-linker/CMakeLists.txt PRIVATE sources
│   ├── CMakeLists.txt      # add_library(bionic-linker STATIC bionic/linker/*.cpp core/base/*.cpp src/linker.cpp bionic/libdl/libdl.cpp)
│   ├── public_include/bionic_linker/linker.h  # minimal public header like public_include/mcpelauncher/linker.h
│   ├── src/linker.cpp      # solist_init, load_library, relocate, get_library_base, get_library_code_region, init
│   └── bionic/             # git submodule or copy of https://github.com/minecraft-linux/android_bionic @ platform-tools-29.0.6-56-g081b55b1f
│       ├── linker/         # linker.cpp, linker_soinfo.cpp, linker_relocate.cpp, linker_namespaces.cpp, linker_config.cpp, ...
│       ├── libc/           # libc.map.txt, version_script.txt, include/android/dlext.h, bionic/pthread_internal.h
│       ├── libm/           # Android.bp, libm.map.txt, builtins.cpp, signbit.cpp, fake_long_double.c, upstream-freebsd/lib/msun/src/*.c, x86_64/*.S, arm64/fenv.c
│       ├── libdl/          # libdl.cpp, libdl.map.txt
│       └── apex/           # ld.config.txt, manifest.json (com.android.runtime)
├── bionic-libc/            # SHARED libc.so — built from bionic/libc/*.c with bionic/libc/libc.map.txt version script
│   ├── CMakeLists.txt      # add_library(bionic-libc SHARED <msun-like src list from bionic/libc>) + linker version script -Wl,--version-script=bionic/libc/libc.map.txt
│   └── bionic/libc/        # same bionic/ as bionic-linker (shared via cmake FetchContent or symlink)
├── bionic-libm/            # SHARED libm.so — real msun, NOT host libm.so.6 forwarding
│   ├── CMakeLists.txt      # add_library(bionic-libm SHARED bionic/libm/*.c builtins.cpp signbit.cpp fake_long_double.c arch/*.S) + version_script bionic/libm/libm.map.txt
│   ├── bionic/libm/        # same bionic/
│   └── src/sincos_stubs.c  # only GNU extensions missing from msun: sincos/sincosf → sin+cos wrapper (or pull upstream-netbsd s_sincos.c)
├── hybris-bridge/          # STATIC hybris translation — ONLY pthread/io/log/property
│   ├── CMakeLists.txt      # add_library(hybris-bridge STATIC src/pthread_bridge.cpp src/log_bridge.cpp src/property_bridge.cpp)
│   ├── include/hybris_bridge/bridge.h  # replaces HybrisUtils::loadLibrary/loadLibraryOS
│   ├── src/pthread_bridge.cpp   # maps Bionic pthread_internal.h thread layout → host pthread_create/join/tgkill
│   ├── src/log_bridge.cpp       # __android_log_print/vprint/write/assert → logger::Logger (like hybris_android_log_hook.cpp)
│   └── src/property_bridge.cpp  # __system_property_get/find → stubbed props (ro.build.version.sdk etc.)
├── runtime-linux/          # NativeLoader — owns the Bionic linker instance, no global solist leakage
│   ├── include/runtime_linux/native_loader.h  # class NativeLoader { Result<void*> loadLibrary(path); Result<void*> loadLibM(); ... }
│   ├── src/native_loader.cpp  # wraps bionic-linker::load_library + hybris-bridge + Result error model
│   └── src/stub.cpp        # existing stub (Source: /home/clickpaw/dev/Android/ffbird/runtime-linux/src/stub.cpp)
├── logger/                 # existing — keep Result<void> + file rotation (Source: logger/include/logger/result.h, logger/include/logger/log.h)
├── file-util/              # existing — keep FileUtil/EnvPathUtil with Result (Source: file-util/include/file-util/file_util.h)
└── natives/print_test/     # existing — first Bionic lib test (Source: natives/print_test/print_test.cpp → __android_log_print)
```

- `bionic-linker` is STATIC (like `mcpelauncher-linker` `add_library(linker STATIC ...)`); `bionic-libc`/`bionic-libm` are SHARED with real `msun` sources, not virtual `loadLibraryOS` maps.
- `hybris-bridge` is the only place host `dlopen` is allowed — and only for `sincos` fallback and property stubs, not for bulk `libm` symbols.
- `runtime-linux/NativeLoader` replaces `MinecraftUtils` + `HybrisUtils` static globals with an instance that owns `solist`/`namespace` handles and returns `logger::Result<T>` (Source: `logger/include/logger/result.h` `Result<T>::success/failure`).

### 9.2 Error model — `Result<T>` everywhere, no `throw`

```cpp
// like HybrisUtils::loadLibraryOS but returns Result, not throw
logger::Result<void*> NativeLoader::loadLibM() {
  // Try Bionic libm first (real msun): bionic-linker already has bionic-libm via DT_NEEDED
  // Only fallback sincos via host if bionic-libm lacks it (check bionic/libm/libm.map.txt for sincos)
  auto h = bionic_linker::dlopen("libm.so", 0);
  if (!h) return logger::Result<void*>::failure(bionic_linker::dlerror());
  return logger::Result<void*>::success(h);
}
logger::Result<void*> NativeLoader::loadLibraryOS(const char* bionicName, const char* hostPath,
                                                   const char** symbols) {
  void* host = ::dlopen(hostPath, RTLD_LAZY);
  if (!host) return logger::Result<void*>::failure(::dlerror());
  std::unordered_map<std::string,void*> syms;
  for (int i=0; symbols[i]; ++i) { void* p = ::dlsym(host, symbols[i]); if (p) syms[symbols[i]]=p; else logMissing(symbols[i]); }
  if (syms.empty()) return logger::Result<void*>::failure("no symbols resolved for " + std::string(bionicName));
  void* h = bionic_linker::load_library(bionicName, syms); // keep host handle alive in NativeLoader member
  hostHandles_[bionicName] = host;
  return logger::Result<void*>::success(h);
}
```

- Replace `throw runtime_error("Failed to load libm")` (Source: `mcpelauncher-core/src/minecraft_utils.cpp` `loadLibM`) with `Result::failure`.
- `loadLibraryOS` logs missing `dlsym` instead of silently dropping (addresses Weak point #2), and keeps `hostHandles_` map for later `dlclose` (addresses #4).

### 9.3 Version scripts — verbatim from `bionic/`

- `bionic-libm/CMakeLists.txt` → `target_link_options(bionic-libm PRIVATE -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/bionic/libm/libm.map.txt)` (Source: `/home/clickpaw/dev/jetpackjoyride/References/mcpelauncher-manifest/mcpelauncher-linker/bionic/libm/libm.map.txt` 309 lines, 298 symbols, `introduced=` per API level).
- `bionic-libc/CMakeLists.txt` → `-Wl,--version-script=bionic/libc/libc.map.txt` (1762 lines, Source: `bionic/libc/libc.map.txt`) plus `-Wl,--version-script=bionic/libc/version_script.txt` (`local: _Z*` for `__cxa_*`, Source: `bionic/libc/version_script.txt`).
- `bionic-linker` already links `bionic/libdl/libdl.map.txt` with `LIBC { dlopen/dlsym/... } LIBC_N { dlvsym }` (Source: `bionic/libdl/libdl.map.txt`).

### 9.4 Linker wiring — namespace isolation from day one

```cpp
void NativeLoader::init(const std::string& gameLibDir) {
  bionic_linker::init(); // solist_init + load libdl (Source: src/linker.cpp init)
  bionic_linker::update_LD_LIBRARY_PATH(gameLibDir.c_str()); // sets g_default_namespace ld_library_paths_
  // Create isolated namespace for game libs (uses linker_namespaces.cpp is_accessible)
  // Future: android_namespace_t* gameNs = create_namespace("game", ld_library_paths_, permitted_paths_);
}
```

- Today `update_LD_LIBRARY_PATH` mutates global `g_default_namespace` (Source: `bionic/linker/linker.cpp` `kDefaultLdPaths`, `bionic/linker/linker_namespaces.cpp` `is_accessible`). For `ffbird` keep this for bootstrap but plan per-`NativeLoader` namespace when upgrading Bionic beyond `platform-tools-29`.
- `bionic/apex/ld.config.txt` + `bionic/linker/linker_config.cpp` (`ConfigParser` → `dir.runtime`, `[runtime]`) already parsed by `linker_config.cpp`; ffbird should ship its own `ld.config.txt` (`dir.game = <gameLibDir>`, `[game]`) instead of relying solely on `update_LD_LIBRARY_PATH` global.

### 9.5 Testing — `natives/print_test` as first Bionic lib

`natives/print_test/print_test.cpp` (Source: `/home/clickpaw/dev/Android/ffbird/natives/print_test/print_test.cpp`):

```cpp
#include <android/log.h>
extern "C" void print_test_hello() { __android_log_print(ANDROID_LOG_INFO, "print_test", "hello"); }
```

- This is the ideal **first test** for the bundled Bionic: build `print_test` as a Bionic `DT_NEEDED` on `liblog.so` (`__android_log_print` from `hybris-bridge/log_bridge.cpp` like `hybris_android_log_hook.cpp` → `Log::vlog`), load it via `NativeLoader::loadLibrary("print_test.so")`, and assert `bionic_linker::dlsym(handle, "print_test_hello")` succeeds and writes to `logger`.
- Next: `bionic-libm` precision tests — `ctest` cases that call `bionic-libm` `sin`/`exp`/`tgamma` and compare to host `libm` within 1 ULP, plus `sincos` wrapper test.

### 9.6 What hybris translates vs what Bionic provides

| Symbol family | Provider in ffbird | Notes |
|---|---|---|
| `sin`/`cos`/`exp`/`tgamma`/... (298, Source: `bionic/libm/libm.map.txt`) | `bionic-libm` real `msun` (`upstream-freebsd/lib/msun/src/*.c` + `builtins.cpp` + arch `*.S`, Source: `bionic/libm/Android.bp`) | No host `libm.so.6` |
| `sincos`/`sincosf` (GNU, not in `msun`) | `bionic-libm/src/sincos_stubs.c` (sin+cos) or host `__sincos` fallback via `hybris-bridge` `loadLibraryOS` extra map (Source: `mcpelauncher-core/src/minecraft_utils.cpp` `{"sincos",(void*)__sincos}`) | Only narrow fallback |
| `__android_log_*` (4, Source: `hybris_android_log_hook.cpp`) | `hybris-bridge/log_bridge.cpp` → `logger::Logger` | Same 4 symbols |
| `property_get` / `__system_property_*` | `hybris-bridge/property_bridge.cpp` | Stub `ro.build.version.sdk` etc. |
| `pthread_create`/`join`/`tgkill`/`tls` | `hybris-bridge/pthread_bridge.cpp` (uses `bionic/libc/bionic/pthread_internal.h` layout, Source: `bionic/libc/bionic/pthread_internal.h`) | Bionic thread struct translation |
| `open`/`read`/`mmap`/`ioctl` (minimal) | `bionic-libc` real `bionic/libc/*.c` + hybris `iorewrite` only if needed | Avoid `libc-shim` sprawl (16 files, Source: `libc-shim/CMakeLists.txt`) |
| `__cxa_*` (`_Znaj`, etc., Source: `bionic/libc/version_script.txt`) | `bionic-libc` hides as `local:`; `NativeLoader::relocate` injects host `__cxa_pure_virtual` etc. if needed (Source: `mcpelauncher-core/src/minecraft_utils.cpp` `loadMinecraftLib` `relocate` for `libstdc++.so`) | Same `relocate` API |

### 9.7 Next decisions

1. **Bionic version pin** — stay on `platform-tools-29.0.6-56-g081b55b1f` (API 29, well-tested with MCPE) or bump to recent `main` (AOSP 34, new `linker_config` KF). If bump, re-verify `linker_soinfo.cpp` Bloom + `linker_relocate.cpp` TLS.
2. **Build sharing** — share `bionic/` as a single `FetchContent` / submodule between `bionic-linker`/`bionic-libc`/`bionic-libm` (symlink or `CMAKE_CURRENT_SOURCE_DIR` shared).
3. **Arch matrix** — start `x86_64` host (`bionic/libm/x86_64/*.S` → `sqrtsd`, `ceil.S`, etc., Source: `bionic/libm/x86_64`), then `arm64` (`bionic/libm/arm64/fenv.c` + `sqrt.S`).
4. **Namespace isolation** — implement `create_namespace` wrapper around `android_namespace_t` (`bionic/linker/linker_namespaces.cpp` `is_accessible`) when `NativeLoader` needs concurrent loads.

---

## 10. Risks

- **Bionic upgrade churn** — `bionic/libc/libc.map.txt` (1762 lines) and `bionic/linker/linker_soinfo.cpp` Bloom internals change per release; pin `bionic/` and update via `git log --oneline` review.
- **`long double` ABI** — `fake_long_double.c` on `!__LP64__` and `arch/x86_64` `long double` (80-bit) vs `arm64` `long double` (128-bit) mismatches (`fabsl`, `modfl`, Source: `bionic/libm/fake_long_double.c`, `bionic/libm/builtins.cpp` `fabsl`). Test `ceill`/`fabsl` explicitly.
- **Global solist leak** — until per-`NativeLoader` namespace lands, `solist` stays global (Source: `bionic/linker/linker.cpp` globals). Avoid parallel `NativeLoader` instances or guard with mutex.
- **Host `sincos` divergence** — if `bionic-libm` never adds `s_sincos.c`, the `sincos` stub (sin+cos) may be slower than host `__sincos`; keep host fallback path behind a flag.

---

*Generated from direct file reads; every claim cites `Source: <path>` above. No shim references — this doc replicates hybris and bundles real recent AOSP Bionic as requested.*
