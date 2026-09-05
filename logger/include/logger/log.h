#ifndef LOGGER_LOG_H
#define LOGGER_LOG_H

#ifndef __linux__
#error "logger requires Linux"
#endif

#include <cstdarg>
#include <fstream>
#include <mutex>
#include <string>

namespace logger {

enum class Level {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4
};

const char* to_string(Level level);

class Logger {
public:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void setMinLevel(Level level);
    Level getMinLevel() const;

    bool setLogFile(const std::string& path);
    void clearLogFile();

    void log(Level level, const char* tag, const char* fmt, ...);
    void log(Level level, const char* tag, const std::string& msg);

    // Global accessor (optional, not a hard singleton)
    static Logger& global();

private:
    void logImpl(Level level, const char* tag, const char* msg);
    bool ensureParentDirs(const std::string& path);

    mutable std::mutex mutex_;
    Level minLevel_;
    std::ofstream file_;
    std::string filePath_;
};

}  // namespace logger

#endif  // LOGGER_LOG_H
