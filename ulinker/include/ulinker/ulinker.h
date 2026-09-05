#ifndef ULINKER_ULINKER_H
#define ULINKER_ULINKER_H

#ifndef __linux__
#error "ulinker requires Linux"
#endif

#include "utils/result.h"
#include <string>
#include <unordered_map>
#include <dlfcn.h>

namespace ulinker {

// Global init — once_flag guarded, solist_init + preload libdl
utils::Result<void> init() noexcept;

// Thin wrappers over host dlopen (future: bionic/linker soinfo) — same Result model
utils::Result<void*> dlopen(const char* filename, int flags) noexcept;
utils::Result<void*> dlsym(void* handle, const char* symbol) noexcept;
utils::Result<int> dlclose(void* handle) noexcept;
utils::Result<int> dladdr(const void* addr, Dl_info* info) noexcept;
const char* dlError() noexcept;

// Bionic-style publish API (like mcpelauncher linker::load_library)
utils::Result<void*> loadLibrary(const char* soname,
    const std::unordered_map<std::string, void*>& extraSymbols) noexcept;
utils::Result<void> relocate(void* handle,
    const std::unordered_map<std::string, void*>& extra) noexcept;
utils::Result<size_t> getLibraryBase(void* handle) noexcept;
utils::Result<void> getLibraryCodeRegion(void* handle, size_t& base, size_t& size) noexcept;
utils::Result<void> updateLdLibraryPath(const char* path) noexcept;

// Introspection (for tests)
bool isInitialized() noexcept;

}  // namespace ulinker

#endif  // ULINKER_ULINKER_H
