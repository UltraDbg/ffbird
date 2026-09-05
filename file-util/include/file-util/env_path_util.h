#ifndef FILE_UTIL_ENV_PATH_UTIL_H
#define FILE_UTIL_ENV_PATH_UTIL_H

#ifndef __linux__
#error "file-util requires Linux"
#endif

#include <string>

namespace file_util {

class EnvPathUtil {
public:
    static std::string getAppDir();
    static std::string getWorkingDir();
    static std::string getHomeDir();
    static std::string getDataHome();
    static bool findInPath(const std::string& what, std::string& out, const char* pathEnv = nullptr, const char* cwd = nullptr);
};

}  // namespace file_util

#endif  // FILE_UTIL_ENV_PATH_UTIL_H
