/*
 * kernel/syscall.c
 *
 * System Call Dispatcher
 *
 * This file implements the central mechanism for handling system calls from
 * userland. It defines the `syscall_dispatch` function, which routes syscall
 * numbers to their corresponding kernel handlers.
 *
 * It serves as the secure boundary between user mode applications and kernel mode privileges.
 */

#include <stdint.h>

// Define syscall numbers
enum {
    SYSCALL_WRITE = 0,
    SYSCALL_EXIT  = 1,
    SYSCALL_MAX   = 16
};

void syscall_dispatch(int num, void *args) {
    // call registered handler based on num
}
