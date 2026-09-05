#include "file-util/file_util.h"
#include "file-util/env_path_util.h"

#include <gtest/gtest.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

using namespace file_util;

TEST(FileUtilGetParent, Basic) {
    EXPECT_EQ(FileUtil::getParent("/a/b/"), "/a");
    EXPECT_EQ(FileUtil::getParent("/a/b/c"), "/a/b");
    EXPECT_EQ(FileUtil::getParent("/a"), "/");
    EXPECT_EQ(FileUtil::getParent("/"), "/");
    EXPECT_EQ(FileUtil::getParent("a/b"), "a");
    EXPECT_EQ(FileUtil::getParent("a"), ".");
}

TEST(FileUtilExistsIsDirectory, Basic) {
    EXPECT_TRUE(FileUtil::exists("/tmp"));
    EXPECT_TRUE(FileUtil::isDirectory("/tmp"));
    EXPECT_FALSE(FileUtil::isDirectory("/tmp/nonexistent_ffbird_test_dir_xyz"));
    EXPECT_FALSE(FileUtil::exists("/tmp/nonexistent_ffbird_test_file_xyz_12345"));
}

TEST(FileUtilMkdirRecursive, Nested) {
    std::string path = "/tmp/ffbird_test_mkdir_" + std::to_string(getpid()) + "/a/b/c";
    // cleanup if exists
    ::unlink(path.c_str());
    auto r = FileUtil::mkdirRecursive(path);
    EXPECT_TRUE(r.ok) << r.error;
    EXPECT_TRUE(FileUtil::exists(path));
    EXPECT_TRUE(FileUtil::isDirectory(path));
    // second call should succeed
    auto r2 = FileUtil::mkdirRecursive(path);
    EXPECT_TRUE(r2.ok);
    // cleanup
    ::rmdir(path.c_str());
    ::rmdir(("/tmp/ffbird_test_mkdir_" + std::to_string(getpid()) + "/a/b").c_str());
    ::rmdir(("/tmp/ffbird_test_mkdir_" + std::to_string(getpid()) + "/a").c_str());
    ::rmdir(("/tmp/ffbird_test_mkdir_" + std::to_string(getpid())).c_str());
}

TEST(FileUtilWriteRead, RoundTrip) {
    std::string path = "/tmp/ffbird_test_rw_" + std::to_string(getpid()) + ".txt";
    ::unlink(path.c_str());
    std::string data = "hello ffbird\nsecond line";
    auto w = FileUtil::writeFile(path, data);
    ASSERT_TRUE(w.ok) << w.error;
    auto r = FileUtil::readFile(path);
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.value, data);
    ::unlink(path.c_str());
}

TEST(FileUtilReadMissing, Fails) {
    auto r = FileUtil::readFile("/nope_ffbird_missing_file_12345_xyz");
    EXPECT_FALSE(r.ok);
    EXPECT_FALSE(r.error.empty());
}

TEST(EnvPathUtilGetAppDir, NonEmpty) {
    std::string dir = EnvPathUtil::getAppDir();
    EXPECT_FALSE(dir.empty());
    // Should be a directory or at least exists
    // getAppDir returns dir of exe, which should exist
    EXPECT_TRUE(FileUtil::exists(dir) || dir == "." || dir == "/");
}

TEST(EnvPathUtilGetDataHome, RespectsEnv) {
    const char* old = getenv("XDG_DATA_HOME");
    std::string oldVal = old ? old : "";
    setenv("XDG_DATA_HOME", "/tmp/custom_data_home_ffbird", 1);
    EXPECT_EQ(EnvPathUtil::getDataHome(), "/tmp/custom_data_home_ffbird");
    if (!oldVal.empty()) {
        setenv("XDG_DATA_HOME", oldVal.c_str(), 1);
    } else {
        unsetenv("XDG_DATA_HOME");
    }
    // Without env, should be ~/.local/share
    unsetenv("XDG_DATA_HOME");
    std::string home = EnvPathUtil::getHomeDir();
    if (!home.empty()) {
        std::string expected = home + "/.local/share";
        // handle trailing slash?
        EXPECT_EQ(EnvPathUtil::getDataHome(), expected);
    }
    if (!oldVal.empty()) {
        setenv("XDG_DATA_HOME", oldVal.c_str(), 1);
    }
}

TEST(EnvPathUtilFindInPath, FindsSh) {
    std::string out;
    bool found = EnvPathUtil::findInPath("sh", out);
    EXPECT_TRUE(found);
    EXPECT_FALSE(out.empty());
    EXPECT_TRUE(FileUtil::exists(out));
}
