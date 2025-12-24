/*
 * ===========================================================================
 * kernel/utils/logging.c
 * ===========================================================================
 *
 * Kernel Logging Facility
 *
 * This file provides a centralized logging mechanism for the kernel. It supports
 * different log levels (INFO, WARNING, ERROR, DEBUG) and abstracts the output
 * destination (VGA console).
 *
 * Usage:
 *   log_info("Kernel started, version %d.%d", major, minor);
 *   log_error("Failed to allocate memory!");
 *
 * ===========================================================================
 */

#include <lib/cstd/stdio.h>
#include <stdarg.h>

/* ---------------------------------------------------------------------------
 * Log Level Definitions
 * --------------------------------------------------------------------------- */
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO  = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_ERROR = 3
} log_level_t;

/* Current minimum log level (messages below this are suppressed) */
static log_level_t current_log_level = LOG_LEVEL_INFO;

/* ---------------------------------------------------------------------------
 * Internal Helper
 * --------------------------------------------------------------------------- */

/**
 * @brief Core logging function with level prefix
 */
static void log_write(const char *prefix, const char *fmt, va_list ap)
{
    kprintf("[%s] ", prefix);
    kvprintf(fmt, ap);
    kprintf("\n");
}

/* ---------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the logging system
 */
void log_init(void)
{
    current_log_level = LOG_LEVEL_INFO;
}

/**
 * @brief Set the minimum log level
 */
void log_set_level(int level)
{
    if (level >= LOG_LEVEL_DEBUG && level <= LOG_LEVEL_ERROR) {
        current_log_level = (log_level_t)level;
    }
}

/**
 * @brief Log an informational message
 */
void log_info(const char *fmt, ...)
{
    if (current_log_level <= LOG_LEVEL_INFO) {
        va_list ap;
        va_start(ap, fmt);
        log_write("INFO", fmt, ap);
        va_end(ap);
    }
}

/**
 * @brief Log a warning message
 */
void log_warn(const char *fmt, ...)
{
    if (current_log_level <= LOG_LEVEL_WARN) {
        va_list ap;
        va_start(ap, fmt);
        log_write("WARN", fmt, ap);
        va_end(ap);
    }
}

/**
 * @brief Log an error message
 */
void log_error(const char *fmt, ...)
{
    if (current_log_level <= LOG_LEVEL_ERROR) {
        va_list ap;
        va_start(ap, fmt);
        log_write("ERROR", fmt, ap);
        va_end(ap);
    }
}

/**
 * @brief Log a debug message
 */
void log_debug(const char *fmt, ...)
{
    if (current_log_level <= LOG_LEVEL_DEBUG) {
        va_list ap;
        va_start(ap, fmt);
        log_write("DEBUG", fmt, ap);
        va_end(ap);
    }
}
