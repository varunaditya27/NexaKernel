/*
 * os_config.h
 *
 * Global Operating System Configuration
 *
 * This file contains compile-time configuration macros that control the
 * behavior and features of the NexaKernel. It defines parameters such as:
 * - Scheduler mode (Preemptive vs Cooperative)
 * - Kernel Heap Size
 * - Debugging flags
 *
 * Include this file in modules that need access to global system settings.
 */

#ifndef OS_CONFIG_H
#define OS_CONFIG_H

#define SCHEDULER_PREEMPTIVE 1
#define KERNEL_HEAP_SIZE (64 * 1024)

#endif /* OS_CONFIG_H */
