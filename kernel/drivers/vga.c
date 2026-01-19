/*
 * NanoSec OS - VGA Text Mode Driver
 */

#include "../kernel.h"

/* VGA constants */
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_CTRL_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5

/* State */
static uint16_t *vga_buffer = (uint16_t *)VGA_MEMORY;
static int cursor_x = 0;
static int cursor_y = 0;
static vga_color_t current_color = VGA_COLOR_LIGHT_GREY;

static inline uint16_t vga_entry(char c, uint8_t color) {
  return (uint16_t)c | ((uint16_t)color << 8);
}

static inline uint8_t vga_color_byte(vga_color_t fg, vga_color_t bg) {
  return fg | (bg << 4);
}

static void update_cursor(void) {
  uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
  outb(VGA_CTRL_PORT, 0x0F);
  outb(VGA_DATA_PORT, (uint8_t)(pos & 0xFF));
  outb(VGA_CTRL_PORT, 0x0E);
  outb(VGA_DATA_PORT, (uint8_t)((pos >> 8) & 0xFF));
}

static void vga_scroll(void) {
  uint8_t color = vga_color_byte(current_color, VGA_COLOR_BLACK);
  for (int y = 0; y < VGA_HEIGHT - 1; y++) {
    for (int x = 0; x < VGA_WIDTH; x++) {
      vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
    }
  }
  for (int x = 0; x < VGA_WIDTH; x++) {
    vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', color);
  }
  cursor_y = VGA_HEIGHT - 1;
}

void vga_init(void) {
  current_color = VGA_COLOR_LIGHT_GREY;
  cursor_x = 0;
  cursor_y = 0;
  outb(VGA_CTRL_PORT, 0x0A);
  outb(VGA_DATA_PORT, (inb(VGA_DATA_PORT) & 0xC0) | 0);
  outb(VGA_CTRL_PORT, 0x0B);
  outb(VGA_DATA_PORT, (inb(VGA_DATA_PORT) & 0xE0) | 15);
  update_cursor();
}

void vga_clear(void) {
  uint8_t color = vga_color_byte(current_color, VGA_COLOR_BLACK);
  for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
    vga_buffer[i] = vga_entry(' ', color);
  }
  cursor_x = 0;
  cursor_y = 0;
  update_cursor();
}

void vga_putchar(char c) {
  uint8_t color = vga_color_byte(current_color, VGA_COLOR_BLACK);

  switch (c) {
  case '\n':
    cursor_x = 0;
    cursor_y++;
    break;
  case '\r':
    cursor_x = 0;
    break;
  case '\t':
    cursor_x = (cursor_x + 8) & ~7;
    break;
  case '\b':
    if (cursor_x > 0)
      cursor_x--;
    break;
  default:
    vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(c, color);
    cursor_x++;
    break;
  }

  if (cursor_x >= VGA_WIDTH) {
    cursor_x = 0;
    cursor_y++;
  }
  if (cursor_y >= VGA_HEIGHT) {
    vga_scroll();
  }
  update_cursor();
}

void vga_puts(const char *str) {
  while (*str)
    vga_putchar(*str++);
}

void vga_set_color(vga_color_t color) { current_color = color; }

vga_color_t vga_get_color(void) { return current_color; }
