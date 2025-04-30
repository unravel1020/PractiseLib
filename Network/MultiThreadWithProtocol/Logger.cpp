#include "Logger.h"

namespace net {
namespace logging {

std::ofstream Logger::logFile;
std::string Logger::baseLogName;
Logger::Level Logger::minLevel = Level::INFO;
int Logger::maxLogHistoryDays = 7;
std::mutex Logger::mtx;

void Logger::init(const std::string& baseName, Level logLevel, int maxHistoryDays) {
    baseLogName = baseName;
    minLevel = logLevel;
    maxLogHistoryDays = maxHistoryDays;

    rotateLogs();

    std::string filename = generateLogFileName();
    logFile.open(filename, std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "[Logger] Failed to open log file: " << filename << std::endl;
    }
}

void Logger::shutdown() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (!shouldLog(level)) return;

    std::lock_guard<std::mutex> lock(mtx);

    std::string levelStr = levelToString(level);
    std::string timestamp = getCurrentTimestamp();

    std::string logLine = "[" + timestamp + "] [" + levelStr + "] " + message;

    // 控制台带颜色输出
    const char* color = "";
    if (level == LogLevel::ERR) color = "\033[31m";     // Red
    else if (level == LogLevel::WARNING) color = "\033[33m"; // Yellow
    else if (level == LogLevel::DEBUG) color = "\033[36m";   // Cyan

    std::cout << color << logLine << "\033[0m" << std::endl;

    if (logFile.is_open()) {
        logFile << logLine << std::endl;
        logFile.flush();
    }
}

bool Logger::shouldLog(LogLevel level) {
    return static_cast<int>(level) >= static_cast<int>(minLevel);
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERR: return "ERROR";
        case LogLevel::DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&in_time_t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Logger::generateLogFileName() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&in_time_t);
    std::ostringstream oss;
    oss << baseLogName << "_" << std::put_time(&tm, "%Y%m%d") << ".log";
    return oss.str();
}

void Logger::rotateLogs() {
    if (maxLogHistoryDays <= 0) return;

    std::string pattern = baseLogName + "_";

    for (const auto& entry : fs::directory_iterator(".")) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.find(pattern) == 0 && filename.size() == pattern.size() + 10 + 4) {
                std::string dateStr = filename.substr(pattern.size(), 8);
                if (dateStr.length() == 8) {
                    int year = std::stoi(dateStr.substr(0, 4));
                    int month = std::stoi(dateStr.substr(4, 2));
                    int day = std::stoi(dateStr.substr(6, 2));

                    std::tm file_tm = {};
                    file_tm.tm_year = year - 1900;
                    file_tm.tm_mon = month - 1;
                    file_tm.tm_mday = day;
                    file_tm.tm_hour = 12;
                    std::time_t file_time = mktime(&file_tm);
                    std::time_t now_time = std::time(nullptr);

                    double diff_days = std::difftime(now_time, file_time) / (60 * 60 * 24);
                    if (diff_days > maxLogHistoryDays) {
                        fs::remove(entry.path());
                    }
                }
            }
        }
    }
}

} // namespace logging
} // namespace net