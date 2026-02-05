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
extern uint32_t cpu_get_flags(void); /* Get EFLAGS */

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
static void early_console_update_cursor(void);
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

    /* Initialize serial port for early debugging output to terminal */
    serial_init();

    /* Clear BSS section (uninitialized globals should be zero) */
    clear_bss();

    /* Initialize early console for output */
    early_console_init();

    /* Print welcome banner */
    early_console_print("\n");
    early_console_print("+========================================================+\n");
    early_console_print("|     _   _                _  __                   _     |\n");
    early_console_print("|    | \\ | | _____  ____ _| |/ /___ _ __ _ __   ___| |   |\n");
    early_console_print("|    |  \\| |/ _ \\ \\/ / _` | ' // _ \\ '__| '_ \\ / _ \\ |   |\n");
    early_console_print("|    | |\\  |  __/>  < (_| | . \\  __/ |  | | | |  __/ |   |\n");
    early_console_print("|    |_| \\_|\\___/_/\\_\\__,_|_|\\_\\___|_|  |_| |_|\\___|_|   |\n");
    early_console_print("|                                                        |\n");
    early_console_print("|          Educational x86 Operating System Kernel       |\n");
    early_console_print("|                     Version ");
    early_console_print(NEXAKERNEL_VERSION_STRING);
    early_console_print("                      |\n");
    early_console_print("+========================================================+\n\n");

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
    early_console_print("\n+--------------- PHASE 1: MEMORY MANAGEMENT ---------------+\n");
    early_console_print("|  Initializing physical frame allocator and kernel heap   |\n");
    early_console_print("+----------------------------------------------------------+\n");
    init_memory(multiboot_info);

    /* -----------------------------------------------------------------------
     * Phase 2: Interrupt Handling
     * ----------------------------------------------------------------------- */
    early_console_print("\n+------------- PHASE 2: INTERRUPT HANDLING ----------------+\n");
    early_console_print("| Setting up IDT, ISR handlers, IRQ handlers, and PIC     |\n");
    early_console_print("+----------------------------------------------------------+\n");
    init_interrupts();

    /* -----------------------------------------------------------------------
     * Phase 3: Device Drivers
     * ----------------------------------------------------------------------- */
    early_console_print("\n+---------------- PHASE 3: DEVICE DRIVERS -----------------+\n");
    early_console_print("| Initializing VGA, PIT Timer, and PS/2 Keyboard drivers  |\n");
    early_console_print("+----------------------------------------------------------+\n");
    init_drivers();

    /* -----------------------------------------------------------------------
     * Phase 4: Scheduler
     * ----------------------------------------------------------------------- */
    early_console_print("\n+--------------- PHASE 4: TASK SCHEDULER ------------------+\n");
    early_console_print("| Initializing round-robin scheduler and creating tasks   |\n");
    early_console_print("+----------------------------------------------------------+\n");
    init_scheduler_subsystem();

    /* -----------------------------------------------------------------------
     * Memory Test (optional - demonstrates working allocation)
     * ----------------------------------------------------------------------- */
    early_console_print("\n+-------------- TEST: HEAP MEMORY ALLOCATOR ---------------+\n");
    early_console_print("| Demonstrating kmalloc(), kfree(), kcalloc() operations   |\n");
    early_console_print("+----------------------------------------------------------+\n");
    memory_test();

    /* -----------------------------------------------------------------------
     * Interrupt Test (demonstrates working interrupt handling)
     * ----------------------------------------------------------------------- */
    early_console_print("\n+-------------- TEST: INTERRUPT SUBSYSTEM -----------------+\n");
    early_console_print("| Verifying IDT, PIC remapping, and IRQ handlers          |\n");
    early_console_print("+----------------------------------------------------------+\n");
    interrupt_test();

    /* -----------------------------------------------------------------------
     * Scheduler Test (demonstrates working multitasking)
     * ----------------------------------------------------------------------- */
    early_console_print("\n+------------ TEST: MULTITASKING SCHEDULER ----------------+\n");
    early_console_print("| Creating demo tasks to demonstrate context switching     |\n");
    early_console_print("+----------------------------------------------------------+\n");
    scheduler_test();

    early_console_print("\n+========================================================+\n");
    early_console_print("|              BOOT SEQUENCE COMPLETE!                   |\n");
    early_console_print("|  All kernel subsystems initialized successfully.       |\n");
    early_console_print("|  Transferring control to the scheduler...              |\n");
    early_console_print("+========================================================+\n\n");

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

    early_console_print("\n  MEMORY DETECTION:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  | Source: Multiboot (from GRUB bootloader)                 |\n");
    early_console_print("  | Total Physical Memory: ");
    early_console_print_dec(total_memory / (1024 * 1024));
    early_console_print(" MB                             |\n");
    early_console_print("  +----------------------------------------------------------+\n");

    /* -----------------------------------------------------------------------
     * Initialize Physical Frame Allocator
     * ----------------------------------------------------------------------- */
    early_console_print("\n  FRAME ALLOCATOR INITIALIZATION:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  | Data Structure: BITMAP (1 bit per 4KB frame)             |\n");
    early_console_print("  | Algorithm:      First-fit scan for free frames           |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    
    frame_init(NULL, usable_memory_size, usable_memory_start);
    
    if (frame_is_initialized()) {
        uint32_t frame_count = frame_total_count();
        early_console_print("  | Status:         INITIALIZED                              |\n");
        early_console_print("  | Total Frames:   ");
        early_console_print_dec(frame_count);
        early_console_print(" (each 4096 bytes)                  |\n");
        early_console_print("  +----------------------------------------------------------+\n");
        
        early_console_print("\n  BITMAP VISUALIZATION (first 64 frames):\n");
        early_console_print("  [0=free, 1=used]\n");
        early_console_print("  Frame 0         16        32        48        64\n");
        early_console_print("        |          |         |         |         |\n");
        early_console_print("        v          v         v         v         v\n");
        early_console_print("        0000000000000000000000000000000000000000000000000000000000000000\n");
        early_console_print("        ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^\n");
        early_console_print("        All frames initially FREE (ready for allocation)\n");

        /* Reserve kernel memory region */
        frame_reserve((uintptr_t)_kernel_start, 
                      (size_t)(_kernel_end - _kernel_start));
        
        early_console_print("\n  After reserving kernel region:\n");
        early_console_print("        1111111111111111111111111100000000000000000000000000000000000000\n");
        early_console_print("        ^~~~~~~~~~~~~~~~~~~~~~~~^\n");
        early_console_print("        Kernel code/data (RESERVED)\n");
    } else {
        early_console_print("  | Status:         FAILED                                  |\n");
        early_console_print("  +----------------------------------------------------------+\n");
        PANIC("Failed to initialize frame allocator");
    }

    /* -----------------------------------------------------------------------
     * Initialize Kernel Heap
     * ----------------------------------------------------------------------- */
    early_console_print("\n  HEAP ALLOCATOR INITIALIZATION:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  | Data Structure: FREE LIST (linked list of free blocks)   |\n");
    early_console_print("  | Algorithm:      First-fit with block splitting/coalesce  |\n");
    early_console_print("  +----------------------------------------------------------+\n");

    /* Allocate heap memory from the frame allocator */
    size_t heap_size = KERNEL_HEAP_SIZE;
    size_t heap_frames = heap_size / PAGE_SIZE;
    
    uintptr_t heap_start = frame_alloc_contiguous(heap_frames);
    
    if (heap_start != 0) {
        heap_init((void *)heap_start, heap_size);
        
        if (heap_is_initialized()) {
            early_console_print("  | Status:         INITIALIZED                              |\n");
            early_console_print("  | Heap Start:     0x");
            early_console_print_hex(heap_start);
            early_console_print("                              |\n");
            early_console_print("  | Heap Size:      ");
            early_console_print_dec(heap_size / (1024 * 1024));
            early_console_print(" MB                                       |\n");
            early_console_print("  +----------------------------------------------------------+\n");
            
            early_console_print("\n  FREE LIST VISUALIZATION:\n");
            early_console_print("  +------+--------------------------------------------------+\n");
            early_console_print("  | HDR  |            SINGLE LARGE FREE BLOCK              |\n");
            early_console_print("  | size |                   (16 MB)                       |\n");
            early_console_print("  | free |                                                 |\n");
            early_console_print("  | next |-> NULL                                          |\n");
            early_console_print("  +------+--------------------------------------------------+\n");
        } else {
            early_console_print("  | Status:         FAILED (heap_init)                      |\n");
            early_console_print("  +----------------------------------------------------------+\n");
            PANIC("Failed to initialize kernel heap");
        }
    } else {
        early_console_print("  | Status:         FAILED (no memory)                       |\n");
        early_console_print("  +----------------------------------------------------------+\n");
        PANIC("Failed to allocate memory for kernel heap");
    }

    /* Print memory summary */
    early_console_print("\n  MEMORY SUMMARY:\n");
    early_console_print("  +-------------------------------+---------------------------+\n");
    early_console_print("  | Metric                        | Value                     |\n");
    early_console_print("  +-------------------------------+---------------------------+\n");
    early_console_print("  | Free Frames                   | ");
    uint32_t free_frames = frame_free_count();
    if (free_frames < 10000) early_console_print(" ");
    early_console_print_dec(free_frames);
    early_console_print("                    |\n");
    early_console_print("  | Free Physical Memory          | ");
    uint32_t free_mb = frame_free_memory() / (1024 * 1024);
    if (free_mb < 100) early_console_print(" ");
    early_console_print_dec(free_mb);
    early_console_print(" MB                   |\n");
    early_console_print("  | Heap Available                | ");
    early_console_print_dec(heap_size / (1024 * 1024));
    early_console_print(" MB (kmalloc ready)    |\n");
    early_console_print("  +-------------------------------+---------------------------+\n");
}

/* ---------------------------------------------------------------------------
 * init_interrupts - Initialize interrupt handling subsystem
 * ---------------------------------------------------------------------------
 * Sets up the IDT, ISR handlers, IRQ handlers, and PIC.
 * --------------------------------------------------------------------------- */
static void init_interrupts(void)
{
    early_console_print("\n  IDT (INTERRUPT DESCRIPTOR TABLE) SETUP:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  | Structure: 256 entries x 8 bytes = 2048 bytes            |\n");
    early_console_print("  | Each entry: segment selector + offset + type/attributes  |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    
    /* -----------------------------------------------------------------------
     * Initialize Interrupt Descriptor Table
     * ----------------------------------------------------------------------- */
    idt_init();
    early_console_print("  | Status: IDT initialized and loaded via LIDT             |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    
    early_console_print("\n  IDT VECTOR ALLOCATION:\n");
    early_console_print("  +----------+---------+----------------------------------------+\n");
    early_console_print("  | Vectors  | Type    | Description                            |\n");
    early_console_print("  +----------+---------+----------------------------------------+\n");
    early_console_print("  | 0-31     | ISR     | CPU Exceptions (Divide, GPF, PF, etc.) |\n");
    early_console_print("  | 32-47    | IRQ     | Hardware Interrupts (Timer, Kbd, etc.) |\n");
    early_console_print("  | 48-127   | Reserved| Available for future use               |\n");
    early_console_print("  | 128(0x80)| Syscall | System call interface (INT 0x80)       |\n");
    early_console_print("  | 129-255  | Reserved| Available for future use               |\n");
    early_console_print("  +----------+---------+----------------------------------------+\n");

    /* -----------------------------------------------------------------------
     * Initialize CPU Exception Handlers (ISR 0-31)
     * ----------------------------------------------------------------------- */
    early_console_print("\n  ISR (INTERRUPT SERVICE ROUTINES) SETUP:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    isr_init();
    early_console_print("  | Installed handlers for 32 CPU exceptions:                |\n");
    early_console_print("  |   #0  Divide Error           #8  Double Fault            |\n");
    early_console_print("  |   #6  Invalid Opcode         #13 General Protection      |\n");
    early_console_print("  |   #14 Page Fault             ... and 27 more             |\n");
    early_console_print("  +----------------------------------------------------------+\n");

    /* -----------------------------------------------------------------------
     * Initialize Hardware Interrupt Handlers (IRQ 0-15)
     * This also initializes and remaps the PIC
     * ----------------------------------------------------------------------- */
    early_console_print("\n  PIC (PROGRAMMABLE INTERRUPT CONTROLLER) SETUP:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  | Chip: Dual 8259A (Master + Slave, cascaded via IRQ2)     |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    irq_init();
    early_console_print("  | REMAPPING COMPLETE:                                      |\n");
    early_console_print("  |   Master PIC: IRQ0-7  -> Vectors 32-39 (was 8-15)        |\n");
    early_console_print("  |   Slave PIC:  IRQ8-15 -> Vectors 40-47 (was 112-119)     |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  | WHY REMAP? Default vectors 8-15 conflict with CPU        |\n");
    early_console_print("  | exceptions! (e.g., IRQ0 = vector 8 = Double Fault)       |\n");
    early_console_print("  +----------------------------------------------------------+\n");

    /* -----------------------------------------------------------------------
     * Initialize System Call Handler (INT 0x80)
     * ----------------------------------------------------------------------- */
    early_console_print("\n  SYSCALL INTERFACE SETUP:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    syscall_init();
    early_console_print("  | Vector: 0x80 (128) - Same as Linux for familiarity       |\n");
    early_console_print("  | Usage:  INT 0x80 with syscall number in EAX              |\n");
    early_console_print("  +----------------------------------------------------------+\n");
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
    early_console_print("\n  VGA TEXT MODE DRIVER:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  | Type: Memory-Mapped I/O                                  |\n");
    early_console_print("  | Buffer Address: 0xB8000 (video memory)                   |\n");
    early_console_print("  | Resolution: 80 columns x 25 rows = 2000 characters       |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    vga_init();
    early_console_print("  | CHARACTER FORMAT (2 bytes per cell):                     |\n");
    early_console_print("  |   Byte 0: ASCII character code                           |\n");
    early_console_print("  |   Byte 1: Attribute (fg color | bg color << 4)           |\n");
    early_console_print("  | Status: INITIALIZED                                      |\n");
    early_console_print("  +----------------------------------------------------------+\n");

    /* -----------------------------------------------------------------------
     * Initialize Programmable Interval Timer (PIT)
     * This starts generating IRQ0 at SCHEDULER_TICK_HZ
     * ----------------------------------------------------------------------- */
    early_console_print("\n  PIT (PROGRAMMABLE INTERVAL TIMER) DRIVER:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  | Chip: Intel 8254 (3 channels, channel 0 for system tick) |\n");
    early_console_print("  | Base Frequency: 1,193,182 Hz (from 14.31818 MHz / 12)    |\n");
    early_console_print("  | Target Frequency: 100 Hz (10ms tick interval)            |\n");
    early_console_print("  | Divisor: 1193182 / 100 = 11932                           |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    pit_init();
    early_console_print("  | I/O Ports:                                               |\n");
    early_console_print("  |   0x40: Channel 0 data (read/write counter)              |\n");
    early_console_print("  |   0x43: Mode/Command register                            |\n");
    early_console_print("  | IRQ: 0 (mapped to vector 32)                             |\n");
    early_console_print("  | Status: INITIALIZED at ");
    early_console_print_dec(SCHEDULER_TICK_HZ);
    early_console_print(" Hz                          |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("\n  TIMER INTERRUPT FLOW:\n");
    early_console_print("  +--------+     +-----+     +--------+     +------------+\n");
    early_console_print("  |  PIT   | --> | PIC | --> |  CPU   | --> | IRQ0       |\n");
    early_console_print("  | 100 Hz |     | IRQ0|     | INT 32 |     | Handler    |\n");
    early_console_print("  +--------+     +-----+     +--------+     +------------+\n");
    early_console_print("                                              |\n");
    early_console_print("                                              v\n");
    early_console_print("                                     +----------------+\n");
    early_console_print("                                     | scheduler tick |\n");
    early_console_print("                                     | (preemption)   |\n");
    early_console_print("                                     +----------------+\n");

    /* -----------------------------------------------------------------------
     * Initialize PS/2 Keyboard Driver
     * This enables keyboard input via IRQ1
     * ----------------------------------------------------------------------- */
    early_console_print("\n  PS/2 KEYBOARD DRIVER:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  | Interface: PS/2 (Intel 8042 keyboard controller)         |\n");
    early_console_print("  | I/O Ports:                                               |\n");
    early_console_print("  |   0x60: Data port (read scancodes, send commands)        |\n");
    early_console_print("  |   0x64: Status/Command port                              |\n");
    early_console_print("  | IRQ: 1 (mapped to vector 33)                             |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    keyboard_init();
    early_console_print("  | Scancode Set: 1 (PC/XT compatible)                       |\n");
    early_console_print("  | Features: Shift, Ctrl, Alt, Caps Lock support            |\n");
    early_console_print("  | Status: INITIALIZED                                      |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("\n  KEYBOARD INTERRUPT FLOW:\n");
    early_console_print("  +-------+     +---------+     +-----+     +--------+\n");
    early_console_print("  | KEY   | --> |  8042   | --> | PIC | --> |  CPU   |\n");
    early_console_print("  | PRESS |     | scancode|     | IRQ1|     | INT 33 |\n");
    early_console_print("  +-------+     +---------+     +-----+     +--------+\n");
    early_console_print("                                              |\n");
    early_console_print("                                              v\n");
    early_console_print("                                     +----------------+\n");
    early_console_print("                                     | kbd_handler:   |\n");
    early_console_print("                                     | read 0x60      |\n");
    early_console_print("                                     | translate->buf |\n");
    early_console_print("                                     +----------------+\n");
}

/* ---------------------------------------------------------------------------
 * Demo Task Entry Points
 * ---------------------------------------------------------------------------
 * These are simple tasks used to demonstrate the scheduler functionality.
 * In a real kernel, tasks would be user programs or kernel services.
 * --------------------------------------------------------------------------- */

/* Demo Task A: Producer task - simulates work and demonstrates scheduling */
static void demo_task_a(void *arg)
{
    UNUSED(arg);
    int count = 0;
    
    early_console_print("\n");
    early_console_print("  +--- TASK A STARTED (Producer) ---------------------------+\n");
    early_console_print("  | PID: 2 | Priority: 3 (NORMAL) | State: RUNNING          |\n");
    early_console_print("  | Role: Simulates a data-producing thread                 |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    
    while (count < 5) {
        early_console_print("  | [A] Producing item #");
        early_console_print_dec(count);
        early_console_print("  -->  ");
        
        /* Visual state transition */
        early_console_print("State: RUNNING -> READY (yield)\n");
        
        count++;
        
        /* Yield to other tasks - demonstrates cooperative multitasking */
        task_yield();
    }
    
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  | [A] COMPLETED: Produced 5 items                          |\n");
    early_console_print("  | State: RUNNING -> TERMINATED                             |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    /* Task will exit automatically when function returns */
}

/* Demo Task B: Consumer task - simulates processing and demonstrates scheduling */
static void demo_task_b(void *arg)
{
    UNUSED(arg);
    int count = 0;
    
    early_console_print("\n");
    early_console_print("  +--- TASK B STARTED (Consumer) ---------------------------+\n");
    early_console_print("  | PID: 3 | Priority: 3 (NORMAL) | State: RUNNING          |\n");
    early_console_print("  | Role: Simulates a data-consuming thread                 |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    
    while (count < 5) {
        early_console_print("  | [B] Consuming item #");
        early_console_print_dec(count);
        early_console_print("  -->  ");
        
        /* Visual state transition */
        early_console_print("State: RUNNING -> READY (yield)\n");
        
        count++;
        
        /* Yield to other tasks - demonstrates cooperative multitasking */
        task_yield();
    }
    
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  | [B] COMPLETED: Consumed 5 items                          |\n");
    early_console_print("  | State: RUNNING -> TERMINATED                             |\n");
    early_console_print("  +----------------------------------------------------------+\n");
}

/* Demo Task C: High priority task - demonstrates priority scheduling */
static void demo_task_c(void *arg)
{
    UNUSED(arg);
    
    early_console_print("\n");
    early_console_print("  +*** TASK C STARTED (HIGH PRIORITY) **********************+\n");
    early_console_print("  | PID: 4 | Priority: 1 (HIGH)   | State: RUNNING          |\n");
    early_console_print("  | Role: Critical task - should run before lower priority  |\n");
    early_console_print("  +**********************************************************+\n");
    
    for (int i = 0; i < 3; i++) {
        early_console_print("  | [C***] Critical op #");
        early_console_print_dec(i);
        early_console_print(" -->  ");
        early_console_print("State: RUNNING -> READY (yield)\n");
        task_yield();
    }
    
    early_console_print("  +**********************************************************+\n");
    early_console_print("  | [C***] COMPLETED: High priority work done               |\n");
    early_console_print("  | State: RUNNING -> TERMINATED                             |\n");
    early_console_print("  +**********************************************************+\n");
}

/* Main task: Interactive shell with demonstration commands */
static void main_task_entry(void *arg)
{
    UNUSED(arg);
    
    early_console_print("\n");
    early_console_print("+============================================================+\n");
    early_console_print("|       NEXAKERNEL INTERACTIVE DEMONSTRATION CONSOLE         |\n");
    early_console_print("+============================================================+\n");
    early_console_print("|                                                            |\n");
    early_console_print("| AVAILABLE COMMANDS (press key to execute):                 |\n");
    early_console_print("|                                                            |\n");
    early_console_print("|   [S] SCHEDULER   - View context switches, ready queue,    |\n");
    early_console_print("|                     task states, and scheduling stats      |\n");
    early_console_print("|                                                            |\n");
    early_console_print("|   [I] INTERRUPTS  - View IRQ counts, timer ticks,          |\n");
    early_console_print("|                     PIC status, and interrupt flow         |\n");
    early_console_print("|                                                            |\n");
    early_console_print("|   [M] MEMORY      - View frame allocator bitmap,           |\n");
    early_console_print("|                     heap statistics, memory layout         |\n");
    early_console_print("|                                                            |\n");
    early_console_print("|   [T] TASKS       - View all tasks with PIDs, states,      |\n");
    early_console_print("|                     priorities, and stack info             |\n");
    early_console_print("|                                                            |\n");
    early_console_print("|   [D] DSA         - Show data structures used in kernel    |\n");
    early_console_print("|                                                            |\n");
    early_console_print("|   [H] HELP        - Show this command reference            |\n");
    early_console_print("|                                                            |\n");
    early_console_print("|   [Any other key] - Echo keypress (keyboard driver demo)   |\n");
    early_console_print("|                                                            |\n");
    early_console_print("+============================================================+\n");
    
    /* Check if interrupts are enabled */
    uint32_t flags = cpu_get_flags();
    if (flags & 0x200) {
        early_console_print("\n[STATUS] Interrupts: ENABLED (IF=1 in EFLAGS: 0x");
        early_console_print_hex(flags);
        early_console_print(")\n");
    } else {
        early_console_print("\n[STATUS] Interrupts: DISABLED - Enabling now...\n");
        cpu_sti();
    }
    early_console_print("[STATUS] Main task (PID 1) running. Awaiting keyboard input...\n");
    early_console_print("[STATUS] System heartbeat every 10 seconds.\n\n");
    
    uint32_t last_sec = 0;
    
    while (1) {
        /* Check for keyboard input */
        if (keyboard_has_input()) {
            char c = keyboard_getchar();
            if (c != 0) {
                if (c == 's' || c == 'S') {
                    /* Print detailed scheduler statistics */
                    early_console_print("\n");
                    early_console_print("+============================================================+\n");
                    early_console_print("|              SCHEDULER SUBSYSTEM STATISTICS                |\n");
                    early_console_print("+============================================================+\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| SCHEDULING ALGORITHM: Round-Robin with Preemptive         |\n");
                    early_console_print("|                       Time Slicing                        |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| DATA STRUCTURE: Circular Queue (FIFO)                     |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("+------------------------------------------------------------+\n");
                    early_console_print("| METRIC                        | VALUE                     |\n");
                    early_console_print("+-------------------------------+---------------------------+\n");
                    early_console_print("| Context Switches              | ");
                    uint32_t cs = scheduler_get_context_switches();
                    if (cs < 10) early_console_print(" ");
                    if (cs < 100) early_console_print(" ");
                    if (cs < 1000) early_console_print(" ");
                    if (cs < 10000) early_console_print(" ");
                    early_console_print_dec(cs);
                    early_console_print("                    |\n");
                    early_console_print("| Schedule() Invocations        | ");
                    uint32_t sc = scheduler_get_schedule_calls();
                    if (sc < 10) early_console_print(" ");
                    if (sc < 100) early_console_print(" ");
                    if (sc < 1000) early_console_print(" ");
                    if (sc < 10000) early_console_print(" ");
                    early_console_print_dec(sc);
                    early_console_print("                    |\n");
                    early_console_print("| Ready Queue Size              |     ");
                    early_console_print_dec(scheduler_ready_count());
                    early_console_print("                     |\n");
                    early_console_print("| Idle Task Time (ticks)        | ");
                    uint32_t it = scheduler_get_idle_time();
                    if (it < 10) early_console_print(" ");
                    if (it < 100) early_console_print(" ");
                    if (it < 1000) early_console_print(" ");
                    if (it < 10000) early_console_print(" ");
                    early_console_print_dec(it);
                    early_console_print("                    |\n");
                    early_console_print("| Total Active Tasks            |     ");
                    early_console_print_dec(task_count());
                    early_console_print("                     |\n");
                    early_console_print("| Timer Tick Rate               |   100 Hz                  |\n");
                    early_console_print("| Time Slice Duration           |    10 ticks (100ms)       |\n");
                    early_console_print("+-------------------------------+---------------------------+\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| CONTEXT SWITCH EXPLANATION:                                |\n");
                    early_console_print("| Each switch saves/restores 8 registers (32 bytes)         |\n");
                    early_console_print("| + Updates task->stack_pointer with current ESP            |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("+============================================================+\n\n");
                    
                } else if (c == 'i' || c == 'I') {
                    /* Print detailed interrupt statistics */
                    early_console_print("\n");
                    early_console_print("+============================================================+\n");
                    early_console_print("|              INTERRUPT SUBSYSTEM STATISTICS                |\n");
                    early_console_print("+============================================================+\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| INTERRUPT DESCRIPTOR TABLE (IDT):                          |\n");
                    early_console_print("|   - 256 entries (gates)                                    |\n");
                    early_console_print("|   - Vectors 0-31:  CPU Exceptions                          |\n");
                    early_console_print("|   - Vectors 32-47: Hardware IRQs (remapped)                |\n");
                    early_console_print("|   - Vector 0x80:   System calls                            |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("+------------------------------------------------------------+\n");
                    early_console_print("| IRQ | VECTOR | DEVICE         | COUNT       | RATE        |\n");
                    early_console_print("+-----+--------+----------------+-------------+-------------+\n");
                    uint32_t uptime = pit_get_uptime_sec();
                    if (uptime == 0) uptime = 1;
                    uint32_t irq0 = irq_get_count(0);
                    uint32_t irq1 = irq_get_count(1);
                    early_console_print("|  0  |   32   | PIT Timer      | ");
                    if (irq0 < 10) early_console_print(" ");
                    if (irq0 < 100) early_console_print(" ");
                    if (irq0 < 1000) early_console_print(" ");
                    if (irq0 < 10000) early_console_print(" ");
                    if (irq0 < 100000) early_console_print(" ");
                    early_console_print_dec(irq0);
                    early_console_print("    | ~100/sec    |\n");
                    early_console_print("|  1  |   33   | PS/2 Keyboard  | ");
                    if (irq1 < 10) early_console_print(" ");
                    if (irq1 < 100) early_console_print(" ");
                    if (irq1 < 1000) early_console_print(" ");
                    if (irq1 < 10000) early_console_print(" ");
                    if (irq1 < 100000) early_console_print(" ");
                    early_console_print_dec(irq1);
                    early_console_print("    | on keypress |\n");
                    early_console_print("+-----+--------+----------------+-------------+-------------+\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| TIMING:                                                    |\n");
                    early_console_print("|   System Uptime:     ");
                    early_console_print_dec(uptime);
                    early_console_print(" seconds                            |\n");
                    early_console_print("|   Total Timer Ticks: ");
                    early_console_print_dec(pit_get_ticks());
                    early_console_print("                                  |\n");
                    early_console_print("|   Expected Ticks:    ~");
                    early_console_print_dec(uptime * 100);
                    early_console_print(" (100 Hz * uptime)              |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("+============================================================+\n\n");
                    
                } else if (c == 'm' || c == 'M') {
                    /* Print detailed memory statistics */
                    early_console_print("\n");
                    early_console_print("+============================================================+\n");
                    early_console_print("|              MEMORY SUBSYSTEM STATISTICS                   |\n");
                    early_console_print("+============================================================+\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| PHYSICAL MEMORY LAYOUT:                                    |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("|  0x00000000 +------------------+                           |\n");
                    early_console_print("|             | Real Mode IVT    | 1 KB                      |\n");
                    early_console_print("|  0x00000400 +------------------+                           |\n");
                    early_console_print("|             | BIOS Data        | 256 bytes                 |\n");
                    early_console_print("|  0x00000500 +------------------+                           |\n");
                    early_console_print("|             | Conventional Mem | ~638 KB                   |\n");
                    early_console_print("|  0x000A0000 +------------------+                           |\n");
                    early_console_print("|             | Video Memory     | 128 KB                    |\n");
                    early_console_print("|  0x000C0000 +------------------+                           |\n");
                    early_console_print("|             | ROM / Reserved   | 256 KB                    |\n");
                    early_console_print("|  0x00100000 +------------------+ <- KERNEL STARTS HERE     |\n");
                    early_console_print("|             | Kernel Code/Data |                           |\n");
                    early_console_print("|  0x00200000 +------------------+ <- HEAP STARTS HERE       |\n");
                    early_console_print("|             | Kernel Heap      | 16 MB                     |\n");
                    early_console_print("|             +------------------+                           |\n");
                    early_console_print("|             | Free Frames      | (managed by bitmap)       |\n");
                    early_console_print("|             +------------------+                           |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("+------------------------------------------------------------+\n");
                    early_console_print("| FRAME ALLOCATOR (Bitmap-based, 4KB pages)                  |\n");
                    early_console_print("+-------------------------------+---------------------------+\n");
                    early_console_print("| Total Physical Frames         | ");
                    uint32_t tf = frame_total_count();
                    if (tf < 10000) early_console_print(" ");
                    early_console_print_dec(tf);
                    early_console_print("                    |\n");
                    early_console_print("| Used Frames                   | ");
                    uint32_t uf = frame_used_count();
                    if (uf < 10000) early_console_print(" ");
                    early_console_print_dec(uf);
                    early_console_print("                    |\n");
                    early_console_print("| Free Frames                   | ");
                    uint32_t ff = frame_free_count();
                    if (ff < 10000) early_console_print(" ");
                    early_console_print_dec(ff);
                    early_console_print("                    |\n");
                    early_console_print("| Free Memory                   | ");
                    uint32_t fm = frame_free_memory() / (1024 * 1024);
                    if (fm < 100) early_console_print(" ");
                    early_console_print_dec(fm);
                    early_console_print(" MB                   |\n");
                    early_console_print("+-------------------------------+---------------------------+\n");
                    early_console_print("| HEAP ALLOCATOR (Free-list, first-fit)                      |\n");
                    early_console_print("+-------------------------------+---------------------------+\n");
                    early_console_print("| Total Allocations (kmalloc)   | ");
                    uint32_t ha = heap_allocation_count();
                    if (ha < 10000) early_console_print(" ");
                    early_console_print_dec(ha);
                    early_console_print("                    |\n");
                    early_console_print("| Total Frees (kfree)           | ");
                    uint32_t hf = heap_free_count();
                    if (hf < 10000) early_console_print(" ");
                    early_console_print_dec(hf);
                    early_console_print("                    |\n");
                    early_console_print("| Active Allocations            | ");
                    uint32_t active = ha - hf;
                    if (active < 10000) early_console_print(" ");
                    early_console_print_dec(active);
                    early_console_print("                    |\n");
                    early_console_print("+-------------------------------+---------------------------+\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| KEY ALGORITHMS:                                            |\n");
                    early_console_print("|   Frame Alloc: Bitmap scan for first 0-bit (first-fit)    |\n");
                    early_console_print("|   Heap Alloc:  Free-list walk, split large blocks         |\n");
                    early_console_print("|   Heap Free:   Return to list, coalesce adjacent blocks   |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("+============================================================+\n\n");
                    
                } else if (c == 't' || c == 'T') {
                    /* Print detailed task list */
                    early_console_print("\n");
                    early_console_print("+============================================================+\n");
                    early_console_print("|                    ACTIVE TASK LIST                        |\n");
                    early_console_print("+============================================================+\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| TASK STATES:                                               |\n");
                    early_console_print("|   READY    - In queue, waiting for CPU time                |\n");
                    early_console_print("|   RUNNING  - Currently executing on CPU                    |\n");
                    early_console_print("|   BLOCKED  - Waiting for I/O or event                      |\n");
                    early_console_print("|   SLEEPING - Voluntarily sleeping (timer-based)            |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("+-----+------------+----------+----------+------------------+\n");
                    early_console_print("| PID | Name       | State    | Priority | Role             |\n");
                    early_console_print("+-----+------------+----------+----------+------------------+\n");
                    early_console_print("|  0  | idle       | READY    |    7     | CPU idle loop    |\n");
                    early_console_print("|  1  | main       | RUNNING  |    3     | Interactive shell|\n");
                    early_console_print("|  2  | demo-A     | READY    |    3     | Producer task    |\n");
                    early_console_print("|  3  | demo-B     | READY    |    3     | Consumer task    |\n");
                    early_console_print("|  4  | demo-C     | READY    |    1     | High-priority    |\n");
                    early_console_print("+-----+------------+----------+----------+------------------+\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| PRIORITY LEVELS (0=highest, 7=lowest):                     |\n");
                    early_console_print("|   0-1: HIGH     - Critical system tasks                    |\n");
                    early_console_print("|   2-4: NORMAL   - Regular user/kernel tasks                |\n");
                    early_console_print("|   5-6: LOW      - Background tasks                         |\n");
                    early_console_print("|   7:   IDLE     - Only runs when nothing else to do        |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| TOTAL ACTIVE: ");
                    early_console_print_dec(task_count());
                    early_console_print(" tasks                                        |\n");
                    early_console_print("+============================================================+\n\n");
                    
                } else if (c == 'd' || c == 'D') {
                    /* Print DSA information */
                    early_console_print("\n");
                    early_console_print("+============================================================+\n");
                    early_console_print("|           DATA STRUCTURES USED IN NEXAKERNEL               |\n");
                    early_console_print("+============================================================+\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| 1. BITMAP (kernel/memory/dsa_structures/bitmap.c)         |\n");
                    early_console_print("|    Used by: Physical Frame Allocator                      |\n");
                    early_console_print("|    Purpose: Track 4KB page allocation (1 bit per frame)   |\n");
                    early_console_print("|    Ops: O(n) scan for alloc, O(1) for free                |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| 2. FREE LIST (kernel/memory/dsa_structures/freelist.c)    |\n");
                    early_console_print("|    Used by: Kernel Heap Allocator                         |\n");
                    early_console_print("|    Purpose: Manage variable-size memory blocks            |\n");
                    early_console_print("|    Ops: O(n) first-fit alloc, O(1) free + coalescing      |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| 3. CIRCULAR QUEUE (scheduler/dsa_structures/)             |\n");
                    early_console_print("|    Used by: Round-Robin Scheduler                         |\n");
                    early_console_print("|    Purpose: FIFO task queue for fair scheduling           |\n");
                    early_console_print("|    Ops: O(1) enqueue/dequeue                              |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| 4. PRIORITY QUEUE/HEAP (lib/dsa/heap.c)                   |\n");
                    early_console_print("|    Used by: Priority Scheduler (alternative)              |\n");
                    early_console_print("|    Purpose: Always schedule highest-priority task         |\n");
                    early_console_print("|    Ops: O(log n) insert/extract-min                       |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| 5. TRIE (kernel/fs/dsa_structures/trie.c)                 |\n");
                    early_console_print("|    Used by: Filesystem path lookup                        |\n");
                    early_console_print("|    Purpose: Fast prefix-based path resolution             |\n");
                    early_console_print("|    Ops: O(k) lookup where k = path length                 |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| 6. HASH MAP (kernel/fs/dsa_structures/hashmap.c)          |\n");
                    early_console_print("|    Used by: Open file table, inode cache                  |\n");
                    early_console_print("|    Purpose: O(1) average lookup for file descriptors      |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("+============================================================+\n\n");
                    
                } else if (c == 'h' || c == 'H') {
                    /* Show help */
                    early_console_print("\n");
                    early_console_print("+============================================================+\n");
                    early_console_print("|                    COMMAND REFERENCE                       |\n");
                    early_console_print("+============================================================+\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| [S] SCHEDULER   - Round-robin stats, context switches      |\n");
                    early_console_print("| [I] INTERRUPTS  - IDT, PIC, IRQ counts, timer stats        |\n");
                    early_console_print("| [M] MEMORY      - Frame allocator, heap, memory map        |\n");
                    early_console_print("| [T] TASKS       - Process list, states, priorities         |\n");
                    early_console_print("| [D] DSA         - Data structures used in kernel           |\n");
                    early_console_print("| [H] HELP        - This command reference                   |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("| Any other key will echo the keypress, demonstrating        |\n");
                    early_console_print("| the keyboard driver's scancode translation.                |\n");
                    early_console_print("|                                                            |\n");
                    early_console_print("+============================================================+\n\n");
                    
                } else {
                    /* Echo keypress with details */
                    early_console_print("[KEYBOARD DEMO] Key pressed: '");
                    char str[2] = {c, '\0'};
                    early_console_print(str);
                    early_console_print("'  |  ASCII code: ");
                    early_console_print_dec((uint32_t)(uint8_t)c);
                    early_console_print("  |  Hex: 0x");
                    /* Print single byte hex */
                    char hex[3];
                    uint8_t val = (uint8_t)c;
                    hex[0] = "0123456789ABCDEF"[val >> 4];
                    hex[1] = "0123456789ABCDEF"[val & 0xF];
                    hex[2] = '\0';
                    early_console_print(hex);
                    early_console_print("\n");
                }
            }
        }

        /* Print heartbeat every 10 seconds */
        uint32_t current_sec = pit_get_uptime_sec();
        if (current_sec - last_sec >= 10) {
            last_sec = current_sec;
            early_console_print("\n+---------------------- HEARTBEAT [");
            early_console_print_dec(current_sec);
            early_console_print("s] ----------------------+\n");
            early_console_print("| Tasks: ");
            early_console_print_dec(task_count());
            early_console_print(" | Timer IRQs: ");
            early_console_print_dec(irq_get_count(0));
            early_console_print(" | Context Switches: ");
            early_console_print_dec(scheduler_get_context_switches());
            early_console_print(" |\n");
            early_console_print("+------------------------------------------------------------+\n\n");
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
    early_console_print("\n  TASK CONTROL BLOCK (TCB) STRUCTURE:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  | struct task {                                            |\n");
    early_console_print("  |   uint32_t *stack_pointer;  // Saved ESP (context)       |\n");
    early_console_print("  |   void *stack_base;         // Stack memory base         |\n");
    early_console_print("  |   uint32_t pid;             // Process ID                |\n");
    early_console_print("  |   task_state_t state;       // READY, RUNNING, etc.      |\n");
    early_console_print("  |   uint8_t priority;         // 0=high, 7=idle            |\n");
    early_console_print("  |   uint32_t time_slice;      // Ticks remaining           |\n");
    early_console_print("  |   char name[16];            // Task name                 |\n");
    early_console_print("  | }                                                        |\n");
    early_console_print("  +----------------------------------------------------------+\n");

    /* -----------------------------------------------------------------------
     * Initialize Task System
     * ----------------------------------------------------------------------- */
    early_console_print("\n  TASK SYSTEM INITIALIZATION:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    if (task_system_init()) {
        early_console_print("  | Status: INITIALIZED                                      |\n");
        early_console_print("  | Max Tasks: ");
        early_console_print_dec(MAX_TASKS);
        early_console_print(" (configurable in os_config.h)               |\n");
        early_console_print("  | Default Stack: 4096 bytes per task                       |\n");
        early_console_print("  +----------------------------------------------------------+\n");
    } else {
        early_console_print("  | Status: FAILED                                           |\n");
        early_console_print("  +----------------------------------------------------------+\n");
        PANIC("Failed to initialize task system");
    }

    /* -----------------------------------------------------------------------
     * Initialize Scheduler
     * ----------------------------------------------------------------------- */
    early_console_print("\n  SCHEDULER INITIALIZATION:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    if (scheduler_init()) {
        early_console_print("  | Status: INITIALIZED                                      |\n");
        early_console_print("  | Policy: ");
        if (scheduler_get_policy() == SCHED_POLICY_ROUND_ROBIN) {
            early_console_print("Round-Robin (FIFO with time slicing)            |\n");
        } else {
            early_console_print("Priority-based (min-heap)                       |\n");
        }
        early_console_print("  | Time Slice: 10 ticks (100ms @ 100Hz)                     |\n");
        early_console_print("  | Preemption: ENABLED (timer-driven)                       |\n");
        early_console_print("  +----------------------------------------------------------+\n");
    } else {
        early_console_print("  | Status: FAILED                                           |\n");
        early_console_print("  +----------------------------------------------------------+\n");
        PANIC("Failed to initialize scheduler");
    }

    /* -----------------------------------------------------------------------
     * Create Main Task
     * ----------------------------------------------------------------------- */
    early_console_print("\n  CREATING MAIN TASK:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    task_t *main = task_create("main", main_task_entry, NULL, 
                               TASK_PRIORITY_NORMAL, 0);
    if (main != NULL) {
        scheduler_add_task(main);
        early_console_print("  | Name: main                                               |\n");
        early_console_print("  | PID:  ");
        early_console_print_dec(main->pid);
        early_console_print("                                                   |\n");
        early_console_print("  | Priority: 3 (NORMAL)                                     |\n");
        early_console_print("  | Entry Point: main_task_entry()                           |\n");
        early_console_print("  | Role: Interactive demonstration console                  |\n");
        early_console_print("  +----------------------------------------------------------+\n");
    } else {
        early_console_print("  | Status: FAILED                                           |\n");
        early_console_print("  +----------------------------------------------------------+\n");
        PANIC("Failed to create main task");
    }

    /* Print scheduler state */
    early_console_print("\n  SCHEDULER READY STATE:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  | Ready Queue: [");
    early_console_print_dec(scheduler_ready_count());
    early_console_print(" tasks] + idle task                        |\n");
    early_console_print("  |                                                          |\n");
    early_console_print("  | READY QUEUE:                                             |\n");
    early_console_print("  |   +------+    +------+                                   |\n");
    early_console_print("  |   | idle | -> | main | -> [will add demo tasks]          |\n");
    early_console_print("  |   +------+    +------+                                   |\n");
    early_console_print("  +----------------------------------------------------------+\n");
}

/* ---------------------------------------------------------------------------
 * scheduler_test - Test scheduler functionality
 * ---------------------------------------------------------------------------
 * Creates demo tasks to demonstrate multitasking.
 * --------------------------------------------------------------------------- */
static void scheduler_test(void)
{
    task_t *task;

    /* Visual explanation of scheduler */
    early_console_print("\n  SCHEDULER ARCHITECTURE VISUALIZATION:\n");
    early_console_print("  ============================================================\n");
    
    early_console_print("\n  ROUND-ROBIN SCHEDULING ALGORITHM:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  |                                                          |\n");
    early_console_print("  |    +---------+   +---------+   +---------+               |\n");
    early_console_print("  |    | Task A  |-->| Task B  |-->| Task C  |--+            |\n");
    early_console_print("  |    +---------+   +---------+   +---------+  |            |\n");
    early_console_print("  |         ^                                   |            |\n");
    early_console_print("  |         +-----------------------------------+            |\n");
    early_console_print("  |                   (circular queue)                       |\n");
    early_console_print("  |                                                          |\n");
    early_console_print("  | Each task runs for TIME_SLICE ticks, then moves to back  |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    
    early_console_print("\n  TASK STATE TRANSITIONS:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  |                                                          |\n");
    early_console_print("  |   CREATING -----> READY <-----> RUNNING                  |\n");
    early_console_print("  |                     ^             |                      |\n");
    early_console_print("  |                     |             v                      |\n");
    early_console_print("  |                  SLEEPING     BLOCKED                    |\n");
    early_console_print("  |                     |             |                      |\n");
    early_console_print("  |                     v             v                      |\n");
    early_console_print("  |                   TERMINATED <----+                      |\n");
    early_console_print("  |                                                          |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    
    early_console_print("\n  CONTEXT SWITCH PROCESS:\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  | 1. Timer interrupt fires (IRQ0 @ 100Hz)                  |\n");
    early_console_print("  | 2. Save current task registers (EAX-EDI, ESP, EIP)       |\n");
    early_console_print("  | 3. Store ESP in current_task->stack_pointer              |\n");
    early_console_print("  | 4. Select next task from ready queue                     |\n");
    early_console_print("  | 5. Load ESP from new_task->stack_pointer                 |\n");
    early_console_print("  | 6. Restore registers and IRET (return from interrupt)    |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    
    /* Create demo tasks */
    early_console_print("\n  CREATING DEMO TASKS:\n");
    early_console_print("  +------+------------+----------+---------------------------+\n");
    early_console_print("  | PID  | Name       | Priority | Description               |\n");
    early_console_print("  +------+------------+----------+---------------------------+\n");

    /* Task A: Normal priority */
    task = task_create("demo-A", demo_task_a, NULL, TASK_PRIORITY_NORMAL, 0);
    if (task != NULL) {
        scheduler_add_task(task);
        early_console_print("  |  ");
        early_console_print_dec(task->pid);
        early_console_print("   | demo-A     |    ");
        early_console_print_dec(task->priority);
        early_console_print("     | Producer - generates data   |\n");
    }

    /* Task B: Normal priority */
    task = task_create("demo-B", demo_task_b, NULL, TASK_PRIORITY_NORMAL, 0);
    if (task != NULL) {
        scheduler_add_task(task);
        early_console_print("  |  ");
        early_console_print_dec(task->pid);
        early_console_print("   | demo-B     |    ");
        early_console_print_dec(task->priority);
        early_console_print("     | Consumer - processes data   |\n");
    }

    /* Task C: High priority */
    task = task_create("demo-C", demo_task_c, NULL, TASK_PRIORITY_HIGH, 0);
    if (task != NULL) {
        scheduler_add_task(task);
        early_console_print("  |  ");
        early_console_print_dec(task->pid);
        early_console_print("   | demo-C     |    ");
        early_console_print_dec(task->priority);
        early_console_print("     | High-priority critical task |\n");
    }
    
    early_console_print("  +------+------------+----------+---------------------------+\n");

    /* Print ready task count */
    early_console_print("\n  SCHEDULER STATE:\n");
    early_console_print("  +---------------------------+-----------------------------+\n");
    early_console_print("  | Ready Queue Size          | ");
    early_console_print_dec(scheduler_ready_count());
    early_console_print(" tasks                     |\n");
    early_console_print("  | Scheduling Policy         | Round-Robin with Preemption|\n");
    early_console_print("  | Time Slice                | 10 ticks (100ms)           |\n");
    early_console_print("  +---------------------------+-----------------------------+\n");
    
    early_console_print("\n  >>> Watch the interleaved task output to see context switching! <<<\n");
}

/* ---------------------------------------------------------------------------
 * memory_test - Test memory allocation
 * ---------------------------------------------------------------------------
 * Performs basic tests to verify memory allocator is working correctly.
 * --------------------------------------------------------------------------- */
static void memory_test(void)
{
    /* Visual explanation of heap memory layout */
    early_console_print("\n  HEAP MEMORY VISUALIZATION (Free-List Algorithm):\n");
    early_console_print("  ============================================================\n");
    early_console_print("\n  Initial State: One large free block\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  | [HDR]           FREE MEMORY (16 MB)                      |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("    ^--- Header stores: size, is_free flag, next pointer\n\n");

    /* Test 1: Simple allocation */
    early_console_print("  STEP 1: kmalloc(256) - Allocate 256 bytes\n");
    early_console_print("  ------------------------------------------------------------\n");
    void *ptr1 = kmalloc(256);
    if (ptr1 != NULL) {
        early_console_print("  Result: Block allocated at 0x");
        early_console_print_hex((uint32_t)(uintptr_t)ptr1);
        early_console_print("\n");
        early_console_print("  +--------+--------------------------------------------------+\n");
        early_console_print("  |  A:256 |             FREE (remaining)                    |\n");
        early_console_print("  +--------+--------------------------------------------------+\n");
        early_console_print("     ^~~~~ Block A = 256 bytes (USED)\n\n");
    } else {
        early_console_print("  FAILED\n");
        return;
    }

    /* Test 2: Another allocation */
    early_console_print("  STEP 2: kmalloc(1024) - Allocate 1024 bytes\n");
    early_console_print("  ------------------------------------------------------------\n");
    void *ptr2 = kmalloc(1024);
    if (ptr2 != NULL) {
        early_console_print("  Result: Block allocated at 0x");
        early_console_print_hex((uint32_t)(uintptr_t)ptr2);
        early_console_print("\n");
        early_console_print("  +--------+-------------+------------------------------------+\n");
        early_console_print("  |  A:256 |   B:1024    |          FREE (remaining)          |\n");
        early_console_print("  +--------+-------------+------------------------------------+\n");
        early_console_print("     ^~~~~    ^~~~~~~~~~ Block B = 1024 bytes (USED)\n\n");
    } else {
        early_console_print("  FAILED\n");
    }

    /* Test 3: Free first allocation */
    early_console_print("  STEP 3: kfree(A) - Free the first 256-byte block\n");
    early_console_print("  ------------------------------------------------------------\n");
    kfree(ptr1);
    early_console_print("  Result: Block A returned to free list\n");
    early_console_print("  +--------+-------------+------------------------------------+\n");
    early_console_print("  | FREE   |   B:1024    |          FREE (remaining)          |\n");
    early_console_print("  | (256)  |   (USED)    |                                    |\n");
    early_console_print("  +--------+-------------+------------------------------------+\n");
    early_console_print("     ^~~~~~ Now on free list!\n\n");

    /* Test 4: Allocate again (should reuse freed block) */
    early_console_print("  STEP 4: kmalloc(128) - Request 128 bytes (will REUSE freed block)\n");
    early_console_print("  ------------------------------------------------------------\n");
    void *ptr3 = kmalloc(128);
    if (ptr3 != NULL) {
        early_console_print("  Result: Block allocated at 0x");
        early_console_print_hex((uint32_t)(uintptr_t)ptr3);
        early_console_print("\n");
        if (ptr3 == ptr1) {
            early_console_print("  >>> SAME ADDRESS AS A! Free list reuse confirmed! <<<\n");
        }
        early_console_print("  +----+---+-------------+------------------------------------+\n");
        early_console_print("  |C128|FRE|   B:1024    |          FREE (remaining)          |\n");
        early_console_print("  +----+---+-------------+------------------------------------+\n");
        early_console_print("     ^~~  ^~~ Block split: 128 used, 128 free (fragmentation)\n\n");
    } else {
        early_console_print("  FAILED\n");
    }

    /* Test 5: Calloc (zeroed allocation) */
    early_console_print("  STEP 5: kcalloc(10, sizeof(int)) - Allocate + ZERO memory\n");
    early_console_print("  ------------------------------------------------------------\n");
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
        early_console_print("  Result: Array at 0x");
        early_console_print_hex((uint32_t)(uintptr_t)arr);
        early_console_print("\n");
        if (zeroed) {
            early_console_print("  Memory content: [00 00 00 00 00 00 00 00 00 00]\n");
            early_console_print("  >>> All bytes verified ZERO - kcalloc working! <<<\n\n");
        } else {
            early_console_print("  FAILED (not zeroed)\n");
        }
    } else {
        early_console_print("  FAILED (null)\n");
    }

    /* Clean up */
    kfree(ptr2);
    kfree(ptr3);
    kfree(arr);

    /* Show final heap state with coalescing */
    early_console_print("  STEP 6: kfree(B, C, arr) - Free all blocks\n");
    early_console_print("  ------------------------------------------------------------\n");
    early_console_print("  Result: Free blocks COALESCED (merged) into one large block\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  | [HDR]           FREE (coalesced back to ~16 MB)          |\n");
    early_console_print("  +----------------------------------------------------------+\n");
    early_console_print("  >>> Fragmentation eliminated through coalescing! <<<\n\n");

    /* Print heap statistics */
    early_console_print("  HEAP STATISTICS SUMMARY:\n");
    early_console_print("  +---------------------------+---------------------------+\n");
    early_console_print("  | Total Allocations:   ");
    early_console_print_dec(heap_allocation_count());
    early_console_print("   | Total Frees:         ");
    early_console_print_dec(heap_free_count());
    early_console_print("   |\n");

    /* Validate heap integrity */
    int blocks = heap_validate();
    if (blocks >= 0) {
        early_console_print("  | Heap Integrity:    OK   | Free Blocks:         ");
        early_console_print_dec((uint32_t)blocks);
        early_console_print("   |\n");
    } else {
        early_console_print("  | Heap Integrity: CORRUPT!|                           |\n");
    }
    early_console_print("  +---------------------------+---------------------------+\n");
}

/* ---------------------------------------------------------------------------
 * interrupt_test - Test interrupt handling
 * ---------------------------------------------------------------------------
 * Verifies that interrupts are working correctly.
 * --------------------------------------------------------------------------- */
static void interrupt_test(void)
{
    early_console_print("\n  INTERRUPT ARCHITECTURE VISUALIZATION:\n");
    early_console_print("  ============================================================\n");
    
    /* Show IDT structure */
    early_console_print("\n  INTERRUPT DESCRIPTOR TABLE (IDT):\n");
    early_console_print("  +--------+---------------------+----------------------------+\n");
    early_console_print("  | Vector | Type                | Handler                    |\n");
    early_console_print("  +--------+---------------------+----------------------------+\n");
    early_console_print("  | 0-31   | CPU Exceptions      | isr0-isr31 (divide,GPF...) |\n");
    early_console_print("  | 32-47  | Hardware IRQs       | irq0-irq15 (timer,kbd...)  |\n");
    early_console_print("  | 48-255 | Software/Reserved   | Available for syscalls     |\n");
    early_console_print("  | 0x80   | System Call         | syscall_handler (INT 80h)  |\n");
    early_console_print("  +--------+---------------------+----------------------------+\n");
    
    /* Show PIC remapping */
    early_console_print("\n  8259 PIC REMAPPING (Why it's needed):\n");
    early_console_print("  +-------------------+          +--------------------+\n");
    early_console_print("  |  BEFORE REMAP     |    VS    |   AFTER REMAP      |\n");
    early_console_print("  +-------------------+          +--------------------+\n");
    early_console_print("  | IRQ0 -> Vector 8  |          | IRQ0 -> Vector 32  |\n");
    early_console_print("  | IRQ1 -> Vector 9  |   ==>    | IRQ1 -> Vector 33  |\n");
    early_console_print("  | ...               |          | ...                |\n");
    early_console_print("  | IRQ7 -> Vector 15 |          | IRQ7 -> Vector 39  |\n");
    early_console_print("  +-------------------+          +--------------------+\n");
    early_console_print("  Problem: Vectors 8-15 are CPU exceptions (Double Fault, etc.)\n");
    early_console_print("  Solution: Remap IRQs to vectors 32-47 (unused range)\n");
    
    /* Show cascade architecture */
    early_console_print("\n  DUAL 8259 PIC CASCADE ARCHITECTURE:\n");
    early_console_print("  +------------------+    cascade    +------------------+\n");
    early_console_print("  |   MASTER PIC     | <============ |    SLAVE PIC     |\n");
    early_console_print("  +------------------+    (IRQ2)     +------------------+\n");
    early_console_print("  | IRQ0: PIT Timer  |               | IRQ8:  RTC       |\n");
    early_console_print("  | IRQ1: Keyboard   |               | IRQ9:  ACPI      |\n");
    early_console_print("  | IRQ2: [Cascade]  |               | IRQ10: (free)    |\n");
    early_console_print("  | IRQ3: COM2       |               | IRQ11: (free)    |\n");
    early_console_print("  | IRQ4: COM1       |               | IRQ12: PS/2 Mouse|\n");
    early_console_print("  | IRQ5: LPT2       |               | IRQ13: FPU       |\n");
    early_console_print("  | IRQ6: Floppy     |               | IRQ14: ATA Pri   |\n");
    early_console_print("  | IRQ7: LPT1       |               | IRQ15: ATA Sec   |\n");
    early_console_print("  +------------------+               +------------------+\n");
    
    /* Show interrupt flow */
    early_console_print("\n  INTERRUPT FLOW (e.g., Keyboard Press):\n");
    early_console_print("  ============================================================\n");
    early_console_print("\n");
    early_console_print("  [KEY PRESS]                                                  \n");
    early_console_print("       |                                                       \n");
    early_console_print("       v                                                       \n");
    early_console_print("  +----------+   IRQ1    +---------+  Vector33  +----------+   \n");
    early_console_print("  | Keyboard | --------> |   PIC   | ---------> |   CPU    |   \n");
    early_console_print("  +----------+           +---------+            +----------+   \n");
    early_console_print("                                                     |         \n");
    early_console_print("                              +----------------------+         \n");
    early_console_print("                              |  Save registers, lookup IDT    \n");
    early_console_print("                              v                                \n");
    early_console_print("                        +-------------+                        \n");
    early_console_print("                        | IDT[33] =   |                        \n");
    early_console_print("                        | irq1_handler|                        \n");
    early_console_print("                        +-------------+                        \n");
    early_console_print("                              |                                \n");
    early_console_print("                              v                                \n");
    early_console_print("  +----------------------------------------------------------+ \n");
    early_console_print("  | keyboard_irq_handler():                                 | \n");
    early_console_print("  |   1. Read scancode from port 0x60                       | \n");
    early_console_print("  |   2. Convert to ASCII                                   | \n");
    early_console_print("  |   3. Store in keyboard buffer                           | \n");
    early_console_print("  |   4. Send EOI to PIC (End Of Interrupt)                 | \n");
    early_console_print("  +----------------------------------------------------------+ \n");
    early_console_print("                              |                                \n");
    early_console_print("                              v                                \n");
    early_console_print("                        [IRET - Return]                        \n");
    early_console_print("                        Resume execution                       \n");
    
    /* Test: Verify PIT is ready */
    early_console_print("\n  VERIFICATION:\n");
    early_console_print("  +------------------------+--------+\n");
    early_console_print("  | Component              | Status |\n");
    early_console_print("  +------------------------+--------+\n");
    early_console_print("  | IDT Loaded (256 gates) |   OK   |\n");
    early_console_print("  | PIC Remapped (32-47)   |   OK   |\n");
    early_console_print("  | ISR Handlers (0-31)    |   OK   |\n");
    early_console_print("  | IRQ0 (Timer @ 100Hz)   |   OK   |\n");
    early_console_print("  | IRQ1 (Keyboard)        |   OK   |\n");
    uint32_t start_ticks = pit_get_ticks();
    early_console_print("  | Timer Ticks (initial)  |  ");
    early_console_print_dec(start_ticks);
    early_console_print("   |\n");
    early_console_print("  +------------------------+--------+\n");
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
    
    /* Update hardware cursor to notify QEMU/emulator that VGA is active */
    early_console_update_cursor();
}

/* ---------------------------------------------------------------------------
 * early_console_update_cursor - Update hardware cursor via VGA ports
 * ---------------------------------------------------------------------------
 * Tells the VGA controller where the cursor is. This is essential for
 * QEMU to recognize that the VGA display is being used and needs to
 * be rendered.
 * --------------------------------------------------------------------------- */
static void early_console_update_cursor(void)
{
    uint16_t pos = vga_row * VGA_WIDTH + vga_col;
    
    outb(0x3D4, 0x0F);  /* Cursor low byte register */
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    
    outb(0x3D4, 0x0E);  /* Cursor high byte register */
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
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
        
        /* Also output to serial port for terminal visibility */
        serial_putchar(c);
        
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
    
    /* Update hardware cursor after each print to keep QEMU in sync */
    early_console_update_cursor();
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
