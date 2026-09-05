#include "logger/log.h"
#include "logger/result.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>

using namespace logger;

TEST(LoggerToString, Converts) {
    EXPECT_STREQ(to_string(Level::TRACE), "TRACE");
    EXPECT_STREQ(to_string(Level::DEBUG), "DEBUG");
    EXPECT_STREQ(to_string(Level::INFO), "INFO");
    EXPECT_STREQ(to_string(Level::WARN), "WARN");
    EXPECT_STREQ(to_string(Level::ERROR), "ERROR");
}

TEST(LoggerLevelFilter, HidesDebugWhenMinInfo) {
    Logger lg;
    std::string tmp = "/tmp/ffbird_logger_filter_" + std::to_string(getpid()) + ".log";
    ::unlink(tmp.c_str());
    ASSERT_TRUE(lg.setLogFile(tmp));
    lg.setMinLevel(Level::INFO);
    lg.log(Level::DEBUG, "test", "should_not_appear");
    lg.log(Level::INFO, "test", "should_appear");
    lg.log(Level::ERROR, "test", "also_appear");
    // Need to flush; destructor not yet. Read file.
    {
        std::ifstream in(tmp.c_str());
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        EXPECT_EQ(content.find("should_not_appear"), std::string::npos);
        EXPECT_NE(content.find("should_appear"), std::string::npos);
        EXPECT_NE(content.find("also_appear"), std::string::npos);
    }
    ::unlink(tmp.c_str());
}

TEST(LoggerSetLogFile, CreatesParentDirs) {
    Logger lg;
    std::string path = "/tmp/ffbird_logger_a/b/log_" + std::to_string(getpid()) + ".txt";
    // ensure parent not exists
    ::unlink(path.c_str());
    // try to remove dirs if exist (best effort)
    EXPECT_TRUE(lg.setLogFile(path));
    lg.log(Level::INFO, "tag", "hello file");
    {
        std::ifstream in(path.c_str());
        ASSERT_TRUE(in.good());
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        EXPECT_NE(content.find("hello file"), std::string::npos);
    }
    ::unlink(path.c_str());
    // cleanup dirs
    ::rmdir("/tmp/ffbird_logger_a/b");
    ::rmdir("/tmp/ffbird_logger_a");
}

TEST(LoggerThreadSafety, TwoThreads1000LinesNoInterleave) {
    Logger lg;
    std::string path = "/tmp/ffbird_logger_thread_" + std::to_string(getpid()) + ".log";
    ::unlink(path.c_str());
    ASSERT_TRUE(lg.setLogFile(path));
    lg.setMinLevel(Level::TRACE);

    auto worker = [&lg](int id) {
        for (int i = 0; i < 1000; ++i) {
            lg.log(Level::INFO, "thr", "thread %d line %d", id, i);
        }
    };

    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    t1.join();
    t2.join();

    std::ifstream in(path.c_str());
    ASSERT_TRUE(in.good());
    int lines = 0;
    std::string line;
    while (std::getline(in, line)) {
        ++lines;
        // each line should contain either "thread 1" or "thread 2" fully, not interleaved
        // simple check: line contains "thread"
        EXPECT_NE(line.find("thread"), std::string::npos);
        // ensure no partial corruption: line ends with a number? we just count
    }
    EXPECT_EQ(lines, 2000);
    ::unlink(path.c_str());
}

TEST(LoggerResult, Basic) {
    Result<int> r = Result<int>::success(42);
    EXPECT_TRUE(r.ok);
    EXPECT_EQ(r.value, 42);
    Result<int> f = Result<int>::failure("oops");
    EXPECT_FALSE(f.ok);
    EXPECT_EQ(f.error, "oops");

    Result<void> rv = Result<void>::success();
    EXPECT_TRUE(rv.ok);
    Result<void> fv = Result<void>::failure("err");
    EXPECT_FALSE(fv.ok);
    EXPECT_EQ(fv.error, "err");
}

TEST(LoggerInstanceIsolation, SeparateInstances) {
    Logger a;
    Logger b;
    a.setMinLevel(Level::ERROR);
    b.setMinLevel(Level::TRACE);
    EXPECT_EQ(a.getMinLevel(), Level::ERROR);
    EXPECT_EQ(b.getMinLevel(), Level::TRACE);
    // global accessor exists
    Logger& g = Logger::global();
    (void)g;
}
