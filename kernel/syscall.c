/*
 * ===========================================================================
 * kernel/syscall.c
 * ===========================================================================
 *
 * System Call Dispatcher
 *
 * This file implements the central mechanism for handling system calls from
 * userland. It defines the syscall table, individual syscall handlers, and
 * the dispatcher function that routes syscall numbers to their handlers.
 *
 * System Call Convention (Linux-like):
 *   EAX = syscall number
 *   EBX = argument 1
 *   ECX = argument 2
 *   EDX = argument 3
 *   ESI = argument 4
 *   EDI = argument 5
 *   EAX = return value
 *
 * Syscall invocation: INT 0x80
 *
 * ===========================================================================
 */

#include "interrupts/interrupts.h"
#include "drivers/drivers.h"
#include "scheduler/scheduler.h"
#include "scheduler/task.h"
#include "memory/memory.h"
#include "../config/os_config.h"

/* ---------------------------------------------------------------------------
 * External Function Declarations
 * --------------------------------------------------------------------------- */

/* IDT function to register syscall handler */
extern void idt_set_gate(uint8_t vector, uint32_t handler, uint16_t selector, uint8_t flags);

/* Syscall stub from assembly */
extern uint32_t syscall_stub_addr;

/* VFS functions (when available) */
extern int vfs_open(const char *path);
extern ssize_t vfs_read(int fd, void *buffer, size_t size);
extern ssize_t vfs_write(int fd, const void *buffer, size_t size);
extern int vfs_close(int fd);

/* String functions */
extern size_t strlen(const char *s);

/* ---------------------------------------------------------------------------
 * System Call Numbers
 * ---------------------------------------------------------------------------
 * Following a Linux-like convention for system call numbering.
 * --------------------------------------------------------------------------- */

#define SYS_EXIT        1       /* Exit the current process */
#define SYS_FORK        2       /* Fork a new process */
#define SYS_READ        3       /* Read from file descriptor */
#define SYS_WRITE       4       /* Write to file descriptor */
#define SYS_OPEN        5       /* Open a file */
#define SYS_CLOSE       6       /* Close a file descriptor */
#define SYS_WAITPID     7       /* Wait for process */
#define SYS_CREAT       8       /* Create a file */
#define SYS_UNLINK      10      /* Delete a file */
#define SYS_EXECVE      11      /* Execute a program */
#define SYS_CHDIR       12      /* Change directory */
#define SYS_TIME        13      /* Get time */
#define SYS_GETPID      20      /* Get process ID */
#define SYS_SLEEP       35      /* Sleep for ticks */
#define SYS_YIELD       158     /* Yield CPU */
#define SYS_SBRK        45      /* Extend heap (simplified) */

/* Maximum syscall number supported */
#define SYS_MAX         256

/* ---------------------------------------------------------------------------
 * Standard File Descriptors
 * --------------------------------------------------------------------------- */
#define STDIN_FD        0       /* Standard input (keyboard) */
#define STDOUT_FD       1       /* Standard output (screen) */
#define STDERR_FD       2       /* Standard error (screen) */

/* ---------------------------------------------------------------------------
 * Syscall Handler Function Type
 * ---------------------------------------------------------------------------
 * Each syscall handler takes the interrupt frame and returns an int32_t result.
 * Parameters are extracted from the saved registers in the frame.
 * --------------------------------------------------------------------------- */
typedef int32_t (*syscall_fn_t)(interrupt_frame_t *frame);

/* ---------------------------------------------------------------------------
 * Forward Declarations of System Call Handlers
 * --------------------------------------------------------------------------- */
static int32_t sys_exit_handler(interrupt_frame_t *frame);
static int32_t sys_fork_handler(interrupt_frame_t *frame);
static int32_t sys_read_handler(interrupt_frame_t *frame);
static int32_t sys_write_handler(interrupt_frame_t *frame);
static int32_t sys_open_handler(interrupt_frame_t *frame);
static int32_t sys_close_handler(interrupt_frame_t *frame);
static int32_t sys_getpid_handler(interrupt_frame_t *frame);
static int32_t sys_sleep_handler(interrupt_frame_t *frame);
static int32_t sys_yield_handler(interrupt_frame_t *frame);
static int32_t sys_sbrk_handler(interrupt_frame_t *frame);

/* ---------------------------------------------------------------------------
 * System Call Table
 * ---------------------------------------------------------------------------
 * Array of function pointers indexed by syscall number.
 * NULL entries indicate unimplemented syscalls.
 * --------------------------------------------------------------------------- */
static syscall_fn_t syscall_table[SYS_MAX] = {
    [0]          = NULL,                /* Reserved */
    [SYS_EXIT]   = sys_exit_handler,    /* 1: exit */
    [SYS_FORK]   = sys_fork_handler,    /* 2: fork */
    [SYS_READ]   = sys_read_handler,    /* 3: read */
    [SYS_WRITE]  = sys_write_handler,   /* 4: write */
    [SYS_OPEN]   = sys_open_handler,    /* 5: open */
    [SYS_CLOSE]  = sys_close_handler,   /* 6: close */
    [SYS_GETPID] = sys_getpid_handler,  /* 20: getpid */
    [SYS_SLEEP]  = sys_sleep_handler,   /* 35: sleep */
    [SYS_SBRK]   = sys_sbrk_handler,    /* 45: sbrk */
    [SYS_YIELD]  = sys_yield_handler,   /* 158: yield */
};

/* ---------------------------------------------------------------------------
 * Syscall Statistics (for debugging)
 * --------------------------------------------------------------------------- */
static uint32_t syscall_count = 0;
static uint32_t syscall_errors = 0;

/* ---------------------------------------------------------------------------
 * sys_exit_handler - Terminate the current process
 * ---------------------------------------------------------------------------
 * Parameters:
 *   EBX = exit status code
 *
 * Returns: Does not return (task is terminated)
 * --------------------------------------------------------------------------- */
static int32_t sys_exit_handler(interrupt_frame_t *frame)
{
    int32_t status = (int32_t)frame->ebx;
    
    /* Get current task and terminate it */
    task_t *current = task_current();
    if (current != NULL) {
        /* Set exit code and terminate */
        task_exit(status);
    }
    
    /* Should not reach here - schedule next task */
    schedule();
    
    return 0;  /* Never reached */
}

/* ---------------------------------------------------------------------------
 * sys_fork_handler - Create a new process (simplified)
 * ---------------------------------------------------------------------------
 * Creates a copy of the current process.
 *
 * Returns:
 *   In parent: PID of child process
 *   In child: 0
 *   On error: -1
 *
 * Note: This is a simplified implementation. Full fork() requires:
 * - Copying address space
 * - Copying file descriptor table
 * - Setting up separate stacks
 * --------------------------------------------------------------------------- */
static int32_t sys_fork_handler(interrupt_frame_t *frame)
{
    (void)frame;  /* Unused for now */
    
    /* TODO: Implement full fork semantics */
    /* For now, return error as fork is complex to implement properly */
    
    /* A minimal implementation would:
     * 1. Create new task structure
     * 2. Copy parent's stack and registers
     * 3. Set child's return value to 0
     * 4. Add child to scheduler
     * 5. Return child PID to parent
     */
    
    return -1;  /* ENOSYS - not implemented */
}

/* ---------------------------------------------------------------------------
 * sys_read_handler - Read from a file descriptor
 * ---------------------------------------------------------------------------
 * Parameters:
 *   EBX = file descriptor
 *   ECX = buffer pointer
 *   EDX = count (bytes to read)
 *
 * Returns: Number of bytes read, or -1 on error
 * --------------------------------------------------------------------------- */
static int32_t sys_read_handler(interrupt_frame_t *frame)
{
    int fd = (int)frame->ebx;
    char *buffer = (char *)frame->ecx;
    size_t count = (size_t)frame->edx;
    
    /* Validate buffer pointer */
    if (buffer == NULL || count == 0) {
        return -1;  /* EINVAL */
    }
    
    /* Handle standard input (keyboard) */
    if (fd == STDIN_FD) {
        size_t bytes_read = 0;
        
        while (bytes_read < count) {
            char c = keyboard_getchar_blocking();
            
            /* Echo character to screen */
            vga_putchar(c);
            
            /* Handle backspace */
            if (c == '\b' && bytes_read > 0) {
                bytes_read--;
                continue;
            }
            
            buffer[bytes_read++] = c;
            
            /* Stop on newline */
            if (c == '\n') {
                break;
            }
        }
        
        return (int32_t)bytes_read;
    }
    
    /* For other file descriptors, use VFS (if available) */
    /* TODO: Check if VFS is initialized */
    return -1;  /* EBADF - bad file descriptor */
}

/* ---------------------------------------------------------------------------
 * sys_write_handler - Write to a file descriptor
 * ---------------------------------------------------------------------------
 * Parameters:
 *   EBX = file descriptor
 *   ECX = buffer pointer
 *   EDX = count (bytes to write)
 *
 * Returns: Number of bytes written, or -1 on error
 * --------------------------------------------------------------------------- */
static int32_t sys_write_handler(interrupt_frame_t *frame)
{
    int fd = (int)frame->ebx;
    const char *buffer = (const char *)frame->ecx;
    size_t count = (size_t)frame->edx;
    
    /* Validate buffer pointer */
    if (buffer == NULL) {
        return -1;  /* EINVAL */
    }
    
    /* Handle standard output and error (VGA screen) */
    if (fd == STDOUT_FD || fd == STDERR_FD) {
        /* Set error color for stderr */
        if (fd == STDERR_FD) {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        }
        
        /* Write each character to screen */
        for (size_t i = 0; i < count; i++) {
            vga_putchar(buffer[i]);
        }
        
        /* Reset color after stderr */
        if (fd == STDERR_FD) {
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        }
        
        return (int32_t)count;
    }
    
    /* For other file descriptors, use VFS (if available) */
    /* TODO: Implement VFS write */
    return -1;  /* EBADF - bad file descriptor */
}

/* ---------------------------------------------------------------------------
 * sys_open_handler - Open a file
 * ---------------------------------------------------------------------------
 * Parameters:
 *   EBX = pathname pointer
 *   ECX = flags (O_RDONLY, O_WRONLY, O_RDWR, etc.)
 *   EDX = mode (permissions, for O_CREAT)
 *
 * Returns: File descriptor on success, -1 on error
 * --------------------------------------------------------------------------- */
static int32_t sys_open_handler(interrupt_frame_t *frame)
{
    const char *pathname = (const char *)frame->ebx;
    /* int flags = (int)frame->ecx; */
    /* int mode = (int)frame->edx; */
    
    /* Validate pathname */
    if (pathname == NULL) {
        return -1;  /* EINVAL */
    }
    
    /* Use VFS to open file */
    return vfs_open(pathname);
}

/* ---------------------------------------------------------------------------
 * sys_close_handler - Close a file descriptor
 * ---------------------------------------------------------------------------
 * Parameters:
 *   EBX = file descriptor
 *
 * Returns: 0 on success, -1 on error
 * --------------------------------------------------------------------------- */
static int32_t sys_close_handler(interrupt_frame_t *frame)
{
    int fd = (int)frame->ebx;
    
    /* Don't allow closing standard streams */
    if (fd == STDIN_FD || fd == STDOUT_FD || fd == STDERR_FD) {
        return -1;  /* EINVAL */
    }
    
    /* Use VFS to close file */
    return vfs_close(fd);
}

/* ---------------------------------------------------------------------------
 * sys_getpid_handler - Get process ID
 * ---------------------------------------------------------------------------
 * Returns: PID of the current process
 * --------------------------------------------------------------------------- */
static int32_t sys_getpid_handler(interrupt_frame_t *frame)
{
    (void)frame;  /* Unused */
    
    task_t *current = task_current();
    if (current != NULL) {
        return (int32_t)current->pid;
    }
    
    return 0;  /* Kernel context */
}

/* ---------------------------------------------------------------------------
 * sys_sleep_handler - Sleep for specified ticks
 * ---------------------------------------------------------------------------
 * Parameters:
 *   EBX = number of ticks to sleep
 *
 * Returns: 0 on success
 * --------------------------------------------------------------------------- */
static int32_t sys_sleep_handler(interrupt_frame_t *frame)
{
    uint32_t ticks = frame->ebx;
    
    if (ticks > 0) {
        task_sleep(ticks);
    }
    
    return 0;
}

/* ---------------------------------------------------------------------------
 * sys_yield_handler - Yield CPU to another process
 * ---------------------------------------------------------------------------
 * Returns: 0 on success
 * --------------------------------------------------------------------------- */
static int32_t sys_yield_handler(interrupt_frame_t *frame)
{
    (void)frame;  /* Unused */
    
    task_yield();
    
    return 0;
}

/* ---------------------------------------------------------------------------
 * sys_sbrk_handler - Extend heap (simplified brk/sbrk)
 * ---------------------------------------------------------------------------
 * Parameters:
 *   EBX = increment (bytes to add to heap)
 *
 * Returns: Previous break value, or -1 on error
 *
 * Note: This is a simplified implementation. Real sbrk needs per-process
 * heap tracking.
 * --------------------------------------------------------------------------- */
static int32_t sys_sbrk_handler(interrupt_frame_t *frame)
{
    (void)frame;  /* Unused for now */
    
    /* TODO: Implement per-process heap management */
    /* For now, userland should use a fixed memory region */
    
    return -1;  /* ENOMEM */
}

/* ---------------------------------------------------------------------------
 * syscall_handler - Main syscall dispatcher (called from assembly)
 * ---------------------------------------------------------------------------
 * This function is called by the syscall stub in isr_stubs.asm.
 * It extracts the syscall number from EAX and dispatches to the
 * appropriate handler.
 *
 * Parameters:
 *   frame - Pointer to the interrupt frame with saved registers
 *
 * Returns: Result code (placed in EAX by assembly stub)
 * --------------------------------------------------------------------------- */
int32_t syscall_handler(interrupt_frame_t *frame)
{
    uint32_t syscall_num = frame->eax;
    int32_t result = -1;  /* Default: error */
    
    syscall_count++;
    
    /* Validate syscall number */
    if (syscall_num >= SYS_MAX) {
        syscall_errors++;
        return -1;  /* ENOSYS - invalid syscall number */
    }
    
    /* Get handler from table */
    syscall_fn_t handler = syscall_table[syscall_num];
    
    if (handler == NULL) {
        syscall_errors++;
        return -1;  /* ENOSYS - syscall not implemented */
    }
    
    /* Call the handler */
    result = handler(frame);
    
    return result;
}

/* ---------------------------------------------------------------------------
 * syscall_init - Initialize the syscall subsystem
 * ---------------------------------------------------------------------------
 * Registers INT 0x80 handler with the IDT.
 * Must be called after idt_init() but before enabling interrupts.
 * --------------------------------------------------------------------------- */
void syscall_init(void)
{
    /*
     * Register syscall handler at vector 0x80 (128)
     * 
     * IDT_GATE_INTERRUPT_USER (0xEE):
     *   - Present = 1
     *   - DPL = 3 (allow user mode to call)
     *   - Type = 32-bit interrupt gate (0xE)
     *
     * This allows ring 3 (user mode) code to trigger INT 0x80.
     */
    #define KERNEL_CS               0x08
    #define IDT_GATE_INTERRUPT_USER 0xEE    /* DPL=3, 32-bit interrupt gate */
    
    idt_set_gate(
        SYSCALL_VECTOR,             /* Vector 0x80 (128) */
        syscall_stub_addr,          /* Handler address */
        KERNEL_CS,                  /* Kernel code segment */
        IDT_GATE_INTERRUPT_USER     /* Allow user mode access */
    );
}

/* ---------------------------------------------------------------------------
 * syscall_get_stats - Get syscall statistics
 * ---------------------------------------------------------------------------
 * Parameters:
 *   count  - Output: total syscalls processed
 *   errors - Output: syscalls that returned errors
 * --------------------------------------------------------------------------- */
void syscall_get_stats(uint32_t *count, uint32_t *errors)
{
    if (count != NULL) {
        *count = syscall_count;
    }
    if (errors != NULL) {
        *errors = syscall_errors;
    }
}
