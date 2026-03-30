#pragma once
// ============================================================
// utils/file_logger.h
// SRP: Writes timestamped log entries to a file only.
// LSP: Fully satisfies ILogger contract.
// ============================================================
#ifndef FILE_LOGGER_H
#define FILE_LOGGER_H

#include "i_logger.h"
#include <fstream>
#include <string>

class FileLogger : public ILogger {
public:
    explicit FileLogger(const std::string& filepath);
    ~FileLogger() override;

    void log(LogLevel level, const std::string& message) override;
    void flush() override;

private:
    std::ofstream file_;
    std::string   filepath_;

    static std::string timestamp();
    static const char* levelStr(LogLevel level);
};

#endif // FILE_LOGGER_H
