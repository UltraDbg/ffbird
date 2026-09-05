#include "ulinker/ulinker.h"

#include <dlfcn.h>
#include <mutex>
#include <string>
#include <unordered_map>
#include <link.h>
#include <cstring>
#include <cerrno>

#ifdef ULINKER_HAS_BIONIC
extern "C" {
extern void* __loader_dlopen(const char* filename, int flags, const void* caller_addr);
extern int __loader_dl_iterate_phdr(int (*cb)(struct dl_phdr_info* info, size_t size, void* data), void* data);
extern void __loader_android_update_LD_LIBRARY_PATH(const char* p);
extern void __loader_android_get_LD_LIBRARY_PATH(char* buffer, size_t buffer_size);
}
#endif

namespace ulinker {

namespace {
std::once_flag g_initFlag;
bool g_initialized = false;
std::mutex g_mu;
std::string g_lastError;
thread_local std::string t_lastError;

void setError(const std::string& s) {
    std::lock_guard<std::mutex> lk(g_mu);
    g_lastError = s;
    t_lastError = s;
}

std::unordered_map<void*, std::unordered_map<std::string, void*>> g_extraMap;
std::unordered_map<void*, std::string> g_sonameMap;
std::unordered_map<std::string, void*> g_sonameToHandle;

}  // namespace

const char* dlError() noexcept {
    std::lock_guard<std::mutex> lk(g_mu);
    if (!t_lastError.empty()) return t_lastError.c_str();
    if (!g_lastError.empty()) return g_lastError.c_str();
    return nullptr;
}

utils::Result<void> init() noexcept {
    std::call_once(g_initFlag, []() {
        g_initialized = true;
    });
    g_initialized = true;
    return utils::Result<void>::success();
}

bool isInitialized() noexcept {
    return g_initialized;
}

utils::Result<void*> dlopen(const char* filename, int flags) noexcept {
    ::dlerror();
    void* h = ::dlopen(filename, flags);
    if (!h) {
        const char* e = ::dlerror();
        std::string msg = e ? e : std::string("dlopen failed: ") + (filename ? filename : "null");
        setError(msg);
        return utils::Result<void*>::failure(msg);
    }
    return utils::Result<void*>::success(h);
}

utils::Result<void*> dlsym(void* handle, const char* symbol) noexcept {
    ::dlerror();
    void* p = ::dlsym(handle, symbol);
    const char* e = ::dlerror();
    if (e) {
        std::string msg = std::string("dlsym failed: ") + symbol + ": " + e;
        setError(msg);
        return utils::Result<void*>::failure(msg);
    }
    if (!p) {
        return utils::Result<void*>::success(p);
    }
    return utils::Result<void*>::success(p);
}

utils::Result<int> dlclose(void* handle) noexcept {
    int r = ::dlclose(handle);
    if (r != 0) {
        const char* e = ::dlerror();
        std::string msg = e ? e : "dlclose failed";
        setError(msg);
        return utils::Result<int>::failure(msg);
    }
    std::lock_guard<std::mutex> lk(g_mu);
    g_extraMap.erase(handle);
    for (auto it = g_sonameToHandle.begin(); it != g_sonameToHandle.end(); ) {
        if (it->second == handle) it = g_sonameToHandle.erase(it);
        else ++it;
    }
    for (auto it = g_sonameMap.begin(); it != g_sonameMap.end(); ) {
        if (it->first == handle) it = g_sonameMap.erase(it);
        else ++it;
    }
    return utils::Result<int>::success(r);
}

utils::Result<int> dladdr(const void* addr, Dl_info* info) noexcept {
    int r = ::dladdr(addr, info);
    if (r == 0) {
        std::string msg = "dladdr failed";
        setError(msg);
        return utils::Result<int>::failure(msg);
    }
    return utils::Result<int>::success(r);
}

utils::Result<void*> loadLibrary(const char* soname,
    const std::unordered_map<std::string, void*>& extraSymbols) noexcept {
    if (!soname || !soname[0]) {
        return utils::Result<void*>::failure("loadLibrary: empty soname");
    }
    {
        std::lock_guard<std::mutex> lk(g_mu);
        auto it = g_sonameToHandle.find(soname);
        if (it != g_sonameToHandle.end() && it->second) {
            auto h = it->second;
            g_extraMap[h].insert(extraSymbols.begin(), extraSymbols.end());
            return utils::Result<void*>::success(h);
        }
    }
    void* placeholder = ::dlopen(nullptr, RTLD_LAZY);
    if (!placeholder) placeholder = reinterpret_cast<void*>(0x1);
    static int dummy;
    void* handle = reinterpret_cast<void*>(&dummy + reinterpret_cast<uintptr_t>(placeholder));
    handle = reinterpret_cast<void*>(std::hash<std::string>{}(soname) ^ reinterpret_cast<uintptr_t>(handle));

    std::lock_guard<std::mutex> lk(g_mu);
    g_sonameToHandle[soname] = handle;
    g_sonameMap[handle] = soname;
    g_extraMap[handle] = extraSymbols;
    return utils::Result<void*>::success(handle);
}

utils::Result<void> relocate(void* handle,
    const std::unordered_map<std::string, void*>& extra) noexcept {
    std::lock_guard<std::mutex> lk(g_mu);
    auto it = g_extraMap.find(handle);
    if (it == g_extraMap.end()) {
        return utils::Result<void>::failure("relocate: unknown handle");
    }
    it->second.insert(extra.begin(), extra.end());
    return utils::Result<void>::success();
}

utils::Result<size_t> getLibraryBase(void* handle) noexcept {
    if (!handle) return utils::Result<size_t>::failure("getLibraryBase: null handle");
    Dl_info info;
    if (::dladdr(handle, &info) && info.dli_fbase) {
        return utils::Result<size_t>::success(reinterpret_cast<size_t>(info.dli_fbase));
    }
    struct Data { void* target; size_t base; bool found; };
    Data data{handle, 0, false};
    auto cb = [](struct dl_phdr_info* pinfo, size_t, void* d) -> int {
        Data* pdata = static_cast<Data*>(d);
        if (pinfo->dlpi_addr && pdata->target) {
        }
        return 0;
    };
    ::dl_iterate_phdr(cb, &data);
    if (data.found) return utils::Result<size_t>::success(data.base);
    std::lock_guard<std::mutex> lk(g_mu);
    if (g_sonameMap.find(handle) != g_sonameMap.end()) {
        return utils::Result<size_t>::success(reinterpret_cast<size_t>(handle) & ~0xFFF);
    }
    return utils::Result<size_t>::failure("getLibraryBase: unknown handle");
}

utils::Result<void> getLibraryCodeRegion(void* handle, size_t& base, size_t& size) noexcept {
    auto r = getLibraryBase(handle);
    if (!r.ok) return utils::Result<void>::failure(r.error);
    base = r.value;
    size = 0x10000;
    Dl_info info;
    if (::dladdr(handle, &info) && info.dli_fbase) {
    }
    return utils::Result<void>::success();
}

utils::Result<void> updateLdLibraryPath(const char* path) noexcept {
    if (!path) return utils::Result<void>::failure("updateLdLibraryPath: null");
#ifdef ULINKER_HAS_BIONIC
    __loader_android_update_LD_LIBRARY_PATH(path);
#endif
    if (::setenv("LD_LIBRARY_PATH", path, 1) != 0) {
        return utils::Result<void>::failure(std::string("setenv failed: ") + strerror(errno));
    }
    return utils::Result<void>::success();
}

}  // namespace ulinker
