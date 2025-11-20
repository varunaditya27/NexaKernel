/*
 * kernel/kernel.c
 *
 * Kernel entry point. Implement `kernel_main()` here. It should perform:
 * - low-level init (GDT/IDT if not done in assembly)
 * - memory and heap initialization
 * - scheduler and driver initialization
 * - jump to a userland shell or demo program
 *
 * Keep this file small: call into specialized modules for each subsystem.
 */

#include "../../config/os_config.h"

void kernel_main(void) {
    // Scaffold: initialize subsystems here.
    // 1. Early console (vga_text)
    // 2. IDT setup
    // 3. Memory allocator initialization
    // 4. Scheduler setup
    // 5. Start initial tasks (e.g., shell)
}
