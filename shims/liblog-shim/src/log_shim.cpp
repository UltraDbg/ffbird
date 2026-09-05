#include <cstdarg>
#include <cstdio>
#include <string>
#include "logger/log.h"

extern "C" {

__attribute__((visibility("default"))) int __android_log_print(int prio, const char* tag, const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    char buf[4096];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    logger::Level lvl = logger::Level::INFO;
    if(prio<=2) lvl=logger::Level::DEBUG;
    else if(prio==4) lvl=logger::Level::WARN;
    else if(prio>=5) lvl=logger::Level::ERROR;
    logger::Logger::global().log(lvl, tag?tag:"android", "%s", buf);
    return 0;
}
__asm__(".symver __android_log_print,__android_log_print@LIBLOG");

__attribute__((visibility("default"))) int __android_log_vprint(int prio, const char* tag, const char* fmt, va_list ap) {
    char buf[4096];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    logger::Level lvl = logger::Level::INFO;
    if(prio<=2) lvl=logger::Level::DEBUG;
    else if(prio==4) lvl=logger::Level::WARN;
    else if(prio>=5) lvl=logger::Level::ERROR;
    logger::Logger::global().log(lvl, tag?tag:"android", "%s", buf);
    return 0;
}
__asm__(".symver __android_log_vprint,__android_log_vprint@LIBLOG");

__attribute__((visibility("default"))) int __android_log_write(int prio, const char* tag, const char* text) {
    logger::Level lvl = logger::Level::INFO;
    if(prio<=2) lvl=logger::Level::DEBUG;
    else if(prio==4) lvl=logger::Level::WARN;
    else if(prio>=5) lvl=logger::Level::ERROR;
    logger::Logger::global().log(lvl, tag?tag:"android", "%s", text?text:"");
    return 0;
}
__asm__(".symver __android_log_write,__android_log_write@LIBLOG");

__attribute__((visibility("default"))) void __android_log_assert(const char* cond, const char* tag, const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    char buf[4096];
    if(fmt) vsnprintf(buf,sizeof(buf),fmt,ap);
    else snprintf(buf,sizeof(buf),"Assertion failed: %s", cond?cond:"");
    va_end(ap);
    logger::Logger::global().log(logger::Level::ERROR, tag?tag:"android", "%s", buf);
    abort();
}
__asm__(".symver __android_log_assert,__android_log_assert@LIBLOG");

}
