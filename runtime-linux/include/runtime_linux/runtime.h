#ifndef RUNTIME_LINUX_RUNTIME_H
#define RUNTIME_LINUX_RUNTIME_H

#ifndef __linux__
#error "runtime-linux requires Linux"
#endif

#include "utils/result.h"
#include <string>
#include <unordered_map>

namespace runtime_linux {

utils::Result<void> init() noexcept;
utils::Result<void*> loadLibrary(const char* soname, const char* hostPath, const char** allowlist) noexcept;
utils::Result<void*> getSymbol(void* handle, const char* name) noexcept;
utils::Result<size_t> getLibraryBase(void* handle) noexcept;

}  // namespace runtime_linux

#endif  // RUNTIME_LINUX_RUNTIME_H
