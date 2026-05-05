#pragma once
// ============================================================
// utils/i_logger.h
// ISP: Narrow interface — logging only.
// DIP: High-level modules depend on this abstraction.
// ============================================================
#ifndef I_LOGGER_H
#define I_LOGGER_H

#include "../global_types.h"
#include <string>

class ILogger {
public:
    virtual ~ILogger() = default;

    // Log a message at the specified level
    virtual void log(LogLevel level, const std::string& message) = 0;

    // Flush any buffered output
    virtual void flush() = 0;
};

#endif // I_LOGGER_H
