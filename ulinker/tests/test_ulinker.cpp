#include "ulinker/ulinker.h"
#include <gtest/gtest.h>
#include <unistd.h>
#include <dlfcn.h>

TEST(UlinkerInit, Idempotent) {
    auto r1 = ulinker::init();
    EXPECT_TRUE(r1.ok) << r1.error;
    auto r2 = ulinker::init();
    EXPECT_TRUE(r2.ok) << r2.error; // idempotent
    EXPECT_TRUE(ulinker::isInitialized());
}

TEST(UlinkerLoadLibrary, PublishAndBase) {
    auto r = ulinker::init();
    ASSERT_TRUE(r.ok);
    std::unordered_map<std::string, void*> syms = {{"my_sym", (void*)0x1234}};
    auto h = ulinker::loadLibrary("libtest_ulinker.so", syms);
    ASSERT_TRUE(h.ok) << h.error;
    EXPECT_NE(h.value, nullptr);
    auto b = ulinker::getLibraryBase(h.value);
    EXPECT_TRUE(b.ok) << b.error;
    EXPECT_NE(b.value, 0u);
    // second load same soname merges
    auto h2 = ulinker::loadLibrary("libtest_ulinker.so", {{"other", (void*)0x5678}});
    EXPECT_TRUE(h2.ok);
    EXPECT_EQ(h.value, h2.value);
}

TEST(UlinkerDlopen, Host) {
    auto r = ulinker::dlopen("libm.so.6", RTLD_LAZY);
    if (!r.ok) {
        // fallback
        r = ulinker::dlopen("libm.so", RTLD_LAZY);
    }
    EXPECT_TRUE(r.ok) << r.error;
    if (r.ok) {
        auto s = ulinker::dlsym(r.value, "sin");
        EXPECT_TRUE(s.ok) << s.error;
        EXPECT_NE(s.value, nullptr);
        ulinker::dlclose(r.value);
    }
}

TEST(UlinkerRelocate, Merge) {
    auto h = ulinker::loadLibrary("libreloc.so", {{"a", (void*)1}});
    ASSERT_TRUE(h.ok);
    auto r = ulinker::relocate(h.value, {{"b", (void*)2}});
    EXPECT_TRUE(r.ok) << r.error;
}
