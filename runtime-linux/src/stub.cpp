#include "runtime_linux/runtime.h"
#include "ulinker/ulinker.h"
#include "hybris_bridge/bridge.h"
#include <string>
#include <unordered_map>

namespace runtime_linux {

utils::Result<void> init() noexcept {
    auto r1 = ulinker::init();
    if (!r1.ok) return utils::Result<void>::failure(r1.error);
    auto r2 = hybris_bridge::init();
    if (!r2.ok) return r2;
    auto r3 = hybris_bridge::publishAndroidLog();
    if (!r3.ok) return r3;
    return utils::Result<void>::success();
}

utils::Result<void*> loadLibrary(const char* soname, const char* hostPath, const char** allowlist) noexcept {
    if (soname && hostPath && allowlist) {
        auto r = hybris_bridge::loadHostLibrary(soname, hostPath, allowlist);
        if (r.ok) return r;
        // Fallback to ulinker direct
        std::unordered_map<std::string, void*> empty;
        return ulinker::loadLibrary(soname, empty);
    }
    if (soname) {
        std::unordered_map<std::string, void*> empty;
        return ulinker::loadLibrary(soname, empty);
    }
    return utils::Result<void*>::failure("loadLibrary: null soname");
}

utils::Result<void*> getSymbol(void* handle, const char* name) noexcept {
    if (!handle || !name) return utils::Result<void*>::failure("getSymbol: null");
    auto r = ulinker::dlsym(handle, name);
    return r;
}

utils::Result<size_t> getLibraryBase(void* handle) noexcept {
    return ulinker::getLibraryBase(handle);
}

}  // namespace runtime_linux
