/*
 * NanoSec OS - Shell (Complete with All Features)
 */

#include "kernel.h"

typedef struct {
  const char *name;
  const char *desc;
  void (*handler)(const char *args);
} cmd_t;

/* Forward declarations */
static void cmd_help(const char *a);
static void cmd_status(const char *a);
static void cmd_firewall(const char *a);
static void cmd_secmon(const char *a);
static void cmd_memory(const char *a);
static void cmd_clear(const char *a);
static void cmd_version(const char *a);
static void cmd_logs(const char *a);
static void cmd_halt(const char *a);
static void cmd_echo(const char *a);
static void cmd_reboot(const char *a);

/* Full command table */
static cmd_t commands[] = {
    /* General */
    {"help", "Show commands", cmd_help},
    {"status", "System status", cmd_status},
    {"clear", "Clear screen", cmd_clear},
    {"version", "OS version", cmd_version},
    {"echo", "Echo text", cmd_echo},
    {"halt", "Shutdown", cmd_halt},
    {"reboot", "Restart", cmd_reboot},

    /* User Management */
    {"login", "Login", cmd_login},
    {"logout", "Logout", cmd_logout},
    {"whoami", "Current user", cmd_whoami_user},
    {"id", "User IDs", cmd_id},
    {"su", "Switch user", cmd_su},
    {"adduser", "Add user", cmd_adduser},
    {"deluser", "Delete user", cmd_deluser},
    {"passwd", "Change password", cmd_passwd_user},
    {"users", "List users", cmd_users},
    {"sudo", "Run as root", cmd_sudo},
    {"lock", "Lock screen", cmd_lock},

    /* System Info */
    {"sysinfo", "System info", cmd_sysinfo},
    {"ps", "Process list", cmd_ps},
    {"uptime", "Show uptime", cmd_uptime},
    {"date", "Date/time", cmd_date_rtc},
    {"time", "Show time", cmd_time},
    {"cal", "Calendar", cmd_cal},
    {"hostname", "Hostname", cmd_hostname},
    {"uname", "System name", cmd_uname},

    /* Filesystem */
    {"ls", "List files", cmd_ls},
    {"cat", "Show file", cmd_cat},
    {"cd", "Change directory", cmd_cd},
    {"mkdir", "Create directory", cmd_mkdir},
    {"touch", "Create file", cmd_touch},
    {"rm", "Remove file", cmd_rm},
    {"cp", "Copy file", cmd_cp},
    {"mv", "Move file", cmd_mv},
    {"pwd", "Current dir", cmd_pwd},
    {"nedit", "Text editor", cmd_nedit},
    {"hexdump", "Hex dump", cmd_hexdump},
    {"head", "First lines", cmd_head},
    {"tail", "Last lines", cmd_tail},
    {"wc", "Word count", cmd_wc},
    {"grep", "Search file", cmd_grep},
    {"chmod", "Change mode", cmd_chmod},
    {"chown", "Change owner", cmd_chown},
    {"man", "Manual pages", cmd_man},
    {"apropos", "Search manual", cmd_apropos},
    {"find", "Find files", cmd_find},
    {"stat", "File info", cmd_stat},
    {"df", "Disk usage", cmd_df},
    {"du", "Dir size", cmd_du},
    {"more", "Page file", cmd_more},
    {"diff", "Compare files", cmd_diff},
    {"ln", "Create link", cmd_ln},
    {"cut", "Extract columns", cmd_cut},
    {"tr", "Translate chars", cmd_tr},
    {"tee", "Tee output", cmd_tee},
    {"xargs", "Build commands", cmd_xargs},
    {"sed", "Stream editor", cmd_sed},
    {"nash", "Run .nsh script", cmd_nash},

    /* Environment */
    {"export", "Set env var", cmd_export},
    {"env", "Show env", cmd_env},
    {"unset", "Unset var", cmd_unset},
    {"history", "Command history", cmd_history},
    {"alias", "Set alias", cmd_alias},
    {"unalias", "Remove alias", cmd_unalias},

    /* Sound */
    {"beep", "Play tone", cmd_beep},
    {"play", "Play melody", cmd_play},

    /* Security */
    {"firewall", "Firewall", cmd_firewall},
    {"secmon", "Security mon", cmd_secmon},
    {"logs", "Security logs", cmd_logs},
    {"audit", "Audit log", cmd_audit},
    {"memory", "Memory usage", cmd_memory},
    {"dmesg", "Kernel messages", cmd_dmesg},

    /* Network */
    {"nifconfig", "Network config", cmd_nifconfig},
    {"nroute", "Routing table", cmd_nroute},
    {"nnetstat", "Network stats", cmd_nnetstat},
    {"nping", "Ping host", cmd_nping},
    {"narp", "ARP cache", cmd_narp},
    {"ndns", "DNS lookup", cmd_ndns},

    {NULL, NULL, NULL}};

static void parse_cmd(const char *in, char *cmd, char *args) {
  while (*in == ' ')
    in++;
  int i = 0;
  while (*in && *in != ' ' && i < 63)
    cmd[i++] = *in++;
  cmd[i] = '\0';
  while (*in == ' ')
    in++;
  i = 0;
  while (*in && i < 191)
    args[i++] = *in++;
  args[i] = '\0';
}

/* Simple command execution (no operators) */
void shell_execute_simple(const char *input) {
  char cmd[64], args[192];

  /* Check for alias */
  parse_cmd(input, cmd, args);
  const char *alias_cmd = alias_get(cmd);
  if (alias_cmd) {
    /* Expand alias */
    char expanded[256];
    strncpy(expanded, alias_cmd, 192);
    if (args[0]) {
      strcat(expanded, " ");
      strcat(expanded, args);
    }
    parse_cmd(expanded, cmd, args);
  }

  if (cmd[0] == '\0')
    return;

  for (int i = 0; commands[i].name; i++) {
    if (strcmp(cmd, commands[i].name) == 0) {
      commands[i].handler(args);
      return;
    }
  }

  kprintf_color("Unknown: ", VGA_COLOR_RED);
  kprintf("%s\n", cmd);
}

/* Main shell execute - supports pipes, redirects, chaining */
extern void shell_execute_advanced(const char *input);

void shell_execute(const char *input) {
  if (input[0] == '\0')
    return;

  /* Log and history */
  audit_log_cmd(input);
  history_add(input);

  /* Use advanced handler for all commands */
  shell_execute_advanced(input);
}

/* === Command Implementations === */

static void cmd_help(const char *a) {
  (void)a;
  kprintf("\n");
  kprintf_color("═══════════════════════════════════════════\n",
                VGA_COLOR_CYAN);
  kprintf_color("         NanoSec OS Commands               \n",
                VGA_COLOR_CYAN);
  kprintf_color("═══════════════════════════════════════════\n\n",
                VGA_COLOR_CYAN);

  kprintf_color("General:\n", VGA_COLOR_YELLOW);
  kprintf("  help status clear version halt reboot\n\n");

  kprintf_color("User:\n", VGA_COLOR_YELLOW);
  kprintf("  login logout whoami id su sudo lock\n");
  kprintf("  adduser deluser passwd users\n\n");

  kprintf_color("System:\n", VGA_COLOR_YELLOW);
  kprintf("  sysinfo ps uptime date time cal uname\n\n");

  kprintf_color("Files:\n", VGA_COLOR_YELLOW);
  kprintf("  ls cat touch rm cp mv pwd nedit\n");
  kprintf("  head tail wc grep chmod chown\n\n");

  kprintf_color("Environment:\n", VGA_COLOR_YELLOW);
  kprintf("  export env unset history alias\n\n");

  kprintf_color("Sound:\n", VGA_COLOR_YELLOW);
  kprintf("  beep play\n\n");

  kprintf_color("Security:\n", VGA_COLOR_YELLOW);
  kprintf("  firewall secmon logs audit memory\n\n");

  kprintf_color("Network:\n", VGA_COLOR_YELLOW);
  kprintf("  nifconfig nroute nnetstat nping narp\n\n");
}

static void cmd_status(const char *a) {
  (void)a;
  kprintf("\n");
  kprintf_color("=== NanoSec Status ===\n\n", VGA_COLOR_CYAN);

  kprintf("User:     %s", user_get_username());
  if (user_is_root())
    kprintf_color(" (root)\n", VGA_COLOR_RED);
  else
    kprintf("\n");

  kprintf("Firewall: ");
  kprintf_color("ACTIVE\n", VGA_COLOR_GREEN);
  kprintf("SecMon:   ");
  kprintf_color("ACTIVE\n", VGA_COLOR_GREEN);
  kprintf("Alerts:   %d\n", secmon_get_alert_count());
  kprintf("\n");
}

static void cmd_firewall(const char *a) {
  if (a[0] == '\0' || strcmp(a, "status") == 0) {
    firewall_status();
  } else if (strcmp(a, "enable") == 0) {
    if (!user_is_root() && !sudo_check()) {
      kprintf_color("Permission denied\n", VGA_COLOR_RED);
      return;
    }
    firewall_enable(1);
    kprintf_color("Firewall enabled\n", VGA_COLOR_GREEN);
  } else if (strcmp(a, "disable") == 0) {
    if (!user_is_root() && !sudo_check()) {
      kprintf_color("Permission denied\n", VGA_COLOR_RED);
      return;
    }
    firewall_enable(0);
    kprintf_color("Firewall disabled\n", VGA_COLOR_YELLOW);
  }
}

static void cmd_secmon(const char *a) {
  if (a[0] == '\0')
    secmon_status();
  else if (strcmp(a, "ack") == 0) {
    secmon_acknowledge_alerts();
    kprintf("Alerts acknowledged.\n");
  }
}

static void cmd_memory(const char *a) {
  (void)a;
  mm_status();
}
static void cmd_logs(const char *a) {
  (void)a;
  secmon_show_logs(10);
}
static void cmd_clear(const char *a) {
  (void)a;
  vga_clear();
}

static void cmd_version(const char *a) {
  (void)a;
  kprintf("\nNanoSec OS v2.0.0 \"Fortress\"\n");
  kprintf("Custom Kernel with Security Suite\n\n");
}

static void cmd_echo(const char *a) {
  /* Expand environment variables */
  char expanded[256];
  env_expand(a, expanded, sizeof(expanded));
  kprintf("%s\n", expanded);
}

static void cmd_halt(const char *a) {
  (void)a;
  if (!user_is_root() && !sudo_check()) {
    kprintf_color("Permission denied\n", VGA_COLOR_RED);
    return;
  }
  kprintf_color("\nSystem halted.\n", VGA_COLOR_YELLOW);
  asm volatile("cli; hlt");
}

static void cmd_reboot(const char *a) {
  (void)a;
  if (!user_is_root() && !sudo_check()) {
    kprintf_color("Permission denied\n", VGA_COLOR_RED);
    return;
  }
  kprintf("Rebooting...\n");
  uint8_t good = 0x02;
  while (good & 0x02)
    good = inb(0x64);
  outb(0x64, 0xFE);
  asm volatile("cli; hlt");
}
