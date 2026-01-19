/*
 * NanoSec OS - RAM Filesystem
 * =============================
 * Simple in-memory filesystem for files
 */

#include "../kernel.h"

/* Filesystem limits */
#define MAX_FILES 32
#define MAX_FILENAME 32
#define MAX_FILESIZE 4096

/* File types */
#define FILE_TYPE_FREE 0
#define FILE_TYPE_FILE 1
#define FILE_TYPE_DIR 2

/* File entry */
typedef struct {
  char name[MAX_FILENAME];
  uint8_t type;
  uint32_t size;
  uint8_t data[MAX_FILESIZE];
  uint32_t created;
  uint32_t modified;
} file_entry_t;

/* Filesystem state */
static file_entry_t files[MAX_FILES];
static int fs_initialized = 0;
static char current_dir[64] = "/";

/*
 * Initialize filesystem
 */
int fs_init(void) {
  memset(files, 0, sizeof(files));

  /* Create root directory */
  strcpy(files[0].name, "/");
  files[0].type = FILE_TYPE_DIR;

  /* Create some default files */
  strcpy(files[1].name, "readme.txt");
  files[1].type = FILE_TYPE_FILE;
  strcpy((char *)files[1].data,
         "Welcome to NanoSec OS!\n"
         "======================\n\n"
         "This is a custom operating system built from scratch.\n"
         "It is NOT based on Linux - everything is custom!\n\n"
         "Type 'help' for available commands.\n");
  files[1].size = strlen((char *)files[1].data);

  strcpy(files[2].name, "system.log");
  files[2].type = FILE_TYPE_FILE;
  strcpy((char *)files[2].data, "[BOOT] NanoSec OS started\n");
  files[2].size = strlen((char *)files[2].data);

  fs_initialized = 1;
  return 0;
}

/*
 * Find file by name
 */
static file_entry_t *fs_find(const char *name) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].type != FILE_TYPE_FREE && strcmp(files[i].name, name) == 0) {
      return &files[i];
    }
  }
  return NULL;
}

/*
 * Find free slot
 */
static file_entry_t *fs_alloc(void) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].type == FILE_TYPE_FREE) {
      return &files[i];
    }
  }
  return NULL;
}

/*
 * List files - ls command
 */
void cmd_ls(const char *args) {
  (void)args;

  kprintf("\n");

  int count = 0;
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].type == FILE_TYPE_FREE)
      continue;
    if (files[i].type == FILE_TYPE_DIR && strcmp(files[i].name, "/") == 0)
      continue;

    if (files[i].type == FILE_TYPE_DIR) {
      kprintf_color(files[i].name, VGA_COLOR_CYAN);
      kprintf("/\n");
    } else {
      kprintf("%-20s %5d bytes\n", files[i].name, files[i].size);
    }
    count++;
  }

  if (count == 0) {
    kprintf("(empty)\n");
  }
  kprintf("\n");
}

/*
 * Cat file - show contents
 */
void cmd_cat(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: cat <filename>\n");
    return;
  }

  file_entry_t *f = fs_find(args);
  if (!f) {
    kprintf_color("File not found: ", VGA_COLOR_RED);
    kprintf("%s\n", args);
    return;
  }

  if (f->type == FILE_TYPE_DIR) {
    kprintf_color("Is a directory\n", VGA_COLOR_RED);
    return;
  }

  kprintf("\n%s", (char *)f->data);
  if (f->size > 0 && f->data[f->size - 1] != '\n') {
    kprintf("\n");
  }
  kprintf("\n");
}

/*
 * Touch - create empty file
 */
void cmd_touch(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: touch <filename>\n");
    return;
  }

  if (fs_find(args)) {
    /* File exists, update time */
    return;
  }

  file_entry_t *f = fs_alloc();
  if (!f) {
    kprintf_color("Filesystem full\n", VGA_COLOR_RED);
    return;
  }

  strncpy(f->name, args, MAX_FILENAME - 1);
  f->type = FILE_TYPE_FILE;
  f->size = 0;
  f->data[0] = '\0';
  f->created = timer_get_ticks();
  f->modified = f->created;

  kprintf("Created: %s\n", args);
}

/*
 * rm - remove file
 */
void cmd_rm(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: rm <filename>\n");
    return;
  }

  file_entry_t *f = fs_find(args);
  if (!f) {
    kprintf_color("File not found\n", VGA_COLOR_RED);
    return;
  }

  if (f->type == FILE_TYPE_DIR) {
    kprintf_color("Cannot remove directory\n", VGA_COLOR_RED);
    return;
  }

  memset(f, 0, sizeof(file_entry_t));
  kprintf("Removed: %s\n", args);

  /* Log to security */
  secmon_log("File deleted", 1);
}

/*
 * Write to file
 */
int fs_write(const char *name, const char *data, size_t len) {
  file_entry_t *f = fs_find(name);

  if (!f) {
    f = fs_alloc();
    if (!f)
      return -1;
    strncpy(f->name, name, MAX_FILENAME - 1);
    f->type = FILE_TYPE_FILE;
    f->created = timer_get_ticks();
  }

  if (len > MAX_FILESIZE)
    len = MAX_FILESIZE;
  memcpy(f->data, data, len);
  f->size = len;
  f->modified = timer_get_ticks();

  return 0;
}

/*
 * Read file
 */
int fs_read(const char *name, char *buf, size_t max) {
  file_entry_t *f = fs_find(name);
  if (!f || f->type == FILE_TYPE_DIR)
    return -1;

  size_t len = f->size;
  if (len > max)
    len = max;
  memcpy(buf, f->data, len);

  return len;
}

/*
 * pwd - print working directory
 */
void cmd_pwd(const char *args) {
  (void)args;
  kprintf("%s\n", current_dir);
}

/*
 * Echo with redirect - echo "text" > file
 */
void cmd_echo_file(const char *args) {
  /* Find > or >> */
  const char *p = args;
  const char *redirect = NULL;
  int append = 0;

  while (*p) {
    if (*p == '>') {
      redirect = p;
      if (*(p + 1) == '>') {
        append = 1;
      }
      break;
    }
    p++;
  }

  if (!redirect) {
    /* Just echo */
    kprintf("%s\n", args);
    return;
  }

  /* Get text (before >) */
  char text[256];
  int i = 0;
  p = args;
  while (p < redirect && i < 255) {
    text[i++] = *p++;
  }
  while (i > 0 && text[i - 1] == ' ')
    i--;
  text[i] = '\0';

  /* Get filename (after >) */
  p = redirect + 1;
  if (append)
    p++;
  while (*p == ' ')
    p++;

  char filename[64];
  i = 0;
  while (*p && *p != ' ' && i < 63) {
    filename[i++] = *p++;
  }
  filename[i] = '\0';

  if (filename[0] == '\0') {
    kprintf("Missing filename\n");
    return;
  }

  /* Write to file */
  file_entry_t *f = fs_find(filename);

  if (append && f) {
    /* Append */
    size_t len = strlen(text);
    if (f->size + len + 1 < MAX_FILESIZE) {
      strcpy((char *)&f->data[f->size], text);
      f->size += len;
      f->data[f->size++] = '\n';
      f->data[f->size] = '\0';
    }
  } else {
    /* Overwrite */
    fs_write(filename, text, strlen(text));
    f = fs_find(filename);
    if (f) {
      f->data[f->size++] = '\n';
      f->data[f->size] = '\0';
    }
  }

  kprintf("Wrote to %s\n", filename);
}
