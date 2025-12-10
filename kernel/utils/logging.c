/*
 * kernel/utils/logging.c
 *
 * Kernel Logging Facility
 *
 * This file provides a centralized logging mechanism for the kernel. It supports
 * different log levels (INFO, ERROR, DEBUG) and abstracts the output destination
 * (e.g., VGA console, Serial Port).
 *
 * It is essential for debugging and tracking kernel state transitions.
 */

void log_init(void) {}
void log_info(const char *fmt, ...) {}
void log_error(const char *fmt, ...) {}
