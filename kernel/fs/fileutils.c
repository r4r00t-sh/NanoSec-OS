/*
 * NanoSec OS - Extended File Commands
 * =====================================
 * cp, mv, head, tail, wc, grep, find
 */

#include "../kernel.h"

/* External filesystem functions */
extern int fs_write(const char *name, const char *data, size_t len);
extern int fs_read(const char *name, char *buf, size_t max);

/*
 * cp - copy file
 */
void cmd_cp(const char *args) {
  char src[64], dst[64];
  int i = 0;

  /* Parse source */
  const char *p = args;
  while (*p && *p != ' ' && i < 63)
    src[i++] = *p++;
  src[i] = '\0';

  /* Parse destination */
  while (*p == ' ')
    p++;
  i = 0;
  while (*p && *p != ' ' && i < 63)
    dst[i++] = *p++;
  dst[i] = '\0';

  if (src[0] == '\0' || dst[0] == '\0') {
    kprintf("Usage: cp <source> <dest>\n");
    return;
  }

  /* Read source */
  char buf[4096];
  int len = fs_read(src, buf, sizeof(buf));
  if (len < 0) {
    kprintf_color("Cannot read: ", VGA_COLOR_RED);
    kprintf("%s\n", src);
    return;
  }

  /* Write destination */
  if (fs_write(dst, buf, len) < 0) {
    kprintf_color("Cannot write: ", VGA_COLOR_RED);
    kprintf("%s\n", dst);
    return;
  }

  kprintf("Copied %s -> %s (%d bytes)\n", src, dst, len);
}

/*
 * mv - move/rename file
 */
void cmd_mv(const char *args) {
  /* For our simple FS, mv = cp + rm */
  char src[64], dst[64];
  int i = 0;

  const char *p = args;
  while (*p && *p != ' ' && i < 63)
    src[i++] = *p++;
  src[i] = '\0';

  while (*p == ' ')
    p++;
  i = 0;
  while (*p && *p != ' ' && i < 63)
    dst[i++] = *p++;
  dst[i] = '\0';

  if (src[0] == '\0' || dst[0] == '\0') {
    kprintf("Usage: mv <source> <dest>\n");
    return;
  }

  /* Read, write, then delete original */
  char buf[4096];
  int len = fs_read(src, buf, sizeof(buf));
  if (len < 0) {
    kprintf_color("Cannot read: ", VGA_COLOR_RED);
    kprintf("%s\n", src);
    return;
  }

  if (fs_write(dst, buf, len) < 0) {
    kprintf_color("Cannot write: ", VGA_COLOR_RED);
    kprintf("%s\n", dst);
    return;
  }

  /* Delete source by calling rm */
  cmd_rm(src);
  kprintf("Moved %s -> %s\n", src, dst);
}

/*
 * head - show first N lines
 */
void cmd_head(const char *args) {
  int n = 10;
  char filename[64];
  int i = 0;

  const char *p = args;

  /* Check for -n option */
  if (*p == '-' && *(p + 1) == 'n') {
    p += 2;
    while (*p == ' ')
      p++;
    n = 0;
    while (*p >= '0' && *p <= '9') {
      n = n * 10 + (*p - '0');
      p++;
    }
    while (*p == ' ')
      p++;
  }

  while (*p && *p != ' ' && i < 63)
    filename[i++] = *p++;
  filename[i] = '\0';

  if (filename[0] == '\0') {
    kprintf("Usage: head [-n N] <file>\n");
    return;
  }

  char buf[4096];
  int len = fs_read(filename, buf, sizeof(buf) - 1);
  if (len < 0) {
    kprintf_color("Cannot read: ", VGA_COLOR_RED);
    kprintf("%s\n", filename);
    return;
  }
  buf[len] = '\0';

  /* Print first N lines */
  int lines = 0;
  for (i = 0; i < len && lines < n; i++) {
    vga_putchar(buf[i]);
    if (buf[i] == '\n')
      lines++;
  }
  if (lines > 0 && buf[i - 1] != '\n')
    kprintf("\n");
}

/*
 * tail - show last N lines
 */
void cmd_tail(const char *args) {
  int n = 10;
  char filename[64];
  int i = 0;

  const char *p = args;

  if (*p == '-' && *(p + 1) == 'n') {
    p += 2;
    while (*p == ' ')
      p++;
    n = 0;
    while (*p >= '0' && *p <= '9') {
      n = n * 10 + (*p - '0');
      p++;
    }
    while (*p == ' ')
      p++;
  }

  while (*p && *p != ' ' && i < 63)
    filename[i++] = *p++;
  filename[i] = '\0';

  if (filename[0] == '\0') {
    kprintf("Usage: tail [-n N] <file>\n");
    return;
  }

  char buf[4096];
  int len = fs_read(filename, buf, sizeof(buf) - 1);
  if (len < 0) {
    kprintf_color("Cannot read: ", VGA_COLOR_RED);
    kprintf("%s\n", filename);
    return;
  }
  buf[len] = '\0';

  /* Count total lines */
  int total_lines = 0;
  for (i = 0; i < len; i++) {
    if (buf[i] == '\n')
      total_lines++;
  }

  /* Find start position */
  int skip_lines = total_lines - n;
  if (skip_lines < 0)
    skip_lines = 0;

  int lines = 0;
  int start = 0;
  for (i = 0; i < len && lines < skip_lines; i++) {
    if (buf[i] == '\n') {
      lines++;
      start = i + 1;
    }
  }

  /* Print remaining */
  for (i = start; i < len; i++) {
    vga_putchar(buf[i]);
  }
  if (len > 0 && buf[len - 1] != '\n')
    kprintf("\n");
}

/*
 * wc - word count
 */
void cmd_wc(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: wc <file>\n");
    return;
  }

  char buf[4096];
  int len = fs_read(args, buf, sizeof(buf) - 1);
  if (len < 0) {
    kprintf_color("Cannot read: ", VGA_COLOR_RED);
    kprintf("%s\n", args);
    return;
  }
  buf[len] = '\0';

  int lines = 0, words = 0, chars = len;
  int in_word = 0;

  for (int i = 0; i < len; i++) {
    if (buf[i] == '\n')
      lines++;

    if (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\t') {
      in_word = 0;
    } else if (!in_word) {
      in_word = 1;
      words++;
    }
  }

  kprintf("  %d  %d  %d %s\n", lines, words, chars, args);
}

/*
 * grep - search in file
 */
void cmd_grep(const char *args) {
  char pattern[64], filename[64];
  int i = 0;

  const char *p = args;
  while (*p && *p != ' ' && i < 63)
    pattern[i++] = *p++;
  pattern[i] = '\0';

  while (*p == ' ')
    p++;
  i = 0;
  while (*p && *p != ' ' && i < 63)
    filename[i++] = *p++;
  filename[i] = '\0';

  if (pattern[0] == '\0' || filename[0] == '\0') {
    kprintf("Usage: grep <pattern> <file>\n");
    return;
  }

  char buf[4096];
  int len = fs_read(filename, buf, sizeof(buf) - 1);
  if (len < 0) {
    kprintf_color("Cannot read: ", VGA_COLOR_RED);
    kprintf("%s\n", filename);
    return;
  }
  buf[len] = '\0';

  /* Search line by line */
  char line[256];
  int line_idx = 0;
  int line_num = 1;
  int matches = 0;

  for (i = 0; i <= len; i++) {
    if (buf[i] == '\n' || buf[i] == '\0') {
      line[line_idx] = '\0';

      /* Simple substring search */
      int found = 0;
      for (int j = 0; j <= line_idx - (int)strlen(pattern); j++) {
        int match = 1;
        for (int k = 0; pattern[k]; k++) {
          if (line[j + k] != pattern[k]) {
            match = 0;
            break;
          }
        }
        if (match) {
          found = 1;
          break;
        }
      }

      if (found) {
        kprintf_color("%d:", VGA_COLOR_YELLOW);
        kprintf("%s\n", line);
        matches++;
      }

      line_idx = 0;
      line_num++;
    } else if (line_idx < 255) {
      line[line_idx++] = buf[i];
    }
  }

  if (matches == 0) {
    kprintf("(no matches)\n");
  }
}
