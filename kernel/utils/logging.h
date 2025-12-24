/*
 * ===========================================================================
 * kernel/utils/logging.h
 * ===========================================================================
 *
 * Kernel Logging Facility Interface
 *
 * Provides a centralized logging mechanism with different severity levels.
 *
 * Usage:
 *   #include <kernel/utils/logging.h>
 *
 *   log_init();
 *   log_info("System starting...");
 *   log_error("Something went wrong!");
 *
 * ===========================================================================
 */

#ifndef NEXA_LOGGING_H
#define NEXA_LOGGING_H

/* ---------------------------------------------------------------------------
 * Log Level Constants
 * --------------------------------------------------------------------------- */
#define LOG_LEVEL_DEBUG  0
#define LOG_LEVEL_INFO   1
#define LOG_LEVEL_WARN   2
#define LOG_LEVEL_ERROR  3

/* ---------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the logging system
 */
void log_init(void);

/**
 * @brief Set the minimum log level
 * @param level One of LOG_LEVEL_* constants
 */
void log_set_level(int level);

/**
 * @brief Log an informational message
 * @param fmt Printf-style format string
 * @param ... Variable arguments
 */
void log_info(const char *fmt, ...);

/**
 * @brief Log a warning message
 * @param fmt Printf-style format string
 * @param ... Variable arguments
 */
void log_warn(const char *fmt, ...);

/**
 * @brief Log an error message
 * @param fmt Printf-style format string
 * @param ... Variable arguments
 */
void log_error(const char *fmt, ...);

/**
 * @brief Log a debug message (only shown if log level is DEBUG)
 * @param fmt Printf-style format string
 * @param ... Variable arguments
 */
void log_debug(const char *fmt, ...);

#endif /* NEXA_LOGGING_H */
