/*
 * NanoSec OS - Command History and Aliases
 * ==========================================
 */

#include "kernel.h"

/* Command history */
#define HISTORY_SIZE 32
#define MAX_CMD_LEN 256

static char history[HISTORY_SIZE][MAX_CMD_LEN];
static int history_count = 0;
static int history_pos = 0;

/* Aliases */
#define MAX_ALIASES 16
#define MAX_ALIAS_NAME 16

typedef struct {
  char name[MAX_ALIAS_NAME];
  char command[MAX_CMD_LEN];
  int set;
} alias_t;

static alias_t aliases[MAX_ALIASES];

/*
 * Add command to history
 */
void history_add(const char *cmd) {
  if (cmd[0] == '\0')
    return;

  /* Don't add duplicates */
  if (history_count > 0) {
    int last = (history_count - 1) % HISTORY_SIZE;
    if (strcmp(history[last], cmd) == 0)
      return;
  }

  strncpy(history[history_count % HISTORY_SIZE], cmd, MAX_CMD_LEN - 1);
  history_count++;
  history_pos = history_count;
}

/*
 * Get previous command (up arrow)
 */
const char *history_prev(void) {
  if (history_pos > 0 && history_count > 0) {
    history_pos--;
    int idx = history_pos % HISTORY_SIZE;
    if (history_pos < history_count - HISTORY_SIZE) {
      history_pos = history_count - HISTORY_SIZE;
    }
    return history[idx];
  }
  return NULL;
}

/*
 * Get next command (down arrow)
 */
const char *history_next(void) {
  if (history_pos < history_count - 1) {
    history_pos++;
    return history[history_pos % HISTORY_SIZE];
  }
  history_pos = history_count;
  return "";
}

/*
 * Show command history
 */
void cmd_history(const char *args) {
  (void)args;

  int start = 0;
  if (history_count > HISTORY_SIZE) {
    start = history_count - HISTORY_SIZE;
  }

  kprintf("\n");
  for (int i = start; i < history_count; i++) {
    kprintf("  %3d  %s\n", i + 1, history[i % HISTORY_SIZE]);
  }
  kprintf("\n");
}

/* ============ Aliases ============ */

/*
 * Set alias
 */
int alias_set(const char *name, const char *command) {
  /* Check if exists */
  for (int i = 0; i < MAX_ALIASES; i++) {
    if (aliases[i].set && strcmp(aliases[i].name, name) == 0) {
      strncpy(aliases[i].command, command, MAX_CMD_LEN - 1);
      return 0;
    }
  }

  /* Find empty slot */
  for (int i = 0; i < MAX_ALIASES; i++) {
    if (!aliases[i].set) {
      strncpy(aliases[i].name, name, MAX_ALIAS_NAME - 1);
      strncpy(aliases[i].command, command, MAX_CMD_LEN - 1);
      aliases[i].set = 1;
      return 0;
    }
  }

  return -1;
}

/*
 * Get alias
 */
const char *alias_get(const char *name) {
  for (int i = 0; i < MAX_ALIASES; i++) {
    if (aliases[i].set && strcmp(aliases[i].name, name) == 0) {
      return aliases[i].command;
    }
  }
  return NULL;
}

/*
 * Unset alias
 */
int alias_unset(const char *name) {
  for (int i = 0; i < MAX_ALIASES; i++) {
    if (aliases[i].set && strcmp(aliases[i].name, name) == 0) {
      aliases[i].set = 0;
      return 0;
    }
  }
  return -1;
}

/*
 * alias command
 * Usage: alias name='command' or alias (show all)
 */
void cmd_alias(const char *args) {
  if (args[0] == '\0') {
    /* Show all */
    kprintf("\n");
    for (int i = 0; i < MAX_ALIASES; i++) {
      if (aliases[i].set) {
        kprintf("alias %s='%s'\n", aliases[i].name, aliases[i].command);
      }
    }
    kprintf("\n");
    return;
  }

  /* Parse name=command or name='command' */
  char name[MAX_ALIAS_NAME], command[MAX_CMD_LEN];
  int i = 0;
  const char *p = args;

  while (*p && *p != '=' && i < MAX_ALIAS_NAME - 1) {
    name[i++] = *p++;
  }
  name[i] = '\0';

  if (*p == '=') {
    p++;
    if (*p == '\'' || *p == '"')
      p++; /* Skip quote */

    i = 0;
    while (*p && *p != '\'' && *p != '"' && i < MAX_CMD_LEN - 1) {
      command[i++] = *p++;
    }
    command[i] = '\0';

    alias_set(name, command);
    kprintf("alias %s='%s'\n", name, command);
  } else {
    /* Show this alias */
    const char *cmd = alias_get(name);
    if (cmd) {
      kprintf("alias %s='%s'\n", name, cmd);
    } else {
      kprintf("alias: %s not found\n", name);
    }
  }
}

/*
 * unalias command
 */
void cmd_unalias(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: unalias <name>\n");
    return;
  }

  if (alias_unset(args) == 0) {
    kprintf("Removed alias: %s\n", args);
  } else {
    kprintf("Alias not found: %s\n", args);
  }
}

/*
 * Initialize default aliases
 */
void alias_init(void) {
  memset(aliases, 0, sizeof(aliases));

  alias_set("ll", "ls");
  alias_set("cls", "clear");
  alias_set("q", "halt");
  alias_set("?", "help");
}
