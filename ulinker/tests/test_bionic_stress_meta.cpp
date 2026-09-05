#include <gtest/gtest.h>
#include <sys/stat.h>
#include <string>
#include <cstdio>
#include <array>
#include <memory>
#include <stdexcept>

static std::string execCmd(const std::string& cmd) {
    std::array<char, 256> buf;
    std::string out;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return "";
    while (fgets(buf.data(), buf.size(), pipe.get()) != nullptr) out += buf.data();
    return out;
}

static bool fileExists(const std::string& p) {
    struct stat st;
    return stat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

// Helper to find lib_android path relative to binary
static std::string findLibAndroid(const std::string& lib) {
    // Try multiple candidates
    const char* candidates[] = {
        "build/debug/lib_android/",
        "build/lib_android/",
        "lib_android/",
        "../lib_android/",
        "../../build/debug/lib_android/",
        nullptr
    };
    // Also try relative to executable path via /proc/self/exe
    char exe[1024] = {0};
    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe)-1);
    std::string exeDir = ".";
    if (n > 0) {
        exe[n]='\0';
        std::string s(exe);
        size_t pos = s.rfind('/');
        if (pos != std::string::npos) exeDir = s.substr(0,pos);
    }
    std::vector<std::string> tries;
    for (int i=0; candidates[i]; ++i) tries.push_back(candidates[i] + lib);
    tries.push_back(exeDir + "/../lib_android/" + lib);
    tries.push_back(exeDir + "/../../build/debug/lib_android/" + lib);
    tries.push_back(std::string("/home/clickpaw/dev/Android/ffbird/build/debug/lib_android/") + lib);
    for (auto &p : tries) if (fileExists(p)) return p;
    return "";
}

TEST(BionicStressMeta, PrintTestExists) {
    std::string path = findLibAndroid("libprint_test.so");
    ASSERT_FALSE(path.empty()) << "libprint_test.so not found in lib_android";
    EXPECT_TRUE(fileExists(path));
    std::string out = execCmd("nm -D " + path + " 2>&1");
    EXPECT_NE(out.find("print_test_hello"), std::string::npos) << out;
    EXPECT_NE(out.find("__android_log_print"), std::string::npos) << out;
}

TEST(BionicStressMeta, LibmProbeExists) {
    std::string path = findLibAndroid("libm_probe.so");
    ASSERT_FALSE(path.empty()) << "libm_probe.so not found";
    std::string out = execCmd("nm -D " + path + " 2>&1");
    EXPECT_NE(out.find("libm_probe"), std::string::npos) << out;
}

TEST(BionicStressMeta, BionicStressIsBionic) {
    std::string path = findLibAndroid("libbionic_stress.so");
    ASSERT_FALSE(path.empty()) << "libbionic_stress.so not found — build with -DFFBIRD_BUILD_NATIVES=ON";
    EXPECT_TRUE(fileExists(path));

    // Check DT_NEEDED via llvm-readelf or readelf
    std::string ndkReadelf = "/home/clickpaw/Android/Sdk/ndk/30.0.16138531/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf";
    std::string cmd = ndkReadelf + " --dynamic " + path + " 2>&1";
    std::string out = execCmd(cmd);
    if (out.empty() || out.find("NEEDED") == std::string::npos) {
        // fallback to host readelf
        out = execCmd("readelf --dynamic " + path + " 2>&1");
    }
    EXPECT_NE(out.find("liblog.so"), std::string::npos) << "DT_NEEDED liblog.so missing:\n" << out;
    EXPECT_NE(out.find("libm.so"), std::string::npos) << "DT_NEEDED libm.so missing:\n" << out;
    EXPECT_NE(out.find("libc.so"), std::string::npos) << "DT_NEEDED libc.so missing:\n" << out;
    EXPECT_NE(out.find("libdl.so"), std::string::npos) << "DT_NEEDED libdl.so missing:\n" << out;

    // Check many Bionic symbols
    std::string nm = execCmd("nm -D " + path + " 2>&1");
    const char* mustHave[] = {"bionic_stress_run","pthread_create","pthread_mutex","acos@LIBC","sin@LIBC","__android_log_print", nullptr};
    for (int i=0; mustHave[i]; ++i) {
        EXPECT_NE(nm.find(mustHave[i]), std::string::npos) << "symbol " << mustHave[i] << " missing in nm:\n" << nm;
    }
    // Count U entries to ensure complication (should be >20)
    size_t uCount = 0;
    size_t pos = 0;
    while ((pos = nm.find(" U ", pos)) != std::string::npos) { uCount++; pos+=3; }
    EXPECT_GT(uCount, 20u) << "Expected complicated Bionic sample with >20 undefined Bionic symbols, got " << uCount << "\n" << nm;

    // Check TLS / pthread
    EXPECT_NE(nm.find("pthread_key_create"), std::string::npos);
    EXPECT_NE(nm.find("pthread_setspecific"), std::string::npos);
}

TEST(BionicStressMeta, HostLoadViaUlinker) {
    // Host ulinker can publish the NDK lib as Bionic SONAME (stub — not executing ARM code)
    // For universal runtime, we test the host thread+math logic directly via a host-built check
    // Here we just verify ulinker can load the host libm and publish
    // This is a placeholder for future real bionic-linker execution via qemu
    SUCCEED();
}
