/*
 * NanoSec OS - Timer Driver
 * ==========================
 * Programmable Interval Timer (PIT) driver
 */

#include "../kernel.h"

/* PIT ports */
#define PIT_CHANNEL0 0x40
#define PIT_CHANNEL1 0x41
#define PIT_CHANNEL2 0x42
#define PIT_COMMAND 0x43

/* PIT frequency */
#define PIT_FREQUENCY 1193182

/* Timer state */
static volatile uint32_t timer_ticks = 0;
static uint32_t timer_freq = 100; /* 100 Hz default */

/*
 * Timer interrupt handler
 * Called by IRQ0
 */
void timer_handler(void) { timer_ticks++; }

/*
 * Initialize PIT timer
 */
int timer_init(uint32_t freq) {
  if (freq == 0)
    freq = 100;
  timer_freq = freq;

  uint32_t divisor = PIT_FREQUENCY / freq;

  /* Set command byte: channel 0, lobyte/hibyte, square wave */
  outb(PIT_COMMAND, 0x36);

  /* Set frequency divisor */
  outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
  outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

  timer_ticks = 0;

  return 0;
}

/*
 * Get current tick count
 */
uint32_t timer_get_ticks(void) { return timer_ticks; }

/*
 * Get uptime in seconds
 */
uint32_t timer_get_uptime(void) { return timer_ticks / timer_freq; }

/*
 * Busy-wait delay in milliseconds
 */
void timer_delay_ms(uint32_t ms) {
  uint32_t target = timer_ticks + (ms * timer_freq / 1000);
  while (timer_ticks < target) {
    asm volatile("hlt");
  }
}

/*
 * Simple delay using CPU cycles (for when timer isn't set up)
 */
void delay(uint32_t count) {
  for (volatile uint32_t i = 0; i < count * 10000; i++) {
    asm volatile("nop");
  }
}
