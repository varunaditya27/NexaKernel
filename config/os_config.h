/*
 * ===========================================================================
 * os_config.h
 * ===========================================================================
 *
 * NexaKernel - Global Operating System Configuration
 *
 * This file contains compile-time configuration macros that control the
 * behavior and features of the kernel. Modify these values to customize
 * the kernel for different requirements or hardware configurations.
 *
 * ===========================================================================
 */

#ifndef OS_CONFIG_H
#define OS_CONFIG_H

/* ---------------------------------------------------------------------------
 * Version Information
 * --------------------------------------------------------------------------- */
#define NEXAKERNEL_VERSION_MAJOR    0
#define NEXAKERNEL_VERSION_MINOR    1
#define NEXAKERNEL_VERSION_PATCH    0
#define NEXAKERNEL_VERSION_STRING   "0.1.0"

/* ---------------------------------------------------------------------------
 * Debugging Configuration
 * --------------------------------------------------------------------------- */
#define DEBUG_ENABLED               1       /* Enable debug output */
#define DEBUG_VERBOSE               0       /* Extra verbose output */
#define DEBUG_MEMORY                0       /* Memory allocator debugging */
#define DEBUG_SCHEDULER             0       /* Scheduler debugging */

/* ---------------------------------------------------------------------------
 * Memory Configuration
 * --------------------------------------------------------------------------- */
#define PAGE_SIZE                   4096    /* 4KB pages */
#define KERNEL_HEAP_SIZE            (16 * 1024 * 1024)  /* 16MB kernel heap */
#define KERNEL_STACK_SIZE           (16 * 1024)         /* 16KB kernel stack */
#define MAX_PHYSICAL_MEMORY         (256 * 1024 * 1024) /* 256MB max RAM */

/* Memory layout addresses */
#define KERNEL_LOAD_ADDRESS         0x00100000  /* 1MB - kernel start */
#define KERNEL_HEAP_START           0x01000000  /* 16MB - heap start */
#define VGA_BUFFER_ADDRESS          0x000B8000  /* VGA text mode buffer */

/* ---------------------------------------------------------------------------
 * Scheduler Configuration
 * --------------------------------------------------------------------------- */
#define SCHEDULER_PREEMPTIVE        1       /* 1 = preemptive, 0 = cooperative */
#define SCHEDULER_TICK_HZ           100     /* Timer interrupts per second */
#define SCHEDULER_TIME_SLICE        10      /* Ticks per time slice */
#define MAX_TASKS                   64      /* Maximum concurrent tasks */
#define MAX_PRIORITY_LEVELS         8       /* Priority queue levels */

/* ---------------------------------------------------------------------------
 * Interrupt Configuration
 * --------------------------------------------------------------------------- */
#define MAX_IRQ_HANDLERS            16      /* Number of IRQ lines */
#define PIC1_BASE                   0x20    /* Master PIC base interrupt */
#define PIC2_BASE                   0x28    /* Slave PIC base interrupt */

/* ---------------------------------------------------------------------------
 * Device Driver Configuration
 * --------------------------------------------------------------------------- */
#define KEYBOARD_BUFFER_SIZE        256     /* Keyboard input buffer */
#define SERIAL_BAUD_RATE            115200  /* Serial port baud rate */

/* ---------------------------------------------------------------------------
 * Filesystem Configuration
 * --------------------------------------------------------------------------- */
#define MAX_OPEN_FILES              32      /* Max open file descriptors */
#define MAX_FILENAME_LENGTH         256     /* Maximum filename length */
#define RAMFS_MAX_SIZE              (4 * 1024 * 1024)  /* 4MB for RAM FS */

/* ---------------------------------------------------------------------------
 * IPC Configuration
 * --------------------------------------------------------------------------- */
#define MAX_MESSAGE_QUEUES          16      /* Maximum message queues */
#define MAX_MESSAGE_SIZE            1024    /* Maximum message size (bytes) */
#define MAX_SHARED_MEMORY_REGIONS   8       /* Maximum shared memory regions */

/* ---------------------------------------------------------------------------
 * Utility Macros
 * --------------------------------------------------------------------------- */
#define ALIGN_UP(x, align)      (((x) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(x, align)    ((x) & ~((align) - 1))
#define ARRAY_SIZE(arr)         (sizeof(arr) / sizeof((arr)[0]))
#define UNUSED(x)               ((void)(x))

/* NULL pointer definition */
#ifndef NULL
#define NULL                    ((void *)0)
#endif

/* Boolean type */
#ifndef __cplusplus
typedef enum { false = 0, true = 1 } bool;
#endif

/* ---------------------------------------------------------------------------
 * Panic Macro
 * --------------------------------------------------------------------------- */
void panic_impl(const char *file, int line, const char *message);
#define PANIC(msg) panic_impl(__FILE__, __LINE__, msg)

/* ---------------------------------------------------------------------------
 * Fixed-width Integer Types
 * --------------------------------------------------------------------------- */
typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef signed short        int16_t;
typedef unsigned short      uint16_t;
typedef signed int          int32_t;
typedef unsigned int        uint32_t;
typedef signed long long    int64_t;
typedef unsigned long long  uint64_t;

typedef uint32_t            size_t;
typedef int32_t             ssize_t;
typedef uint32_t            uintptr_t;
typedef int32_t             intptr_t;

#endif /* OS_CONFIG_H */
