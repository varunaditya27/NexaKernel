/*
 * userland/lib/syscall_wrappers.c
 *
 * Thin userland wrappers around kernel syscalls. Provide stable ABI for
 * user programs like `write`, `exit`, `open`, etc.
 */

int write(int fd, const void *buf, int size) { return -1; }
int exit(int status) { return -1; }
