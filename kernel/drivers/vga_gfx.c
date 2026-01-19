/*
 * NanoSec OS - VGA Graphics Mode
 * =================================
 * Mode 13h (320x200 256 colors)
 */

#include "../kernel.h"

/* VGA ports */
#define VGA_MISC_WRITE 0x3C2
#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA 0x3C5
#define VGA_GC_INDEX 0x3CE
#define VGA_GC_DATA 0x3CF
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA 0x3D5
#define VGA_AC_INDEX 0x3C0
#define VGA_AC_WRITE 0x3C0
#define VGA_AC_READ 0x3C1
#define VGA_INSTAT_READ 0x3DA
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA 0x3C9

/* Framebuffer */
#define VGA_GFX_ADDRESS 0xA0000
static uint8_t *framebuffer = (uint8_t *)VGA_GFX_ADDRESS;

/* Screen dimensions */
#define GFX_WIDTH 320
#define GFX_HEIGHT 200

/* Current mode */
static int graphics_mode = 0;

/* Mode 13h register values */
static uint8_t mode_13h_misc = 0x63;

static uint8_t mode_13h_seq[] = {0x03, 0x01, 0x0F, 0x00, 0x0E};

static uint8_t mode_13h_crtc[] = {0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF,
                                  0x1F, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x9C, 0x0E, 0x8F, 0x28, 0x40,
                                  0x96, 0xB9, 0xA3, 0xFF};

static uint8_t mode_13h_gc[] = {0x00, 0x00, 0x00, 0x00, 0x00,
                                0x40, 0x05, 0x0F, 0xFF};

static uint8_t mode_13h_ac[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                                0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
                                0x0E, 0x0F, 0x41, 0x00, 0x0F, 0x00, 0x00};

/* Forward declarations */
static uint8_t crtc_read(uint8_t index);
void gfx_clear(uint8_t color);

/*
 * Write to sequencer register
 */
static void seq_write(uint8_t index, uint8_t value) {
  outb(VGA_SEQ_INDEX, index);
  outb(VGA_SEQ_DATA, value);
}

/*
 * Write to CRTC register
 */
static void crtc_write(uint8_t index, uint8_t value) {
  outb(VGA_CRTC_INDEX, index);
  outb(VGA_CRTC_DATA, value);
}

/*
 * Write to graphics controller register
 */
static void gc_write(uint8_t index, uint8_t value) {
  outb(VGA_GC_INDEX, index);
  outb(VGA_GC_DATA, value);
}

/*
 * Write to attribute controller register
 */
static void ac_write(uint8_t index, uint8_t value) {
  inb(VGA_INSTAT_READ); /* Reset flip-flop */
  outb(VGA_AC_INDEX, index);
  outb(VGA_AC_WRITE, value);
}

/*
 * Enter Mode 13h (320x200x256)
 */
int gfx_init(void) {
  /* Write misc register */
  outb(VGA_MISC_WRITE, mode_13h_misc);

  /* Write sequencer registers */
  for (int i = 0; i < 5; i++) {
    seq_write(i, mode_13h_seq[i]);
  }

  /* Unlock CRTC */
  crtc_write(0x03, crtc_read(0x03) | 0x80);
  crtc_write(0x11, crtc_read(0x11) & 0x7F);

  /* Write CRTC registers */
  for (int i = 0; i < 25; i++) {
    crtc_write(i, mode_13h_crtc[i]);
  }

  /* Write GC registers */
  for (int i = 0; i < 9; i++) {
    gc_write(i, mode_13h_gc[i]);
  }

  /* Write AC registers */
  for (int i = 0; i < 21; i++) {
    ac_write(i, mode_13h_ac[i]);
  }

  /* Enable video */
  inb(VGA_INSTAT_READ);
  outb(VGA_AC_INDEX, 0x20);

  graphics_mode = 1;

  /* Clear screen */
  gfx_clear(0);

  return 0;
}

/* Helper to read CRTC */
static uint8_t crtc_read(uint8_t index) {
  outb(VGA_CRTC_INDEX, index);
  return inb(VGA_CRTC_DATA);
}

/*
 * Clear screen
 */
void gfx_clear(uint8_t color) {
  if (!graphics_mode)
    return;
  memset(framebuffer, color, GFX_WIDTH * GFX_HEIGHT);
}

/*
 * Put pixel
 */
void gfx_put_pixel(int x, int y, uint8_t color) {
  if (!graphics_mode)
    return;
  if (x < 0 || x >= GFX_WIDTH || y < 0 || y >= GFX_HEIGHT)
    return;
  framebuffer[y * GFX_WIDTH + x] = color;
}

/*
 * Get pixel
 */
uint8_t gfx_get_pixel(int x, int y) {
  if (!graphics_mode)
    return 0;
  if (x < 0 || x >= GFX_WIDTH || y < 0 || y >= GFX_HEIGHT)
    return 0;
  return framebuffer[y * GFX_WIDTH + x];
}

/*
 * Draw line (Bresenham)
 */
void gfx_line(int x0, int y0, int x1, int y1, uint8_t color) {
  int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
  int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (1) {
    gfx_put_pixel(x0, y0, color);

    if (x0 == x1 && y0 == y1)
      break;

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

/*
 * Draw rectangle
 */
void gfx_rect(int x, int y, int w, int h, uint8_t color) {
  gfx_line(x, y, x + w - 1, y, color);
  gfx_line(x + w - 1, y, x + w - 1, y + h - 1, color);
  gfx_line(x + w - 1, y + h - 1, x, y + h - 1, color);
  gfx_line(x, y + h - 1, x, y, color);
}

/*
 * Fill rectangle
 */
void gfx_fill_rect(int x, int y, int w, int h, uint8_t color) {
  for (int j = y; j < y + h; j++) {
    for (int i = x; i < x + w; i++) {
      gfx_put_pixel(i, j, color);
    }
  }
}

/*
 * Draw circle
 */
void gfx_circle(int cx, int cy, int r, uint8_t color) {
  int x = r, y = 0;
  int err = 0;

  while (x >= y) {
    gfx_put_pixel(cx + x, cy + y, color);
    gfx_put_pixel(cx + y, cy + x, color);
    gfx_put_pixel(cx - y, cy + x, color);
    gfx_put_pixel(cx - x, cy + y, color);
    gfx_put_pixel(cx - x, cy - y, color);
    gfx_put_pixel(cx - y, cy - x, color);
    gfx_put_pixel(cx + y, cy - x, color);
    gfx_put_pixel(cx + x, cy - y, color);

    y++;
    if (err <= 0) {
      err += 2 * y + 1;
    }
    if (err > 0) {
      x--;
      err -= 2 * x + 1;
    }
  }
}

/*
 * Set palette color
 */
void gfx_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
  outb(VGA_DAC_WRITE_INDEX, index);
  outb(VGA_DAC_DATA, r >> 2); /* VGA uses 6-bit colors */
  outb(VGA_DAC_DATA, g >> 2);
  outb(VGA_DAC_DATA, b >> 2);
}

/*
 * Check if in graphics mode
 */
int gfx_is_active(void) { return graphics_mode; }
