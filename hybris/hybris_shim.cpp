// hybris_shim.cpp — minimal host implementation
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

extern "C" {

void* __hybris_get_hooked_symbol(const char* sym, const char* lib) {
    (void)lib;
    if (!sym) return nullptr;
    void* p = dlsym(RTLD_DEFAULT, sym);
    if (p) return p;
    p = dlsym(RTLD_NEXT, sym);
    return p;
}

int __system_property_get(const char* key, char* value) {
    if (!key || !value) return 0;
    const char* v = getenv(key);
    if (v) {
        strncpy(value, v, 91);
        value[91]='\0';
        return (int)strlen(value);
    }
    value[0]='\0';
    return 0;
}
int __system_property_set(const char* key, const char* value) {
    if (!key || !value) return -1;
    return setenv(key, value, 1);
}

void hybris_init() {}
void* hybris_create_wrapper(const char* name, void* func, int type) { (void)name; (void)type; return func; }

int my_property_get(const char *key, char *value, const char *default_value) {
    int r = __system_property_get(key, value);
    if (r==0 && default_value) { strncpy(value, default_value, 91); value[91]='\0'; return (int)strlen(value); }
    return r;
}
int my_property_set(const char *key, const char *value) { return __system_property_set(key,value); }
int my_property_list(void (*propfn)(const char *key, const char *value, void *cookie), void *cookie) { (void)propfn; (void)cookie; return 0; }

void* hybris_dso_handle = nullptr;
}
