/*
 * NanoSec OS - Priority 2: Essential Unix Commands
 * ==================================================
 * find, stat, df, du, more, diff, ln, cut
 */

#include "../kernel.h"

/* External filesystem functions */
extern const char *fhs_getcwd(void);
extern int fs_read(const char *name, char *buf, size_t max);

/* Simple strstr implementation */
static char *utils_strstr(const char *haystack, const char *needle) {
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

/* Simple snprintf implementation */
static int utils_snprintf(char *str, size_t size, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  size_t i = 0;
  while (*fmt && i < size - 1) {
    if (*fmt == '%') {
      fmt++;
      if (*fmt == 's') {
        const char *s = va_arg(args, const char *);
        while (*s && i < size - 1)
          str[i++] = *s++;
      } else if (*fmt == 'd') {
        int n = va_arg(args, int);
        char num[16];
        int j = 0;
        if (n < 0) {
          str[i++] = '-';
          n = -n;
        }
        if (n == 0) {
          str[i++] = '0';
        } else {
          while (n > 0 && j < 15) {
            num[j++] = '0' + (n % 10);
            n /= 10;
          }
          while (j > 0 && i < size - 1)
            str[i++] = num[--j];
        }
      } else {
        str[i++] = *fmt;
      }
      fmt++;
    } else {
      str[i++] = *fmt++;
    }
  }
  str[i] = '\0';
  va_end(args);
  return i;
}

/* External from ramfs.c - we need access to nodes */
#define MAX_NODES 128
#define MAX_NAME 32
#define NODE_FREE 0
#define NODE_FILE 1
#define NODE_DIR 2

typedef struct {
  char name[MAX_NAME];
  uint8_t type;
  int parent;
  uint32_t size;
  uint8_t data[4096];
  uint32_t created;
  uint32_t modified;
} fs_node_ext_t;

/* Get node array from ramfs */
extern fs_node_ext_t *fs_get_nodes(void);

/*
 * find - Search for files
 * Usage: find [path] -name <pattern>
 */
static void find_recursive(int parent, const char *pattern,
                           const char *base_path) {
  fs_node_ext_t *nodes = fs_get_nodes();
  if (!nodes)
    return;

  for (int i = 0; i < MAX_NODES; i++) {
    if (nodes[i].type != NODE_FREE && nodes[i].parent == parent) {
      /* Build full path */
      char path[256];
      if (strcmp(base_path, "/") == 0) {
        utils_snprintf(path, 256, "/%s", nodes[i].name);
      } else {
        utils_snprintf(path, 256, "%s/%s", base_path, nodes[i].name);
      }

      /* Check if name matches pattern (simple substring) */
      if (pattern[0] == '\0' || utils_strstr(nodes[i].name, pattern)) {
        if (nodes[i].type == NODE_DIR) {
          kprintf_color(path, VGA_COLOR_CYAN);
          kprintf("/\n");
        } else {
          kprintf("%s\n", path);
        }
      }

      /* Recurse into directories */
      if (nodes[i].type == NODE_DIR) {
        find_recursive(i, pattern, path);
      }
    }
  }
}

void cmd_find(const char *args) {
  char path[64] = "/";
  char pattern[64] = "";

  /* Parse arguments */
  const char *p = args;
  while (*p == ' ')
    p++;

  /* Check for starting path */
  if (*p && *p != '-') {
    int i = 0;
    while (*p && *p != ' ' && i < 63)
      path[i++] = *p++;
    path[i] = '\0';
  }

  /* Look for -name */
  const char *name_arg = utils_strstr(args, "-name");
  if (name_arg) {
    name_arg += 5;
    while (*name_arg == ' ')
      name_arg++;
    int i = 0;
    while (*name_arg && *name_arg != ' ' && i < 63) {
      pattern[i++] = *name_arg++;
    }
    pattern[i] = '\0';
  }

  kprintf("\n");
  find_recursive(0, pattern, "");
  kprintf("\n");
}

/*
 * stat - Display file information
 */
void cmd_stat(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: stat <file>\n");
    return;
  }

  fs_node_ext_t *nodes = fs_get_nodes();
  if (!nodes) {
    kprintf("stat: filesystem error\n");
    return;
  }

  /* Find the file */
  for (int i = 0; i < MAX_NODES; i++) {
    if (nodes[i].type != NODE_FREE && strcmp(nodes[i].name, args) == 0) {
      kprintf("\n");
      kprintf("  File: %s\n", nodes[i].name);
      kprintf("  Size: %d bytes\n", nodes[i].size);
      kprintf("  Type: %s\n",
              nodes[i].type == NODE_DIR ? "directory" : "regular file");
      kprintf("  Inode: %d\n", i);
      kprintf("  Parent: %d\n", nodes[i].parent);
      kprintf("\n");
      return;
    }
  }

  kprintf("stat: '%s': No such file\n", args);
}

/*
 * df - Display filesystem usage
 */
void cmd_df(const char *args) {
  (void)args;

  fs_node_ext_t *nodes = fs_get_nodes();
  int used_nodes = 0;
  int total_size = 0;
  int dir_count = 0;
  int file_count = 0;

  if (nodes) {
    for (int i = 0; i < MAX_NODES; i++) {
      if (nodes[i].type != NODE_FREE) {
        used_nodes++;
        total_size += nodes[i].size;
        if (nodes[i].type == NODE_DIR)
          dir_count++;
        else
          file_count++;
      }
    }
  }

  kprintf("\n");
  kprintf("Filesystem      Size    Used    Avail   Use%%  Mounted on\n");
  kprintf("ramfs           512K    %dK      %dK      %d%%    /\n",
          total_size / 1024, (512 - total_size / 1024),
          (total_size * 100) / (512 * 1024));
  kprintf("\n");
  kprintf("Inodes: %d/%d used (%d dirs, %d files)\n", used_nodes, MAX_NODES,
          dir_count, file_count);
  kprintf("\n");
}

/*
 * du - Display directory size
 */
static int du_recursive(int parent) {
  fs_node_ext_t *nodes = fs_get_nodes();
  if (!nodes)
    return 0;

  int total = 0;
  for (int i = 0; i < MAX_NODES; i++) {
    if (nodes[i].type != NODE_FREE && nodes[i].parent == parent) {
      total += nodes[i].size;
      if (nodes[i].type == NODE_DIR) {
        total += du_recursive(i);
      }
    }
  }
  return total;
}

void cmd_du(const char *args) {
  (void)args;

  fs_node_ext_t *nodes = fs_get_nodes();
  if (!nodes) {
    kprintf("du: filesystem error\n");
    return;
  }

  kprintf("\n");

  /* Show size of each top-level directory */
  for (int i = 0; i < MAX_NODES; i++) {
    if (nodes[i].type == NODE_DIR && nodes[i].parent == 0) {
      int size = du_recursive(i);
      kprintf("%d\t/%s\n", size, nodes[i].name);
    }
  }

  int total = du_recursive(0);
  kprintf("%d\ttotal\n", total);
  kprintf("\n");
}

/*
 * more - Page through file
 */
void cmd_more(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: more <file>\n");
    return;
  }

  char buffer[4096];
  int len = fs_read(args, buffer, 4095);

  if (len < 0) {
    kprintf("more: cannot open '%s'\n", args);
    return;
  }

  buffer[len] = '\0';

  int lines = 0;
  const char *p = buffer;

  while (*p) {
    /* Print one line */
    while (*p && *p != '\n') {
      kprintf("%c", *p++);
    }
    if (*p == '\n') {
      kprintf("\n");
      p++;
      lines++;
    }

    /* Pause every 20 lines */
    if (lines >= 20 && *p) {
      kprintf("--More-- (Press any key)");
      keyboard_getchar();
      kprintf("\r                        \r");
      lines = 0;
    }
  }
}

/*
 * diff - Compare two files
 */
void cmd_diff(const char *args) {
  char file1[64], file2[64];

  /* Parse two filenames */
  const char *p = args;
  while (*p == ' ')
    p++;

  int i = 0;
  while (*p && *p != ' ' && i < 63)
    file1[i++] = *p++;
  file1[i] = '\0';

  while (*p == ' ')
    p++;

  i = 0;
  while (*p && *p != ' ' && i < 63)
    file2[i++] = *p++;
  file2[i] = '\0';

  if (file1[0] == '\0' || file2[0] == '\0') {
    kprintf("Usage: diff <file1> <file2>\n");
    return;
  }

  char buf1[4096], buf2[4096];
  int len1 = fs_read(file1, buf1, 4095);
  int len2 = fs_read(file2, buf2, 4095);

  if (len1 < 0) {
    kprintf("diff: %s: No such file\n", file1);
    return;
  }
  if (len2 < 0) {
    kprintf("diff: %s: No such file\n", file2);
    return;
  }

  buf1[len1] = '\0';
  buf2[len2] = '\0';

  if (strcmp(buf1, buf2) == 0) {
    kprintf("Files are identical\n");
    return;
  }

  /* Simple line-by-line diff */
  kprintf("\n");

  const char *l1 = buf1, *l2 = buf2;
  int line = 1;

  while (*l1 || *l2) {
    /* Get line from file1 */
    char line1[256] = "";
    i = 0;
    while (*l1 && *l1 != '\n' && i < 255)
      line1[i++] = *l1++;
    line1[i] = '\0';
    if (*l1 == '\n')
      l1++;

    /* Get line from file2 */
    char line2[256] = "";
    i = 0;
    while (*l2 && *l2 != '\n' && i < 255)
      line2[i++] = *l2++;
    line2[i] = '\0';
    if (*l2 == '\n')
      l2++;

    if (strcmp(line1, line2) != 0) {
      kprintf("%dc%d\n", line, line);
      kprintf_color("< ", VGA_COLOR_RED);
      kprintf("%s\n", line1);
      kprintf("---\n");
      kprintf_color("> ", VGA_COLOR_GREEN);
      kprintf("%s\n", line2);
    }

    line++;
    if (!*l1 && !*l2)
      break;
  }

  kprintf("\n");
}

/*
 * ln - Create symbolic link (simulated)
 */
void cmd_ln(const char *args) {
  kprintf("ln: symbolic links not supported in ramfs\n");
  kprintf("Hint: Use 'cp' to copy files instead\n");
  (void)args;
}

/*
 * cut - Extract columns from text
 * Usage: cut -d<delim> -f<field> [file]
 */
void cmd_cut(const char *args) {
  char delim = '\t';
  int field = 1;
  char filename[64] = "";

  const char *p = args;

  while (*p) {
    while (*p == ' ')
      p++;

    if (*p == '-' && *(p + 1) == 'd') {
      p += 2;
      if (*p)
        delim = *p++;
    } else if (*p == '-' && *(p + 1) == 'f') {
      p += 2;
      field = 0;
      while (*p >= '0' && *p <= '9') {
        field = field * 10 + (*p - '0');
        p++;
      }
    } else if (*p && *p != '-') {
      int i = 0;
      while (*p && *p != ' ' && i < 63)
        filename[i++] = *p++;
      filename[i] = '\0';
    } else {
      p++;
    }
  }

  if (field < 1)
    field = 1;

  char buffer[4096];
  int len = 0;

  if (filename[0]) {
    len = fs_read(filename, buffer, 4095);
    if (len < 0) {
      kprintf("cut: %s: No such file\n", filename);
      return;
    }
    buffer[len] = '\0';
  } else {
    kprintf("Usage: cut -d<delim> -f<field> <file>\n");
    return;
  }

  /* Process each line */
  const char *line = buffer;
  while (*line) {
    /* Find the requested field */
    int current_field = 1;
    const char *field_start = line;
    const char *field_end = line;

    while (*field_end && *field_end != '\n') {
      if (*field_end == delim) {
        if (current_field == field)
          break;
        current_field++;
        field_start = field_end + 1;
      }
      field_end++;
    }

    if (current_field == field) {
      /* Print the field */
      const char *end = field_end;
      if (*end == delim)
        end--;
      while (field_start <= end && *field_start != delim &&
             *field_start != '\n') {
        kprintf("%c", *field_start++);
      }
      kprintf("\n");
    }

    /* Move to next line */
    while (*line && *line != '\n')
      line++;
    if (*line == '\n')
      line++;
  }
}
