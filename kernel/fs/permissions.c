/*
 * NanoSec OS - File Permissions
 * ===============================
 * chmod, chown commands
 */

#include "../kernel.h"

/* Permission bits */
#define PERM_R 0x04
#define PERM_W 0x02
#define PERM_X 0x01

/* Extended file structure would have these fields */
typedef struct {
  uint16_t owner_uid;
  uint16_t owner_gid;
  uint16_t mode; /* rwxrwxrwx */
} file_perms_t;

/* Simple file permission storage */
#define MAX_PERM_ENTRIES 32

typedef struct {
  char filename[32];
  uint16_t uid;
  uint16_t gid;
  uint16_t mode;
} perm_entry_t;

static perm_entry_t perms[MAX_PERM_ENTRIES];
static int perm_count = 0;

/*
 * Initialize default permissions
 */
void perms_init(void) {
  memset(perms, 0, sizeof(perms));
  perm_count = 0;

  /* Set default permissions for existing files */
  strcpy(perms[0].filename, "readme.txt");
  perms[0].uid = 0;
  perms[0].gid = 0;
  perms[0].mode = 0644; /* rw-r--r-- */

  strcpy(perms[1].filename, "system.log");
  perms[1].uid = 0;
  perms[1].gid = 0;
  perms[1].mode = 0600; /* rw------- */

  perm_count = 2;
}

/*
 * Find permission entry
 */
static perm_entry_t *find_perm(const char *filename) {
  for (int i = 0; i < perm_count; i++) {
    if (strcmp(perms[i].filename, filename) == 0) {
      return &perms[i];
    }
  }
  return NULL;
}

/*
 * Set file permission
 */
int set_file_mode(const char *filename, uint16_t mode) {
  perm_entry_t *p = find_perm(filename);

  if (p) {
    /* Check ownership */
    if (!user_is_root() && p->uid != user_get_uid()) {
      return -1; /* Permission denied */
    }
    p->mode = mode;
    return 0;
  }

  /* Create new entry */
  if (perm_count < MAX_PERM_ENTRIES) {
    strncpy(perms[perm_count].filename, filename, 31);
    perms[perm_count].uid = user_get_uid();
    perms[perm_count].gid = 0;
    perms[perm_count].mode = mode;
    perm_count++;
    return 0;
  }

  return -2; /* No space */
}

/*
 * Set file owner
 */
int set_file_owner(const char *filename, uint16_t uid, uint16_t gid) {
  if (!user_is_root()) {
    return -1; /* Only root can chown */
  }

  perm_entry_t *p = find_perm(filename);

  if (p) {
    p->uid = uid;
    p->gid = gid;
    return 0;
  }

  /* Create new entry */
  if (perm_count < MAX_PERM_ENTRIES) {
    strncpy(perms[perm_count].filename, filename, 31);
    perms[perm_count].uid = uid;
    perms[perm_count].gid = gid;
    perms[perm_count].mode = 0644;
    perm_count++;
    return 0;
  }

  return -2;
}

/*
 * Format mode as string (rwxrwxrwx)
 */
static void mode_to_str(uint16_t mode, char *buf) {
  buf[0] = (mode & 0400) ? 'r' : '-';
  buf[1] = (mode & 0200) ? 'w' : '-';
  buf[2] = (mode & 0100) ? 'x' : '-';
  buf[3] = (mode & 0040) ? 'r' : '-';
  buf[4] = (mode & 0020) ? 'w' : '-';
  buf[5] = (mode & 0010) ? 'x' : '-';
  buf[6] = (mode & 0004) ? 'r' : '-';
  buf[7] = (mode & 0002) ? 'w' : '-';
  buf[8] = (mode & 0001) ? 'x' : '-';
  buf[9] = '\0';
}

/*
 * Parse octal mode
 */
static uint16_t parse_mode(const char *str) {
  uint16_t mode = 0;
  while (*str >= '0' && *str <= '7') {
    mode = (mode << 3) | (*str - '0');
    str++;
  }
  return mode;
}

/* ============ Shell Commands ============ */

/*
 * chmod - change file mode
 * Usage: chmod 755 file
 */
void cmd_chmod(const char *args) {
  char mode_str[16], filename[64];
  int i = 0;

  const char *p = args;
  while (*p && *p != ' ' && i < 15)
    mode_str[i++] = *p++;
  mode_str[i] = '\0';

  while (*p == ' ')
    p++;
  i = 0;
  while (*p && *p != ' ' && i < 63)
    filename[i++] = *p++;
  filename[i] = '\0';

  if (mode_str[0] == '\0' || filename[0] == '\0') {
    kprintf("Usage: chmod <mode> <file>\n");
    kprintf("Example: chmod 755 myfile\n");
    return;
  }

  uint16_t mode = parse_mode(mode_str);

  if (set_file_mode(filename, mode) == 0) {
    char buf[10];
    mode_to_str(mode, buf);
    kprintf("Changed mode: %s -> %s\n", filename, buf);
  } else {
    kprintf_color("Permission denied\n", VGA_COLOR_RED);
  }
}

/*
 * chown - change file owner
 * Usage: chown user file or chown user:group file
 */
void cmd_chown(const char *args) {
  char owner[32], filename[64];
  int i = 0;

  const char *p = args;
  while (*p && *p != ' ' && i < 31)
    owner[i++] = *p++;
  owner[i] = '\0';

  while (*p == ' ')
    p++;
  i = 0;
  while (*p && *p != ' ' && i < 63)
    filename[i++] = *p++;
  filename[i] = '\0';

  if (owner[0] == '\0' || filename[0] == '\0') {
    kprintf("Usage: chown <user> <file>\n");
    return;
  }

  /* Parse user (just use UID 0 for root, 1000 for others) */
  uint16_t uid = 0;
  if (strcmp(owner, "root") == 0) {
    uid = 0;
  } else {
    uid = 1000;
  }

  if (set_file_owner(filename, uid, 0) == 0) {
    kprintf("Changed owner: %s -> %s\n", filename, owner);
  } else {
    kprintf_color("Permission denied (root only)\n", VGA_COLOR_RED);
  }
}

/*
 * ls -l style output
 */
void cmd_ls_long(const char *args) {
  (void)args;

  kprintf("\n");
  kprintf("Mode       Owner   Size  Name\n");
  kprintf("---------  -----  -----  ----\n");

  /* Show files with permissions */
  for (int i = 0; i < perm_count; i++) {
    char mode_buf[10];
    mode_to_str(perms[i].mode, mode_buf);

    kprintf("-%s  %5d  %5d  %s\n", mode_buf, perms[i].uid,
            0, /* Size would come from filesystem */
            perms[i].filename);
  }
  kprintf("\n");
}
