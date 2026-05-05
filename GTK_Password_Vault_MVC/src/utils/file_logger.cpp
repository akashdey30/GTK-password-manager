// ============================================================
// utils/file_logger.cpp
// ============================================================
#include "../../include/utils/file_logger.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

FileLogger::FileLogger(const std::string& filepath)
    : filepath_(filepath)
{
    file_.open(filepath_, std::ios::app);
    if (!file_.is_open()) {
        // Silently degrade — logging should never crash the app
    }
    log(LogLevel::INFO, "=== GTK Password Vault started ===");
}

FileLogger::~FileLogger() {
    log(LogLevel::INFO, "=== GTK Password Vault stopped ===");
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

void FileLogger::log(LogLevel level, const std::string& message) {
    if (!file_.is_open()) return;
    file_ << "[" << timestamp() << "] "
          << "[" << levelStr(level) << "] "
          << message << "\n";
    // Auto-flush on WARNING and ERROR
    if (level != LogLevel::INFO) file_.flush();
}

void FileLogger::flush() {
    if (file_.is_open()) file_.flush();
}

std::string FileLogger::timestamp() {
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

const char* FileLogger::levelStr(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:          return "INFO ";
        case LogLevel::WARNING:       return "WARN ";
        case LogLevel::ERROR_LEVEL:   return "ERROR";
    }
    return "?????";
}
