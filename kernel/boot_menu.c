/*
 * NanoSec OS - Boot Menu
 * =======================
 * Allows user to select CLI or GUI mode at boot
 */

#include "kernel.h"

/* Boot mode selection */
#define BOOT_MODE_CLI 1
#define BOOT_MODE_GUI 2

static int boot_mode = BOOT_MODE_CLI;

/*
 * Display boot menu and wait for selection
 */
int boot_menu_show(void) {
  vga_clear();

  /* Draw header */
  vga_set_color(VGA_COLOR_CYAN);
  kprintf("\n");
  kprintf("  ███╗   ██╗ █████╗ ███╗   ██╗ ██████╗ ███████╗███████╗ ██████╗\n");
  kprintf("  ████╗  ██║██╔══██╗████╗  ██║██╔═══██╗██╔════╝██╔════╝██╔════╝\n");
  kprintf("  ██╔██╗ ██║███████║██╔██╗ ██║██║   ██║███████╗█████╗  ██║     \n");
  kprintf("  ██║╚██╗██║██╔══██║██║╚██╗██║██║   ██║╚════██║██╔══╝  ██║     \n");
  kprintf("  ██║ ╚████║██║  ██║██║ ╚████║╚██████╔╝███████║███████╗╚██████╗\n");
  kprintf("  ╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝╚══════╝ ╚═════╝\n");

  vga_set_color(VGA_COLOR_WHITE);
  kprintf("\n                    Security-Focused Operating System\n");
  kprintf("                           Version 1.0.0\n\n");

  /* Draw menu box */
  vga_set_color(VGA_COLOR_LIGHT_GREY);
  kprintf("  ┌────────────────────────────────────────────────────────────┐\n");
  kprintf("  │                     SELECT BOOT MODE                       │\n");
  kprintf("  ├────────────────────────────────────────────────────────────┤\n");
  kprintf("  │                                                            │\n");

  vga_set_color(VGA_COLOR_GREEN);
  kprintf("  │       [1]  CLI Mode  ");
  vga_set_color(VGA_COLOR_LIGHT_GREY);
  kprintf("- Text-based command line            │\n");
  kprintf("  │                                                            │\n");

  vga_set_color(VGA_COLOR_CYAN);
  kprintf("  │       [2]  GUI Mode  ");
  vga_set_color(VGA_COLOR_LIGHT_GREY);
  kprintf("- Graphical desktop                  │\n");
  kprintf("  │                                                            │\n");
  kprintf(
      "  └────────────────────────────────────────────────────────────┘\n\n");

  vga_set_color(VGA_COLOR_YELLOW);
  kprintf("  Press 1 or 2 to select (Default: CLI in 10 seconds)\n\n");

  vga_set_color(VGA_COLOR_LIGHT_GREY);

  /* Wait for input with timeout - longer timeout for user to react */
  int seconds_left = 10;

  while (seconds_left > 0) {
    kprintf("\r  Timeout: %d seconds   ", seconds_left);

    /* Check keyboard multiple times per second */
    for (int check = 0; check < 50; check++) {
      char c = keyboard_getchar_nonblocking();

      if (c == '1') {
        boot_mode = BOOT_MODE_CLI;
        vga_set_color(VGA_COLOR_GREEN);
        kprintf("\n\n  >> Selected: CLI Mode\n");
        for (volatile int d = 0; d < 50000000; d++)
          ; /* Brief pause */
        return BOOT_MODE_CLI;
      } else if (c == '2') {
        boot_mode = BOOT_MODE_GUI;
        vga_set_color(VGA_COLOR_CYAN);
        kprintf("\n\n  >> Selected: GUI Mode\n");
        for (volatile int d = 0; d < 50000000; d++)
          ; /* Brief pause */
        return BOOT_MODE_GUI;
      }

      /* Delay ~20ms per check (50 checks = ~1 second) */
      for (volatile int i = 0; i < 5000000; i++)
        ;
    }

    seconds_left--;
  }

  /* Default to CLI on timeout */
  vga_set_color(VGA_COLOR_YELLOW);
  kprintf("\n\n  Timeout - Starting CLI Mode...\n");
  for (volatile int d = 0; d < 50000000; d++)
    ; /* Brief pause */
  boot_mode = BOOT_MODE_CLI;
  return BOOT_MODE_CLI;
}

/*
 * Get current boot mode
 */
int boot_get_mode(void) { return boot_mode; }

/*
 * Check if GUI mode
 */
int boot_is_gui(void) { return boot_mode == BOOT_MODE_GUI; }

/*
 * Check if CLI mode
 */
int boot_is_cli(void) { return boot_mode == BOOT_MODE_CLI; }
