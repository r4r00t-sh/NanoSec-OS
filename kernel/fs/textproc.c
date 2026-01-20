/*
 * NanoSec OS - Priority 3: Text Processing Commands
 * ===================================================
 * tr, tee, xargs, sed
 */

#include "../kernel.h"

/* External functions */
extern int fs_read(const char *name, char *buf, size_t max);
extern int fs_write(const char *name, const char *data, size_t len);
extern void shell_execute_simple(const char *cmd);
extern int pipe_is_active(void);
extern void pipe_write_char(char c);

/*
 * tr - Translate characters
 * Usage: tr <from> <to>
 * Example: tr a-z A-Z    (uppercase)
 *          tr abc xyz    (a->x, b->y, c->z)
 */
void cmd_tr(const char *args) {
  char from[64] = "", to[64] = "";

  /* Parse arguments */
  const char *p = args;
  while (*p == ' ')
    p++;

  /* Get 'from' set */
  int i = 0;
  while (*p && *p != ' ' && i < 63)
    from[i++] = *p++;
  from[i] = '\0';

  while (*p == ' ')
    p++;

  /* Get 'to' set */
  i = 0;
  while (*p && *p != ' ' && i < 63)
    to[i++] = *p++;
  to[i] = '\0';

  if (from[0] == '\0' || to[0] == '\0') {
    kprintf("Usage: tr <from> <to>\n");
    kprintf("  e.g: echo hello | tr a-z A-Z\n");
    return;
  }

  /* Expand ranges like a-z */
  char from_exp[256], to_exp[256];
  int fi = 0, ti = 0;

  for (p = from; *p && fi < 255; p++) {
    if (*(p + 1) == '-' && *(p + 2)) {
      char start = *p, end = *(p + 2);
      for (char c = start; c <= end && fi < 255; c++) {
        from_exp[fi++] = c;
      }
      p += 2;
    } else {
      from_exp[fi++] = *p;
    }
  }
  from_exp[fi] = '\0';

  for (p = to; *p && ti < 255; p++) {
    if (*(p + 1) == '-' && *(p + 2)) {
      char start = *p, end = *(p + 2);
      for (char c = start; c <= end && ti < 255; c++) {
        to_exp[ti++] = c;
      }
      p += 2;
    } else {
      to_exp[ti++] = *p;
    }
  }
  to_exp[ti] = '\0';

  /* tr requires piped input - this shows usage */
  kprintf("tr: use with pipe, e.g: cat file | tr a-z A-Z\n");
}

/*
 * tr with piped input (called from shell_pipe.c)
 */
void tr_process(const char *input, const char *from, const char *to) {
  /* Build translation table */
  char trans[256];
  for (int i = 0; i < 256; i++)
    trans[i] = i;

  /* Expand ranges */
  char from_exp[256], to_exp[256];
  int fi = 0, ti = 0;
  const char *p;

  for (p = from; *p && fi < 255; p++) {
    if (*(p + 1) == '-' && *(p + 2)) {
      char start = *p, end = *(p + 2);
      for (char c = start; c <= end && fi < 255; c++)
        from_exp[fi++] = c;
      p += 2;
    } else {
      from_exp[fi++] = *p;
    }
  }
  from_exp[fi] = '\0';

  for (p = to; *p && ti < 255; p++) {
    if (*(p + 1) == '-' && *(p + 2)) {
      char start = *p, end = *(p + 2);
      for (char c = start; c <= end && ti < 255; c++)
        to_exp[ti++] = c;
      p += 2;
    } else {
      to_exp[ti++] = *p;
    }
  }
  to_exp[ti] = '\0';

  /* Build translation */
  int len = fi < ti ? fi : ti;
  for (int i = 0; i < len; i++) {
    trans[(unsigned char)from_exp[i]] = to_exp[i];
  }

  /* Translate and output */
  for (const char *c = input; *c; c++) {
    kprintf("%c", trans[(unsigned char)*c]);
  }
}

/*
 * tee - Read from stdin and write to both stdout and file
 * Usage: cmd | tee <file>
 */
void cmd_tee(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: cmd | tee <file>\n");
    return;
  }

  /* tee without pipe just shows usage */
  kprintf("tee: use with pipe, e.g: ls | tee output.txt\n");
}

/*
 * tee with piped input
 */
void tee_process(const char *input, const char *filename) {
  /* Write to stdout */
  kprintf("%s", input);

  /* Write to file */
  fs_write(filename, input, strlen(input));
}

/*
 * xargs - Build and execute command from stdin
 * Usage: cmd | xargs <command>
 */
void cmd_xargs(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: cmd | xargs <command>\n");
    kprintf("  e.g: find -name txt | xargs cat\n");
    return;
  }

  kprintf("xargs: use with pipe\n");
}

/*
 * xargs with piped input
 */
void xargs_process(const char *input, const char *cmd) {
  /* Split input into words/lines and execute cmd for each */
  char line[256];
  const char *p = input;

  while (*p) {
    /* Get a line/word */
    int i = 0;
    while (*p && *p != '\n' && *p != ' ' && i < 255) {
      line[i++] = *p++;
    }
    line[i] = '\0';

    /* Skip whitespace */
    while (*p == ' ' || *p == '\n')
      p++;

    if (line[0]) {
      /* Build command: cmd + arg */
      char full_cmd[512];
      int j = 0;
      const char *c = cmd;
      while (*c && j < 400)
        full_cmd[j++] = *c++;
      full_cmd[j++] = ' ';
      c = line;
      while (*c && j < 510)
        full_cmd[j++] = *c++;
      full_cmd[j] = '\0';

      /* Execute */
      shell_execute_simple(full_cmd);
    }
  }
}

/*
 * sed forward declaration
 */
void sed_process(const char *input, const char *pattern, const char *replace,
                 int global);

/*
 * sed - Stream editor (basic)
 * Usage: sed s/pattern/replacement/ [file]
 *        sed s/old/new/g            (global replace)
 */
void cmd_sed(const char *args) {
  const char *p = args;
  while (*p == ' ')
    p++;

  if (*p == '\0') {
    kprintf("Usage: sed s/pattern/replacement/ [file]\n");
    return;
  }

  /* Check for s command */
  if (*p != 's' || *(p + 1) != '/') {
    kprintf("sed: only s/pattern/replacement/ supported\n");
    return;
  }

  p += 2; /* Skip 's/' */

  /* Extract pattern */
  char pattern[64], replacement[64];
  int i = 0;
  while (*p && *p != '/' && i < 63)
    pattern[i++] = *p++;
  pattern[i] = '\0';

  if (*p != '/') {
    kprintf("sed: invalid syntax\n");
    return;
  }
  p++; /* Skip '/' */

  /* Extract replacement */
  i = 0;
  while (*p && *p != '/' && i < 63)
    replacement[i++] = *p++;
  replacement[i] = '\0';

  /* Check for flags and filename */
  int global = 0;
  char filename[64] = "";

  if (*p == '/') {
    p++;
    if (*p == 'g') {
      global = 1;
      p++;
    }
  }

  while (*p == ' ')
    p++;

  i = 0;
  while (*p && *p != ' ' && i < 63)
    filename[i++] = *p++;
  filename[i] = '\0';

  if (filename[0] == '\0') {
    kprintf("sed: use with file or pipe\n");
    kprintf("  e.g: sed s/old/new/g file.txt\n");
    kprintf("       cat file | sed s/old/new/\n");
    return;
  }

  /* Read file and process */
  char buffer[4096];
  int len = fs_read(filename, buffer, 4095);
  if (len < 0) {
    kprintf("sed: cannot read %s\n", filename);
    return;
  }
  buffer[len] = '\0';

  sed_process(buffer, pattern, replacement, global);
}

/*
 * sed processing
 */
void sed_process(const char *input, const char *pattern, const char *replace,
                 int global) {
  int pat_len = strlen(pattern);
  int rep_len = strlen(replace);

  const char *p = input;
  while (*p) {
    /* Check for pattern match */
    int match = 1;
    for (int i = 0; i < pat_len && p[i]; i++) {
      if (p[i] != pattern[i]) {
        match = 0;
        break;
      }
    }

    if (match && pat_len > 0) {
      /* Output replacement */
      kprintf("%s", replace);
      p += pat_len;
      if (!global) {
        /* Output rest unchanged */
        kprintf("%s", p);
        return;
      }
    } else {
      kprintf("%c", *p++);
    }
  }
}

/*
 * strlen implementation
 */
static int str_len(const char *s) {
  int len = 0;
  while (*s++)
    len++;
  return len;
}
