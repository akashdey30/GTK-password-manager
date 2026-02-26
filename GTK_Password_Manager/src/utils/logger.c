#include "utils/logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

static FILE     *g_log_file  = NULL;
static LogLevel  g_min_level = LOG_INFO;
static int       g_owns_file = 0;

static const char *level_str[] = { "DEBUG", "INFO ", "WARN ", "ERROR" };

void logger_init(const char *log_path, LogLevel min_level)
{
    g_min_level = min_level;
    if (log_path) {
        g_log_file = fopen(log_path, "a");
        g_owns_file = 1;
    }
    if (!g_log_file) {
        g_log_file = stderr;
        g_owns_file = 0;
    }
}

void logger_close(void)
{
    if (g_owns_file && g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
}

void logger_log(LogLevel level, const char *fmt, ...)
{
    if (level < g_min_level || !g_log_file) return;

    time_t now = time(NULL);
    struct tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &now);
#else
    localtime_r(&now, &tm_buf);
#endif
    char ts[20];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm_buf);

    fprintf(g_log_file, "[%s] [%s] ", ts, level_str[level]);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_log_file, fmt, ap);
    va_end(ap);
    fprintf(g_log_file, "\n");
    fflush(g_log_file);
}
