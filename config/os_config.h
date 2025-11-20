/*
 * os_config.h
 *
 * Project-specific, discoverable configuration flags for NexaKernel.
 * This file should hold macro definitions that control build-time features,
 * such as whether the scheduler is preemptive, kernel heap sizes, or debug flags.
 *
 * Example: define SCHEDULER_PREEMPTIVE 1 to enable PIT-driven preemption.
 * Keep this file minimal and use it across kernel modules to avoid hardcoded values.
 */

#ifndef OS_CONFIG_H
#define OS_CONFIG_H

#define SCHEDULER_PREEMPTIVE 1
#define KERNEL_HEAP_SIZE (64 * 1024)

#endif /* OS_CONFIG_H */
