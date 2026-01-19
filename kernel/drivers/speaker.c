/*
 * NanoSec OS - PC Speaker Driver
 * ================================
 * Play tones through the PC speaker
 */

#include "../kernel.h"

/* PIT registers for speaker */
#define PIT_CHANNEL2 0x42
#define PIT_COMMAND 0x43
#define SPEAKER_PORT 0x61

/* PIT frequency */
#define PIT_FREQUENCY 1193182

/*
 * Set speaker frequency
 */
static void speaker_set_freq(uint32_t freq) {
  if (freq == 0)
    return;

  uint32_t divisor = PIT_FREQUENCY / freq;

  /* Configure PIT channel 2 for square wave */
  outb(PIT_COMMAND, 0xB6); /* Channel 2, lobyte/hibyte, square wave */
  outb(PIT_CHANNEL2, (uint8_t)(divisor & 0xFF));
  outb(PIT_CHANNEL2, (uint8_t)((divisor >> 8) & 0xFF));
}

/*
 * Turn speaker on
 */
static void speaker_on(void) {
  uint8_t val = inb(SPEAKER_PORT);
  if ((val & 0x03) != 0x03) {
    outb(SPEAKER_PORT, val | 0x03);
  }
}

/*
 * Turn speaker off
 */
static void speaker_off(void) {
  uint8_t val = inb(SPEAKER_PORT);
  outb(SPEAKER_PORT, val & 0xFC);
}

/*
 * Simple delay using busy loop
 */
static void speaker_delay(uint32_t ms) {
  for (volatile uint32_t i = 0; i < ms * 5000; i++) {
    asm volatile("nop");
  }
}

/*
 * Play a tone for specified duration
 */
void speaker_beep(uint32_t freq, uint32_t duration_ms) {
  speaker_set_freq(freq);
  speaker_on();
  speaker_delay(duration_ms);
  speaker_off();
}

/*
 * Play startup sound
 */
void speaker_startup(void) {
  speaker_beep(880, 100);  /* A5 */
  speaker_beep(1047, 100); /* C6 */
  speaker_beep(1319, 150); /* E6 */
}

/*
 * Play error sound
 */
void speaker_error(void) {
  speaker_beep(200, 200);
  speaker_beep(150, 300);
}

/*
 * Play alert sound
 */
void speaker_alert(void) {
  for (int i = 0; i < 3; i++) {
    speaker_beep(1000, 100);
    speaker_delay(50);
  }
}

/* ============ Shell Commands ============ */

/*
 * beep command
 * Usage: beep [freq] [duration]
 */
void cmd_beep(const char *args) {
  uint32_t freq = 1000;
  uint32_t duration = 200;

  if (args[0] != '\0') {
    /* Parse frequency */
    freq = 0;
    const char *p = args;
    while (*p >= '0' && *p <= '9') {
      freq = freq * 10 + (*p - '0');
      p++;
    }
    if (freq == 0)
      freq = 1000;

    /* Parse duration */
    while (*p == ' ')
      p++;
    if (*p) {
      duration = 0;
      while (*p >= '0' && *p <= '9') {
        duration = duration * 10 + (*p - '0');
        p++;
      }
      if (duration == 0)
        duration = 200;
    }
  }

  kprintf("Beep: %d Hz, %d ms\n", freq, duration);
  speaker_beep(freq, duration);
}

/*
 * play command - play a simple melody
 */
void cmd_play(const char *args) {
  (void)args;

  kprintf("Playing melody...\n");

  /* Simple scale */
  uint32_t notes[] = {262, 294, 330, 349, 392, 440, 494, 523};
  for (int i = 0; i < 8; i++) {
    speaker_beep(notes[i], 150);
    speaker_delay(50);
  }

  kprintf("Done.\n");
}
