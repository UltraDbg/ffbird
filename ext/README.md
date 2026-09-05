# ext — reserved for external CMake packages

This directory is reserved for future external CMake packages (e.g. `sdl3`).

Do not put project sources here. Vendored dependencies will be added as
subdirectories or via `FetchContent`/`ExternalProject` under `ext/`.

Currently empty on purpose — the build does not require anything from `ext/`.
