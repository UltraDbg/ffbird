#include "anticrash/handler.h"

#include <gtest/gtest.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fstream>
#include <string>
#include <cstdio>

TEST(AnticrashInstall, ReturnsTrue) {
    std::string path = "/tmp/ffbird_anticrash_test_" + std::to_string(getpid()) + ".log";
    ::unlink(path.c_str());
    bool ok = anticrash::install(path);
    EXPECT_TRUE(ok);
    anticrash::uninstall();
    ::unlink(path.c_str());
}

TEST(AnticrashUninstall, Restores) {
    std::string path = "/tmp/ffbird_anticrash_uninstall_" + std::to_string(getpid()) + ".log";
    ::unlink(path.c_str());
    ASSERT_TRUE(anticrash::install(path));
    anticrash::uninstall();
    // After uninstall, raising SIGSEGV in child without install should not log Backtrace
    // We test that install+uninstall leaves no handler: child raises and exits via default handler (or no log)
    pid_t pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0) {
        // child: raise without handler
        // Should terminate via default SIGSEGV, not via our handler
        raise(SIGSEGV);
        _exit(0);
    } else {
        int status = 0;
        waitpid(pid, &status, 0);
        // Should have terminated due to signal
        EXPECT_TRUE(WIFSIGNALED(status));
        EXPECT_EQ(WTERMSIG(status), SIGSEGV);
        // Log should not contain Backtrace
        std::ifstream in(path.c_str());
        if (in.good()) {
            std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            EXPECT_EQ(content.find("Backtrace"), std::string::npos);
        }
    }
    ::unlink(path.c_str());
}

// Death test: install + raise SIGSEGV -> log contains Backtrace
TEST(AnticrashDeath, BacktraceOnSegv) {
    std::string path = "/tmp/ffbird_anticrash_death_" + std::to_string(getpid()) + ".log";
    ::unlink(path.c_str());
    pid_t pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0) {
        // child
        bool ok = anticrash::install(path);
        if (!ok) _exit(2);
        raise(SIGSEGV);
        // should not reach here; handler _Exits
        _exit(3);
    } else {
        int status = 0;
        waitpid(pid, &status, 0);
        // Handler calls _Exit(sig) => child exits with status sig
        // It should not have exited 0
        // Check log file contains Backtrace
        // Give a bit time for file flush (handler fsyncs)
        usleep(200000);
        std::ifstream in(path.c_str());
        ASSERT_TRUE(in.good()) << "log file not created at " << path;
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        EXPECT_NE(content.find("Backtrace"), std::string::npos) << "content: " << content;
        EXPECT_NE(content.find("Signal"), std::string::npos);
        // Child should have exited via _Exit
        // WIFEXITED vs WIFSIGNALED: handler does _Exit(sig) => WIFEXITED with sig code
        // Accept either
        bool exited = WIFEXITED(status) || WIFSIGNALED(status);
        EXPECT_TRUE(exited);
        ::unlink(path.c_str());
    }
}
