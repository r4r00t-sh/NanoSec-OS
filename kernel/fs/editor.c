/*
 * NanoSec OS - Simple Text Editor
 * =================================
 * nedit - NanoSec Editor (like nano/vi)
 */

#include "../kernel.h"

#define EDIT_MAX_LINES 50
#define EDIT_MAX_COLS 80

static char edit_buffer[EDIT_MAX_LINES][EDIT_MAX_COLS];
static int edit_lines = 0;
static int edit_cursor_line = 0;
static int edit_cursor_col = 0;
static char edit_filename[64];
static int edit_modified = 0;

/* External filesystem functions */
extern int fs_write(const char *name, const char *data, size_t len);
extern int fs_read(const char *name, char *buf, size_t max);

/*
 * Redraw editor screen
 */
static void edit_redraw(void) {
  vga_clear();

  /* Header */
  vga_set_color(VGA_COLOR_BLACK + (VGA_COLOR_CYAN << 4));
  kprintf(" NanoSec Editor - %s%s", edit_filename,
          edit_modified ? " [modified]" : "");
  for (int i = strlen(edit_filename) + 20; i < 80; i++)
    kprintf(" ");
  vga_set_color(VGA_COLOR_LIGHT_GREY);
  kprintf("\n");

  /* Content */
  for (int i = 0; i < 22; i++) {
    if (i < edit_lines) {
      kprintf("%s", edit_buffer[i]);
    }
    kprintf("\n");
  }

  /* Footer */
  vga_set_color(VGA_COLOR_BLACK + (VGA_COLOR_GREEN << 4));
  kprintf(" ^S Save  ^Q Quit  ^G Help                                Line %d, "
          "Col %d ",
          edit_cursor_line + 1, edit_cursor_col + 1);
  vga_set_color(VGA_COLOR_LIGHT_GREY);
}

/*
 * Load file into editor
 */
static int edit_load(const char *filename) {
  char buf[4096];
  int len = fs_read(filename, buf, sizeof(buf) - 1);

  memset(edit_buffer, 0, sizeof(edit_buffer));
  edit_lines = 0;
  edit_cursor_line = 0;
  edit_cursor_col = 0;
  edit_modified = 0;
  strncpy(edit_filename, filename, 63);

  if (len < 0) {
    /* New file */
    edit_lines = 1;
    return 0;
  }

  buf[len] = '\0';

  /* Parse lines */
  int line = 0, col = 0;
  for (int i = 0; i < len && line < EDIT_MAX_LINES; i++) {
    if (buf[i] == '\n') {
      edit_buffer[line][col] = '\0';
      line++;
      col = 0;
    } else if (col < EDIT_MAX_COLS - 1) {
      edit_buffer[line][col++] = buf[i];
    }
  }

  edit_lines = line + 1;
  if (edit_lines < 1)
    edit_lines = 1;

  return 0;
}

/*
 * Save file from editor
 */
static int edit_save(void) {
  char buf[4096];
  int pos = 0;

  for (int i = 0; i < edit_lines && pos < 4000; i++) {
    int len = strlen(edit_buffer[i]);
    memcpy(&buf[pos], edit_buffer[i], len);
    pos += len;
    buf[pos++] = '\n';
  }
  buf[pos] = '\0';

  fs_write(edit_filename, buf, pos);
  edit_modified = 0;

  return 0;
}

/*
 * Editor main loop
 */
void cmd_nedit(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: nedit <filename>\n");
    return;
  }

  /* Load file */
  edit_load(args);
  edit_redraw();

  /* Editor loop */
  while (1) {
    char c = keyboard_getchar();

    /* Ctrl+Q = Quit */
    if (c == 17) { /* Ctrl+Q */
      break;
    }

    /* Ctrl+S = Save */
    if (c == 19) { /* Ctrl+S */
      edit_save();
      edit_redraw();
      continue;
    }

    /* Handle special keys */
    if (c == '\n') {
      /* New line */
      if (edit_lines < EDIT_MAX_LINES - 1) {
        edit_cursor_line++;
        edit_cursor_col = 0;
        if (edit_cursor_line >= edit_lines) {
          edit_lines = edit_cursor_line + 1;
        }
        edit_modified = 1;
      }
    } else if (c == '\b') {
      /* Backspace */
      if (edit_cursor_col > 0) {
        edit_cursor_col--;
        /* Shift line left */
        char *line = edit_buffer[edit_cursor_line];
        int len = strlen(line);
        for (int i = edit_cursor_col; i < len; i++) {
          line[i] = line[i + 1];
        }
        edit_modified = 1;
      }
    } else if (c >= 32 && c < 127) {
      /* Regular character */
      if (edit_cursor_col < EDIT_MAX_COLS - 2) {
        edit_buffer[edit_cursor_line][edit_cursor_col++] = c;
        edit_buffer[edit_cursor_line][edit_cursor_col] = '\0';
        edit_modified = 1;
      }
    }

    edit_redraw();
  }

  /* Clear and return to shell */
  vga_clear();
  kprintf("Exited editor.\n");
}

/*
 * hexdump - show file as hex
 */
void cmd_hexdump(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: hexdump <filename>\n");
    return;
  }

  char buf[256];
  int len = fs_read(args, buf, sizeof(buf));

  if (len < 0) {
    kprintf_color("File not found\n", VGA_COLOR_RED);
    return;
  }

  kprintf("\n");
  for (int i = 0; i < len; i += 16) {
    kprintf("%04x: ", i);

    for (int j = 0; j < 16; j++) {
      if (i + j < len) {
        kprintf("%02x ", (uint8_t)buf[i + j]);
      } else {
        kprintf("   ");
      }
    }

    kprintf(" |");
    for (int j = 0; j < 16 && i + j < len; j++) {
      char c = buf[i + j];
      kprintf("%c", (c >= 32 && c < 127) ? c : '.');
    }
    kprintf("|\n");
  }
  kprintf("\n");
}
