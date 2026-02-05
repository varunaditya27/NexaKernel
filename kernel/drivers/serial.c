/*
 * ===========================================================================
 * kernel/drivers/serial.c
 * ===========================================================================
 *
 * Serial Port (UART) Driver
 *
 * This driver provides basic support for the 16550 UART serial controller.
 * It is primarily used for kernel debugging and logging to the host terminal
 * via the COM1 port (0x3F8).
 *
 * ===========================================================================
 */

#include "drivers.h"

/* ---------------------------------------------------------------------------
 * External Functions (from startup.asm)
 * --------------------------------------------------------------------------- */
extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);

/* ---------------------------------------------------------------------------
 * Serial Port Registers (Offsets from Base)
 * --------------------------------------------------------------------------- */
#define SERIAL_DATA_REG         0   /* Data register (R/W) */
#define SERIAL_IER_REG          1   /* Interrupt Enable Register (R/W) */
#define SERIAL_IIR_REG          2   /* Interrupt Identification Register (R) */
#define SERIAL_FCR_REG          2   /* FIFO Control Register (W) */
#define SERIAL_LCR_REG          3   /* Line Control Register (R/W) */
#define SERIAL_MCR_REG          4   /* Modem Control Register (R/W) */
#define SERIAL_LSR_REG          5   /* Line Status Register (R) */
#define SERIAL_MSR_REG          6   /* Modem Status Register (R) */
#define SERIAL_SCRATCH_REG      7   /* Scratch Register (R/W) */

/* DLAB (Divisor Latch Access Bit) offsets */
#define SERIAL_DLL_REG          0   /* Divisor Latch Low (R/W) */
#define SERIAL_DLH_REG          1   /* Divisor Latch High (R/W) */

/* ---------------------------------------------------------------------------
 * Line Status Register Bits
 * --------------------------------------------------------------------------- */
#define LSR_DATA_READY          0x01    /* Data available to read */
#define LSR_TRANSMIT_EMPTY      0x20    /* Transmit holding register empty */

/* ---------------------------------------------------------------------------
 * serial_init - Initialize the COM1 serial port
 * --------------------------------------------------------------------------- */
int serial_init(void)
{
    /* Disable all interrupts */
    outb(SERIAL_COM1_PORT + SERIAL_IER_REG, 0x00);
    
    /* Enable DLAB (set baud rate divisor) */
    outb(SERIAL_COM1_PORT + SERIAL_LCR_REG, 0x80);
    
    /* Set divisor to 3 (38400 baud) */
    /* 115200 / 3 = 38400 */
    outb(SERIAL_COM1_PORT + SERIAL_DLL_REG, 0x03);
    outb(SERIAL_COM1_PORT + SERIAL_DLH_REG, 0x00);
    
    /* 8 bits, no parity, one stop bit */
    outb(SERIAL_COM1_PORT + SERIAL_LCR_REG, 0x03);
    
    /* Enable FIFO, clear them, with 14-byte threshold */
    outb(SERIAL_COM1_PORT + SERIAL_FCR_REG, 0xC7);
    
    /* IRQs enabled, RTS/DSR set */
    outb(SERIAL_COM1_PORT + SERIAL_MCR_REG, 0x0B);
    
    /* Set in loopback mode, test the serial chip */
    outb(SERIAL_COM1_PORT + SERIAL_MCR_REG, 0x1E);
    outb(SERIAL_COM1_PORT + SERIAL_DATA_REG, 0xAE);
    
    /* Check if serial is faulty (i.e: not same byte as sent) */
    if (inb(SERIAL_COM1_PORT + SERIAL_DATA_REG) != 0xAE) {
        return 1;
    }
    
    /* If serial is not faulty set it in normal operation mode
       (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled) */
    outb(SERIAL_COM1_PORT + SERIAL_MCR_REG, 0x0F);
    
    return 0;
}

/* ---------------------------------------------------------------------------
 * is_transmit_empty - Check if the transmit buffer is empty
 * --------------------------------------------------------------------------- */
static int is_transmit_empty(void)
{
    return inb(SERIAL_COM1_PORT + SERIAL_LSR_REG) & LSR_TRANSMIT_EMPTY;
}

/* ---------------------------------------------------------------------------
 * serial_putchar - Write a character to the serial port
 * --------------------------------------------------------------------------- */
void serial_putchar(char c)
{
    while (is_transmit_empty() == 0);
    outb(SERIAL_COM1_PORT + SERIAL_DATA_REG, c);
}

/* ---------------------------------------------------------------------------
 * serial_write_string - Write a string to the serial port
 * --------------------------------------------------------------------------- */
void serial_write_string(const char *str)
{
    while (*str) {
        serial_putchar(*str++);
    }
}

/* ---------------------------------------------------------------------------
 * serial_received - Check if data has been received
 * --------------------------------------------------------------------------- */
bool serial_received(void)
{
    return inb(SERIAL_COM1_PORT + SERIAL_LSR_REG) & LSR_DATA_READY;
}

/* ---------------------------------------------------------------------------
 * serial_getchar - Read a character from the serial port
 * --------------------------------------------------------------------------- */
char serial_getchar(void)
{
    while (serial_received() == 0);
    return inb(SERIAL_COM1_PORT + SERIAL_DATA_REG);
}
