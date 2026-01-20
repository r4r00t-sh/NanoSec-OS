/*
 * NanoSec OS - Login Screen
 * ==========================
 * Graphical login using the unified graphics API
 * Automatically adapts to available resolution
 */

#include "../kernel.h"

/* Color palette */
#define COL_BG 0x0A1628     /* Dark navy */
#define COL_BOX 0x1E3A5F    /* Blue-grey */
#define COL_ACCENT 0x3498DB /* Light blue */
#define COL_WHITE 0xFFFFFF
#define COL_GREY 0x888888
#define COL_GREEN 0x27AE60
#define COL_RED 0xE74C3C

/*
 * Draw input field
 */
static void draw_input_field(int x, int y, int w, const char *value,
                             int is_password, int focused) {
  gfx_draw_fill_rect(x, y, w, 28, 0x0D1B2A);
  gfx_draw_rect(x, y, w, 28, focused ? COL_ACCENT : COL_GREY);

  int tx = x + 8;
  if (is_password) {
    const char *v = value;
    while (*v) {
      gfx_draw_char(tx, y + 6, '*', COL_WHITE);
      tx += 10;
      v++;
    }
  } else {
    gfx_draw_text(x + 8, y + 6, value, COL_WHITE);
  }

  /* Cursor */
  if (focused) {
    int cursor_x = x + 8 + (is_password ? 10 : 8) * gfx_strlen(value);
    gfx_draw_fill_rect(cursor_x, y + 6, 2, 16, COL_WHITE);
  }
}

/*
 * Draw button
 */
static void draw_button(int x, int y, int w, int h, const char *text) {
  gfx_draw_fill_rect(x, y, w, h, 0x2980B9);
  gfx_draw_rect(x, y, w, h, COL_WHITE);

  int len = gfx_strlen(text);
  int tx = x + (w - len * 8) / 2;
  int ty = y + (h - 16) / 2;
  gfx_draw_text(tx, ty, text, COL_WHITE);
}

/*
 * Login screen - returns 1 on success, 0 on failure/exit
 */
int login_show(void) {
  char username[32] = "";
  char password[32] = "";
  int username_len = 0;
  int password_len = 0;
  int field = 0; /* 0=username, 1=password */
  int error = 0;

  int sw, sh;
  gfx_get_screen_size(&sw, &sh);

  while (1) {
    /* Clear screen */
    gfx_clear_screen(COL_BG);

    /* Title */
    gfx_draw_text(sw / 2 - 44, 80, "NANOSEC OS", 0x5DADE2);
    gfx_draw_text(sw / 2 - 140, 110, "Security-Focused Operating System",
                  COL_GREY);

    /* Login box */
    int bx = sw / 2 - 150, by = 180, bw = 300, bh = 280;
    gfx_draw_fill_rect(bx, by, bw, bh, COL_BOX);
    gfx_draw_rect(bx, by, bw, bh, 0x5DADE2);

    /* Box title */
    gfx_draw_text(bx + 115, by + 20, "LOGIN", COL_WHITE);
    gfx_draw_fill_rect(bx + 100, by + 45, 100, 2, COL_ACCENT);

    /* Username */
    gfx_draw_text(bx + 30, by + 70, "Username:", COL_GREY);
    draw_input_field(bx + 30, by + 90, 240, username, 0, field == 0);

    /* Password */
    gfx_draw_text(bx + 30, by + 135, "Password:", COL_GREY);
    draw_input_field(bx + 30, by + 155, 240, password, 1, field == 1);

    /* Login button */
    draw_button(bx + 80, by + 210, 140, 40, "LOGIN");

    /* Error message */
    if (error) {
      gfx_draw_text(bx + 65, by + 260, "Invalid credentials!", COL_RED);
    }

    /* Footer */
    gfx_draw_text(sw / 2 - 150, sh - 80, "TAB to switch fields, ENTER to login",
                  COL_GREY);
    gfx_draw_text(sw / 2 - 80, sh - 60, "Default: root / root", 0x555555);

    /* Handle input */
    char c = keyboard_getchar();

    if (c == '\n') {
      if (user_login(username, password) == 0) {
        return 1;
      } else {
        error = 1;
        password_len = 0;
        password[0] = '\0';
      }
    } else if (c == '\t') {
      field = 1 - field;
      error = 0;
    } else if (c == '\b') {
      if (field == 0 && username_len > 0) {
        username[--username_len] = '\0';
      } else if (field == 1 && password_len > 0) {
        password[--password_len] = '\0';
      }
    } else if (c >= ' ' && c <= '~') {
      if (field == 0 && username_len < 20) {
        username[username_len++] = c;
        username[username_len] = '\0';
      } else if (field == 1 && password_len < 20) {
        password[password_len++] = c;
        password[password_len] = '\0';
      }
    }
  }

  return 0;
}

/*
 * Start display manager
 */
void dm_start(void) {
  if (login_show()) {
    desktop_start();
  }
}
