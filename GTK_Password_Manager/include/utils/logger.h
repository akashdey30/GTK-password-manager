#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
} LogLevel;

void logger_init(const char *log_path, LogLevel min_level);
void logger_close(void);
void logger_log(LogLevel level, const char *fmt, ...);

#define LOG_D(...)  logger_log(LOG_DEBUG, __VA_ARGS__)
#define LOG_I(...)  logger_log(LOG_INFO,  __VA_ARGS__)
#define LOG_W(...)  logger_log(LOG_WARN,  __VA_ARGS__)
#define LOG_E(...)  logger_log(LOG_ERROR, __VA_ARGS__)

#endif
