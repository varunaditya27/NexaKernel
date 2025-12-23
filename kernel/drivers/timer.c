/*
 * ===========================================================================
 * kernel/drivers/timer.c
 * ===========================================================================
 *
 * Programmable Interval Timer (PIT) Driver
 *
 * This driver configures the Intel 8253/8254 PIT chip to generate periodic
 * interrupts that serve as the system's heartbeat. The PIT interrupt drives:
 *
 * 1. System tick counter (uptime tracking)
 * 2. Preemptive scheduler (time slicing)
 * 3. Sleep and delay functions
 *
 * PIT Architecture:
 * ┌──────────────────────────────────────────────────────────────────────────┐
 * │  The 8254 PIT has 3 channels:                                            │
 * │  - Channel 0: Connected to IRQ0 (system timer) - WE USE THIS             │
 * │  - Channel 1: DRAM refresh (legacy, not available on modern systems)     │
 * │  - Channel 2: PC Speaker                                                 │
 * │                                                                          │
 * │  Each channel has a 16-bit counter that decrements at 1.193182 MHz.      │
 * │  When it reaches 0, it generates an output pulse and reloads.            │
 * │                                                                          │
 * │  Frequency = 1193182 / divisor                                           │
 * │  For 100 Hz: divisor = 1193182 / 100 = 11932                            │
 * └──────────────────────────────────────────────────────────────────────────┘
 *
 * ===========================================================================
 */

#include "drivers.h"
#include "../interrupts/interrupts.h"

/* ---------------------------------------------------------------------------
 * External Functions (from startup.asm)
 * --------------------------------------------------------------------------- */
extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);

/* ---------------------------------------------------------------------------
 * PIT I/O Ports
 * --------------------------------------------------------------------------- */
#define PIT_CHANNEL0_DATA   0x40    /* Channel 0 data port */
#define PIT_CHANNEL1_DATA   0x41    /* Channel 1 data port (not used) */
#define PIT_CHANNEL2_DATA   0x42    /* Channel 2 data port (PC speaker) */
#define PIT_COMMAND         0x43    /* Command/mode register */

/* ---------------------------------------------------------------------------
 * PIT Command Register Bits
 * ---------------------------------------------------------------------------
 * Bits 7-6: Channel select
 *   00 = Channel 0
 *   01 = Channel 1
 *   10 = Channel 2
 *   11 = Read-back command
 *
 * Bits 5-4: Access mode
 *   00 = Latch count
 *   01 = Low byte only
 *   10 = High byte only
 *   11 = Low byte, then high byte
 *
 * Bits 3-1: Operating mode
 *   000 = Mode 0 (interrupt on terminal count)
 *   001 = Mode 1 (hardware retriggerable one-shot)
 *   010 = Mode 2 (rate generator) - WE USE THIS
 *   011 = Mode 3 (square wave generator)
 *   100 = Mode 4 (software triggered strobe)
 *   101 = Mode 5 (hardware triggered strobe)
 *
 * Bit 0: BCD/Binary
 *   0 = 16-bit binary counter
 *   1 = 4-digit BCD counter
 * --------------------------------------------------------------------------- */
#define PIT_CMD_CHANNEL0    0x00    /* Select channel 0 */
#define PIT_CMD_CHANNEL1    0x40    /* Select channel 1 */
#define PIT_CMD_CHANNEL2    0x80    /* Select channel 2 */

#define PIT_CMD_LATCH       0x00    /* Latch count value */
#define PIT_CMD_LOBYTE      0x10    /* Access low byte only */
#define PIT_CMD_HIBYTE      0x20    /* Access high byte only */
#define PIT_CMD_LOHI        0x30    /* Access low byte, then high byte */

#define PIT_CMD_MODE0       0x00    /* Interrupt on terminal count */
#define PIT_CMD_MODE1       0x02    /* One-shot */
#define PIT_CMD_MODE2       0x04    /* Rate generator */
#define PIT_CMD_MODE3       0x06    /* Square wave */
#define PIT_CMD_MODE4       0x08    /* Software strobe */
#define PIT_CMD_MODE5       0x0A    /* Hardware strobe */

#define PIT_CMD_BINARY      0x00    /* 16-bit binary */
#define PIT_CMD_BCD         0x01    /* BCD counter */

/* ---------------------------------------------------------------------------
 * PIT Constants
 * --------------------------------------------------------------------------- */
#define PIT_BASE_FREQUENCY  1193182 /* PIT oscillator frequency (Hz) */
#define PIT_MIN_FREQUENCY   19      /* Minimum achievable frequency */
#define PIT_MAX_FREQUENCY   1193182 /* Maximum (divisor = 1) */

/* ---------------------------------------------------------------------------
 * Static Variables
 * --------------------------------------------------------------------------- */

/* Current tick count since boot */
static volatile uint32_t tick_count = 0;

/* Current timer frequency in Hz */
static uint32_t timer_frequency = SCHEDULER_TICK_HZ;

/* Milliseconds per tick (for uptime calculation) */
static uint32_t ms_per_tick = 10;  /* Default: 1000ms / 100Hz = 10ms */

/* Optional callback function for each tick */
static pit_callback_t tick_callback = NULL;

/* ---------------------------------------------------------------------------
 * pit_irq_handler - IRQ0 handler for timer interrupts
 * ---------------------------------------------------------------------------
 * Called by the IRQ subsystem when the PIT generates an interrupt.
 * This runs in interrupt context, so it should be fast and not block.
 * --------------------------------------------------------------------------- */
static void pit_irq_handler(interrupt_frame_t *frame)
{
    /* Suppress unused parameter warning */
    UNUSED(frame);

    /* Increment the tick counter */
    tick_count++;

    /* Call the registered callback (usually the scheduler) */
    if (tick_callback != NULL) {
        tick_callback();
    }
}

/* ---------------------------------------------------------------------------
 * pit_set_frequency - Set the timer interrupt frequency
 * ---------------------------------------------------------------------------
 * Configures the PIT to generate interrupts at the specified frequency.
 *
 * Parameters:
 *   hz - Desired frequency in Hz
 *
 * Note: The actual frequency may differ slightly due to integer division.
 *       Minimum frequency is ~19 Hz (divisor = 65535).
 *       Maximum frequency is 1193182 Hz (divisor = 1), but that's too fast.
 * --------------------------------------------------------------------------- */
void pit_set_frequency(uint32_t hz)
{
    /* Clamp frequency to valid range */
    if (hz < PIT_MIN_FREQUENCY) {
        hz = PIT_MIN_FREQUENCY;
    }
    if (hz > 10000) {
        hz = 10000;  /* Don't go above 10 kHz for sanity */
    }

    /* Calculate the divisor */
    uint32_t divisor = PIT_BASE_FREQUENCY / hz;

    /* Clamp divisor to valid range (1-65535) */
    if (divisor > 65535) {
        divisor = 65535;
    }
    if (divisor < 1) {
        divisor = 1;
    }

    /* Update frequency and ms_per_tick */
    timer_frequency = PIT_BASE_FREQUENCY / divisor;
    ms_per_tick = 1000 / timer_frequency;
    if (ms_per_tick == 0) {
        ms_per_tick = 1;  /* At least 1ms per tick */
    }

    /*
     * Configure Channel 0:
     * - Select channel 0
     * - Access mode: low byte, then high byte
     * - Operating mode: rate generator (mode 2)
     * - Binary counter
     */
    uint8_t command = PIT_CMD_CHANNEL0 | PIT_CMD_LOHI | PIT_CMD_MODE2 | PIT_CMD_BINARY;
    outb(PIT_COMMAND, command);

    /* Send the divisor (low byte first, then high byte) */
    outb(PIT_CHANNEL0_DATA, (uint8_t)(divisor & 0xFF));         /* Low byte */
    outb(PIT_CHANNEL0_DATA, (uint8_t)((divisor >> 8) & 0xFF));  /* High byte */
}

/* ---------------------------------------------------------------------------
 * pit_init - Initialize the Programmable Interval Timer
 * ---------------------------------------------------------------------------
 * Sets up the PIT to generate interrupts at SCHEDULER_TICK_HZ and
 * registers the IRQ handler.
 * --------------------------------------------------------------------------- */
void pit_init(void)
{
    /* Reset tick count */
    tick_count = 0;
    tick_callback = NULL;

    /* Set the timer frequency */
    pit_set_frequency(SCHEDULER_TICK_HZ);

    /* Register our IRQ handler */
    irq_register_handler(IRQ0_TIMER, pit_irq_handler);

    /* Enable IRQ0 (timer) */
    irq_enable(IRQ0_TIMER);
}

/* ---------------------------------------------------------------------------
 * pit_get_ticks - Get the current tick count
 * ---------------------------------------------------------------------------
 * Returns:
 *   Number of timer ticks since boot
 *
 * Note: This may wrap around after ~497 days at 100 Hz.
 * --------------------------------------------------------------------------- */
uint32_t pit_get_ticks(void)
{
    return tick_count;
}

/* ---------------------------------------------------------------------------
 * pit_get_uptime_ms - Get system uptime in milliseconds
 * ---------------------------------------------------------------------------
 * Returns:
 *   Milliseconds since boot
 *
 * Note: This may wrap around after ~49 days.
 * --------------------------------------------------------------------------- */
uint32_t pit_get_uptime_ms(void)
{
    return tick_count * ms_per_tick;
}

/* ---------------------------------------------------------------------------
 * pit_get_uptime_sec - Get system uptime in seconds
 * ---------------------------------------------------------------------------
 * Returns:
 *   Seconds since boot
 * --------------------------------------------------------------------------- */
uint32_t pit_get_uptime_sec(void)
{
    return tick_count / timer_frequency;
}

/* ---------------------------------------------------------------------------
 * pit_sleep_ticks - Sleep for a specified number of ticks
 * ---------------------------------------------------------------------------
 * Parameters:
 *   ticks - Number of timer ticks to sleep
 *
 * Warning: This is a busy-wait! The CPU spins doing nothing.
 *          For real multitasking, use the scheduler's sleep function.
 * --------------------------------------------------------------------------- */
void pit_sleep_ticks(uint32_t ticks)
{
    uint32_t target = tick_count + ticks;
    
    /* Wait for tick_count to reach target */
    while (tick_count < target) {
        /* 
         * Hint to CPU that we're in a spin loop.
         * This reduces power consumption and improves performance
         * on hyperthreaded CPUs.
         */
        __asm__ volatile("pause");
    }
}

/* ---------------------------------------------------------------------------
 * pit_sleep_ms - Sleep for a specified number of milliseconds
 * ---------------------------------------------------------------------------
 * Parameters:
 *   ms - Number of milliseconds to sleep
 *
 * Warning: This is a busy-wait! See pit_sleep_ticks.
 * --------------------------------------------------------------------------- */
void pit_sleep_ms(uint32_t ms)
{
    /* Convert milliseconds to ticks */
    uint32_t ticks = ms / ms_per_tick;
    if (ticks == 0 && ms > 0) {
        ticks = 1;  /* Sleep at least one tick */
    }
    pit_sleep_ticks(ticks);
}

/* ---------------------------------------------------------------------------
 * pit_register_callback - Register a callback for each timer tick
 * ---------------------------------------------------------------------------
 * Parameters:
 *   callback - Function to call on each tick (NULL to disable)
 *
 * The callback runs in interrupt context, so it must be fast and not block.
 * This is primarily used by the scheduler to implement time slicing.
 * --------------------------------------------------------------------------- */
void pit_register_callback(pit_callback_t callback)
{
    tick_callback = callback;
}

/* ---------------------------------------------------------------------------
 * pit_read_count - Read the current counter value (for precise timing)
 * ---------------------------------------------------------------------------
 * Returns:
 *   Current counter value (counts down from divisor to 0)
 *
 * This can be used for sub-tick timing measurements.
 * --------------------------------------------------------------------------- */
uint16_t pit_read_count(void)
{
    uint16_t count;
    
    /* Latch the current count */
    outb(PIT_COMMAND, PIT_CMD_CHANNEL0 | PIT_CMD_LATCH);
    
    /* Read low byte, then high byte */
    count = inb(PIT_CHANNEL0_DATA);
    count |= (uint16_t)inb(PIT_CHANNEL0_DATA) << 8;
    
    return count;
}
