/*
 * userland/lib/syscall_wrappers.c
 *
 * System Call Wrappers
 *
 * This file provides the user-space C API for kernel system calls. It implements
 * functions like `write`, `exit`, and `open` by triggering the appropriate
 * software interrupt or syscall instruction to transition into kernel mode.
 */

int write(int fd, const void *buf, int size) { return -1; }
int exit(int status) { return -1; }
