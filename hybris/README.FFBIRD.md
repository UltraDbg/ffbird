# hybris — vendored cleaned latest libhybris (read-only)

This directory will contain a **cleaned latest libhybris** from
`https://github.com/libhybris/libhybris` (`master` at `7079712` already cloned to `/tmp/libhybris` for research).

**Do not edit files here** — they are vendored.

To vendor/update:

```
./scripts/vendor-hybris.sh
```

After vendoring, `hybris/.hybris_rev` records exact rev.

What we keep (from comparison in `docs/research/mcpelauncher-linker-hybris-libm.md`):
- `hybris/common/hooks.c` (~800 hooks, pthread `0xFFFF` shm, `SYS_futex`, `sincos`, `property`)
- `hybris/common/wrappers.c` + `wrapper_code_generic_arm.c` (`mmap(RWX)` ARM trampolines)
- `hybris/common/dso_handle_counters.cpp`, `logging.c`, `sysconf.c`, `native_handle.c`, `legacy_properties/`
- **Single** linker version `hybris/common/n/` (or `q/` matching Bionic API 29+), **not** `jb/mm/o/q` multiplex
- `hybris/egl/ws.c` + `hybris/common/n/linker.cpp` glue (add `media`/`camera` only on demand)

Removed: `hybris/camera`, `media`, `sf`, `hardware`, `hwc2`, `gralloc`, `vulkan`, `wifi`, `compat/*` — these are Halium HALs not needed for `libminecraftpe.so`/`libprint_test.so`.

Until vendored, `hybris-bridge/` links against a stub `hybris-common`.
