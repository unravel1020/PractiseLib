#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <mutex>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

namespace net {
namespace logging {

enum class LogLevel {
    INFO,
    WARNING,
    ERR,
    DEBUG
};

class Logger {
public:
    enum class Level : int {
        INFO = 0,
        WARNING = 1,
        ERR = 2,
        DEBUG = 3
    };

    static void init(const std::string& baseName = "server",
                     Level logLevel = Level::INFO,
                     int maxHistoryDays = 7);

    static void log(LogLevel level, const std::string& message);
    static void shutdown();
    static std::string levelToString(LogLevel level);
    static void setLogLevel(Level level) { minLevel = level; }

private:
    static std::ofstream logFile;
    static std::string baseLogName;
    static Level minLevel;
    static std::mutex mtx;
    static int maxLogHistoryDays;

    static std::string generateLogFileName();
    static void rotateLogs();
    static std::string getCurrentTimestamp();
    static bool shouldLog(LogLevel level);
};

class LoggerStream {
public:
    LoggerStream(LogLevel level, const char* file, int line)
        : level(level) {
        ss << "[" << Logger::levelToString(level) << "] "
           << file << ":" << line << " | ";
    }

    ~LoggerStream() {
        Logger::log(level, ss.str());
    }

    template<typename T>
    LoggerStream& operator<<(const T& value) {
        ss << value;
        return *this;
    }

private:
    LogLevel level;
    std::stringstream ss;
};

} // namespace logging
} // namespace net

// 使用方式：LOG(LogLevel::INFO) << "This is a log message.";
#define LOG(level) net::logging::LoggerStream(level, __FILE__, __LINE__)