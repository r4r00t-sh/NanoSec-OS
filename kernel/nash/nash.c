/*
 * NanoSec OS - Nash Scripting Language v2
 * =========================================
 * Custom shell scripting language with UNIQUE syntax
 * File extension: .nsh
 *
 * Nash Unique Syntax:
 * ===================
 *
 * VARIABLES:
 *   @name = "value"           (not $VAR=value)
 *   print @name               (access variable)
 *
 * CONDITIONALS:
 *   when <condition> do       (not if/then)
 *     ...
 *   otherwise                 (not else)
 *     ...
 *   end                       (not fi)
 *
 * LOOPS:
 *   repeat <n> times          (count loop)
 *     ...
 *   end
 *
 *   each <item> in <list> do  (for-each)
 *     ...
 *   end
 *
 * FUNCTIONS:
 *   define <name> as          (not function)
 *     ...
 *   end
 *
 * OUTPUT:
 *   print "text"              (not echo)
 *   show @var                 (display variable)
 *
 * COMMENTS:
 *   -- comment                (not #)
 *   :: comment                (alternative)
 *
 * OPERATORS:
 *   @a eq @b                  (equals)
 *   @a ne @b                  (not equals)
 *   @a gt @b                  (greater than)
 *   @a lt @b                  (less than)
 *   and, or, not              (logical)
 */

#include "../kernel.h"

/* External functions */
extern void shell_execute_simple(const char *cmd);
extern int fs_read(const char *name, char *buf, size_t max);

/* Nash state */
#define MAX_VARS 32
#define MAX_VAR_NAME 32
#define MAX_VAR_VALUE 256
#define MAX_SCRIPT_SIZE 8192

typedef struct {
  char name[MAX_VAR_NAME];
  char value[MAX_VAR_VALUE];
} nash_var_t;

static nash_var_t nash_vars[MAX_VARS];
static int nash_last_result = 0;

/*
 * Set a Nash variable
 */
static void nash_set_var(const char *name, const char *value) {
  for (int i = 0; i < MAX_VARS; i++) {
    if (nash_vars[i].name[0] && strcmp(nash_vars[i].name, name) == 0) {
      strncpy(nash_vars[i].value, value, MAX_VAR_VALUE - 1);
      return;
    }
  }
  for (int i = 0; i < MAX_VARS; i++) {
    if (nash_vars[i].name[0] == '\0') {
      strncpy(nash_vars[i].name, name, MAX_VAR_NAME - 1);
      strncpy(nash_vars[i].value, value, MAX_VAR_VALUE - 1);
      return;
    }
  }
}

static const char *nash_get_var(const char *name) {
  for (int i = 0; i < MAX_VARS; i++) {
    if (nash_vars[i].name[0] && strcmp(nash_vars[i].name, name) == 0) {
      return nash_vars[i].value;
    }
  }
  return "";
}

/*
 * Expand @variables in string
 */
static void nash_expand(const char *input, char *output, size_t size) {
  const char *p = input;
  size_t i = 0;

  while (*p && i < size - 1) {
    if (*p == '@') {
      p++;
      char varname[MAX_VAR_NAME];
      int j = 0;
      while (*p &&
             ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
              (*p >= '0' && *p <= '9') || *p == '_') &&
             j < MAX_VAR_NAME - 1) {
        varname[j++] = *p++;
      }
      varname[j] = '\0';

      const char *val = nash_get_var(varname);
      while (*val && i < size - 1)
        output[i++] = *val++;
    } else {
      output[i++] = *p++;
    }
  }
  output[i] = '\0';
}

/*
 * Skip whitespace
 */
static const char *skip_ws(const char *p) {
  while (*p == ' ' || *p == '\t')
    p++;
  return p;
}

/*
 * Check if line starts with keyword
 */
static int starts_with(const char *line, const char *keyword) {
  while (*keyword) {
    if (*line++ != *keyword++)
      return 0;
  }
  return (*line == '\0' || *line == ' ' || *line == '\t');
}

/*
 * Parse and execute Nash script
 */
static void nash_execute_script(const char *script) {
  char line[512];
  const char *p = script;
  int in_when = 0, skip_block = 0;
  int repeat_count = 0;
  const char *repeat_start = NULL;

  while (*p) {
    /* Skip leading whitespace */
    while (*p == ' ' || *p == '\t')
      p++;

    /* Get a line */
    int i = 0;
    while (*p && *p != '\n' && i < 511) {
      line[i++] = *p++;
    }
    line[i] = '\0';
    if (*p == '\n')
      p++;

    /* Skip empty lines and comments (-- or ::) */
    const char *l = skip_ws(line);
    if (*l == '\0')
      continue;
    if (l[0] == '-' && l[1] == '-')
      continue;
    if (l[0] == ':' && l[1] == ':')
      continue;

    /* Handle: end */
    if (starts_with(l, "end")) {
      if (repeat_count > 0 && repeat_start) {
        repeat_count--;
        if (repeat_count > 0) {
          p = repeat_start;
        } else {
          repeat_start = NULL;
        }
      }
      in_when = 0;
      skip_block = 0;
      continue;
    }

    /* Handle: otherwise */
    if (starts_with(l, "otherwise")) {
      if (in_when)
        skip_block = !skip_block;
      continue;
    }

    if (skip_block)
      continue;

    /* Handle: @var = "value" or @var = value */
    if (*l == '@') {
      const char *v = l + 1;
      char varname[MAX_VAR_NAME];
      int j = 0;
      while (*v && *v != ' ' && *v != '=' && j < MAX_VAR_NAME - 1) {
        varname[j++] = *v++;
      }
      varname[j] = '\0';

      v = skip_ws(v);
      if (*v == '=') {
        v++;
        v = skip_ws(v);

        char value[MAX_VAR_VALUE];
        j = 0;
        if (*v == '"') {
          v++;
          while (*v && *v != '"' && j < MAX_VAR_VALUE - 1) {
            value[j++] = *v++;
          }
        } else {
          while (*v && *v != ' ' && *v != '\n' && j < MAX_VAR_VALUE - 1) {
            value[j++] = *v++;
          }
        }
        value[j] = '\0';

        char expanded[MAX_VAR_VALUE];
        nash_expand(value, expanded, MAX_VAR_VALUE);
        nash_set_var(varname, expanded);
      }
      continue;
    }

    /* Handle: print "text" or print @var */
    if (starts_with(l, "print")) {
      const char *msg = skip_ws(l + 5);
      char expanded[512];

      if (*msg == '"') {
        msg++;
        char text[256];
        int j = 0;
        while (*msg && *msg != '"' && j < 255) {
          text[j++] = *msg++;
        }
        text[j] = '\0';
        nash_expand(text, expanded, 512);
      } else {
        nash_expand(msg, expanded, 512);
      }

      kprintf("%s\n", expanded);
      continue;
    }

    /* Handle: show @var */
    if (starts_with(l, "show")) {
      const char *var = skip_ws(l + 4);
      if (*var == '@') {
        var++;
        char varname[MAX_VAR_NAME];
        int j = 0;
        while (*var && *var != ' ' && j < MAX_VAR_NAME - 1) {
          varname[j++] = *var++;
        }
        varname[j] = '\0';
        kprintf("%s = %s\n", varname, nash_get_var(varname));
      }
      continue;
    }

    /* Handle: when <cond> do */
    if (starts_with(l, "when")) {
      in_when = 1;
      const char *cond = skip_ws(l + 4);

      /* Parse condition: @a eq @b, @a ne @b, etc */
      char left[256], right[256], op[8];
      int j = 0;

      /* Get left operand */
      while (*cond && *cond != ' ' && j < 255)
        left[j++] = *cond++;
      left[j] = '\0';

      cond = skip_ws(cond);

      /* Get operator */
      j = 0;
      while (*cond && *cond != ' ' && j < 7)
        op[j++] = *cond++;
      op[j] = '\0';

      cond = skip_ws(cond);

      /* Get right operand */
      j = 0;
      while (*cond && *cond != ' ' && strcmp(cond, "do") != 0 && j < 255) {
        right[j++] = *cond++;
      }
      right[j] = '\0';

      /* Expand variables */
      char el[256], er[256];
      nash_expand(left, el, 256);
      nash_expand(right, er, 256);

      /* Evaluate */
      int result = 0;
      if (strcmp(op, "eq") == 0) {
        result = (strcmp(el, er) == 0);
      } else if (strcmp(op, "ne") == 0) {
        result = (strcmp(el, er) != 0);
      } else if (strcmp(op, "gt") == 0) {
        /* Simple numeric comparison */
        int a = 0, b = 0;
        for (const char *x = el; *x >= '0' && *x <= '9'; x++)
          a = a * 10 + (*x - '0');
        for (const char *x = er; *x >= '0' && *x <= '9'; x++)
          b = b * 10 + (*x - '0');
        result = (a > b);
      } else if (strcmp(op, "lt") == 0) {
        int a = 0, b = 0;
        for (const char *x = el; *x >= '0' && *x <= '9'; x++)
          a = a * 10 + (*x - '0');
        for (const char *x = er; *x >= '0' && *x <= '9'; x++)
          b = b * 10 + (*x - '0');
        result = (a < b);
      }

      skip_block = !result;
      continue;
    }

    /* Handle: repeat <n> times */
    if (starts_with(l, "repeat")) {
      const char *n = skip_ws(l + 6);
      char num[16];
      int j = 0;
      while (*n >= '0' && *n <= '9' && j < 15) {
        num[j++] = *n++;
      }
      num[j] = '\0';

      repeat_count = 0;
      for (j = 0; num[j]; j++) {
        repeat_count = repeat_count * 10 + (num[j] - '0');
      }
      repeat_start = p;
      continue;
    }

    /* Handle: run <command> (execute shell command) */
    if (starts_with(l, "run")) {
      const char *cmd = skip_ws(l + 3);
      char expanded[512];
      nash_expand(cmd, expanded, 512);
      shell_execute_simple(expanded);
      continue;
    }

    /* Default: treat as shell command */
    char expanded[512];
    nash_expand(l, expanded, 512);
    shell_execute_simple(expanded);
  }
}

/*
 * cmd_nash - Execute a .nsh script
 */
void cmd_nash(const char *args) {
  if (args[0] == '\0') {
    kprintf("Nash Scripting Language v2\n");
    kprintf("==========================\n");
    kprintf("Usage: nash <script.nsh>\n\n");
    kprintf("Syntax:\n");
    kprintf("  @var = \"value\"    -- Set variable\n");
    kprintf("  print \"text\"      -- Print text\n");
    kprintf("  show @var         -- Show variable\n");
    kprintf("  when @a eq @b do  -- Conditional\n");
    kprintf("    ...\n");
    kprintf("  otherwise\n");
    kprintf("    ...\n");
    kprintf("  end\n");
    kprintf("  repeat 5 times    -- Loop\n");
    kprintf("    ...\n");
    kprintf("  end\n");
    kprintf("  run <command>     -- Execute command\n");
    kprintf("  -- comment        -- Comment\n");
    return;
  }

  /* Check extension */
  const char *ext = args;
  while (*ext)
    ext++;
  if (ext - args < 4 || (*(ext - 4) != '.' || *(ext - 3) != 'n' ||
                         *(ext - 2) != 's' || *(ext - 1) != 'h')) {
    kprintf("nash: script must have .nsh extension\n");
    return;
  }

  /* Read script */
  char script[MAX_SCRIPT_SIZE];
  int len = fs_read(args, script, MAX_SCRIPT_SIZE - 1);

  if (len < 0) {
    kprintf("nash: cannot read '%s'\n", args);
    return;
  }

  script[len] = '\0';

  /* Clear state */
  memset(nash_vars, 0, sizeof(nash_vars));
  nash_last_result = 0;

  /* Set defaults */
  nash_set_var("shell", "nash");
  nash_set_var("version", "2.0");

  nash_execute_script(script);
}

void cmd_nash_vars(const char *args) {
  (void)args;
  kprintf("\nNash Variables:\n");
  for (int i = 0; i < MAX_VARS; i++) {
    if (nash_vars[i].name[0]) {
      kprintf("  @%s = \"%s\"\n", nash_vars[i].name, nash_vars[i].value);
    }
  }
  kprintf("\n");
}
