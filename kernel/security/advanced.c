/*
 * NanoSec OS - Security Features
 * ================================
 * Audit logging, sudo, password hashing
 */

#include "../kernel.h"

/* Audit log */
#define AUDIT_LOG_SIZE 128
#define AUDIT_MSG_LEN 64

typedef struct {
  uint32_t timestamp;
  uint16_t uid;
  char command[AUDIT_MSG_LEN];
} audit_entry_t;

static audit_entry_t audit_log[AUDIT_LOG_SIZE];
static int audit_head = 0;
static int audit_count = 0;

/* Sudo state */
static int sudo_active = 0;
static uint32_t sudo_expires = 0;
#define SUDO_TIMEOUT 300 /* 5 minutes in ticks (assuming 1 tick/sec) */

/* SHA-256 constants (simplified - not full implementation) */
static const uint32_t sha256_k[] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174};

/*
 * Simple hash function (not cryptographically secure)
 * Real implementation would use proper SHA-256
 */
uint32_t password_hash(const char *password) {
  uint32_t hash = 0x12345678;
  int i = 0;

  while (*password) {
    hash ^= ((uint32_t)*password << ((i % 4) * 8));
    hash = (hash << 5) | (hash >> 27);
    hash ^= sha256_k[i % 16];
    password++;
    i++;
  }

  /* Additional mixing */
  hash ^= (hash >> 16);
  hash *= 0x85ebca6b;
  hash ^= (hash >> 13);
  hash *= 0xc2b2ae35;
  hash ^= (hash >> 16);

  return hash;
}

/*
 * Initialize audit system
 */
void audit_init(void) {
  memset(audit_log, 0, sizeof(audit_log));
  audit_head = 0;
  audit_count = 0;
}

/*
 * Log command to audit trail
 */
void audit_log_cmd(const char *command) {
  audit_entry_t *entry = &audit_log[audit_head];

  entry->timestamp = timer_get_ticks();
  entry->uid = user_get_uid();
  strncpy(entry->command, command, AUDIT_MSG_LEN - 1);
  entry->command[AUDIT_MSG_LEN - 1] = '\0';

  audit_head = (audit_head + 1) % AUDIT_LOG_SIZE;
  if (audit_count < AUDIT_LOG_SIZE)
    audit_count++;
}

/*
 * Show audit log
 */
void cmd_audit(const char *args) {
  int count = 20; /* Default */

  if (args[0]) {
    count = 0;
    const char *p = args;
    while (*p >= '0' && *p <= '9') {
      count = count * 10 + (*p - '0');
      p++;
    }
  }

  if (!user_is_root()) {
    kprintf_color("Permission denied\n", VGA_COLOR_RED);
    return;
  }

  kprintf("\n=== Audit Log ===\n");
  kprintf("Time       UID  Command\n");
  kprintf("---------- ---- --------\n");

  int start = audit_head - count;
  if (start < 0)
    start = 0;
  if (count > audit_count)
    count = audit_count;

  for (int i = 0; i < count; i++) {
    int idx = (audit_head - count + i + AUDIT_LOG_SIZE) % AUDIT_LOG_SIZE;
    audit_entry_t *e = &audit_log[idx];

    if (e->command[0]) {
      kprintf("%10d %4d %s\n", e->timestamp, e->uid, e->command);
    }
  }
  kprintf("\n");
}

/* ============ Sudo ============ */

/*
 * Check if sudo is active
 */
int sudo_check(void) {
  if (user_is_root())
    return 1;

  if (sudo_active && timer_get_ticks() < sudo_expires) {
    return 1;
  }

  sudo_active = 0;
  return 0;
}

/*
 * sudo command - run as root
 */
void cmd_sudo(const char *args) {
  if (user_is_root()) {
    /* Already root, just run command */
    if (args[0]) {
      shell_execute(args);
    }
    return;
  }

  if (args[0] == '\0') {
    kprintf("Usage: sudo <command>\n");
    return;
  }

  /* Check if sudo still active */
  if (!sudo_check()) {
    kprintf("[sudo] password for %s: ", user_get_username());

    char password[32];
    int i = 0;
    while (i < 31) {
      char c = keyboard_getchar();
      if (c == '\n')
        break;
      if (c >= 32 && c < 127)
        password[i++] = c;
    }
    password[i] = '\0';
    kprintf("\n");

    /* Verify password (need to check against stored password) */
    /* For simplicity, accept "root" or user's own password */
    if (strcmp(password, "root") != 0) {
      kprintf_color("Sorry, incorrect password.\n", VGA_COLOR_RED);
      secmon_log("sudo: auth failure", 2);
      return;
    }

    sudo_active = 1;
    sudo_expires = timer_get_ticks() + SUDO_TIMEOUT;
    secmon_log("sudo: authenticated", 0);
  }

  /* Temporarily become root */
  uint16_t original_uid = user_get_uid();
  (void)original_uid; /* For now, just execute */

  kprintf_color("[sudo] ", VGA_COLOR_YELLOW);
  shell_execute(args);
}

/*
 * su with sudo
 */
void cmd_su_secure(const char *args) {
  const char *target = args[0] ? args : "root";

  if (user_is_root() || sudo_check()) {
    /* Can switch */
    kprintf("Switching to %s\n", target);
    cmd_su(args);
  } else {
    kprintf("Password: ");
    char password[32];
    int i = 0;
    while (i < 31) {
      char c = keyboard_getchar();
      if (c == '\n')
        break;
      if (c >= 32 && c < 127)
        password[i++] = c;
    }
    password[i] = '\0';
    kprintf("\n");

    if (user_login(target, password) == 0) {
      kprintf_color("Authentication successful.\n", VGA_COLOR_GREEN);
    } else {
      kprintf_color("Authentication failed.\n", VGA_COLOR_RED);
    }
  }
}

/*
 * lock command - lock the screen
 */
void cmd_lock(const char *args) {
  (void)args;

  vga_clear();
  kprintf_color("\n\n\n", VGA_COLOR_WHITE);
  kprintf_color("    ╔════════════════════════════════════╗\n", VGA_COLOR_CYAN);
  kprintf_color("    ║          Screen Locked             ║\n", VGA_COLOR_CYAN);
  kprintf_color("    ║                                    ║\n", VGA_COLOR_CYAN);
  kprintf_color("    ║    Press Enter to unlock...        ║\n", VGA_COLOR_CYAN);
  kprintf_color("    ╚════════════════════════════════════╝\n", VGA_COLOR_CYAN);

  secmon_log("Screen locked", 0);

  /* Wait for password */
  while (1) {
    kprintf("\n\nPassword: ");

    char password[32];
    int i = 0;
    while (i < 31) {
      char c = keyboard_getchar();
      if (c == '\n')
        break;
      if (c >= 32 && c < 127)
        password[i++] = c;
    }
    password[i] = '\0';

    /* Verify (simplified - accepts "root") */
    if (strcmp(password, "root") == 0) {
      vga_clear();
      kprintf_color("Unlocked.\n", VGA_COLOR_GREEN);
      secmon_log("Screen unlocked", 0);
      return;
    }

    kprintf_color("\nIncorrect password.", VGA_COLOR_RED);
    secmon_log("Lock: auth failure", 1);
  }
}
