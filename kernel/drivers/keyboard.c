/*
 * NanoSec OS - Keyboard Driver
 */

#include "../kernel.h"

#define KB_DATA_PORT 0x60
#define KB_STATUS_PORT 0x64

/* Keyboard buffer */
#define KB_BUFFER_SIZE 256
static char kb_buffer[KB_BUFFER_SIZE];
static volatile int kb_buffer_start = 0;
static volatile int kb_buffer_end = 0;

/* Modifier state */
static uint8_t shift_pressed = 0;
static uint8_t ctrl_pressed = 0;
static uint8_t caps_lock = 0;

/* Scan code to ASCII */
static const char scancode_ascii[] = {
    0,   0,   '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
    '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
    'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
    'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' '};

static const char scancode_ascii_shift[] = {
    0,   0,   '!',  '@',  '#',  '$', '%', '^', '&', '*', '(', ')',
    '_', '+', '\b', '\t', 'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{',  '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
    'J', 'K', 'L',  ':',  '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M',  '<',  '>',  '?', 0,   '*', 0,   ' '};

/* Keyboard interrupt handler - called from IRQ1 */
void keyboard_handler(void) {
  uint8_t scancode = inb(KB_DATA_PORT);

  /* Key release */
  if (scancode & 0x80) {
    scancode &= 0x7F;
    if (scancode == 0x2A || scancode == 0x36)
      shift_pressed = 0;
    if (scancode == 0x1D)
      ctrl_pressed = 0; /* Left Ctrl release */
    return;
  }

  /* Modifier keys - press */
  if (scancode == 0x2A || scancode == 0x36) {
    shift_pressed = 1;
    return;
  }
  if (scancode == 0x1D) {
    ctrl_pressed = 1;
    return;
  } /* Left Ctrl */
  if (scancode == 0x3A) {
    caps_lock = !caps_lock;
    return;
  }

  /* Translate */
  char c = 0;
  if (scancode < sizeof(scancode_ascii)) {
    c = shift_pressed ? scancode_ascii_shift[scancode]
                      : scancode_ascii[scancode];
    if (caps_lock && c >= 'a' && c <= 'z')
      c -= 32;
    else if (caps_lock && c >= 'A' && c <= 'Z')
      c += 32;
  }

  /* Handle Ctrl+key combinations - only for letter keys */
  if (ctrl_pressed && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
    if (c >= 'a' && c <= 'z') {
      c = c - 'a' + 1; /* Ctrl+A=1, Ctrl+S=19, Ctrl+Q=17 */
    } else {
      c = c - 'A' + 1;
    }
  }

  /* Add to buffer */
  if (c != 0) {
    int next = (kb_buffer_end + 1) % KB_BUFFER_SIZE;
    if (next != kb_buffer_start) {
      kb_buffer[kb_buffer_end] = c;
      kb_buffer_end = next;
    }
  }
}

int keyboard_init(void) {
  while (inb(KB_STATUS_PORT) & 0x01)
    inb(KB_DATA_PORT);
  return 0;
}

int keyboard_available(void) { return kb_buffer_start != kb_buffer_end; }

/* Polling-based getchar for now */
char keyboard_getchar(void) {
  while (1) {
    if (inb(KB_STATUS_PORT) & 0x01) {
      keyboard_handler();
    }
    if (keyboard_available()) {
      char c = kb_buffer[kb_buffer_start];
      kb_buffer_start = (kb_buffer_start + 1) % KB_BUFFER_SIZE;
      return c;
    }
  }
}

/*
 * Non-blocking getchar - returns 0 if no key available
 */
char keyboard_getchar_nonblocking(void) {
  /* Check for new input */
  if (inb(KB_STATUS_PORT) & 0x01) {
    keyboard_handler();
  }

  if (keyboard_available()) {
    char c = kb_buffer[kb_buffer_start];
    kb_buffer_start = (kb_buffer_start + 1) % KB_BUFFER_SIZE;
    return c;
  }

  return 0;
}

/*
 * Read a line of input with echo
 */
void keyboard_getline(char *buf, int max) {
  int i = 0;
  while (i < max - 1) {
    char c = keyboard_getchar();

    if (c == '\n') {
      buf[i] = '\0';
      vga_putchar('\n');
      return;
    } else if (c == '\b') {
      if (i > 0) {
        i--;
        vga_putchar('\b');
        vga_putchar(' ');
        vga_putchar('\b');
      }
    } else if (c >= ' ' && c <= '~') {
      buf[i++] = c;
      vga_putchar(c);
    }
  }
  buf[i] = '\0';
}
