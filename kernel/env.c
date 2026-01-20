/*
 * NanoSec OS - Environment Variables
 * ====================================
 */

#include "kernel.h"

#define MAX_ENV_VARS 32
#define MAX_VAR_NAME 32
#define MAX_VAR_VALUE 128

typedef struct {
  char name[MAX_VAR_NAME];
  char value[MAX_VAR_VALUE];
  int set;
} env_var_t;

static env_var_t env_vars[MAX_ENV_VARS];
static int env_initialized = 0;

/*
 * Initialize environment
 */
void env_init(void) {
  memset(env_vars, 0, sizeof(env_vars));

  /* Set default variables */
  env_set("HOME", "/root");
  env_set("PATH", "/bin");
  env_set("SHELL", "/bin/nash");
  env_set("USER", "root");
  env_set("HOSTNAME", "nanosec");
  env_set("PS1", "nanosec# ");

  env_initialized = 1;
}

/*
 * Set environment variable
 */
int env_set(const char *name, const char *value) {
  /* Check if exists */
  for (int i = 0; i < MAX_ENV_VARS; i++) {
    if (env_vars[i].set && strcmp(env_vars[i].name, name) == 0) {
      strncpy(env_vars[i].value, value, MAX_VAR_VALUE - 1);
      return 0;
    }
  }

  /* Find empty slot */
  for (int i = 0; i < MAX_ENV_VARS; i++) {
    if (!env_vars[i].set) {
      strncpy(env_vars[i].name, name, MAX_VAR_NAME - 1);
      strncpy(env_vars[i].value, value, MAX_VAR_VALUE - 1);
      env_vars[i].set = 1;
      return 0;
    }
  }

  return -1; /* No space */
}

/*
 * Get environment variable
 */
const char *env_get(const char *name) {
  for (int i = 0; i < MAX_ENV_VARS; i++) {
    if (env_vars[i].set && strcmp(env_vars[i].name, name) == 0) {
      return env_vars[i].value;
    }
  }
  return NULL;
}

/*
 * Unset environment variable
 */
int env_unset(const char *name) {
  for (int i = 0; i < MAX_ENV_VARS; i++) {
    if (env_vars[i].set && strcmp(env_vars[i].name, name) == 0) {
      env_vars[i].set = 0;
      return 0;
    }
  }
  return -1;
}

/* ============ Shell Commands ============ */

/*
 * export command - set/show env vars
 * Usage: export VAR=value or export
 */
void cmd_export(const char *args) {
  if (args[0] == '\0') {
    /* Show all */
    kprintf("\n");
    for (int i = 0; i < MAX_ENV_VARS; i++) {
      if (env_vars[i].set) {
        kprintf("%s=%s\n", env_vars[i].name, env_vars[i].value);
      }
    }
    kprintf("\n");
    return;
  }

  /* Parse VAR=value */
  char name[MAX_VAR_NAME], value[MAX_VAR_VALUE];
  int i = 0;
  const char *p = args;

  while (*p && *p != '=' && i < MAX_VAR_NAME - 1) {
    name[i++] = *p++;
  }
  name[i] = '\0';

  if (*p == '=') {
    p++;
    i = 0;
    while (*p && i < MAX_VAR_VALUE - 1) {
      value[i++] = *p++;
    }
    value[i] = '\0';

    env_set(name, value);
    kprintf("%s=%s\n", name, value);
  } else {
    /* Just show this variable */
    const char *val = env_get(name);
    if (val) {
      kprintf("%s=%s\n", name, val);
    } else {
      kprintf("%s: not set\n", name);
    }
  }
}

/*
 * env command - show all env vars
 */
void cmd_env(const char *args) {
  (void)args;
  cmd_export("");
}

/*
 * unset command
 */
void cmd_unset(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: unset <VAR>\n");
    return;
  }

  if (env_unset(args) == 0) {
    kprintf("Unset: %s\n", args);
  } else {
    kprintf("Not found: %s\n", args);
  }
}

/*
 * Expand variables in string ($VAR -> value)
 */
void env_expand(const char *src, char *dst, size_t max) {
  size_t di = 0;
  const char *p = src;

  while (*p && di < max - 1) {
    if (*p == '$') {
      p++;
      char varname[MAX_VAR_NAME];
      int vi = 0;

      while (*p &&
             ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
              (*p >= '0' && *p <= '9') || *p == '_') &&
             vi < MAX_VAR_NAME - 1) {
        varname[vi++] = *p++;
      }
      varname[vi] = '\0';

      const char *val = env_get(varname);
      if (val) {
        while (*val && di < max - 1) {
          dst[di++] = *val++;
        }
      }
    } else {
      dst[di++] = *p++;
    }
  }
  dst[di] = '\0';
}
