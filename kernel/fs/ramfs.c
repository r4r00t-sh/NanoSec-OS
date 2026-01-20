/*
 * NanoSec OS - Hierarchical RAM Filesystem
 * ==========================================
 * Tree-based filesystem with proper directory support
 */

#include "../kernel.h"

/* Filesystem limits */
#define MAX_NODES 128
#define MAX_NAME 32
#define MAX_DATA 4096
#define MAX_PATH 256

/* Node types */
#define NODE_FREE 0
#define NODE_FILE 1
#define NODE_DIR 2

/* Filesystem node */
typedef struct fs_node {
  char name[MAX_NAME];
  uint8_t type;
  int parent; /* Index of parent node (-1 for root) */
  uint32_t size;
  uint8_t data[MAX_DATA];
  uint32_t created;
  uint32_t modified;
} fs_node_t;

/* Filesystem state */
static fs_node_t nodes[MAX_NODES];
static int current_dir = 0; /* Index of current directory (0 = root) */

/* Forward declarations */
static fs_node_t *find_in_dir(int parent, const char *name);
static int find_node_index(int parent, const char *name);
static int alloc_node(void);

/* Export nodes for utils.c */
fs_node_t *fs_get_nodes(void) { return nodes; }

/*
 * Initialize filesystem with FHS structure
 */
int fs_init(void) {
  memset(nodes, 0, sizeof(nodes));

  /* Create root directory */
  nodes[0].type = NODE_DIR;
  strcpy(nodes[0].name, "/");
  nodes[0].parent = -1;

  /* Create FHS directories */
  const char *fhs_dirs[] = {"bin", "sbin", "etc", "var",  "tmp", "home", "root",
                            "usr", "lib",  "dev", "proc", "mnt", "opt",  NULL};

  for (int i = 0; fhs_dirs[i]; i++) {
    int idx = alloc_node();
    if (idx > 0) {
      nodes[idx].type = NODE_DIR;
      strncpy(nodes[idx].name, fhs_dirs[i], MAX_NAME - 1);
      nodes[idx].parent = 0; /* Parent is root */
    }
  }

  /* Create /var/log */
  int var_idx = find_node_index(0, "var");
  if (var_idx > 0) {
    int log_idx = alloc_node();
    if (log_idx > 0) {
      nodes[log_idx].type = NODE_DIR;
      strcpy(nodes[log_idx].name, "log");
      nodes[log_idx].parent = var_idx;
    }
  }

  /* Create /home/guest */
  int home_idx = find_node_index(0, "home");
  if (home_idx > 0) {
    int guest_idx = alloc_node();
    if (guest_idx > 0) {
      nodes[guest_idx].type = NODE_DIR;
      strcpy(nodes[guest_idx].name, "guest");
      nodes[guest_idx].parent = home_idx;
    }
  }

  /* Create readme.txt in root */
  int readme_idx = alloc_node();
  if (readme_idx > 0) {
    nodes[readme_idx].type = NODE_FILE;
    strcpy(nodes[readme_idx].name, "readme.txt");
    nodes[readme_idx].parent = 0;
    strcpy((char *)nodes[readme_idx].data,
           "Welcome to NanoSec OS!\n"
           "======================\n\n"
           "This is a custom operating system.\n"
           "Type 'help' for available commands.\n");
    nodes[readme_idx].size = strlen((char *)nodes[readme_idx].data);
  }

  /* Create command binaries in /bin */
  int bin_idx = find_node_index(0, "bin");
  if (bin_idx > 0) {
    const char *bin_cmds[] = {
        "ls",   "cat",     "cd",    "pwd",  "mkdir",  "touch", "rm",   "cp",
        "mv",   "echo",    "clear", "help", "man",    "head",  "tail", "wc",
        "grep", "history", "alias", "env",  "export", NULL};
    for (int i = 0; bin_cmds[i]; i++) {
      int idx = alloc_node();
      if (idx > 0) {
        nodes[idx].type = NODE_FILE;
        strncpy(nodes[idx].name, bin_cmds[i], MAX_NAME - 1);
        nodes[idx].parent = bin_idx;
        strcpy((char *)nodes[idx].data, "#!/bin/sh\n# NanoSec builtin\n");
        nodes[idx].size = strlen((char *)nodes[idx].data);
      }
    }
  }

  /* Create system commands in /sbin */
  int sbin_idx = find_node_index(0, "sbin");
  if (sbin_idx > 0) {
    const char *sbin_cmds[] = {"reboot",   "shutdown", "halt",     "init",
                               "mount",    "umount",   "ifconfig", "route",
                               "iptables", "modprobe", NULL};
    for (int i = 0; sbin_cmds[i]; i++) {
      int idx = alloc_node();
      if (idx > 0) {
        nodes[idx].type = NODE_FILE;
        strncpy(nodes[idx].name, sbin_cmds[i], MAX_NAME - 1);
        nodes[idx].parent = sbin_idx;
        strcpy((char *)nodes[idx].data, "#!/bin/sh\n# NanoSec system cmd\n");
        nodes[idx].size = strlen((char *)nodes[idx].data);
      }
    }
  }

  /* Create /etc config files */
  int etc_idx = find_node_index(0, "etc");
  if (etc_idx > 0) {
    /* hostname */
    int idx = alloc_node();
    if (idx > 0) {
      nodes[idx].type = NODE_FILE;
      strcpy(nodes[idx].name, "hostname");
      nodes[idx].parent = etc_idx;
      strcpy((char *)nodes[idx].data, "nanosec\n");
      nodes[idx].size = strlen((char *)nodes[idx].data);
    }
    /* passwd */
    idx = alloc_node();
    if (idx > 0) {
      nodes[idx].type = NODE_FILE;
      strcpy(nodes[idx].name, "passwd");
      nodes[idx].parent = etc_idx;
      strcpy((char *)nodes[idx].data,
             "root:x:0:0:root:/root:/bin/sh\nguest:x:1000:1000:Guest:/home/"
             "guest:/bin/sh\n");
      nodes[idx].size = strlen((char *)nodes[idx].data);
    }
    /* motd */
    idx = alloc_node();
    if (idx > 0) {
      nodes[idx].type = NODE_FILE;
      strcpy(nodes[idx].name, "motd");
      nodes[idx].parent = etc_idx;
      strcpy((char *)nodes[idx].data, "Welcome to NanoSec OS!\n");
      nodes[idx].size = strlen((char *)nodes[idx].data);
    }
  }

  current_dir = 0;
  return 0;
}

/*
 * Allocate a new node
 */
static int alloc_node(void) {
  for (int i = 1; i < MAX_NODES; i++) {
    if (nodes[i].type == NODE_FREE) {
      memset(&nodes[i], 0, sizeof(fs_node_t));
      nodes[i].created = timer_get_ticks();
      nodes[i].modified = nodes[i].created;
      return i;
    }
  }
  return -1;
}

/*
 * Find node by name in a parent directory
 */
static int find_node_index(int parent, const char *name) {
  for (int i = 0; i < MAX_NODES; i++) {
    if (nodes[i].type != NODE_FREE && nodes[i].parent == parent &&
        strcmp(nodes[i].name, name) == 0) {
      return i;
    }
  }
  return -1;
}

static fs_node_t *find_in_dir(int parent, const char *name) {
  int idx = find_node_index(parent, name);
  return (idx >= 0) ? &nodes[idx] : NULL;
}

/*
 * Resolve path to node index
 * Returns node index or -1 if not found
 */
static int resolve_path(const char *path) {
  if (path[0] == '\0' || strcmp(path, "/") == 0) {
    return 0; /* Root */
  }

  int dir = (path[0] == '/') ? 0 : current_dir;
  const char *p = (path[0] == '/') ? path + 1 : path;

  char component[MAX_NAME];

  while (*p) {
    /* Extract path component */
    int i = 0;
    while (*p && *p != '/' && i < MAX_NAME - 1) {
      component[i++] = *p++;
    }
    component[i] = '\0';

    if (*p == '/')
      p++;

    if (component[0] == '\0')
      continue;

    /* Handle . and .. */
    if (strcmp(component, ".") == 0) {
      continue;
    } else if (strcmp(component, "..") == 0) {
      if (nodes[dir].parent >= 0) {
        dir = nodes[dir].parent;
      }
      continue;
    }

    /* Find component in current dir */
    int idx = find_node_index(dir, component);
    if (idx < 0) {
      return -1; /* Not found */
    }

    dir = idx;
  }

  return dir;
}

/*
 * Get full path of a node
 */
static void get_full_path(int idx, char *path, size_t size) {
  if (idx == 0) {
    strcpy(path, "/");
    return;
  }

  /* Build path backwards */
  char temp[MAX_PATH];
  temp[0] = '\0';

  while (idx > 0) {
    char part[MAX_NAME + 2];
    strcpy(part, "/");
    strcat(part, nodes[idx].name);

    char old[MAX_PATH];
    strcpy(old, temp);
    strcpy(temp, part);
    strcat(temp, old);

    idx = nodes[idx].parent;
  }

  if (temp[0] == '\0')
    strcpy(temp, "/");
  strncpy(path, temp, size - 1);
  path[size - 1] = '\0';
}

/*
 * Get current working directory path
 */
const char *fhs_getcwd(void) {
  static char cwd_path[MAX_PATH];
  get_full_path(current_dir, cwd_path, MAX_PATH);
  return cwd_path;
}

/*
 * Create directory
 */
int fs_mkdir(const char *name) {
  /* Check if already exists */
  if (find_in_dir(current_dir, name)) {
    return -1;
  }

  int idx = alloc_node();
  if (idx < 0)
    return -1;

  nodes[idx].type = NODE_DIR;
  strncpy(nodes[idx].name, name, MAX_NAME - 1);
  nodes[idx].parent = current_dir;

  return 0;
}

/*
 * Check if path is a directory
 */
int fs_isdir(const char *name) {
  int idx = resolve_path(name);
  return (idx >= 0 && nodes[idx].type == NODE_DIR);
}

/*
 * Change directory
 */
int fhs_chdir(const char *path) {
  if (strcmp(path, "/") == 0) {
    current_dir = 0;
    return 0;
  }

  int idx = resolve_path(path);
  if (idx < 0 || nodes[idx].type != NODE_DIR) {
    return -1;
  }

  current_dir = idx;
  return 0;
}

/*
 * ls - List directory contents
 */
void cmd_ls(const char *args) {
  int dir = current_dir;

  if (args[0] != '\0') {
    dir = resolve_path(args);
    if (dir < 0) {
      kprintf("ls: %s: No such directory\n", args);
      return;
    }
  }

  kprintf("\n");

  int count = 0;
  for (int i = 0; i < MAX_NODES; i++) {
    if (nodes[i].type != NODE_FREE && nodes[i].parent == dir) {
      if (nodes[i].type == NODE_DIR) {
        kprintf_color(nodes[i].name, VGA_COLOR_CYAN);
        kprintf("/\n");
      } else {
        /* Simple format without width specifier */
        kprintf("%s", nodes[i].name);
        /* Pad to 20 chars */
        int len = strlen(nodes[i].name);
        for (int p = len; p < 20; p++)
          kprintf(" ");
        kprintf("%d bytes\n", nodes[i].size);
      }
      count++;
    }
  }

  if (count == 0) {
    kprintf("(empty)\n");
  }
  kprintf("\n");
}

/*
 * cat - Show file contents
 */
void cmd_cat(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: cat <filename>\n");
    return;
  }

  int idx = resolve_path(args);
  if (idx < 0) {
    kprintf_color("File not found: ", VGA_COLOR_RED);
    kprintf("%s\n", args);
    return;
  }

  if (nodes[idx].type == NODE_DIR) {
    kprintf_color("Is a directory\n", VGA_COLOR_RED);
    return;
  }

  kprintf("\n%s", (char *)nodes[idx].data);
  if (nodes[idx].size > 0 && nodes[idx].data[nodes[idx].size - 1] != '\n') {
    kprintf("\n");
  }
  kprintf("\n");
}

/*
 * touch - Create empty file
 */
void cmd_touch(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: touch <filename>\n");
    return;
  }

  if (find_in_dir(current_dir, args)) {
    return; /* Already exists */
  }

  int idx = alloc_node();
  if (idx < 0) {
    kprintf_color("Filesystem full\n", VGA_COLOR_RED);
    return;
  }

  nodes[idx].type = NODE_FILE;
  strncpy(nodes[idx].name, args, MAX_NAME - 1);
  nodes[idx].parent = current_dir;
  nodes[idx].size = 0;

  kprintf("Created: %s\n", args);
}

/*
 * rm - Remove file or directory
 */
void cmd_rm(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: rm [-rf] <file>\n");
    return;
  }

  const char *target = args;
  int recursive = 0;

  if (args[0] == '-') {
    if (strncmp(args, "-rf", 3) == 0 || strncmp(args, "-r", 2) == 0) {
      recursive = 1;
    }
    while (*target && *target != ' ')
      target++;
    while (*target == ' ')
      target++;
  }

  int idx = resolve_path(target);
  if (idx < 0) {
    kprintf("rm: %s: No such file\n", target);
    return;
  }

  if (idx == 0) {
    kprintf("rm: cannot remove root\n");
    return;
  }

  if (nodes[idx].type == NODE_DIR && !recursive) {
    kprintf("rm: %s: Is a directory (use -rf)\n", target);
    return;
  }

  /* Remove node and children */
  if (nodes[idx].type == NODE_DIR) {
    /* Remove children first */
    for (int i = 0; i < MAX_NODES; i++) {
      if (nodes[i].parent == idx) {
        nodes[i].type = NODE_FREE;
      }
    }
  }

  nodes[idx].type = NODE_FREE;
  kprintf("Removed: %s\n", target);
}

/*
 * pwd - Print working directory
 */
void cmd_pwd(const char *args) {
  (void)args;
  kprintf("%s\n", fhs_getcwd());
}

/*
 * fs_write - Write to file
 */
int fs_write(const char *name, const char *data, size_t len) {
  int idx = resolve_path(name);

  if (idx < 0) {
    /* Create new file */
    idx = alloc_node();
    if (idx < 0)
      return -1;

    nodes[idx].type = NODE_FILE;
    strncpy(nodes[idx].name, name, MAX_NAME - 1);
    nodes[idx].parent = current_dir;
  }

  if (len > MAX_DATA)
    len = MAX_DATA;
  memcpy(nodes[idx].data, data, len);
  nodes[idx].size = len;
  nodes[idx].modified = timer_get_ticks();

  return 0;
}

/*
 * fs_read - Read file
 */
int fs_read(const char *name, char *buf, size_t max) {
  int idx = resolve_path(name);
  if (idx < 0 || nodes[idx].type == NODE_DIR) {
    return -1;
  }

  size_t len = nodes[idx].size;
  if (len > max)
    len = max;
  memcpy(buf, nodes[idx].data, len);

  return len;
}

/*
 * Echo with redirect
 */
void cmd_echo_file(const char *args) {
  const char *p = args;
  const char *redirect = NULL;
  int append = 0;

  while (*p) {
    if (*p == '>') {
      redirect = p;
      if (*(p + 1) == '>')
        append = 1;
      break;
    }
    p++;
  }

  if (!redirect) {
    kprintf("%s\n", args);
    return;
  }

  /* Get text */
  char text[256];
  int i = 0;
  p = args;
  while (p < redirect && i < 255) {
    text[i++] = *p++;
  }
  while (i > 0 && text[i - 1] == ' ')
    i--;
  text[i] = '\0';

  /* Get filename */
  p = redirect + 1 + append;
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

  /* Append newline */
  strcat(text, "\n");

  fs_write(filename, text, strlen(text));
  kprintf("Wrote to %s\n", filename);
}
