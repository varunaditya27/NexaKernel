/*
 * kernel/syscall.c
 *
 * System call dispatcher and table. Implement a fixed table of syscall handlers
 * and a function `syscall_dispatch(int id, registers_t *regs)` to call them.
 *
 * Keep the syscall ABI stable: `int syscall(int number, ...)` and document what
 * each syscall requires in `userland/lib/syscall_wrappers.c`.
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
