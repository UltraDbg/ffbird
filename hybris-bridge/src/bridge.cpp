#include "hybris_bridge/bridge.h"
#include "ulinker/ulinker.h"
#include "logger/log.h"

#include <dlfcn.h>
#include <mutex>
#include <cstdarg>
#include <cstdio>
#include <string.h>

extern "C" void* __hybris_get_hooked_symbol(const char* sym, const char* lib);
extern "C" int __system_property_get(const char* key, char* value);

namespace hybris_bridge {

namespace {
std::once_flag g_initFlag;
bool g_inited = false;

int hook_log_print(int prio, const char* tag, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[4096];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    logger::Level lvl = logger::Level::INFO;
    if (prio <= 2) lvl = logger::Level::DEBUG;
    else if (prio == 4) lvl = logger::Level::WARN;
    else if (prio >= 5) lvl = logger::Level::ERROR;
    logger::Logger::global().log(lvl, tag ? tag : "android", "%s", buf);
    return 0;
}

int hook_log_vprint(int prio, const char* tag, const char* fmt, va_list ap) {
    char buf[4096];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    logger::Level lvl = logger::Level::INFO;
    if (prio <= 2) lvl = logger::Level::DEBUG;
    else if (prio == 4) lvl = logger::Level::WARN;
    else if (prio >= 5) lvl = logger::Level::ERROR;
    logger::Logger::global().log(lvl, tag ? tag : "android", "%s", buf);
    return 0;
}

int hook_log_write(int prio, const char* tag, const char* text) {
    logger::Level lvl = logger::Level::INFO;
    if (prio <= 2) lvl = logger::Level::DEBUG;
    else if (prio == 4) lvl = logger::Level::WARN;
    else if (prio >= 5) lvl = logger::Level::ERROR;
    logger::Logger::global().log(lvl, tag ? tag : "android", "%s", text ? text : "");
    return 0;
}

void hook_log_assert(const char* cond, const char* tag, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[4096];
    if (fmt) vsnprintf(buf, sizeof(buf), fmt, ap);
    else snprintf(buf, sizeof(buf), "Assertion failed: %s", cond ? cond : "");
    va_end(ap);
    logger::Logger::global().log(logger::Level::ERROR, tag ? tag : "android", "%s", buf);
    abort();
}
} // namespace

utils::Result<void> init() noexcept {
    std::call_once(g_initFlag, []() {
        g_inited = true;
    });
    if (!g_inited) g_inited = true;
    return utils::Result<void>::success();
}

std::vector<std::string> getMissingSymbols(const char* hostPath, const char** allowlist) noexcept {
    std::vector<std::string> missing;
    if (!hostPath || !allowlist) return missing;
    void* h = dlopen(hostPath, RTLD_LAZY);
    if (!h) {
        for (int i=0; allowlist[i]; ++i) missing.push_back(allowlist[i]);
        return missing;
    }
    for (int i=0; allowlist[i]; ++i) {
        void* p = dlsym(h, allowlist[i]);
        if (!p) missing.push_back(allowlist[i]);
    }
    dlclose(h);
    return missing;
}

utils::Result<void*> loadHostLibrary(const char* bionicSoname, const char* hostPath,
    const char** bionicAllowlist,
    const std::unordered_map<std::string, void*>& extra) noexcept {
    if (!bionicSoname || !bionicSoname[0]) return utils::Result<void*>::failure("loadHostLibrary: empty bionicSoname");
    if (!hostPath || !hostPath[0]) return utils::Result<void*>::failure("loadHostLibrary: empty hostPath");
    if (!bionicAllowlist) return utils::Result<void*>::failure("loadHostLibrary: null allowlist");

    void* hostH = dlopen(hostPath, RTLD_LAZY);
    if (!hostH) {
        const char* e = dlerror();
        std::string msg = std::string("dlopen host \"") + hostPath + "\" failed: " + (e?e:"unknown");
        return utils::Result<void*>::failure(msg);
    }

    std::unordered_map<std::string, void*> syms = extra;
    std::vector<std::string> missing;
    for (int i=0; bionicAllowlist[i]; ++i) {
        const char* sym = bionicAllowlist[i];
        void* hooked = __hybris_get_hooked_symbol(sym, bionicSoname);
        if (hooked) {
            syms[sym] = hooked;
            continue;
        }
        void* p = dlsym(hostH, sym);
        if (p) syms[sym] = p;
        else missing.push_back(sym);
    }

    if (!missing.empty()) {
        std::string msg = "missing host syms for " + std::string(bionicSoname) + " from " + hostPath + ": ";
        for (size_t i=0;i<missing.size() && i<5;++i) {
            if (i) msg += ", ";
            msg += missing[i];
        }
        if (missing.size()>5) msg += " ...";
        dlclose(hostH);
        return utils::Result<void*>::failure(msg);
    }

    auto r = ulinker::loadLibrary(bionicSoname, syms);
    if (!r.ok) {
        dlclose(hostH);
        return utils::Result<void*>::failure("ulinker::loadLibrary failed: " + r.error);
    }
    return utils::Result<void*>::success(r.value);
}

utils::Result<void> publishAndroidLog() noexcept {
    auto r1 = ulinker::init();
    if (!r1.ok) return r1;
    std::unordered_map<std::string, void*> syms;
    syms["__android_log_print"] = reinterpret_cast<void*>(hook_log_print);
    syms["__android_log_vprint"] = reinterpret_cast<void*>(hook_log_vprint);
    syms["__android_log_write"] = reinterpret_cast<void*>(hook_log_write);
    syms["__android_log_assert"] = reinterpret_cast<void*>(hook_log_assert);
    auto r = ulinker::loadLibrary("liblog.so", syms);
    if (!r.ok) return utils::Result<void>::failure(r.error);
    return utils::Result<void>::success();
}

utils::Result<void> stubSymbols(const char* bionicSoname, const char** symbols, void* stub) noexcept {
    if (!bionicSoname || !symbols || !stub) return utils::Result<void>::failure("stubSymbols: null arg");
    std::unordered_map<std::string, void*> syms;
    for (int i=0; symbols[i]; ++i) syms[symbols[i]] = stub;
    auto r = ulinker::loadLibrary(bionicSoname, syms);
    if (!r.ok) return utils::Result<void>::failure(r.error);
    return utils::Result<void>::success();
}

utils::Result<int> getSystemProperty(const char* key, char* value) noexcept {
    if(!key || !value) return utils::Result<int>::failure("null");
    int r = __system_property_get(key, value);
    return utils::Result<int>::success(r);
}

}  // namespace hybris_bridge
