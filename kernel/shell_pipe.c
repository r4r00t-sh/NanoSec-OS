/*
 * NanoSec OS - Shell Pipeline & Redirection
 * ==========================================
 * Implements Unix-style pipes, redirects, and command chaining
 */

#include "kernel.h"

/* Output buffer for piping */
#define PIPE_BUF_SIZE 4096
static char pipe_buffer[PIPE_BUF_SIZE];
static int pipe_buffer_len = 0;
static int pipe_mode = 0; /* 0=normal, 1=capturing output */

/* Redirect mode */
static int redirect_mode = 0; /* 0=none, 1=write, 2=append */
static char redirect_file[64];

/* External functions */
extern void shell_execute_simple(const char *input);
extern int fs_write(const char *name, const char *data, size_t len);
extern int fs_read(const char *name, char *buf, size_t max);

/*
 * Simple atoi implementation
 */
static int pipe_atoi(const char *s) {
  int n = 0;
  int neg = 0;
  while (*s == ' ')
    s++;
  if (*s == '-') {
    neg = 1;
    s++;
  }
  while (*s >= '0' && *s <= '9') {
    n = n * 10 + (*s - '0');
    s++;
  }
  return neg ? -n : n;
}

/*
 * Simple strstr implementation
 */
static char *pipe_strstr(const char *haystack, const char *needle) {
  if (!*needle)
    return (char *)haystack;
  for (; *haystack; haystack++) {
    const char *h = haystack;
    const char *n = needle;
    while (*h && *n && *h == *n) {
      h++;
      n++;
    }
    if (!*n)
      return (char *)haystack;
  }
  return NULL;
}

/*
 * Capture output to pipe buffer instead of screen
 */
void pipe_capture_start(void) {
  pipe_mode = 1;
  pipe_buffer_len = 0;
  pipe_buffer[0] = '\0';
}

void pipe_capture_end(void) { pipe_mode = 0; }

void pipe_write_char(char c) {
  if (pipe_buffer_len < PIPE_BUF_SIZE - 1) {
    pipe_buffer[pipe_buffer_len++] = c;
    pipe_buffer[pipe_buffer_len] = '\0';
  }
}

const char *pipe_get_buffer(void) { return pipe_buffer; }

int pipe_is_active(void) { return pipe_mode; }

/*
 * Parse command line for pipes, redirects, and chaining
 * Returns: type of operation found
 *   0 = simple command
 *   1 = pipe (|)
 *   2 = output redirect (>)
 *   3 = append redirect (>>)
 *   4 = input redirect (<)
 *   5 = AND chain (&&)
 *   6 = OR chain (||)
 *   7 = semicolon chain (;)
 */
static int find_operator(const char *cmd, int *pos) {
  const char *p = cmd;
  int i = 0;

  while (*p) {
    /* Skip quoted strings */
    if (*p == '"') {
      p++;
      i++;
      while (*p && *p != '"') {
        p++;
        i++;
      }
      if (*p) {
        p++;
        i++;
      }
      continue;
    }
    if (*p == '\'') {
      p++;
      i++;
      while (*p && *p != '\'') {
        p++;
        i++;
      }
      if (*p) {
        p++;
        i++;
      }
      continue;
    }

    /* Check operators (order matters!) */
    if (p[0] == '|' && p[1] == '|') {
      *pos = i;
      return 6;
    } /* || */
    if (p[0] == '&' && p[1] == '&') {
      *pos = i;
      return 5;
    } /* && */
    if (p[0] == '>' && p[1] == '>') {
      *pos = i;
      return 3;
    } /* >> */
    if (p[0] == '|') {
      *pos = i;
      return 1;
    } /* | */
    if (p[0] == '>') {
      *pos = i;
      return 2;
    } /* > */
    if (p[0] == '<') {
      *pos = i;
      return 4;
    } /* < */
    if (p[0] == ';') {
      *pos = i;
      return 7;
    } /* ; */

    p++;
    i++;
  }

  return 0;
}

/*
 * Trim whitespace from both ends
 */
static void trim(char *str) {
  /* Leading */
  char *p = str;
  while (*p == ' ' || *p == '\t')
    p++;
  if (p != str) {
    char *dst = str;
    while (*p)
      *dst++ = *p++;
    *dst = '\0';
  }
  /* Trailing */
  int len = strlen(str);
  while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t')) {
    str[--len] = '\0';
  }
}

/*
 * Execute a simple command (no operators)
 */
static int execute_simple_cmd(const char *cmd, const char *input_data) {
  char command[256];
  strncpy(command, cmd, 255);
  command[255] = '\0';
  trim(command);

  if (command[0] == '\0')
    return 0;

  /* If we have input data, some commands can use it */
  /* For now, just execute the command normally */
  shell_execute_simple(command);

  return 1; /* Success */
}

/*
 * Execute command with piped input
 */
static int execute_with_pipe_input(const char *cmd, const char *input) {
  /* Some commands that can accept piped input */
  char command[64], args[256];

  /* Parse command */
  const char *p = cmd;
  while (*p == ' ')
    p++;
  int i = 0;
  while (*p && *p != ' ' && i < 63)
    command[i++] = *p++;
  command[i] = '\0';
  while (*p == ' ')
    p++;
  strncpy(args, p, 255);

  /* Handle commands that accept stdin */
  if (strcmp(command, "wc") == 0) {
    /* Count lines, words, chars in input */
    int lines = 0, words = 0, chars = 0;
    int in_word = 0;
    for (const char *c = input; *c; c++) {
      chars++;
      if (*c == '\n')
        lines++;
      if (*c == ' ' || *c == '\n' || *c == '\t') {
        in_word = 0;
      } else if (!in_word) {
        in_word = 1;
        words++;
      }
    }
    kprintf("%d %d %d\n", lines, words, chars);
    return 1;
  }

  if (strcmp(command, "cat") == 0 && args[0] == '\0') {
    /* cat with no args = echo stdin */
    kprintf("%s", input);
    return 1;
  }

  if (strcmp(command, "grep") == 0) {
    /* Simple grep on piped input */
    const char *pattern = args;
    const char *line_start = input;

    while (*line_start) {
      const char *line_end = line_start;
      while (*line_end && *line_end != '\n')
        line_end++;

      /* Check if pattern in line */
      char line[256];
      int len = line_end - line_start;
      if (len > 255)
        len = 255;
      memcpy(line, line_start, len);
      line[len] = '\0';

      /* Simple substring search */
      if (pattern[0] && pipe_strstr(line, pattern)) {
        kprintf("%s\n", line);
      }

      line_start = *line_end ? line_end + 1 : line_end;
    }
    return 1;
  }

  if (strcmp(command, "head") == 0) {
    int n = 10;
    if (args[0])
      n = pipe_atoi(args);
    if (n <= 0)
      n = 10;

    int lines = 0;
    const char *p = input;
    while (*p && lines < n) {
      if (*p == '\n')
        lines++;
      kprintf("%c", *p++);
    }
    return 1;
  }

  if (strcmp(command, "tail") == 0) {
    int n = 10;
    if (args[0])
      n = pipe_atoi(args);
    if (n <= 0)
      n = 10;

    /* Count total lines */
    int total = 0;
    for (const char *p = input; *p; p++) {
      if (*p == '\n')
        total++;
    }

    /* Skip to last n lines */
    int skip = total - n;
    if (skip < 0)
      skip = 0;

    int lines = 0;
    const char *p = input;
    while (*p && lines < skip) {
      if (*p++ == '\n')
        lines++;
    }
    kprintf("%s", p);
    return 1;
  }

  if (strcmp(command, "sort") == 0) {
    /* Simple line sort - collect lines then sort */
    char *lines[100];
    char buffer[4096];
    strncpy(buffer, input, 4095);

    int count = 0;
    char *tok = buffer;
    while (*tok && count < 100) {
      lines[count++] = tok;
      while (*tok && *tok != '\n')
        tok++;
      if (*tok)
        *tok++ = '\0';
    }

    /* Bubble sort */
    for (int i = 0; i < count - 1; i++) {
      for (int j = 0; j < count - i - 1; j++) {
        if (strcmp(lines[j], lines[j + 1]) > 0) {
          char *tmp = lines[j];
          lines[j] = lines[j + 1];
          lines[j + 1] = tmp;
        }
      }
    }

    for (int i = 0; i < count; i++) {
      kprintf("%s\n", lines[i]);
    }
    return 1;
  }

  if (strcmp(command, "uniq") == 0) {
    char prev[256] = "";
    const char *line_start = input;

    while (*line_start) {
      const char *line_end = line_start;
      while (*line_end && *line_end != '\n')
        line_end++;

      char line[256];
      int len = line_end - line_start;
      if (len > 255)
        len = 255;
      memcpy(line, line_start, len);
      line[len] = '\0';

      if (strcmp(line, prev) != 0) {
        kprintf("%s\n", line);
        strcpy(prev, line);
      }

      line_start = *line_end ? line_end + 1 : line_end;
    }
    return 1;
  }

  /* Default: just run the command normally */
  return execute_simple_cmd(cmd, NULL);
}

/*
 * Main shell execute with full operator support
 */
void shell_execute_advanced(const char *input) {
  char cmdline[512];
  strncpy(cmdline, input, 511);
  cmdline[511] = '\0';

  int pos = 0;
  int op = find_operator(cmdline, &pos);

  if (op == 0) {
    /* Simple command */
    shell_execute_simple(cmdline);
    return;
  }

  /* Split at operator */
  char left[256], right[256];
  memcpy(left, cmdline, pos);
  left[pos] = '\0';

  int skip = (op == 3 || op == 5 || op == 6) ? 2 : 1; /* >> && || are 2 chars */
  strcpy(right, cmdline + pos + skip);

  trim(left);
  trim(right);

  switch (op) {
  case 1: /* Pipe: cmd1 | cmd2 */
    pipe_capture_start();
    shell_execute_simple(left);
    pipe_capture_end();
    execute_with_pipe_input(right, pipe_get_buffer());
    break;

  case 2: /* Redirect: cmd > file */
    pipe_capture_start();
    shell_execute_simple(left);
    pipe_capture_end();
    fs_write(right, pipe_get_buffer(), pipe_buffer_len);
    break;

  case 3: /* Append: cmd >> file */
  {
    char existing[4096] = "";
    int existing_len = fs_read(right, existing, 4095);
    if (existing_len < 0)
      existing_len = 0;

    pipe_capture_start();
    shell_execute_simple(left);
    pipe_capture_end();

    /* Append new content */
    if (existing_len + pipe_buffer_len < 4095) {
      memcpy(existing + existing_len, pipe_buffer, pipe_buffer_len);
      existing_len += pipe_buffer_len;
    }
    fs_write(right, existing, existing_len);
  } break;

  case 4: /* Input: cmd < file */
  {
    char file_content[4096];
    int len = fs_read(right, file_content, 4095);
    if (len >= 0) {
      file_content[len] = '\0';
      execute_with_pipe_input(left, file_content);
    } else {
      kprintf("Cannot read: %s\n", right);
    }
  } break;

  case 5: /* AND: cmd1 && cmd2 */
    shell_execute_simple(left);
    /* TODO: check exit status */
    shell_execute_advanced(right);
    break;

  case 6: /* OR: cmd1 || cmd2 */
    shell_execute_simple(left);
    /* TODO: only run right if left fails */
    break;

  case 7: /* Semicolon: cmd1 ; cmd2 */
    shell_execute_simple(left);
    shell_execute_advanced(right);
    break;
  }
}
