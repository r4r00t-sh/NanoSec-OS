/*
 * NanoSec OS - Desktop Environment
 * =================================
 * Graphical desktop using the unified graphics API
 * Automatically adapts to available resolution
 */

#include "../kernel.h"

/* Color palette */
#define COL_BG 0x0A1628      /* Dark navy */
#define COL_TASKBAR 0x1A1A2E /* Dark purple */
#define COL_WINDOW 0x1E3A5F  /* Blue-grey */
#define COL_CLIENT 0x0D1B2A  /* Darker blue */
#define COL_TITLE 0x2980B9   /* Blue */
#define COL_WHITE 0xFFFFFF
#define COL_GREY 0xAAAAAA
#define COL_ACCENT 0x3498DB
#define COL_GREEN 0x27AE60
#define COL_RED 0xE74C3C
#define COL_YELLOW 0xF39C12

/* Screen state */
static int screen_w = 800;
static int screen_h = 600;
static int taskbar_h = 40;

/* Desktop state */
static int running = 0;
static int cursor_x = 400;
static int cursor_y = 300;
static int show_menu = 0;
static int active_app = 0; /* 0=none, 1=terminal, 2=files, 3=about */

/*
 * Draw a window with title bar
 */
static void draw_window(int x, int y, int w, int h, const char *title) {
  /* Shadow */
  gfx_draw_fill_rect(x + 6, y + 6, w, h, 0x000000);

  /* Window body */
  gfx_draw_fill_rect(x, y, w, h, COL_WINDOW);
  gfx_draw_rect(x, y, w, h, COL_ACCENT);

  /* Title bar */
  gfx_draw_fill_rect(x + 1, y + 1, w - 2, 28, COL_TITLE);
  gfx_draw_text(x + 10, y + 7, title, COL_WHITE);

  /* Close button */
  gfx_draw_fill_rect(x + w - 30, y + 5, 22, 18, COL_RED);
  gfx_draw_char(x + w - 24, y + 6, 'X', COL_WHITE);

  /* Client area */
  gfx_draw_fill_rect(x + 2, y + 30, w - 4, h - 32, COL_CLIENT);
}

/*
 * Draw desktop icons
 */
static void draw_icons(void) {
  int x = 30, y = 30;

  /* Terminal */
  gfx_draw_fill_rect(x, y, 64, 64, COL_TASKBAR);
  gfx_draw_rect(x, y, 64, 64, COL_GREY);
  gfx_draw_fill_rect(x + 8, y + 8, 48, 12, COL_TITLE);
  gfx_draw_char(x + 20, y + 28, '>', COL_GREEN);
  gfx_draw_char(x + 30, y + 28, '_', COL_WHITE);
  gfx_draw_text(x + 8, y + 72, "Terminal", COL_WHITE);

  /* Files */
  y += 110;
  gfx_draw_fill_rect(x, y, 64, 64, COL_TASKBAR);
  gfx_draw_rect(x, y, 64, 64, COL_GREY);
  gfx_draw_fill_rect(x + 12, y + 20, 40, 34, COL_YELLOW);
  gfx_draw_fill_rect(x + 12, y + 12, 20, 12, COL_YELLOW);
  gfx_draw_text(x + 16, y + 72, "Files", COL_WHITE);

  /* About */
  y += 110;
  gfx_draw_fill_rect(x, y, 64, 64, COL_TASKBAR);
  gfx_draw_rect(x, y, 64, 64, COL_GREY);
  gfx_draw_fill_rect(x + 22, y + 10, 20, 44, COL_ACCENT);
  gfx_draw_char(x + 27, y + 14, 'i', COL_WHITE);
  gfx_draw_text(x + 14, y + 72, "About", COL_WHITE);
}

/*
 * Draw taskbar
 */
static void draw_taskbar(void) {
  int y = screen_h - taskbar_h;

  gfx_draw_fill_rect(0, y, screen_w, taskbar_h, COL_TASKBAR);
  gfx_draw_hline(0, y, screen_w, 0x333344);

  /* Start button */
  int btn_color = show_menu ? COL_ACCENT : COL_TITLE;
  gfx_draw_fill_rect(5, y + 5, 80, 30, btn_color);
  gfx_draw_rect(5, y + 5, 80, 30, COL_WHITE);
  gfx_draw_text(20, y + 12, "START", COL_WHITE);

  /* Clock */
  gfx_draw_text(screen_w - 60, y + 12, "12:00", COL_WHITE);

  /* Username */
  const char *user = user_get_username();
  gfx_draw_text(screen_w - 180, y + 12, user, COL_ACCENT);
}

/*
 * Draw start menu
 */
static void draw_start_menu(void) {
  if (!show_menu)
    return;

  int x = 5, y = screen_h - taskbar_h - 200;
  int w = 180, h = 200;

  gfx_draw_fill_rect(x, y, w, h, COL_TASKBAR);
  gfx_draw_rect(x, y, w, h, COL_ACCENT);

  gfx_draw_text(x + 15, y + 20, "Terminal", COL_WHITE);
  gfx_draw_text(x + 15, y + 50, "Files", COL_WHITE);
  gfx_draw_text(x + 15, y + 80, "About", COL_WHITE);
  gfx_draw_hline(x + 10, y + 110, w - 20, COL_GREY);
  gfx_draw_text(x + 15, y + 130, "Settings", COL_GREY);
  gfx_draw_hline(x + 10, y + 160, w - 20, COL_GREY);
  gfx_draw_text(x + 15, y + 175, "Logout", COL_RED);
}

/*
 * Draw cursor
 */
static void draw_cursor(void) {
  for (int i = 0; i < 16; i++) {
    int len = 12 - i > 0 ? 12 - i : 1;
    gfx_draw_hline(cursor_x, cursor_y + i, len, COL_WHITE);
  }
  gfx_draw_line(cursor_x, cursor_y, cursor_x + 12, cursor_y + 12, COL_WHITE);
}

/*
 * Wait for a new keypress
 */
static void wait_for_key(void) {
  for (volatile int d = 0; d < 3000000; d++)
    ;
  while (keyboard_getchar_nonblocking() != 0)
    ;
  while (keyboard_getchar_nonblocking() == 0) {
    for (volatile int i = 0; i < 50000; i++)
      ;
  }
}

/*
 * Show terminal window
 */
static void show_terminal(void) {
  draw_window(150, 80, 500, 400, "Terminal");

  int tx = 162, ty = 120;
  gfx_draw_text(tx, ty, "NanoSec Shell v1.0.0", COL_GREEN);
  gfx_draw_text(tx, ty + 24, "Type 'help' for commands", COL_GREY);
  gfx_draw_text(tx, ty + 56, "root@nanosec:/$ _", COL_WHITE);
  gfx_draw_text(tx, ty + 100, "[Full terminal in CLI mode]", COL_GREY);
  gfx_draw_text(tx, ty + 124, "[Press Q to exit desktop]", COL_YELLOW);
  gfx_draw_text(tx + 100, ty + 220, "Press any key", COL_ACCENT);

  wait_for_key();
}

/*
 * Show files window
 */
static void show_files(void) {
  draw_window(120, 60, 560, 440, "File Manager");

  int tx = 140, ty = 100;
  gfx_draw_fill_rect(130, 95, 540, 30, COL_CLIENT);
  gfx_draw_text(140, 102, "Location: /home/root", COL_GREY);

  ty = 140;
  gfx_draw_text(tx, ty, "Directories:", COL_ACCENT);

  const char *dirs[] = {"/bin", "/sbin", "/etc", "/home", "/var", "/tmp"};
  for (int i = 0; i < 6; i++) {
    int col = i % 3;
    int row = i / 3;
    gfx_draw_fill_rect(tx + col * 170, ty + 24 + row * 50, 160, 40,
                       COL_TASKBAR);
    gfx_draw_rect(tx + col * 170, ty + 24 + row * 50, 160, 40, COL_GREY);
    gfx_draw_text(tx + col * 170 + 10, ty + 36 + row * 50, dirs[i], COL_YELLOW);
  }

  ty += 150;
  gfx_draw_text(tx, ty, "Files:", COL_ACCENT);
  gfx_draw_text(tx, ty + 20, "readme.txt", COL_WHITE);
  gfx_draw_text(tx + 150, ty + 20, "config.nsh", COL_WHITE);

  gfx_draw_text(tx + 160, ty + 120, "Press any key", COL_ACCENT);

  wait_for_key();
}

/*
 * Show about window
 */
static void show_about(void) {
  draw_window(200, 120, 400, 320, "About NanoSec OS");

  int tx = 230, ty = 170;
  gfx_draw_text(tx + 80, ty, "NANOSEC OS", COL_ACCENT);
  gfx_draw_text(tx + 100, ty + 30, "v1.0.0", COL_WHITE);
  gfx_draw_text(tx, ty + 70, "Security-Focused Operating System", COL_GREY);
  gfx_draw_text(tx, ty + 90, "Written in C and x86 Assembly", COL_GREY);

  gfx_draw_text(tx + 30, ty + 130, "Features:", COL_WHITE);
  gfx_draw_text(tx + 30, ty + 150, "* Unix-like Shell with Pipes", COL_GREEN);
  gfx_draw_text(tx + 30, ty + 170, "* Nash Scripting Language", COL_GREEN);
  gfx_draw_text(tx + 30, ty + 190, "* VESA Graphics Desktop", COL_GREEN);
  gfx_draw_text(tx + 30, ty + 210, "* TCP/IP Networking", COL_GREEN);

  gfx_draw_text(tx + 80, ty + 260, "Press any key", COL_ACCENT);

  wait_for_key();
}

/*
 * Handle keyboard input
 */
static int handle_input(void) {
  char c = keyboard_getchar_nonblocking();
  if (c == 0)
    return 0;

  /* App shortcuts */
  if (c == '1') {
    active_app = 1;
    return 0;
  }
  if (c == '2') {
    active_app = 2;
    return 0;
  }
  if (c == '3') {
    active_app = 3;
    return 0;
  }

  /* Cursor movement */
  if (c == 'w' || c == 'W') {
    cursor_y -= 15;
    if (cursor_y < 0)
      cursor_y = 0;
  }
  if (c == 's' || c == 'S') {
    cursor_y += 15;
    if (cursor_y >= screen_h)
      cursor_y = screen_h - 1;
  }
  if (c == 'a' || c == 'A') {
    cursor_x -= 15;
    if (cursor_x < 0)
      cursor_x = 0;
  }
  if (c == 'd' || c == 'D') {
    cursor_x += 15;
    if (cursor_x >= screen_w)
      cursor_x = screen_w - 1;
  }

  /* Start menu toggle */
  if (c == ' ' || c == '\n') {
    int ty = screen_h - taskbar_h;
    if (cursor_x >= 5 && cursor_x <= 85 && cursor_y >= ty + 5) {
      show_menu = !show_menu;
    }
  }

  /* Quit */
  if (c == 'q' || c == 'Q' || c == 27) {
    return 1;
  }

  return 0;
}

/*
 * Start the desktop environment
 */
void desktop_start(void) {
  gfx_get_screen_size(&screen_w, &screen_h);
  cursor_x = screen_w / 2;
  cursor_y = screen_h / 2;
  running = 1;
  active_app = 0;
  show_menu = 0;

  while (running) {
    /* Draw desktop */
    gfx_clear_screen(COL_BG);
    draw_icons();
    draw_taskbar();
    draw_start_menu();

    /* Help text */
    gfx_draw_text(screen_w - 220, screen_h - 80, "1:Term 2:Files 3:About",
                  COL_GREY);
    gfx_draw_text(screen_w - 180, screen_h - 60, "Q:Quit WASD:Move", COL_GREY);

    /* Handle active application */
    if (active_app == 1) {
      show_terminal();
      active_app = 0;
    } else if (active_app == 2) {
      show_files();
      active_app = 0;
    } else if (active_app == 3) {
      show_about();
      active_app = 0;
    }

    /* Draw cursor */
    draw_cursor();

    /* Handle input */
    if (handle_input()) {
      running = 0;
    }

    /* Frame delay */
    for (volatile int i = 0; i < 50000; i++)
      ;
  }
}

/*
 * Stop the desktop
 */
void desktop_stop(void) { running = 0; }
