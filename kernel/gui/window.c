/*
 * NanoSec OS - Window Manager
 * =============================
 * Simple GUI framework with windows
 */

#include "../kernel.h"

/* Maximum windows */
#define MAX_WINDOWS 16

/* Window flags */
#define WIN_VISIBLE 0x01
#define WIN_ACTIVE 0x02
#define WIN_MOVABLE 0x04
#define WIN_RESIZABLE 0x08
#define WIN_TITLE_BAR 0x10
#define WIN_BORDER 0x20

/* Colors (VGA 256-color palette) */
#define COLOR_DESKTOP 1       /* Dark blue */
#define COLOR_WINDOW_BG 15    /* White */
#define COLOR_TITLE_BAR 9     /* Light blue */
#define COLOR_TITLE_ACTIVE 12 /* Light red */
#define COLOR_TITLE_TEXT 15   /* White */
#define COLOR_BORDER 8        /* Dark grey */
#define COLOR_BUTTON 7        /* Light grey */
#define COLOR_BUTTON_TEXT 0   /* Black */

/* Window structure */
typedef struct {
  int x, y, width, height;
  char title[32];
  uint8_t flags;
  uint8_t bg_color;
  void (*draw_callback)(int win_id, int x, int y, int w, int h);
  void (*click_callback)(int win_id, int x, int y, int button);
} window_t;

/* Window manager state */
static struct {
  window_t windows[MAX_WINDOWS];
  int active_window;
  int dragging;
  int drag_offset_x, drag_offset_y;
  int cursor_x, cursor_y;
  int running;
} wm;

/* External functions */
extern void gfx_clear(uint8_t color);
extern void gfx_put_pixel(int x, int y, uint8_t color);
extern void gfx_line(int x0, int y0, int x1, int y1, uint8_t color);
extern void gfx_rect(int x, int y, int w, int h, uint8_t color);
extern void gfx_fill_rect(int x, int y, int w, int h, uint8_t color);
extern void mouse_get_pos(int *x, int *y);
extern uint8_t mouse_get_buttons(void);
extern int mouse_left_pressed(void);

/* Simple 8x8 font for GUI */
static const uint8_t font_8x8[96][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* space */
    {0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00}, /* ! */
    /* ... More characters would be here */
};

/*
 * Draw character
 */
static void draw_char(int x, int y, char c, uint8_t color) {
  if (c < 32 || c > 127)
    c = '?';
  int idx = c - 32;

  for (int row = 0; row < 8; row++) {
    uint8_t bits = font_8x8[idx][row];
    for (int col = 0; col < 8; col++) {
      if (bits & (0x80 >> col)) {
        gfx_put_pixel(x + col, y + row, color);
      }
    }
  }
}

/*
 * Draw string
 */
static void draw_string(int x, int y, const char *str, uint8_t color) {
  while (*str) {
    draw_char(x, y, *str++, color);
    x += 8;
  }
}

/*
 * Initialize window manager
 */
void wm_init(void) {
  for (int i = 0; i < MAX_WINDOWS; i++) {
    wm.windows[i].flags = 0;
  }
  wm.active_window = -1;
  wm.dragging = -1;
  wm.running = 0;

  kprintf("  [OK] Window Manager\n");
}

/*
 * Create window
 */
int wm_create_window(int x, int y, int w, int h, const char *title,
                     uint8_t flags) {
  for (int i = 0; i < MAX_WINDOWS; i++) {
    if (!(wm.windows[i].flags & WIN_VISIBLE)) {
      window_t *win = &wm.windows[i];
      win->x = x;
      win->y = y;
      win->width = w;
      win->height = h;
      win->flags = flags | WIN_VISIBLE;
      win->bg_color = COLOR_WINDOW_BG;
      win->draw_callback = NULL;
      win->click_callback = NULL;
      strncpy(win->title, title, 31);
      win->title[31] = '\0';

      if (wm.active_window < 0) {
        wm.active_window = i;
        win->flags |= WIN_ACTIVE;
      }

      return i;
    }
  }
  return -1;
}

/*
 * Destroy window
 */
void wm_destroy_window(int id) {
  if (id >= 0 && id < MAX_WINDOWS) {
    wm.windows[id].flags = 0;
    if (wm.active_window == id) {
      wm.active_window = -1;
    }
  }
}

/*
 * Set window callbacks
 */
void wm_set_draw_callback(int id, void (*callback)(int, int, int, int, int)) {
  if (id >= 0 && id < MAX_WINDOWS) {
    wm.windows[id].draw_callback = callback;
  }
}

void wm_set_click_callback(int id, void (*callback)(int, int, int, int)) {
  if (id >= 0 && id < MAX_WINDOWS) {
    wm.windows[id].click_callback = callback;
  }
}

/*
 * Draw single window
 */
static void draw_window(int id) {
  window_t *win = &wm.windows[id];
  if (!(win->flags & WIN_VISIBLE))
    return;

  int title_height = (win->flags & WIN_TITLE_BAR) ? 16 : 0;

  /* Draw border */
  if (win->flags & WIN_BORDER) {
    gfx_rect(win->x - 1, win->y - 1, win->width + 2,
             win->height + title_height + 2, COLOR_BORDER);
  }

  /* Draw title bar */
  if (win->flags & WIN_TITLE_BAR) {
    uint8_t title_color =
        (win->flags & WIN_ACTIVE) ? COLOR_TITLE_ACTIVE : COLOR_TITLE_BAR;
    gfx_fill_rect(win->x, win->y, win->width, title_height, title_color);
    draw_string(win->x + 4, win->y + 4, win->title, COLOR_TITLE_TEXT);

    /* Close button */
    gfx_fill_rect(win->x + win->width - 14, win->y + 2, 12, 12, COLOR_BUTTON);
    draw_char(win->x + win->width - 12, win->y + 4, 'X', COLOR_BUTTON_TEXT);
  }

  /* Draw window background */
  gfx_fill_rect(win->x, win->y + title_height, win->width, win->height,
                win->bg_color);

  /* Call custom draw */
  if (win->draw_callback) {
    win->draw_callback(id, win->x, win->y + title_height, win->width,
                       win->height);
  }
}

/*
 * Draw cursor
 */
static void draw_cursor(int x, int y) {
  /* Simple arrow cursor */
  for (int i = 0; i < 10; i++) {
    gfx_put_pixel(x, y + i, 15);
  }
  for (int i = 0; i < 5; i++) {
    gfx_put_pixel(x + i, y + 5 + i, 15);
  }
  gfx_line(x, y, x + 7, y + 7, 15);
}

/*
 * Draw all windows and desktop
 */
void wm_draw(void) {
  /* Draw desktop */
  gfx_clear(COLOR_DESKTOP);

  /* Draw windows (back to front) */
  for (int i = 0; i < MAX_WINDOWS; i++) {
    if (i != wm.active_window) {
      draw_window(i);
    }
  }

  /* Draw active window on top */
  if (wm.active_window >= 0) {
    draw_window(wm.active_window);
  }

  /* Draw cursor */
  mouse_get_pos(&wm.cursor_x, &wm.cursor_y);
  draw_cursor(wm.cursor_x, wm.cursor_y);
}

/*
 * Handle mouse input
 */
void wm_handle_input(void) {
  int mx, my;
  mouse_get_pos(&mx, &my);
  int buttons = mouse_get_buttons();

  static int last_buttons = 0;
  int clicked = (buttons & 1) && !(last_buttons & 1);
  int released = !(buttons & 1) && (last_buttons & 1);
  last_buttons = buttons;

  /* Handle drag */
  if (wm.dragging >= 0) {
    if (buttons & 1) {
      window_t *win = &wm.windows[wm.dragging];
      win->x = mx - wm.drag_offset_x;
      win->y = my - wm.drag_offset_y;
    } else {
      wm.dragging = -1;
    }
    return;
  }

  /* Check window clicks */
  if (clicked) {
    for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
      window_t *win = &wm.windows[i];
      if (!(win->flags & WIN_VISIBLE))
        continue;

      int title_height = (win->flags & WIN_TITLE_BAR) ? 16 : 0;

      /* Check title bar */
      if (win->flags & WIN_TITLE_BAR && mx >= win->x &&
          mx < win->x + win->width && my >= win->y &&
          my < win->y + title_height) {

        /* Check close button */
        if (mx >= win->x + win->width - 14) {
          wm_destroy_window(i);
          return;
        }

        /* Start drag */
        if (win->flags & WIN_MOVABLE) {
          wm.dragging = i;
          wm.drag_offset_x = mx - win->x;
          wm.drag_offset_y = my - win->y;
        }

        /* Activate window */
        if (wm.active_window != i) {
          if (wm.active_window >= 0) {
            wm.windows[wm.active_window].flags &= ~WIN_ACTIVE;
          }
          wm.active_window = i;
          win->flags |= WIN_ACTIVE;
        }
        return;
      }

      /* Check window content */
      if (mx >= win->x && mx < win->x + win->width &&
          my >= win->y + title_height &&
          my < win->y + title_height + win->height) {

        if (win->click_callback) {
          win->click_callback(i, mx - win->x, my - win->y - title_height,
                              buttons);
        }
        return;
      }
    }
  }
}

/*
 * Run window manager main loop
 */
void wm_run(void) {
  wm.running = 1;

  while (wm.running) {
    wm_handle_input();
    wm_draw();

    /* Small delay */
    for (volatile int i = 0; i < 10000; i++)
      ;
  }
}

/*
 * Stop window manager
 */
void wm_stop(void) { wm.running = 0; }
