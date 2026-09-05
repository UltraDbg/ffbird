# bionic — integrated recent AOSP Bionic (owned)

This directory is **integrated** AOSP Bionic from
`https://android.googlesource.com/platform/bionic` at `731631f` (main, 2025-09).

It is **not a vendor read-only** — we have removed source tracking (no .aosp_rev)
and own the build. You can modify sources directly for our universal runtime.

**We own the build:** `bionic/CMakeLists.txt` builds `bionic_m_real` (libm.so) from
`bionic/libm` msun + `builtins.cpp` + `libm.map.txt` 298, using CMake not Android.bp.

Kept: `linker/`, `libc/`, `libm/`, `libdl/`, `apex/` (pruned of `benchmarks/tests/docs`).
Removed: `benchmarks/`, `tests/`, `docs/`, `cpu_target_features/` to keep repo lean (17M vs 63M).

To update to newer AOSP: `git clone --depth 1 https://android.googlesource.com/platform/bionic /tmp/bionic-latest && rsync -a --delete /tmp/bionic-latest/ bionic/ && rm -rf bionic/benchmarks bionic/tests ...` then commit.

**Our modifications are allowed** — no hackish vendor shims, we integrate for our purpose.
