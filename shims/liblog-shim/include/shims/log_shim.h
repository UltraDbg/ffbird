#ifndef SHIMS_LOG_SHIM_H
#define SHIMS_LOG_SHIM_H
#ifndef __linux__
#error "shims requires Linux"
#endif
// Own shim: __android_log_print → logger
// Version script: shims/liblog-shim/LibLog.version (LIBLOG {global: __android_log_print; ...})
#endif
