#include <math.h>
#include <android/log.h>
extern "C" void libm_probe() {
    __android_log_print(ANDROID_LOG_INFO, "libm_probe", "sin=%f", sin(0.5));
    volatile double a = acos(0.5);
    volatile double b = sin(a);
    (void)b;
}
