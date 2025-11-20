/*
 * logging.c
 *
 * Lightweight logging helpers for the kernel; keep different log levels
 * and a simple interface to print to VGA and optionally to a serial port.
 */

void log_init(void) {}
void log_info(const char *fmt, ...) {}
void log_error(const char *fmt, ...) {}
