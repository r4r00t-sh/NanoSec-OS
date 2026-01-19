/*
 * NanoSec OS - Serial Console Driver
 * ====================================
 * COM1 serial port for debugging
 */

#include "../kernel.h"

/* COM port addresses */
#define COM1_BASE 0x3F8
#define COM2_BASE 0x2F8

/* Serial port registers */
#define SERIAL_DATA 0         /* Data register */
#define SERIAL_INT_ENABLE 1   /* Interrupt enable */
#define SERIAL_FIFO_CTRL 2    /* FIFO control */
#define SERIAL_LINE_CTRL 3    /* Line control */
#define SERIAL_MODEM_CTRL 4   /* Modem control */
#define SERIAL_LINE_STATUS 5  /* Line status */
#define SERIAL_MODEM_STATUS 6 /* Modem status */

/* Baud rate divisor */
#define SERIAL_BAUD_115200 1
#define SERIAL_BAUD_57600 2
#define SERIAL_BAUD_38400 3
#define SERIAL_BAUD_9600 12

static int serial_initialized = 0;
static uint16_t serial_base = COM1_BASE;

/*
 * Initialize serial port
 */
int serial_init(uint16_t base, int baud_divisor) {
  serial_base = base;

  /* Disable interrupts */
  outb(serial_base + SERIAL_INT_ENABLE, 0x00);

  /* Enable DLAB (set baud rate divisor) */
  outb(serial_base + SERIAL_LINE_CTRL, 0x80);

  /* Set baud rate */
  outb(serial_base + SERIAL_DATA, baud_divisor & 0xFF);
  outb(serial_base + SERIAL_INT_ENABLE, (baud_divisor >> 8) & 0xFF);

  /* 8 bits, no parity, one stop bit */
  outb(serial_base + SERIAL_LINE_CTRL, 0x03);

  /* Enable FIFO, clear them, with 14-byte threshold */
  outb(serial_base + SERIAL_FIFO_CTRL, 0xC7);

  /* IRQs enabled, RTS/DSR set */
  outb(serial_base + SERIAL_MODEM_CTRL, 0x0B);

  /* Test serial chip (set loopback mode) */
  outb(serial_base + SERIAL_MODEM_CTRL, 0x1E);
  outb(serial_base + SERIAL_DATA, 0xAE);

  /* Check if loopback works */
  if (inb(serial_base + SERIAL_DATA) != 0xAE) {
    return -1; /* Serial port not working */
  }

  /* Set normal operation mode */
  outb(serial_base + SERIAL_MODEM_CTRL, 0x0F);

  serial_initialized = 1;
  return 0;
}

/*
 * Check if transmit buffer is empty
 */
static int serial_transmit_empty(void) {
  return inb(serial_base + SERIAL_LINE_STATUS) & 0x20;
}

/*
 * Write character to serial
 */
void serial_putchar(char c) {
  if (!serial_initialized)
    return;

  while (!serial_transmit_empty())
    ;
  outb(serial_base + SERIAL_DATA, c);
}

/*
 * Write string to serial
 */
void serial_puts(const char *str) {
  while (*str) {
    if (*str == '\n')
      serial_putchar('\r');
    serial_putchar(*str++);
  }
}

/*
 * Write formatted output to serial (simple)
 */
void serial_printf(const char *fmt, ...) {
  /* Simple implementation - just print the format string */
  serial_puts(fmt);
}

/*
 * Check if data available
 */
int serial_received(void) {
  return inb(serial_base + SERIAL_LINE_STATUS) & 0x01;
}

/*
 * Read character from serial
 */
char serial_getchar(void) {
  while (!serial_received())
    ;
  return inb(serial_base + SERIAL_DATA);
}

/* ============ Shell Commands ============ */

/*
 * dmesg - show kernel messages (sent to serial)
 */
void cmd_dmesg(const char *args) {
  (void)args;
  kprintf("(Kernel messages go to COM1 serial port)\n");
  kprintf("Connect with: screen /dev/ttyS0 115200\n");
}

/*
 * Log message to serial
 */
void klog(const char *msg) {
  if (serial_initialized) {
    serial_puts("[KERNEL] ");
    serial_puts(msg);
    serial_puts("\n");
  }
}
