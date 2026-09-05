#ifndef HYBRIS_BRIDGE_BRIDGE_H
#define HYBRIS_BRIDGE_BRIDGE_H

#ifndef __linux__
#error "hybris-bridge requires Linux"
#endif

#include "utils/result.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace hybris_bridge {

utils::Result<void> init() noexcept;

utils::Result<void*> loadHostLibrary(const char* bionicSoname, const char* hostPath,
    const char** bionicAllowlist,
    const std::unordered_map<std::string, void*>& extra = {}) noexcept;

utils::Result<void> publishAndroidLog() noexcept;
utils::Result<void> stubSymbols(const char* bionicSoname, const char** symbols, void* stub) noexcept;

// Introspection
std::vector<std::string> getMissingSymbols(const char* hostPath, const char** allowlist) noexcept;

}  // namespace hybris_bridge

#endif  // HYBRIS_BRIDGE_BRIDGE_H
