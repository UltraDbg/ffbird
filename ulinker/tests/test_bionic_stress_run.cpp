#include "ulinker/ulinker.h"
#include "hybris_bridge/bridge.h"
#include <gtest/gtest.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string>

// Forked test that loads Bionic stress lib via ulinker
TEST(BionicStressRun, ForkedLoadAndCall) {
    // Fork to isolate global solist
    pid_t pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0) {
        // Child
        auto r1 = ulinker::init();
        if (!r1.ok) _exit(1);
        auto r2 = hybris_bridge::init();
        if (!r2.ok) _exit(2);
        auto r3 = hybris_bridge::publishAndroidLog();
        if (!r3.ok) _exit(3);

        // Try to load bionic libm if built
        // bionic-libm provides libm.so — try to load via ulinker
        auto hm = ulinker::loadLibrary("libm.so", {});
        // Not fatal if fails — we just check it doesn't crash
        (void)hm;

        // Try to load bionic_stress from lib_android
        const char* paths[] = {
            "build/debug/lib_android/libbionic_stress.so",
            "build/ci/lib_android/libbionic_stress.so",
            "lib_android/libbionic_stress.so",
            "build/debug/lib_android/x86_64/libbionic_stress.so",
            "build/debug/lib_android/arm64-v8a/libbionic_stress.so",
            nullptr
        };
        void* handle = nullptr;
        std::string found;
        for (int i=0; paths[i]; ++i) {
            handle = dlopen(paths[i], RTLD_LAZY);
            if (handle) { found = paths[i]; break; }
        }
        if (!handle) {
            // No NDK lib built — skip gracefully (parent will see exit 0)
            _exit(0);
        }
        // Try via ulinker as well
        auto hb = ulinker::loadLibrary("libbionic_stress.so", {});
        if (hb.ok) {
            auto base = ulinker::getLibraryBase(hb.value);
            if (!base.ok) _exit(4);
        }
        // dlsym bionic_stress_run (defined in bionic_stress.cpp)
        void* sym = dlsym(handle, "bionic_stress_run");
        if (!sym) {
            // Try alternative name
            sym = dlsym(handle, "_Z17bionic_stress_runv");
        }
        if (sym) {
            // Call it — should not crash, may log via __android_log
            using Fn = int(*)();
            Fn f = reinterpret_cast<Fn>(sym);
            int rc = f();
            (void)rc;
        }
        dlclose(handle);
        _exit(0);
    } else {
        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            EXPECT_EQ(code, 0) << "child exited with " << code;
        } else if (WIFSIGNALED(status)) {
            FAIL() << "child signaled " << WTERMSIG(status);
        }
    }
}
