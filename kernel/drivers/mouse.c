/*
 * NanoSec OS - PS/2 Mouse Driver
 * ================================
 * PS/2 mouse with interrupt support
 */

#include "../cpu/idt.h"
#include "../kernel.h"


/* PS/2 controller ports */
#define PS2_DATA 0x60
#define PS2_STATUS 0x64
#define PS2_CMD 0x64

/* Mouse commands */
#define MOUSE_CMD_RESET 0xFF
#define MOUSE_CMD_ENABLE 0xF4
#define MOUSE_CMD_DISABLE 0xF5
#define MOUSE_CMD_SET_DEFAULTS 0xF6
#define MOUSE_CMD_SET_RATE 0xF3

/* PS/2 controller commands */
#define PS2_CMD_ENABLE_AUX 0xA8
#define PS2_CMD_DISABLE_AUX 0xA7
#define PS2_CMD_READ_CONFIG 0x20
#define PS2_CMD_WRITE_CONFIG 0x60
#define PS2_CMD_WRITE_MOUSE 0xD4

/* Mouse state */
static struct {
  int x, y;
  uint8_t buttons;
  int cycle;
  uint8_t packet[3];
  int initialized;
} mouse;

/* Screen bounds */
static int screen_width = 320;
static int screen_height = 200;

/*
 * Wait for PS/2 controller to be ready for input
 */
static void ps2_wait_input(void) {
  int timeout = 100000;
  while (timeout-- && (inb(PS2_STATUS) & 0x02))
    ;
}

/*
 * Wait for PS/2 controller to have output
 */
static void ps2_wait_output(void) {
  int timeout = 100000;
  while (timeout-- && !(inb(PS2_STATUS) & 0x01))
    ;
}

/*
 * Send command to PS/2 controller
 */
static void ps2_cmd(uint8_t cmd) {
  ps2_wait_input();
  outb(PS2_CMD, cmd);
}

/*
 * Send command to mouse
 */
static void mouse_cmd(uint8_t cmd) {
  ps2_cmd(PS2_CMD_WRITE_MOUSE);
  ps2_wait_input();
  outb(PS2_DATA, cmd);
  ps2_wait_output();
  inb(PS2_DATA); /* Read ACK */
}

/*
 * Mouse interrupt handler (IRQ12)
 */
static void mouse_handler(interrupt_frame_t *frame) {
  (void)frame;

  uint8_t data = inb(PS2_DATA);

  mouse.packet[mouse.cycle++] = data;

  if (mouse.cycle == 3) {
    mouse.cycle = 0;

    /* Parse packet */
    uint8_t status = mouse.packet[0];

    /* Check for overflow */
    if (status & 0xC0)
      return;

    /* Update buttons */
    mouse.buttons = status & 0x07;

    /* Update position */
    int dx = mouse.packet[1];
    int dy = mouse.packet[2];

    /* Handle sign */
    if (status & 0x10)
      dx |= 0xFFFFFF00;
    if (status & 0x20)
      dy |= 0xFFFFFF00;

    mouse.x += dx;
    mouse.y -= dy; /* Y is inverted in PS/2 */

    /* Clamp to screen */
    if (mouse.x < 0)
      mouse.x = 0;
    if (mouse.x >= screen_width)
      mouse.x = screen_width - 1;
    if (mouse.y < 0)
      mouse.y = 0;
    if (mouse.y >= screen_height)
      mouse.y = screen_height - 1;
  }
}

/*
 * Initialize mouse
 */
int mouse_init(void) {
  mouse.x = screen_width / 2;
  mouse.y = screen_height / 2;
  mouse.buttons = 0;
  mouse.cycle = 0;
  mouse.initialized = 0;

  /* Enable auxiliary device (mouse) */
  ps2_cmd(PS2_CMD_ENABLE_AUX);

  /* Read and modify configuration */
  ps2_cmd(PS2_CMD_READ_CONFIG);
  ps2_wait_output();
  uint8_t config = inb(PS2_DATA);
  config |= 0x02;  /* Enable IRQ12 */
  config &= ~0x20; /* Enable mouse clock */

  ps2_cmd(PS2_CMD_WRITE_CONFIG);
  ps2_wait_input();
  outb(PS2_DATA, config);

  /* Reset mouse */
  mouse_cmd(MOUSE_CMD_RESET);
  ps2_wait_output();
  inb(PS2_DATA); /* Self-test result */
  ps2_wait_output();
  inb(PS2_DATA); /* Mouse ID */

  /* Set defaults and enable */
  mouse_cmd(MOUSE_CMD_SET_DEFAULTS);
  mouse_cmd(MOUSE_CMD_ENABLE);

  /* Register handler */
  isr_register_handler(IRQ12, mouse_handler);

  mouse.initialized = 1;
  kprintf("  [OK] PS/2 Mouse\n");

  return 0;
}

/*
 * Get mouse position
 */
void mouse_get_pos(int *x, int *y) {
  if (x)
    *x = mouse.x;
  if (y)
    *y = mouse.y;
}

/*
 * Get mouse buttons
 */
uint8_t mouse_get_buttons(void) { return mouse.buttons; }

/*
 * Set mouse bounds
 */
void mouse_set_bounds(int width, int height) {
  screen_width = width;
  screen_height = height;

  /* Clamp current position */
  if (mouse.x >= width)
    mouse.x = width - 1;
  if (mouse.y >= height)
    mouse.y = height - 1;
}

/*
 * Check if left button pressed
 */
int mouse_left_pressed(void) { return mouse.buttons & 0x01; }

/*
 * Check if right button pressed
 */
int mouse_right_pressed(void) { return mouse.buttons & 0x02; }
