#ifndef BIONIC_LIBM_LIBM_H
#define BIONIC_LIBM_LIBM_H
#ifndef __linux__
#error "bionic-libm requires Linux"
#endif
// Placeholder for real Bionic libm (msun) — will be replaced by bionic/libm build
// For now, this header exposes the version script path for verification
#define BIONIC_LIBM_MAP_PATH "bionic/libm/libm.map.txt"
#define BIONIC_LIBM_SYMS 298
#endif
