#include "hybris_bridge/bridge.h"
#include "ulinker/ulinker.h"
#include <gtest/gtest.h>

static const char* kLibmAllow[] = {"sin","cos","pow",nullptr};
static const char* kMissingAllow[] = {"this_symbol_does_not_exist_xyz", nullptr};

TEST(HybrisBridgeInit, Idempotent) {
    auto r = hybris_bridge::init();
    EXPECT_TRUE(r.ok) << r.error;
    auto r2 = hybris_bridge::init();
    EXPECT_TRUE(r2.ok);
}

TEST(HybrisBridgeLoadHost, Success) {
    auto r = hybris_bridge::loadHostLibrary("libm.so", "libm.so.6", kLibmAllow);
    if (!r.ok) {
        // fallback to libm.so
        r = hybris_bridge::loadHostLibrary("libm.so", "libm.so", kLibmAllow);
    }
    EXPECT_TRUE(r.ok) << r.error;
    if (r.ok) {
        auto base = ulinker::getLibraryBase(r.value);
        EXPECT_TRUE(base.ok) << base.error;
    }
}

TEST(HybrisBridgeLoadHost, MissingReturnsFailure) {
    auto r = hybris_bridge::loadHostLibrary("libm.so", "libm.so.6", kMissingAllow);
    EXPECT_FALSE(r.ok);
    EXPECT_FALSE(r.error.empty());
    // error should contain missing list
    EXPECT_NE(r.error.find("missing"), std::string::npos);
}

TEST(HybrisBridgePublishLog, Hooks) {
    auto r = hybris_bridge::publishAndroidLog();
    EXPECT_TRUE(r.ok) << r.error;
    if (r.ok) {
        // liblog.so should now be published — check ulinker knows it
        auto h = ulinker::loadLibrary("liblog.so", {});
        EXPECT_TRUE(h.ok);
    }
}

TEST(HybrisBridgeGetMissing, Helper) {
    auto missing = hybris_bridge::getMissingSymbols("libm.so.6", kMissingAllow);
    EXPECT_EQ(missing.size(), 1u);
    EXPECT_EQ(missing[0], "this_symbol_does_not_exist_xyz");
}
