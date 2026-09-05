// bionic/linker_compat.cpp — minimal host implementation
#include "linker_compat.h"
#include <link.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include <mutex>
#include <unordered_map>
#include <string>

extern "C" {

void* __loader_dlopen(const char* filename, int flags, const void* caller_addr) {
    (void)caller_addr;
    return dlopen(filename, flags);
}
char* __loader_dlerror() { return dlerror(); }
void* __loader_dlsym(void* handle, const char* symbol, const void* caller_addr) {
    (void)caller_addr;
    return dlsym(handle, symbol);
}
void* __loader_dlvsym(void* handle, const char* symbol, const char* version, const void* caller_addr) {
    (void)caller_addr; (void)version;
    return dlsym(handle, symbol);
}
int __loader_dladdr(const void* addr, Dl_info* info) { return dladdr(addr, info); }
int __loader_dlclose(void* handle) { return dlclose(handle); }
int __loader_dl_iterate_phdr(int (*cb)(struct dl_phdr_info* info, size_t size, void* data), void* data) {
    return dl_iterate_phdr(cb, data);
}
void __loader_android_get_LD_LIBRARY_PATH(char* buffer, size_t buffer_size) {
    const char* v = getenv("LD_LIBRARY_PATH");
    if (!v) v = "";
    strncpy(buffer, v, buffer_size-1);
    buffer[buffer_size-1]='\0';
}
void __loader_android_update_LD_LIBRARY_PATH(const char* p) {
    if(p) setenv("LD_LIBRARY_PATH", p, 1);
}
void* __loader_android_dlopen_ext(const char* filename, int flag, const void* extinfo, const void* caller_addr) {
    (void)extinfo; (void)caller_addr;
    return dlopen(filename, flag);
}
int __loader_android_get_application_target_sdk_version() { return 34; }
bool __loader_android_handle_signal(int, void*, void*) { return false; }

struct android_namespace_t { const char* name; };
static android_namespace_t g_default_namespace = {"default"};
android_namespace_t* g_default_namespace_ptr = &g_default_namespace;

struct soinfo {
    void* base;
    size_t size;
    const char* realpath;
};

static std::mutex g_mu;
static std::unordered_map<void*, soinfo*> g_handle_to_soinfo;

soinfo* soinfo_from_handle(void* handle) {
    std::lock_guard<std::mutex> lk(g_mu);
    auto it = g_handle_to_soinfo.find(handle);
    if (it != g_handle_to_soinfo.end()) return it->second;
    return nullptr;
}

extern "C" void* solist_get_head() { return nullptr; }
extern "C" int solist_init() { return 0; }

} // extern C
