# hybris — integrated cleaned libhybris (owned)

This directory is **integrated** cleaned libhybris from
`https://github.com/libhybris/libhybris` at `7079712`.

It is **not vendor read-only** — we own the build.

Kept: `hybris/common` (`hooks.c` ~800, `wrappers.c` mmap ARM trampolines, `dso_handle_counters`, `logging.c`, `sysconf.c`, `legacy_properties`) + single `n` linker version.
Removed: `camera`, `media`, `sf`, `hardware`, `hwc2`, `gralloc`, `vulkan`, `wifi`, `compat/` — Halium HALs not needed for universal Bionic load.

Build: `hybris/CMakeLists.txt` builds `hybris-common` STATIC for `hybris-bridge`.

We can modify `hooks.c`/`wrappers.c` directly for our runtime — no upstream tracking hack.
