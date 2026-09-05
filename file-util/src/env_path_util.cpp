#include "file-util/env_path_util.h"

#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cstdio>

namespace file_util {

std::string EnvPathUtil::getAppDir() {
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len < 0) {
        return "";
    }
    buf[len] = '\0';
    std::string path(buf);
    std::string::size_type pos = path.rfind('/');
    if (pos == std::string::npos) {
        return ".";
    }
    if (pos == 0) {
        return "/";
    }
    return path.substr(0, pos);
}

std::string EnvPathUtil::getWorkingDir() {
    char buf[PATH_MAX];
    if (getcwd(buf, sizeof(buf)) == NULL) {
        return "";
    }
    return std::string(buf);
}

std::string EnvPathUtil::getHomeDir() {
    const char* home = getenv("HOME");
    if (home && home[0] != '\0') {
        return std::string(home);
    }
    struct passwd pw;
    struct passwd* result = NULL;
    char buf[4096];
    if (getpwuid_r(getuid(), &pw, buf, sizeof(buf), &result) == 0 && result != NULL) {
        if (pw.pw_dir) {
            return std::string(pw.pw_dir);
        }
    }
    return "";
}

std::string EnvPathUtil::getDataHome() {
    const char* xdg = getenv("XDG_DATA_HOME");
    if (xdg && xdg[0] != '\0') {
        return std::string(xdg);
    }
    std::string home = getHomeDir();
    if (home.empty()) {
        return "";
    }
    if (home[home.size() - 1] == '/') {
        return home + ".local/share";
    } else {
        return home + "/.local/share";
    }
}

bool EnvPathUtil::findInPath(const std::string& what, std::string& out, const char* pathEnv, const char* cwd) {
    if (what.empty()) {
        return false;
    }
    // If what contains '/', check directly
    if (what.find('/') != std::string::npos) {
        std::string candidate;
        if (what[0] == '/') {
            candidate = what;
        } else {
            std::string base = cwd ? std::string(cwd) : getWorkingDir();
            if (base.empty()) base = ".";
            if (base[base.size() - 1] == '/') {
                candidate = base + what;
            } else {
                candidate = base + "/" + what;
            }
        }
        if (access(candidate.c_str(), X_OK) == 0) {
            struct stat st;
            if (stat(candidate.c_str(), &st) == 0 && !S_ISDIR(st.st_mode)) {
                out = candidate;
                return true;
            }
        }
        return false;
    }
    const char* pathStr = pathEnv ? pathEnv : getenv("PATH");
    if (!pathStr) {
        pathStr = "/usr/local/bin:/usr/bin:/bin";
    }
    std::string path(pathStr);
    std::string::size_type start = 0;
    while (start <= path.size()) {
        std::string::size_type colon = path.find(':', start);
        std::string dir;
        if (colon == std::string::npos) {
            dir = path.substr(start);
            start = path.size() + 1;
        } else {
            dir = path.substr(start, colon - start);
            start = colon + 1;
        }
        if (dir.empty()) {
            dir = cwd ? std::string(cwd) : getWorkingDir();
            if (dir.empty()) dir = ".";
        }
        std::string candidate;
        if (dir[dir.size() - 1] == '/') {
            candidate = dir + what;
        } else {
            candidate = dir + "/" + what;
        }
        if (access(candidate.c_str(), X_OK) == 0) {
            struct stat st;
            if (stat(candidate.c_str(), &st) == 0 && !S_ISDIR(st.st_mode)) {
                out = candidate;
                return true;
            }
        }
    }
    return false;
}

}  // namespace file_util
