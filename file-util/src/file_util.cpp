#include "file-util/file_util.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cstdio>

#ifdef HAVE_LOGGER
#include "logger/log.h"
#endif

namespace file_util {

std::string FileUtil::getParent(const std::string& path) {
    if (path.empty()) {
        return "";
    }
    std::string p = path;
    // trim trailing slashes (but keep root "/")
    while (p.size() > 1 && p[p.size() - 1] == '/') {
        p.erase(p.size() - 1);
    }
    if (p == "/" || p.empty()) {
        return "/";
    }
    std::string::size_type pos = p.rfind('/');
    if (pos == std::string::npos) {
        return ".";
    }
    if (pos == 0) {
        return "/";
    }
    std::string parent = p.substr(0, pos);
    // trim trailing slashes again
    while (parent.size() > 1 && parent[parent.size() - 1] == '/') {
        parent.erase(parent.size() - 1);
    }
    if (parent.empty()) {
        return "/";
    }
    return parent;
}

bool FileUtil::exists(const std::string& path) {
    return access(path.c_str(), F_OK) == 0;
}

bool FileUtil::isDirectory(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

utils::Result<void> FileUtil::mkdirRecursive(const std::string& path) {
    if (path.empty()) {
        return utils::Result<void>::failure("empty path");
    }
    // Already exists as dir?
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return utils::Result<void>::success();
        } else {
            return utils::Result<void>::failure("path exists but is not a directory: " + path);
        }
    }
    // Build incrementally
    std::string cur;
    bool isAbsolute = !path.empty() && path[0] == '/';
    if (isAbsolute) {
        cur = "/";
    }
    std::string::size_type start = isAbsolute ? 1 : 0;
    while (start <= path.size()) {
        std::string::size_type slash = path.find('/', start);
        std::string part;
        if (slash == std::string::npos) {
            part = path.substr(start);
            start = path.size() + 1;
        } else {
            part = path.substr(start, slash - start);
            start = slash + 1;
        }
        if (part.empty()) {
            continue;
        }
        if (!cur.empty() && cur[cur.size() - 1] != '/') {
            cur += "/";
        }
        cur += part;
        if (stat(cur.c_str(), &st) != 0) {
            if (mkdir(cur.c_str(), 0755) != 0 && errno != EEXIST) {
                std::string err = std::string("mkdir failed for ") + cur + ": " + strerror(errno);
#ifdef HAVE_LOGGER
                logger::Logger::global().log(logger::Level::WARN, "file-util", "%s", err.c_str());
#endif
                return utils::Result<void>::failure(err);
            }
        } else {
            if (!S_ISDIR(st.st_mode)) {
                return utils::Result<void>::failure("path exists but is not a directory: " + cur);
            }
        }
    }
    return utils::Result<void>::success();
}

utils::Result<std::string> FileUtil::readFile(const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        std::string err = std::string("open failed: ") + path + ": " + strerror(errno);
#ifdef HAVE_LOGGER
        logger::Logger::global().log(logger::Level::DEBUG, "file-util", "%s", err.c_str());
#endif
        return utils::Result<std::string>::failure(err);
    }
    // Get size via lseek
    off_t size = lseek(fd, 0, SEEK_END);
    if (size < 0) {
        // fallback: read chunked without size
        lseek(fd, 0, SEEK_SET);
        std::string out;
        char buf[4096];
        ssize_t n;
        while ((n = read(fd, buf, sizeof(buf))) > 0) {
            out.append(buf, n);
        }
        close(fd);
        if (n < 0) {
            return utils::Result<std::string>::failure(std::string("read failed: ") + strerror(errno));
        }
        return utils::Result<std::string>::success(out);
    }
    lseek(fd, 0, SEEK_SET);
    std::string out;
    out.resize(static_cast<size_t>(size));
    size_t total = 0;
    while (total < static_cast<size_t>(size)) {
        ssize_t n = read(fd, &out[total], static_cast<size_t>(size) - total);
        if (n < 0) {
            if (errno == EINTR) continue;
            close(fd);
            return utils::Result<std::string>::failure(std::string("read failed: ") + strerror(errno));
        }
        if (n == 0) break;
        total += static_cast<size_t>(n);
    }
    close(fd);
    out.resize(total);
    return utils::Result<std::string>::success(out);
}

utils::Result<void> FileUtil::writeFile(const std::string& path, const std::string& data) {
    // Ensure parent dirs exist
    std::string parent = getParent(path);
    if (parent != "." && parent != "/" && !parent.empty()) {
        utils::Result<void> r = mkdirRecursive(parent);
        if (!r.ok) {
            return r;
        }
    }
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        std::string err = std::string("open for write failed: ") + path + ": " + strerror(errno);
#ifdef HAVE_LOGGER
        logger::Logger::global().log(logger::Level::WARN, "file-util", "%s", err.c_str());
#endif
        return utils::Result<void>::failure(err);
    }
    size_t total = 0;
    while (total < data.size()) {
        ssize_t n = write(fd, data.data() + total, data.size() - total);
        if (n < 0) {
            if (errno == EINTR) continue;
            close(fd);
            return utils::Result<void>::failure(std::string("write failed: ") + strerror(errno));
        }
        total += static_cast<size_t>(n);
    }
    close(fd);
    return utils::Result<void>::success();
}

}  // namespace file_util
