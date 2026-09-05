#ifndef FILE_UTIL_FILE_UTIL_H
#define FILE_UTIL_FILE_UTIL_H

#ifndef __linux__
#error "file-util requires Linux"
#endif

#include <string>
#include "utils/result.h"

namespace file_util {

class FileUtil {
public:
    static std::string getParent(const std::string& path);
    static bool exists(const std::string& path);
    static bool isDirectory(const std::string& path);
    static utils::Result<void> mkdirRecursive(const std::string& path);
    static utils::Result<std::string> readFile(const std::string& path);
    static utils::Result<void> writeFile(const std::string& path, const std::string& data);
};

}  // namespace file_util

#endif  // FILE_UTIL_FILE_UTIL_H
