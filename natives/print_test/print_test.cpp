#include <android/log.h>

extern "C" void print_test_hello() {
    __android_log_print(ANDROID_LOG_INFO, "print_test", "hello");
}
