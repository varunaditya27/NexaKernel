/*
 * kernel/kernel.c
 *
 * Kernel Entry Point
 *
 * This file contains the main C entry point for the kernel (`kernel_main`).
 * It is responsible for orchestrating the initialization of all kernel subsystems,
 * including memory management, scheduling, file systems, and drivers.
 *
 * After initialization, it typically yields control to the scheduler or an idle loop.
 */

#include "../../config/os_config.h"
#include "memory/memory.h"
#include "scheduler/scheduler.h" // Assuming this exists or I declare prototypes
#include "fs/ramfs.h" // Assuming this exists or I declare prototypes
#include "utils/test_dsa.h"

// Declare external init functions if headers are missing
extern void init_scheduler(void);
extern void ramfs_init(void);

// Mock memory map (in real OS, passed from bootloader)
#define MEMORY_SIZE (128 * 1024 * 1024) // 128 MB
#define KERNEL_HEAP_START 0x1000000 // 16 MB mark
#define KERNEL_HEAP_SIZE (16 * 1024 * 1024) // 16 MB
#define BITMAP_BUFFER_ADDR 0x500000 // Arbitrary safe address

void kernel_main(void) {
    // 1. Initialize Memory Manager
    // Initialize frame allocator (bitmap)
    frame_init((void *)BITMAP_BUFFER_ADDR, MEMORY_SIZE, 0x0);
    
    // Initialize kernel heap
    heap_init((void *)KERNEL_HEAP_START, KERNEL_HEAP_SIZE);

    // 2. Run DSA Verification Tests
    test_dsa_all();

    // 3. Initialize Scheduler
    init_scheduler();

    // 4. Initialize File System
    ramfs_init();

    // 5. Enter infinite loop (idle task)
    while (1) {
        // schedule(); // Uncomment when interrupts are set up
    }
}
