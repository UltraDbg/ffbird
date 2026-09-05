#include "ulinker/ulinker.h"

#include <dlfcn.h>
#include <mutex>
#include <string>
#include <unordered_map>
#include <link.h>
#include <cstring>
#include <cerrno>

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

// Host handle -> Bionic extra symbols map (for publish API)
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
    ::dlerror(); // clear
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
        // dlsym may return nullptr for symbol value 0 without error — treat as not found if error cleared?
        // Keep success with nullptr to allow caller to check, but also provide failure if symbol truly missing
        // For universal runtime, we treat nullptr as failure if caller expects non-null
        // Return success with nullptr so caller can decide; but also set no error
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
    // Clean extra maps
    std::lock_guard<std::mutex> lk(g_mu);
    g_extraMap.erase(handle);
    // Also remove soname mapping if matches
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
    // Check if already published as Bionic SONAME
    {
        std::lock_guard<std::mutex> lk(g_mu);
        auto it = g_sonameToHandle.find(soname);
        if (it != g_sonameToHandle.end() && it->second) {
            // Already have a handle for this soname — merge extra symbols
            auto h = it->second;
            g_extraMap[h].insert(extraSymbols.begin(), extraSymbols.end());
            return utils::Result<void*>::success(h);
        }
    }
    // For now, publish as host dlopen of a stub that provides SONAME
    // Universal runtime future: real Bionic soinfo::load_library.
    // Today: create a handle that represents the Bionic SONAME and keep extra map.
    // Use dlopen(nullptr) as a placeholder handle for the Bionic namespace entry
    void* placeholder = ::dlopen(nullptr, RTLD_LAZY);
    if (!placeholder) placeholder = reinterpret_cast<void*>(0x1); // fallback sentinel
    // Actually use a unique handle per soname via dlopen on empty string trick? Simpler: allocate dummy
    static int dummy;
    void* handle = reinterpret_cast<void*>(&dummy + reinterpret_cast<uintptr_t>(placeholder));
    // Make it unique per soname via hash
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
    // Try to resolve via dladdr on handle's address
    Dl_info info;
    if (::dladdr(handle, &info) && info.dli_fbase) {
        return utils::Result<size_t>::success(reinterpret_cast<size_t>(info.dli_fbase));
    }
    // Fallback: iterate phdr
    // Use dl_iterate_phdr to find base
    struct Data { void* target; size_t base; bool found; };
    Data data{handle, 0, false};
    auto cb = [](struct dl_phdr_info* pinfo, size_t, void* d) -> int {
        Data* pdata = static_cast<Data*>(d);
        if (pinfo->dlpi_addr && pdata->target) {
            // Placeholder — real Bionic will scan PT_LOAD
        }
        return 0;
    };
    ::dl_iterate_phdr(cb, &data);
    if (data.found) return utils::Result<size_t>::success(data.base);
    // For Bionic-published handles, return a synthetic base
    std::lock_guard<std::mutex> lk(g_mu);
    if (g_sonameMap.find(handle) != g_sonameMap.end()) {
        // Synthetic base for stub — use handle value as base
        return utils::Result<size_t>::success(reinterpret_cast<size_t>(handle) & ~0xFFF);
    }
    return utils::Result<size_t>::failure("getLibraryBase: unknown handle");
}

utils::Result<void> getLibraryCodeRegion(void* handle, size_t& base, size_t& size) noexcept {
    auto r = getLibraryBase(handle);
    if (!r.ok) return utils::Result<void>::failure(r.error);
    base = r.value;
    // Estimate size as 0x10000 for stub; real Bionic will scan phdr PT_LOAD|PF_X
    size = 0x10000;
    Dl_info info;
    if (::dladdr(handle, &info) && info.dli_fbase) {
        // Try to find executable segment size via dl_iterate_phdr
        // For now keep estimate
    }
    return utils::Result<void>::success();
}

utils::Result<void> updateLdLibraryPath(const char* path) noexcept {
    if (!path) return utils::Result<void>::failure("updateLdLibraryPath: null");
    // Host: setenv LD_LIBRARY_PATH; Bionic: would mutate g_default_namespace
    if (::setenv("LD_LIBRARY_PATH", path, 1) != 0) {
        return utils::Result<void>::failure(std::string("setenv failed: ") + strerror(errno));
    }
    return utils::Result<void>::success();
}

}  // namespace ulinker
