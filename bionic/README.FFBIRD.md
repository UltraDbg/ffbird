# bionic — vendored recent AOSP Bionic (read-only)

This directory will contain a **real recent AOSP Bionic** checkout from
`https://android.googlesource.com/platform/bionic` (branch `aosp-main`).

**Do not edit files here** — they are vendored.

To vendor/update:

```
./scripts/vendor-bionic.sh          # defaults to aosp-main latest
./scripts/vendor-bionic.sh android14-release  # pin to specific branch/tag
```

After vendoring, `bionic/.aosp_rev` and `bionic/.aosp_desc` record the exact rev
(`git describe` e.g. `platform-tools-29.0.6-56-g081b55b1f` for mcpelauncher baseline).

What we keep (read `bionic/README.md` upstream):
- `bionic/linker/` — Bionic linker (`linker.cpp`, `linker_soinfo.cpp`, `linker_relocate.cpp`, `linker_namespaces.cpp`, `linker_config.cpp`, `linker_phdr.cpp`, `linker_block_allocator.cpp`, etc.)
- `bionic/libc/` — `libc.map.txt`, `version_script.txt`, `arch-*/string/*.S`
- `bionic/libm/` — 216 `msun` sources + `builtins.cpp`/`signbit.cpp`/`fake_long_double.c`, `libm.map.txt` (298 syms), `arch/x86_64/*.S`
- `bionic/libdl/` — `libdl.cpp`, `libdl.map.txt`
- `bionic/apex/` — `ld.config.txt`, `manifest.json`

Do **not** vendor `bionic/tests`, `benchmarks`, `tools/versioner` beyond reference.

Until vendored, this directory is a placeholder. `ulinker/` and `bionic-libm/` build against it when present, otherwise they build as stubs.
