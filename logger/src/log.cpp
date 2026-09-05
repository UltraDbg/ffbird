#include "logger/log.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>

namespace logger {

const char* to_string(Level level) {
    switch (level) {
        case Level::TRACE: return "TRACE";
        case Level::DEBUG: return "DEBUG";
        case Level::INFO: return "INFO";
        case Level::WARN: return "WARN";
        case Level::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

Logger::Logger() : minLevel_(Level::TRACE) {}

Logger::~Logger() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.close();
    }
}

void Logger::setMinLevel(Level level) {
    std::lock_guard<std::mutex> lock(mutex_);
    minLevel_ = level;
}

Level Logger::getMinLevel() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return minLevel_;
}

bool Logger::ensureParentDirs(const std::string& path) {
    std::string::size_type pos = path.rfind('/');
    if (pos == std::string::npos) {
        return true;
    }
    std::string dir = path.substr(0, pos);
    if (dir.empty()) {
        return true;
    }
    // mkdir -p
    std::string cur;
    if (!dir.empty() && dir[0] == '/') {
        cur = "/";
    }
    std::string::size_type start = (cur == "/" ? 1 : 0);
    while (start <= dir.size()) {
        std::string::size_type slash = dir.find('/', start);
        std::string part;
        if (slash == std::string::npos) {
            part = dir.substr(start);
            start = dir.size() + 1;
        } else {
            part = dir.substr(start, slash - start);
            start = slash + 1;
        }
        if (part.empty()) {
            continue;
        }
        if (!cur.empty() && cur[cur.size() - 1] != '/') {
            cur += "/";
        }
        cur += part;
        struct stat st;
        if (stat(cur.c_str(), &st) != 0) {
            if (mkdir(cur.c_str(), 0755) != 0 && errno != EEXIST) {
                return false;
            }
        } else {
            if (!S_ISDIR(st.st_mode)) {
                return false;
            }
        }
    }
    return true;
}

bool Logger::setLogFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.close();
    }
    if (!ensureParentDirs(path)) {
        return false;
    }
    file_.open(path.c_str(), std::ios::out | std::ios::app);
    if (!file_.is_open()) {
        return false;
    }
    filePath_ = path;
    return true;
}

void Logger::clearLogFile() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.close();
    }
    filePath_.clear();
}

void Logger::log(Level level, const char* tag, const char* fmt, ...) {
    if (fmt == NULL) {
        return;
    }
    // quick level check without lock? Need lock for minLevel_ but we can check under lock inside logImpl
    char buffer[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    // trim trailing \r\n
    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
        buffer[len - 1] = '\0';
        len--;
    }
    logImpl(level, tag, buffer);
}

void Logger::log(Level level, const char* tag, const std::string& msg) {
    logImpl(level, tag, msg.c_str());
}

void Logger::logImpl(Level level, const char* tag, const char* msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (static_cast<int>(level) < static_cast<int>(minLevel_)) {
        return;
    }
    // time
    time_t now = time(NULL);
    struct tm tm_buf;
    localtime_r(&now, &tm_buf);
    char timeBuf[32];
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &tm_buf);

    const char* lvl = to_string(level);
    const char* t = tag ? tag : "unknown";

    // build line
    char line[4608];
    snprintf(line, sizeof(line), "[%s] %s/%s: %s\n", timeBuf, lvl, t, msg ? msg : "");

    // stdout
    fputs(line, stdout);
    fflush(stdout);

    // file
    if (file_.is_open()) {
        file_ << line;
        file_.flush();
    }
}

Logger& Logger::global() {
    static Logger instance;
    return instance;
}

}  // namespace logger
