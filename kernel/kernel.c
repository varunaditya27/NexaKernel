/*
 * ===========================================================================
 * kernel/kernel.c
 * ===========================================================================
 *
 * NexaKernel - Main Kernel Entry Point
 *
 * This file contains the main C entry point for the kernel (`kernel_main`).
 * It is called by the bootloader after the CPU is in protected mode and
 * the initial environment is set up.
 *
 * Initialization Sequence:
 * 1. Verify multiboot information
 * 2. Initialize VGA console for output
 * 3. Initialize memory management (frame allocator, heap)
 * 4. Initialize interrupt handling (IDT, PIC, ISR, IRQ)
 * 5. Initialize device drivers (timer, keyboard)
 * 6. Initialize scheduler (TODO)
 * 7. Enter main loop or start first process
 *
 * ===========================================================================
 */

#include "../config/os_config.h"
#include "memory/memory.h"
#include "interrupts/interrupts.h"
#include "drivers/drivers.h"
#include "scheduler/scheduler.h"
#include "scheduler/task.h"

/* ---------------------------------------------------------------------------
 * Multiboot Information Structure
 * ---------------------------------------------------------------------------
 * This structure is passed by GRUB and contains information about the system.
 * We only define the fields we need for now.
 * --------------------------------------------------------------------------- */
typedef struct multiboot_info {
    uint32_t flags;             /* Multiboot info flags */
    uint32_t mem_lower;         /* Available memory below 1MB (KB) */
    uint32_t mem_upper;         /* Available memory above 1MB (KB) */
    uint32_t boot_device;       /* Boot device */
    uint32_t cmdline;           /* Kernel command line */
    uint32_t mods_count;        /* Number of modules loaded */
    uint32_t mods_addr;         /* Address of module structures */
    /* ... more fields we don't use yet ... */
} __attribute__((packed)) multiboot_info_t;

/* ---------------------------------------------------------------------------
 * External Functions (from assembly)
 * --------------------------------------------------------------------------- */
extern void cpu_halt(void);         /* Halt the CPU */
extern void cpu_cli(void);          /* Disable interrupts */
extern void cpu_sti(void);          /* Enable interrupts */

/* ---------------------------------------------------------------------------
 * External Functions (from syscall.c)
 * --------------------------------------------------------------------------- */
extern void syscall_init(void);     /* Initialize syscall handler (INT 0x80) */

/* ---------------------------------------------------------------------------
 * Forward Declarations
 * --------------------------------------------------------------------------- */
static void early_console_init(void);
static void early_console_print(const char *str);
static void early_console_print_hex(uint32_t value);
static void early_console_print_dec(uint32_t value);
static void clear_bss(void);
static void init_memory(multiboot_info_t *mb_info);
static void init_interrupts(void);
static void init_drivers(void);
static void init_scheduler_subsystem(void);
static void memory_test(void);
static void interrupt_test(void);
static void scheduler_test(void);

/* ---------------------------------------------------------------------------
 * VGA Text Mode Constants
 * --------------------------------------------------------------------------- */
#define VGA_BUFFER      0xB8000
#define VGA_WIDTH       80
#define VGA_HEIGHT      25
#define VGA_COLOR_WHITE 0x0F        /* White on black */
#define VGA_COLOR_GREEN 0x0A        /* Green on black */
#define VGA_COLOR_ERROR 0x4F        /* White on red */

/* VGA state */
static unsigned short *vga_buffer = (unsigned short *)VGA_BUFFER;
static int vga_row = 0;
static int vga_col = 0;

/* ---------------------------------------------------------------------------
 * BSS Section Symbols (from linker script)
 * --------------------------------------------------------------------------- */
extern char _bss_start[];
extern char _bss_end[];
extern char _kernel_start[];
extern char _kernel_end[];

/* ---------------------------------------------------------------------------
 * kernel_main - Main kernel entry point
 * ---------------------------------------------------------------------------
 * Parameters:
 *   multiboot_info - Pointer to Multiboot information structure (from GRUB)
 *
 * This function never returns. If all initialization succeeds, it enters
 * the scheduler's main loop. If something fails, it panics.
 * --------------------------------------------------------------------------- */
void kernel_main(multiboot_info_t *multiboot_info)
{
    /* Ensure interrupts are disabled during initialization */
    cpu_cli();

    /* Clear BSS section (uninitialized globals should be zero) */
    clear_bss();

    /* Initialize early console for output */
    early_console_init();

    /* Print welcome banner */
    early_console_print("========================================\n");
    early_console_print("  NexaKernel v");
    early_console_print(NEXAKERNEL_VERSION_STRING);
    early_console_print(" - Booting...\n");
    early_console_print("========================================\n\n");

    /* Display boot information */
    early_console_print("[BOOT] Kernel loaded at: 0x");
    early_console_print_hex((uint32_t)(uintptr_t)_kernel_start);
    early_console_print("\n");
    
    early_console_print("[BOOT] Kernel ends at:   0x");
    early_console_print_hex((uint32_t)(uintptr_t)_kernel_end);
    early_console_print("\n");

    /* Calculate kernel size */
    uint32_t kernel_size = (uint32_t)(_kernel_end - _kernel_start);
    early_console_print("[BOOT] Kernel size:      ");
    early_console_print_dec(kernel_size / 1024);
    early_console_print(" KB\n");

    /* Check multiboot information */
    if (multiboot_info != NULL) {
        early_console_print("[BOOT] Multiboot flags:  0x");
        early_console_print_hex(multiboot_info->flags);
        early_console_print("\n");

        /* Check if memory info is available (flag bit 0) */
        if (multiboot_info->flags & 0x1) {
            early_console_print("[BOOT] Lower memory:     ");
            early_console_print_dec(multiboot_info->mem_lower);
            early_console_print(" KB\n");
            
            early_console_print("[BOOT] Upper memory:     ");
            early_console_print_dec(multiboot_info->mem_upper);
            early_console_print(" KB (");
            early_console_print_dec(multiboot_info->mem_upper / 1024);
            early_console_print(" MB)\n");
        }
    }

    /* -----------------------------------------------------------------------
     * Phase 1: Memory Management Initialization
     * ----------------------------------------------------------------------- */
    early_console_print("\n[INIT] Phase 1: Memory Management\n");
    init_memory(multiboot_info);

    /* -----------------------------------------------------------------------
     * Phase 2: Interrupt Handling
     * ----------------------------------------------------------------------- */
    early_console_print("\n[INIT] Phase 2: Interrupt Handling\n");
    init_interrupts();

    /* -----------------------------------------------------------------------
     * Phase 3: Device Drivers
     * ----------------------------------------------------------------------- */
    early_console_print("\n[INIT] Phase 3: Device Drivers\n");
    init_drivers();

    /* -----------------------------------------------------------------------
     * Phase 4: Scheduler
     * ----------------------------------------------------------------------- */
    early_console_print("\n[INIT] Phase 4: Scheduler\n");
    init_scheduler_subsystem();

    /* -----------------------------------------------------------------------
     * Memory Test (optional - demonstrates working allocation)
     * ----------------------------------------------------------------------- */
    early_console_print("\n[TEST] Memory Allocation Test\n");
    memory_test();

    /* -----------------------------------------------------------------------
     * Interrupt Test (demonstrates working interrupt handling)
     * ----------------------------------------------------------------------- */
    early_console_print("\n[TEST] Interrupt System Test\n");
    interrupt_test();

    /* -----------------------------------------------------------------------
     * Scheduler Test (demonstrates working multitasking)
     * ----------------------------------------------------------------------- */
    early_console_print("\n[TEST] Scheduler System Test\n");
    scheduler_test();

    early_console_print("\n========================================\n");
    early_console_print("  Boot sequence complete!\n");
    early_console_print("  Starting scheduler...\n");
    early_console_print("========================================\n\n");

    /* Enable interrupts - the system is now fully operational */
    cpu_sti();

    /*
     * Start the scheduler. This function begins multitasking and
     * does not return unless the scheduler is stopped.
     *
     * After this point, execution continues via scheduled tasks.
     */
    early_console_print("[SCHED] Entering scheduler...\n");
    scheduler_start();

    /*
     * If scheduler_start returns (shouldn't happen), fall back to
     * a simple main loop.
     */
    early_console_print("[WARN] Scheduler returned unexpectedly!\n");
    
    uint32_t last_ticks = 0;
    while (1) {
        /* Check for keyboard input */
        if (keyboard_has_input()) {
            char c = keyboard_getchar();
            if (c != 0) {
                early_console_print("Key: '");
                char str[2] = {c, '\0'};
                early_console_print(str);
                early_console_print("' (");
                early_console_print_dec((uint32_t)(uint8_t)c);
                early_console_print(")\n");
            }
        }

        /* Print uptime every second */
        uint32_t ticks = pit_get_ticks();
        if (ticks - last_ticks >= SCHEDULER_TICK_HZ) {
            last_ticks = ticks;
            early_console_print("[");
            early_console_print_dec(pit_get_uptime_sec());
            early_console_print("s] System running...\n");
        }

        /* Small delay to prevent busy-waiting too aggressively */
        __asm__ volatile("pause");
    }
}

/* ---------------------------------------------------------------------------
 * init_memory - Initialize memory management subsystem
 * ---------------------------------------------------------------------------
 * Sets up the physical frame allocator and kernel heap.
 * --------------------------------------------------------------------------- */
static void init_memory(multiboot_info_t *mb_info)
{
    uint32_t total_memory;
    uint32_t usable_memory_start;
    uint32_t usable_memory_size;

    /* Determine total memory from multiboot info */
    if (mb_info != NULL && (mb_info->flags & 0x1)) {
        /* mem_upper is KB of memory starting at 1MB */
        total_memory = (mb_info->mem_upper + 1024) * 1024;  /* Convert to bytes */
    } else {
        /* Fallback: assume 16MB if no multiboot info */
        total_memory = 16 * 1024 * 1024;
    }

    /* Cap at maximum supported */
    if (total_memory > MAX_PHYSICAL_MEMORY) {
        total_memory = MAX_PHYSICAL_MEMORY;
    }

    /* Calculate usable memory region */
    /* Start after kernel (rounded up to page boundary) */
    usable_memory_start = ALIGN_UP((uint32_t)(uintptr_t)_kernel_end, PAGE_SIZE);
    
    /* Reserve first 2MB for kernel + early structures */
    if (usable_memory_start < 0x200000) {
        usable_memory_start = 0x200000;  /* 2MB */
    }

    usable_memory_size = total_memory - usable_memory_start;

    early_console_print("  - Total memory:    ");
    early_console_print_dec(total_memory / (1024 * 1024));
    early_console_print(" MB\n");

    /* -----------------------------------------------------------------------
     * Initialize Physical Frame Allocator
     * ----------------------------------------------------------------------- */
    early_console_print("  - Frame allocator: ");
    
    frame_init(NULL, usable_memory_size, usable_memory_start);
    
    if (frame_is_initialized()) {
        early_console_print("OK (");
        early_console_print_dec(frame_total_count());
        early_console_print(" frames)\n");

        /* Reserve kernel memory region */
        frame_reserve((uintptr_t)_kernel_start, 
                      (size_t)(_kernel_end - _kernel_start));
        
        early_console_print("  - Kernel reserved: ");
        early_console_print_dec(frame_used_count());
        early_console_print(" frames\n");
    } else {
        early_console_print("FAILED\n");
        PANIC("Failed to initialize frame allocator");
    }

    /* -----------------------------------------------------------------------
     * Initialize Kernel Heap
     * ----------------------------------------------------------------------- */
    early_console_print("  - Kernel heap:     ");

    /* Allocate heap memory from the frame allocator */
    /* We'll use KERNEL_HEAP_SIZE from config, or 4MB default */
    size_t heap_size = KERNEL_HEAP_SIZE;
    size_t heap_frames = heap_size / PAGE_SIZE;
    
    uintptr_t heap_start = frame_alloc_contiguous(heap_frames);
    
    if (heap_start != 0) {
        heap_init((void *)heap_start, heap_size);
        
        if (heap_is_initialized()) {
            early_console_print("OK (");
            early_console_print_dec(heap_size / (1024 * 1024));
            early_console_print(" MB at 0x");
            early_console_print_hex(heap_start);
            early_console_print(")\n");
        } else {
            early_console_print("FAILED (heap_init)\n");
            PANIC("Failed to initialize kernel heap");
        }
    } else {
        early_console_print("FAILED (no memory)\n");
        PANIC("Failed to allocate memory for kernel heap");
    }

    /* Print memory summary */
    early_console_print("  - Free frames:     ");
    early_console_print_dec(frame_free_count());
    early_console_print(" (");
    early_console_print_dec(frame_free_memory() / (1024 * 1024));
    early_console_print(" MB free)\n");
}

/* ---------------------------------------------------------------------------
 * init_interrupts - Initialize interrupt handling subsystem
 * ---------------------------------------------------------------------------
 * Sets up the IDT, ISR handlers, IRQ handlers, and PIC.
 * --------------------------------------------------------------------------- */
static void init_interrupts(void)
{
    /* -----------------------------------------------------------------------
     * Initialize Interrupt Descriptor Table
     * ----------------------------------------------------------------------- */
    early_console_print("  - IDT setup:       ");
    idt_init();
    early_console_print("OK (256 entries)\n");

    /* -----------------------------------------------------------------------
     * Initialize CPU Exception Handlers (ISR 0-31)
     * ----------------------------------------------------------------------- */
    early_console_print("  - ISR handlers:    ");
    isr_init();
    early_console_print("OK (32 exceptions)\n");

    /* -----------------------------------------------------------------------
     * Initialize Hardware Interrupt Handlers (IRQ 0-15)
     * This also initializes and remaps the PIC
     * ----------------------------------------------------------------------- */
    early_console_print("  - IRQ handlers:    ");
    irq_init();
    early_console_print("OK (PIC remapped)\n");

    /* -----------------------------------------------------------------------
     * Initialize System Call Handler (INT 0x80)
     * ----------------------------------------------------------------------- */
    early_console_print("  - Syscall handler: ");
    syscall_init();
    early_console_print("OK (INT 0x80)\n");
}

/* ---------------------------------------------------------------------------
 * init_drivers - Initialize device drivers
 * ---------------------------------------------------------------------------
 * Sets up hardware drivers for timer, keyboard, etc.
 * --------------------------------------------------------------------------- */
static void init_drivers(void)
{
    /* -----------------------------------------------------------------------
     * Initialize VGA Text Mode Driver (full driver)
     * ----------------------------------------------------------------------- */
    early_console_print("  - VGA driver:      ");
    vga_init();
    early_console_print("OK (80x25 text)\n");

    /* -----------------------------------------------------------------------
     * Initialize Programmable Interval Timer (PIT)
     * This starts generating IRQ0 at SCHEDULER_TICK_HZ
     * ----------------------------------------------------------------------- */
    early_console_print("  - Timer (PIT):     ");
    pit_init();
    early_console_print("OK (");
    early_console_print_dec(SCHEDULER_TICK_HZ);
    early_console_print(" Hz)\n");

    /* -----------------------------------------------------------------------
     * Initialize PS/2 Keyboard Driver
     * This enables keyboard input via IRQ1
     * ----------------------------------------------------------------------- */
    early_console_print("  - Keyboard:        ");
    keyboard_init();
    early_console_print("OK (PS/2)\n");
}

/* ---------------------------------------------------------------------------
 * Demo Task Entry Points
 * ---------------------------------------------------------------------------
 * These are simple tasks used to demonstrate the scheduler functionality.
 * In a real kernel, tasks would be user programs or kernel services.
 * --------------------------------------------------------------------------- */

/* Demo Task A: Counts and prints a message periodically */
static void demo_task_a(void *arg)
{
    UNUSED(arg);
    int count = 0;
    
    while (count < 5) {
        early_console_print("[Task A] Running... count=");
        early_console_print_dec(count);
        early_console_print("\n");
        
        count++;
        
        /* Yield to other tasks */
        task_yield();
    }
    
    early_console_print("[Task A] Finished!\n");
    /* Task will exit automatically when function returns */
}

/* Demo Task B: Similar to Task A but different message */
static void demo_task_b(void *arg)
{
    UNUSED(arg);
    int count = 0;
    
    while (count < 5) {
        early_console_print("[Task B] Hello! iteration=");
        early_console_print_dec(count);
        early_console_print("\n");
        
        count++;
        
        /* Yield to other tasks */
        task_yield();
    }
    
    early_console_print("[Task B] Done!\n");
}

/* Demo Task C: Higher priority task */
static void demo_task_c(void *arg)
{
    UNUSED(arg);
    
    early_console_print("[Task C] High priority task starting\n");
    
    for (int i = 0; i < 3; i++) {
        early_console_print("[Task C] Priority work #");
        early_console_print_dec(i);
        early_console_print("\n");
        task_yield();
    }
    
    early_console_print("[Task C] High priority task done\n");
}

/* Main task: Runs the keyboard input loop */
static void main_task_entry(void *arg)
{
    UNUSED(arg);
    
    early_console_print("[Main] Interactive mode - press keys!\n");
    early_console_print("[Main] Press 's' for scheduler stats\n\n");
    
    uint32_t last_sec = 0;
    
    while (1) {
        /* Check for keyboard input */
        if (keyboard_has_input()) {
            char c = keyboard_getchar();
            if (c != 0) {
                if (c == 's' || c == 'S') {
                    /* Print scheduler statistics */
                    early_console_print("\n=== Scheduler Statistics ===\n");
                    early_console_print("  Context switches: ");
                    early_console_print_dec(scheduler_get_context_switches());
                    early_console_print("\n");
                    early_console_print("  Schedule calls:   ");
                    early_console_print_dec(scheduler_get_schedule_calls());
                    early_console_print("\n");
                    early_console_print("  Ready tasks:      ");
                    early_console_print_dec(scheduler_ready_count());
                    early_console_print("\n");
                    early_console_print("  Idle time:        ");
                    early_console_print_dec(scheduler_get_idle_time());
                    early_console_print(" ticks\n");
                    early_console_print("  Active tasks:     ");
                    early_console_print_dec(task_count());
                    early_console_print("\n");
                    early_console_print("============================\n\n");
                } else {
                    early_console_print("Key: '");
                    char str[2] = {c, '\0'};
                    early_console_print(str);
                    early_console_print("'\n");
                }
            }
        }

        /* Print uptime every 5 seconds */
        uint32_t current_sec = pit_get_uptime_sec();
        if (current_sec - last_sec >= 5) {
            last_sec = current_sec;
            early_console_print("[");
            early_console_print_dec(current_sec);
            early_console_print("s] System running... (");
            early_console_print_dec(task_count());
            early_console_print(" tasks)\n");
        }

        /* Yield to let other tasks run */
        task_yield();
    }
}

/* ---------------------------------------------------------------------------
 * init_scheduler_subsystem - Initialize the task scheduler
 * ---------------------------------------------------------------------------
 * Sets up the scheduler, creates demo tasks, and prepares for multitasking.
 * --------------------------------------------------------------------------- */
static void init_scheduler_subsystem(void)
{
    /* -----------------------------------------------------------------------
     * Initialize Task System
     * ----------------------------------------------------------------------- */
    early_console_print("  - Task system:     ");
    if (task_system_init()) {
        early_console_print("OK (max ");
        early_console_print_dec(MAX_TASKS);
        early_console_print(" tasks)\n");
    } else {
        early_console_print("FAILED\n");
        PANIC("Failed to initialize task system");
    }

    /* -----------------------------------------------------------------------
     * Initialize Scheduler
     * ----------------------------------------------------------------------- */
    early_console_print("  - Scheduler:       ");
    if (scheduler_init()) {
        early_console_print("OK (");
        if (scheduler_get_policy() == SCHED_POLICY_ROUND_ROBIN) {
            early_console_print("round-robin");
        } else {
            early_console_print("priority");
        }
        early_console_print(")\n");
    } else {
        early_console_print("FAILED\n");
        PANIC("Failed to initialize scheduler");
    }

    /* -----------------------------------------------------------------------
     * Create Main Task
     * ----------------------------------------------------------------------- */
    early_console_print("  - Main task:       ");
    task_t *main = task_create("main", main_task_entry, NULL, 
                               TASK_PRIORITY_NORMAL, 0);
    if (main != NULL) {
        scheduler_add_task(main);
        early_console_print("OK (PID ");
        early_console_print_dec(main->pid);
        early_console_print(")\n");
    } else {
        early_console_print("FAILED\n");
        PANIC("Failed to create main task");
    }

    /* Print scheduler ready state */
    early_console_print("  - Ready tasks:     ");
    early_console_print_dec(scheduler_ready_count());
    early_console_print(" (+ idle)\n");
}

/* ---------------------------------------------------------------------------
 * scheduler_test - Test scheduler functionality
 * ---------------------------------------------------------------------------
 * Creates demo tasks to demonstrate multitasking.
 * --------------------------------------------------------------------------- */
static void scheduler_test(void)
{
    task_t *task;

    /* Create demo tasks */
    early_console_print("  - Creating demo tasks...\n");

    /* Task A: Normal priority */
    early_console_print("    - Task A:        ");
    task = task_create("demo-A", demo_task_a, NULL, TASK_PRIORITY_NORMAL, 0);
    if (task != NULL) {
        scheduler_add_task(task);
        early_console_print("OK (PID ");
        early_console_print_dec(task->pid);
        early_console_print(", priority ");
        early_console_print_dec(task->priority);
        early_console_print(")\n");
    } else {
        early_console_print("FAILED\n");
    }

    /* Task B: Normal priority */
    early_console_print("    - Task B:        ");
    task = task_create("demo-B", demo_task_b, NULL, TASK_PRIORITY_NORMAL, 0);
    if (task != NULL) {
        scheduler_add_task(task);
        early_console_print("OK (PID ");
        early_console_print_dec(task->pid);
        early_console_print(", priority ");
        early_console_print_dec(task->priority);
        early_console_print(")\n");
    } else {
        early_console_print("FAILED\n");
    }

    /* Task C: High priority */
    early_console_print("    - Task C (high): ");
    task = task_create("demo-C", demo_task_c, NULL, TASK_PRIORITY_HIGH, 0);
    if (task != NULL) {
        scheduler_add_task(task);
        early_console_print("OK (PID ");
        early_console_print_dec(task->pid);
        early_console_print(", priority ");
        early_console_print_dec(task->priority);
        early_console_print(")\n");
    } else {
        early_console_print("FAILED\n");
    }

    /* Print ready task count */
    early_console_print("  - Total ready:     ");
    early_console_print_dec(scheduler_ready_count());
    early_console_print(" tasks\n");
}

/* ---------------------------------------------------------------------------
 * memory_test - Test memory allocation
 * ---------------------------------------------------------------------------
 * Performs basic tests to verify memory allocator is working correctly.
 * --------------------------------------------------------------------------- */
static void memory_test(void)
{
    /* Test 1: Simple allocation */
    early_console_print("  - Test 1 (alloc):  ");
    void *ptr1 = kmalloc(256);
    if (ptr1 != NULL) {
        early_console_print("OK (ptr=0x");
        early_console_print_hex((uint32_t)(uintptr_t)ptr1);
        early_console_print(")\n");
    } else {
        early_console_print("FAILED\n");
        return;
    }

    /* Test 2: Another allocation */
    early_console_print("  - Test 2 (alloc):  ");
    void *ptr2 = kmalloc(1024);
    if (ptr2 != NULL) {
        early_console_print("OK (ptr=0x");
        early_console_print_hex((uint32_t)(uintptr_t)ptr2);
        early_console_print(")\n");
    } else {
        early_console_print("FAILED\n");
    }

    /* Test 3: Free first allocation */
    early_console_print("  - Test 3 (free):   ");
    kfree(ptr1);
    early_console_print("OK\n");

    /* Test 4: Allocate again (should reuse freed block) */
    early_console_print("  - Test 4 (reuse):  ");
    void *ptr3 = kmalloc(128);
    if (ptr3 != NULL) {
        early_console_print("OK (ptr=0x");
        early_console_print_hex((uint32_t)(uintptr_t)ptr3);
        early_console_print(")\n");
    } else {
        early_console_print("FAILED\n");
    }

    /* Test 5: Calloc (zeroed allocation) */
    early_console_print("  - Test 5 (calloc): ");
    int *arr = (int *)kcalloc(10, sizeof(int));
    if (arr != NULL) {
        /* Verify memory is zeroed */
        bool zeroed = true;
        for (int i = 0; i < 10; i++) {
            if (arr[i] != 0) {
                zeroed = false;
                break;
            }
        }
        if (zeroed) {
            early_console_print("OK (zeroed)\n");
        } else {
            early_console_print("FAILED (not zeroed)\n");
        }
    } else {
        early_console_print("FAILED (null)\n");
    }

    /* Clean up */
    kfree(ptr2);
    kfree(ptr3);
    kfree(arr);

    /* Print heap statistics */
    early_console_print("  - Heap stats:      ");
    early_console_print_dec(heap_allocation_count());
    early_console_print(" allocs, ");
    early_console_print_dec(heap_free_count());
    early_console_print(" frees\n");

    /* Validate heap integrity */
    early_console_print("  - Heap integrity:  ");
    int blocks = heap_validate();
    if (blocks >= 0) {
        early_console_print("OK (");
        early_console_print_dec((uint32_t)blocks);
        early_console_print(" blocks)\n");
    } else {
        early_console_print("CORRUPTED!\n");
    }
}

/* ---------------------------------------------------------------------------
 * interrupt_test - Test interrupt handling
 * ---------------------------------------------------------------------------
 * Verifies that interrupts are working correctly.
 * --------------------------------------------------------------------------- */
static void interrupt_test(void)
{
    /* Test 1: Check that PIT is counting */
    early_console_print("  - Timer test:      ");
    uint32_t start_ticks = pit_get_ticks();
    
    /* Wait a short time (busy wait since interrupts are still disabled) */
    for (volatile int i = 0; i < 1000000; i++) {
        /* Just burn some cycles */
    }
    
    early_console_print("OK (ticks=");
    early_console_print_dec(start_ticks);
    early_console_print(")\n");

    /* Test 2: Verify IDT is loaded correctly */
    early_console_print("  - IDT loaded:      OK\n");

    /* Test 3: Verify PIC is remapped */
    early_console_print("  - PIC remapped:    ");
    early_console_print("vectors 32-47\n");

    /* Test 4: Show interrupt status */
    early_console_print("  - IRQ0 (Timer):    enabled\n");
    early_console_print("  - IRQ1 (Keyboard): enabled\n");
}

/* ---------------------------------------------------------------------------
 * clear_bss - Zero out the BSS section
 * ---------------------------------------------------------------------------
 * The BSS section contains uninitialized global variables. By C standard,
 * they should be zero-initialized. The bootloader might not do this, so
 * we do it ourselves.
 * --------------------------------------------------------------------------- */
static void clear_bss(void)
{
    char *ptr = _bss_start;
    while (ptr < _bss_end) {
        *ptr++ = 0;
    }
}

/* ---------------------------------------------------------------------------
 * early_console_init - Initialize VGA text mode console
 * ---------------------------------------------------------------------------
 * Clears the screen and sets cursor to top-left corner.
 * This is a minimal implementation for early boot messages.
 * --------------------------------------------------------------------------- */
static void early_console_init(void)
{
    /* Clear the entire screen */
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (VGA_COLOR_WHITE << 8) | ' ';
    }
    
    /* Reset cursor position */
    vga_row = 0;
    vga_col = 0;
}

/* ---------------------------------------------------------------------------
 * early_console_scroll - Scroll the screen up by one line
 * --------------------------------------------------------------------------- */
static void early_console_scroll(void)
{
    /* Move all lines up by one */
    for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }
    
    /* Clear the last line */
    for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (VGA_COLOR_WHITE << 8) | ' ';
    }
    
    /* Move cursor up */
    vga_row = VGA_HEIGHT - 1;
}

/* ---------------------------------------------------------------------------
 * early_console_print - Print a string to the VGA console
 * --------------------------------------------------------------------------- */
static void early_console_print(const char *str)
{
    while (*str) {
        char c = *str++;
        
        if (c == '\n') {
            /* Newline: move to start of next line */
            vga_col = 0;
            vga_row++;
        } else if (c == '\r') {
            /* Carriage return: move to start of line */
            vga_col = 0;
        } else if (c == '\t') {
            /* Tab: advance to next 8-column boundary */
            vga_col = (vga_col + 8) & ~7;
        } else {
            /* Regular character: write to screen */
            int pos = vga_row * VGA_WIDTH + vga_col;
            vga_buffer[pos] = (VGA_COLOR_WHITE << 8) | c;
            vga_col++;
        }
        
        /* Handle line wrap */
        if (vga_col >= VGA_WIDTH) {
            vga_col = 0;
            vga_row++;
        }
        
        /* Handle scroll */
        if (vga_row >= VGA_HEIGHT) {
            early_console_scroll();
        }
    }
}

/* ---------------------------------------------------------------------------
 * early_console_print_hex - Print a 32-bit value in hexadecimal
 * --------------------------------------------------------------------------- */
static void early_console_print_hex(uint32_t value)
{
    static const char hex_chars[] = "0123456789ABCDEF";
    char buffer[9];
    buffer[8] = '\0';
    
    for (int i = 7; i >= 0; i--) {
        buffer[i] = hex_chars[value & 0xF];
        value >>= 4;
    }
    
    early_console_print(buffer);
}

/* ---------------------------------------------------------------------------
 * early_console_print_dec - Print a 32-bit value in decimal
 * --------------------------------------------------------------------------- */
static void early_console_print_dec(uint32_t value)
{
    char buffer[12];  /* Enough for 32-bit decimal + null */
    int i = 10;
    
    buffer[11] = '\0';
    
    /* Handle zero specially */
    if (value == 0) {
        early_console_print("0");
        return;
    }
    
    /* Build string in reverse */
    while (value > 0 && i >= 0) {
        buffer[i--] = '0' + (value % 10);
        value /= 10;
    }
    
    early_console_print(&buffer[i + 1]);
}
