/*
 * ===========================================================================
 * userland/lib/syscall_wrappers.c
 * ===========================================================================
 *
 * System Call Wrappers for User Programs
 *
 * This file provides the user-space C API for kernel system calls. It implements
 * functions like write(), exit(), and open() by triggering INT 0x80 to
 * transition into kernel mode.
 *
 * System Call Convention:
 *   EAX = syscall number
 *   EBX = argument 1
 *   ECX = argument 2
 *   EDX = argument 3
 *   ESI = argument 4
 *   EDI = argument 5
 *   EAX = return value
 *
 * ===========================================================================
 */

#include <stddef.h>

/* ---------------------------------------------------------------------------
 * System Call Numbers (must match kernel/syscall.c)
 * --------------------------------------------------------------------------- */
#define SYS_EXIT        1
#define SYS_FORK        2
#define SYS_READ        3
#define SYS_WRITE       4
#define SYS_OPEN        5
#define SYS_CLOSE       6
#define SYS_GETPID      20
#define SYS_SLEEP       35
#define SYS_SBRK        45
#define SYS_YIELD       158

/* ---------------------------------------------------------------------------
 * syscall0 - System call with no arguments
 * --------------------------------------------------------------------------- */
static inline int syscall0(int num)
{
    int result;
    __asm__ volatile (
        "int $0x80"
        : "=a" (result)
        : "a" (num)
        : "memory"
    );
    return result;
}

/* ---------------------------------------------------------------------------
 * syscall1 - System call with one argument
 * --------------------------------------------------------------------------- */
static inline int syscall1(int num, int arg1)
{
    int result;
    __asm__ volatile (
        "int $0x80"
        : "=a" (result)
        : "a" (num), "b" (arg1)
        : "memory"
    );
    return result;
}

/* ---------------------------------------------------------------------------
 * syscall2 - System call with two arguments
 * --------------------------------------------------------------------------- */
static inline int syscall2(int num, int arg1, int arg2)
{
    int result;
    __asm__ volatile (
        "int $0x80"
        : "=a" (result)
        : "a" (num), "b" (arg1), "c" (arg2)
        : "memory"
    );
    return result;
}

/* ---------------------------------------------------------------------------
 * syscall3 - System call with three arguments
 * --------------------------------------------------------------------------- */
static inline int syscall3(int num, int arg1, int arg2, int arg3)
{
    int result;
    __asm__ volatile (
        "int $0x80"
        : "=a" (result)
        : "a" (num), "b" (arg1), "c" (arg2), "d" (arg3)
        : "memory"
    );
    return result;
}

/* ===========================================================================
 * User-Space System Call Wrappers
 * =========================================================================== */

/* ---------------------------------------------------------------------------
 * exit - Terminate the current process
 * ---------------------------------------------------------------------------
 * Parameters:
 *   status - Exit status code
 *
 * Note: This function does not return.
 * --------------------------------------------------------------------------- */
void exit(int status)
{
    syscall1(SYS_EXIT, status);
    
    /* Should never reach here, but loop forever if it does */
    while (1) {
        __asm__ volatile ("hlt");
    }
}

/* ---------------------------------------------------------------------------
 * fork - Create a new process
 * ---------------------------------------------------------------------------
 * Returns:
 *   In parent: PID of child
 *   In child: 0
 *   On error: -1
 * --------------------------------------------------------------------------- */
int fork(void)
{
    return syscall0(SYS_FORK);
}

/* ---------------------------------------------------------------------------
 * read - Read from a file descriptor
 * ---------------------------------------------------------------------------
 * Parameters:
 *   fd    - File descriptor
 *   buf   - Buffer to read into
 *   count - Maximum bytes to read
 *
 * Returns: Number of bytes read, or -1 on error
 * --------------------------------------------------------------------------- */
int read(int fd, void *buf, size_t count)
{
    return syscall3(SYS_READ, fd, (int)buf, (int)count);
}

/* ---------------------------------------------------------------------------
 * write - Write to a file descriptor
 * ---------------------------------------------------------------------------
 * Parameters:
 *   fd    - File descriptor
 *   buf   - Buffer containing data
 *   count - Number of bytes to write
 *
 * Returns: Number of bytes written, or -1 on error
 * --------------------------------------------------------------------------- */
int write(int fd, const void *buf, size_t count)
{
    return syscall3(SYS_WRITE, fd, (int)buf, (int)count);
}

/* ---------------------------------------------------------------------------
 * open - Open a file
 * ---------------------------------------------------------------------------
 * Parameters:
 *   pathname - Path to the file
 *   flags    - Open flags (O_RDONLY, O_WRONLY, O_RDWR, etc.)
 *
 * Returns: File descriptor on success, -1 on error
 * --------------------------------------------------------------------------- */
int open(const char *pathname, int flags)
{
    return syscall2(SYS_OPEN, (int)pathname, flags);
}

/* ---------------------------------------------------------------------------
 * close - Close a file descriptor
 * ---------------------------------------------------------------------------
 * Parameters:
 *   fd - File descriptor to close
 *
 * Returns: 0 on success, -1 on error
 * --------------------------------------------------------------------------- */
int close(int fd)
{
    return syscall1(SYS_CLOSE, fd);
}

/* ---------------------------------------------------------------------------
 * getpid - Get process ID
 * ---------------------------------------------------------------------------
 * Returns: PID of the current process
 * --------------------------------------------------------------------------- */
int getpid(void)
{
    return syscall0(SYS_GETPID);
}

/* ---------------------------------------------------------------------------
 * sleep - Sleep for specified ticks
 * ---------------------------------------------------------------------------
 * Parameters:
 *   ticks - Number of timer ticks to sleep
 *
 * Returns: 0 on success
 * --------------------------------------------------------------------------- */
int sleep(unsigned int ticks)
{
    return syscall1(SYS_SLEEP, (int)ticks);
}

/* ---------------------------------------------------------------------------
 * yield - Yield CPU to other processes
 * ---------------------------------------------------------------------------
 * Returns: 0 on success
 * --------------------------------------------------------------------------- */
int yield(void)
{
    return syscall0(SYS_YIELD);
}

/* ---------------------------------------------------------------------------
 * sbrk - Extend process heap
 * ---------------------------------------------------------------------------
 * Parameters:
 *   increment - Bytes to add to heap
 *
 * Returns: Previous break value, or -1 on error
 * --------------------------------------------------------------------------- */
void *sbrk(int increment)
{
    return (void *)syscall1(SYS_SBRK, increment);
}

/* ===========================================================================
 * Convenience Functions
 * =========================================================================== */

/* ---------------------------------------------------------------------------
 * puts - Write a string to stdout
 * --------------------------------------------------------------------------- */
int puts(const char *s)
{
    if (s == NULL) {
        return -1;
    }
    
    /* Calculate length */
    size_t len = 0;
    while (s[len] != '\0') {
        len++;
    }
    
    int result = write(1, s, len);
    
    /* Write newline */
    write(1, "\n", 1);
    
    return result;
}

/* ---------------------------------------------------------------------------
 * putchar - Write a character to stdout
 * --------------------------------------------------------------------------- */
int putchar(int c)
{
    char ch = (char)c;
    return write(1, &ch, 1);
}

/* ---------------------------------------------------------------------------
 * getchar - Read a character from stdin
 * --------------------------------------------------------------------------- */
int getchar(void)
{
    char c;
    if (read(0, &c, 1) <= 0) {
        return -1;
    }
    return (int)c;
}

/* ---------------------------------------------------------------------------
 * gets - Read a line from stdin (unsafe, but simple)
 * --------------------------------------------------------------------------- */
char *gets(char *s, int size)
{
    if (s == NULL || size <= 0) {
        return NULL;
    }
    
    int i = 0;
    while (i < size - 1) {
        char c;
        if (read(0, &c, 1) <= 0) {
            break;
        }
        
        if (c == '\n') {
            break;
        }
        
        s[i++] = c;
    }
    
    s[i] = '\0';
    return s;
}
