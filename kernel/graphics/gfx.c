/*
 * NanoSec OS - Graphics Abstraction Layer
 * ========================================
 * Unified graphics API that automatically uses VESA or VGA
 * Provides consistent interface regardless of video mode
 */

#include "../kernel.h"

/* Graphics mode state */
static int gfx_mode = 0; /* 0=none, 1=VGA 320x200, 2=VESA 800x600 */
static int screen_width = 0;
static int screen_height = 0;

/*
 * Initialize graphics - auto-detect best mode
 * Returns: 0=success, -1=failure
 */
int gfx_init_auto(uint32_t mb_magic, uint32_t *mb_info) {
  /* Try VESA first (if multiboot with framebuffer) */
  if (vesa_init(mb_magic, mb_info) == 0) {
    gfx_mode = 2;
    screen_width = 800;
    screen_height = 600;
    return 0;
  }

  /* Fall back to VGA Mode 13h */
  /* gfx_init() would set VGA mode - for now just fail */
  gfx_mode = 0;
  return -1;
}

/*
 * Check if graphics mode is active
 */
int gfx_mode_active(void) { return gfx_mode > 0; }

/*
 * Check if using VESA high resolution
 */
int gfx_is_vesa(void) { return gfx_mode == 2; }

/*
 * Get screen dimensions
 */
void gfx_get_screen_size(int *w, int *h) {
  if (w)
    *w = screen_width;
  if (h)
    *h = screen_height;
}

/*
 * Clear screen with color
 */
void gfx_clear_screen(uint32_t color) {
  if (gfx_mode == 2) {
    vesa_clear(color);
  }
}

/*
 * Draw pixel
 */
void gfx_pixel(int x, int y, uint32_t color) {
  if (gfx_mode == 2) {
    vesa_put_pixel(x, y, color);
  }
}

/*
 * Draw line
 */
void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
  if (gfx_mode == 2) {
    vesa_line(x0, y0, x1, y1, color);
  }
}

/*
 * Draw rectangle outline
 */
void gfx_draw_rect(int x, int y, int w, int h, uint32_t color) {
  if (gfx_mode == 2) {
    vesa_rect(x, y, w, h, color);
  }
}

/*
 * Fill rectangle
 */
void gfx_draw_fill_rect(int x, int y, int w, int h, uint32_t color) {
  if (gfx_mode == 2) {
    vesa_fill_rect(x, y, w, h, color);
  }
}

/*
 * Draw horizontal line
 */
void gfx_draw_hline(int x, int y, int len, uint32_t color) {
  if (gfx_mode == 2) {
    vesa_hline(x, y, len, color);
  }
}

/*
 * Draw character
 */
void gfx_draw_char(int x, int y, char c, uint32_t color) {
  if (gfx_mode == 2) {
    vesa_draw_char(x, y, c, color);
  }
}

/*
 * Draw string
 */
void gfx_draw_text(int x, int y, const char *str, uint32_t color) {
  if (gfx_mode == 2) {
    vesa_draw_string(x, y, str, color);
  }
}

/*
 * String length helper
 */
int gfx_strlen(const char *s) {
  int len = 0;
  while (*s++)
    len++;
  return len;
}
