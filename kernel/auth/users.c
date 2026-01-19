/*
 * NanoSec OS - User Management System
 * =====================================
 * Users, groups, authentication, and permissions
 */

#include "../kernel.h"

/* User system limits */
#define MAX_USERS 16
#define MAX_GROUPS 8
#define MAX_USERNAME 16
#define MAX_PASSWORD 32
#define MAX_HOMEDIR 32

/* User flags */
#define USER_FLAG_ACTIVE 0x01
#define USER_FLAG_ADMIN 0x02
#define USER_FLAG_LOCKED 0x04

/* Permission bits (like Unix) */
#define PERM_READ 0x04
#define PERM_WRITE 0x02
#define PERM_EXEC 0x01

#define PERM_OWNER_R 0x100
#define PERM_OWNER_W 0x080
#define PERM_OWNER_X 0x040
#define PERM_GROUP_R 0x020
#define PERM_GROUP_W 0x010
#define PERM_GROUP_X 0x008
#define PERM_OTHER_R 0x004
#define PERM_OTHER_W 0x002
#define PERM_OTHER_X 0x001

/* User structure */
typedef struct {
  uint16_t uid;
  uint16_t gid;
  uint8_t flags;
  char username[MAX_USERNAME];
  char password[MAX_PASSWORD]; /* In real OS, this would be hashed! */
  char home[MAX_HOMEDIR];
  char shell[16];
} user_t;

/* Group structure */
typedef struct {
  uint16_t gid;
  char name[16];
  uint16_t members[8]; /* UIDs */
  int member_count;
} group_t;

/* Current session */
typedef struct {
  int logged_in;
  uint16_t uid;
  uint16_t gid;
  char username[MAX_USERNAME];
  int is_root;
} session_t;

/* Global state */
static user_t users[MAX_USERS];
static group_t groups[MAX_GROUPS];
static session_t current_session;
static int user_count = 0;
static int group_count = 0;

/*
 * Simple password hash (NOT secure - just for demo!)
 * In a real OS, use proper crypto like bcrypt/scrypt
 */
static uint32_t simple_hash(const char *str) {
  uint32_t hash = 5381;
  while (*str) {
    hash = ((hash << 5) + hash) + *str++;
  }
  return hash;
}

/*
 * Initialize user system
 */
int user_init(void) {
  memset(users, 0, sizeof(users));
  memset(groups, 0, sizeof(groups));
  memset(&current_session, 0, sizeof(current_session));

  /* Create root group */
  groups[0].gid = 0;
  strcpy(groups[0].name, "root");
  groups[0].members[0] = 0;
  groups[0].member_count = 1;
  group_count = 1;

  /* Create users group */
  groups[1].gid = 100;
  strcpy(groups[1].name, "users");
  group_count = 2;

  /* Create root user */
  users[0].uid = 0;
  users[0].gid = 0;
  users[0].flags = USER_FLAG_ACTIVE | USER_FLAG_ADMIN;
  strcpy(users[0].username, "root");
  strcpy(users[0].password, "root"); /* Default password */
  strcpy(users[0].home, "/root");
  strcpy(users[0].shell, "/bin/nash");
  user_count = 1;

  /* Create guest user */
  users[1].uid = 1000;
  users[1].gid = 100;
  users[1].flags = USER_FLAG_ACTIVE;
  strcpy(users[1].username, "guest");
  strcpy(users[1].password, "guest");
  strcpy(users[1].home, "/home/guest");
  strcpy(users[1].shell, "/bin/nash");
  user_count = 2;

  return 0;
}

/*
 * Find user by username
 */
static user_t *find_user(const char *username) {
  for (int i = 0; i < user_count; i++) {
    if (strcmp(users[i].username, username) == 0) {
      return &users[i];
    }
  }
  return NULL;
}

/*
 * Find user by UID
 */
static user_t *find_user_by_uid(uint16_t uid) {
  for (int i = 0; i < user_count; i++) {
    if (users[i].uid == uid) {
      return &users[i];
    }
  }
  return NULL;
}

/*
 * Authenticate user
 */
int user_authenticate(const char *username, const char *password) {
  user_t *user = find_user(username);

  if (!user) {
    secmon_log("Login failed: invalid user", 1);
    return -1;
  }

  if (user->flags & USER_FLAG_LOCKED) {
    secmon_log("Login failed: account locked", 2);
    return -2;
  }

  if (strcmp(user->password, password) != 0) {
    secmon_log("Login failed: wrong password", 1);
    return -3;
  }

  return 0;
}

/*
 * Login user - creates session
 */
int user_login(const char *username, const char *password) {
  if (user_authenticate(username, password) != 0) {
    return -1;
  }

  user_t *user = find_user(username);

  current_session.logged_in = 1;
  current_session.uid = user->uid;
  current_session.gid = user->gid;
  strcpy(current_session.username, username);
  current_session.is_root = (user->uid == 0);

  secmon_log("User logged in", 0);

  return 0;
}

/*
 * Logout current user
 */
void user_logout(void) {
  secmon_log("User logged out", 0);
  memset(&current_session, 0, sizeof(current_session));
}

/*
 * Get current session
 */
session_t *user_get_session(void) { return &current_session; }

/*
 * Check if current user is root
 */
int user_is_root(void) { return current_session.is_root; }

/*
 * Get current UID
 */
uint16_t user_get_uid(void) { return current_session.uid; }

/*
 * Get current username
 */
const char *user_get_username(void) {
  if (!current_session.logged_in) {
    return "nobody";
  }
  return current_session.username;
}

/*
 * Add new user
 */
int user_add(const char *username, const char *password, int is_admin) {
  if (!user_is_root()) {
    kprintf_color("Permission denied\n", VGA_COLOR_RED);
    return -1;
  }

  if (user_count >= MAX_USERS) {
    kprintf_color("Maximum users reached\n", VGA_COLOR_RED);
    return -2;
  }

  if (find_user(username)) {
    kprintf_color("User already exists\n", VGA_COLOR_RED);
    return -3;
  }

  user_t *user = &users[user_count];
  user->uid = 1000 + user_count;
  user->gid = 100; /* users group */
  user->flags = USER_FLAG_ACTIVE;
  if (is_admin)
    user->flags |= USER_FLAG_ADMIN;

  strncpy(user->username, username, MAX_USERNAME - 1);
  strncpy(user->password, password, MAX_PASSWORD - 1);

  /* Create home directory path */
  strcpy(user->home, "/home/");
  strncpy(user->home + 6, username, MAX_HOMEDIR - 7);

  strcpy(user->shell, "/bin/nash");

  user_count++;

  secmon_log("New user created", 0);
  kprintf("User '%s' created (UID=%d)\n", username, user->uid);

  return 0;
}

/*
 * Delete user
 */
int user_del(const char *username) {
  if (!user_is_root()) {
    kprintf_color("Permission denied\n", VGA_COLOR_RED);
    return -1;
  }

  if (strcmp(username, "root") == 0) {
    kprintf_color("Cannot delete root user\n", VGA_COLOR_RED);
    return -2;
  }

  user_t *user = find_user(username);
  if (!user) {
    kprintf_color("User not found\n", VGA_COLOR_RED);
    return -3;
  }

  memset(user, 0, sizeof(user_t));
  secmon_log("User deleted", 1);
  kprintf("User '%s' deleted\n", username);

  return 0;
}

/*
 * Change password
 */
int user_passwd(const char *username, const char *old_pass,
                const char *new_pass) {
  user_t *user = find_user(username);

  if (!user) {
    kprintf_color("User not found\n", VGA_COLOR_RED);
    return -1;
  }

  /* Only root or the user themselves can change password */
  if (!user_is_root() && strcmp(current_session.username, username) != 0) {
    kprintf_color("Permission denied\n", VGA_COLOR_RED);
    return -2;
  }

  /* Verify old password (unless root) */
  if (!user_is_root()) {
    if (strcmp(user->password, old_pass) != 0) {
      kprintf_color("Wrong password\n", VGA_COLOR_RED);
      return -3;
    }
  }

  strncpy(user->password, new_pass, MAX_PASSWORD - 1);
  secmon_log("Password changed", 0);
  kprintf("Password changed for '%s'\n", username);

  return 0;
}

/*
 * Switch user (su)
 */
int user_switch(const char *username, const char *password) {
  /* Root can switch without password */
  if (user_is_root()) {
    user_t *user = find_user(username);
    if (!user) {
      kprintf_color("User not found\n", VGA_COLOR_RED);
      return -1;
    }

    current_session.uid = user->uid;
    current_session.gid = user->gid;
    strcpy(current_session.username, username);
    current_session.is_root = (user->uid == 0);

    kprintf("Switched to user '%s'\n", username);
    return 0;
  }

  /* Non-root needs password */
  return user_login(username, password);
}

/*
 * Check file permission
 */
int user_check_permission(uint16_t file_uid, uint16_t file_gid,
                          uint16_t file_mode, int access_type) {
  /* Root can do anything */
  if (user_is_root()) {
    return 1;
  }

  uint16_t uid = current_session.uid;
  uint16_t gid = current_session.gid;

  if (uid == file_uid) {
    /* Owner */
    if (access_type == PERM_READ && (file_mode & PERM_OWNER_R))
      return 1;
    if (access_type == PERM_WRITE && (file_mode & PERM_OWNER_W))
      return 1;
    if (access_type == PERM_EXEC && (file_mode & PERM_OWNER_X))
      return 1;
  } else if (gid == file_gid) {
    /* Group */
    if (access_type == PERM_READ && (file_mode & PERM_GROUP_R))
      return 1;
    if (access_type == PERM_WRITE && (file_mode & PERM_GROUP_W))
      return 1;
    if (access_type == PERM_EXEC && (file_mode & PERM_GROUP_X))
      return 1;
  } else {
    /* Other */
    if (access_type == PERM_READ && (file_mode & PERM_OTHER_R))
      return 1;
    if (access_type == PERM_WRITE && (file_mode & PERM_OTHER_W))
      return 1;
    if (access_type == PERM_EXEC && (file_mode & PERM_OTHER_X))
      return 1;
  }

  return 0; /* Permission denied */
}

/* ============================================
 * Shell Commands
 * ============================================ */

/*
 * login command
 */
void cmd_login(const char *args) {
  char username[32], password[32];

  if (args[0] != '\0') {
    /* Username provided */
    strncpy(username, args, 31);
  } else {
    kprintf("Username: ");
    /* Read username */
    int i = 0;
    while (i < 31) {
      char c = keyboard_getchar();
      if (c == '\n')
        break;
      if (c == '\b' && i > 0) {
        i--;
        vga_putchar('\b');
        vga_putchar(' ');
        vga_putchar('\b');
        continue;
      }
      if (c >= 32 && c < 127) {
        username[i++] = c;
        vga_putchar(c);
      }
    }
    username[i] = '\0';
    kprintf("\n");
  }

  kprintf("Password: ");
  /* Read password (hidden) */
  int i = 0;
  while (i < 31) {
    char c = keyboard_getchar();
    if (c == '\n')
      break;
    if (c == '\b' && i > 0) {
      i--;
      vga_putchar('\b');
      vga_putchar(' ');
      vga_putchar('\b');
      continue;
    }
    if (c >= 32 && c < 127) {
      password[i++] = c;
      vga_putchar('*');
    }
  }
  password[i] = '\0';
  kprintf("\n");

  if (user_login(username, password) == 0) {
    kprintf_color("Login successful.\n", VGA_COLOR_GREEN);
  } else {
    kprintf_color("Login failed.\n", VGA_COLOR_RED);
  }
}

/*
 * logout command
 */
void cmd_logout(const char *args) {
  (void)args;
  user_logout();
  kprintf("Logged out.\n");
}

/*
 * whoami command (updated)
 */
void cmd_whoami_user(const char *args) {
  (void)args;
  kprintf("%s\n", user_get_username());
}

/*
 * id command
 */
void cmd_id(const char *args) {
  (void)args;
  session_t *s = user_get_session();

  if (!s->logged_in) {
    kprintf("Not logged in\n");
    return;
  }

  kprintf("uid=%d(%s) gid=%d", s->uid, s->username, s->gid);
  if (s->is_root) {
    kprintf(" groups=0(root)");
  }
  kprintf("\n");
}

/*
 * adduser command
 */
void cmd_adduser(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: adduser <username>\n");
    return;
  }

  char password[32];
  kprintf("Enter password for '%s': ", args);

  int i = 0;
  while (i < 31) {
    char c = keyboard_getchar();
    if (c == '\n')
      break;
    if (c >= 32 && c < 127) {
      password[i++] = c;
      vga_putchar('*');
    }
  }
  password[i] = '\0';
  kprintf("\n");

  user_add(args, password, 0);
}

/*
 * deluser command
 */
void cmd_deluser(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: deluser <username>\n");
    return;
  }
  user_del(args);
}

/*
 * passwd command
 */
void cmd_passwd_user(const char *args) {
  const char *target = args[0] ? args : current_session.username;
  char old_pass[32] = "", new_pass[32];

  if (!user_is_root()) {
    kprintf("Current password: ");
    int i = 0;
    while (i < 31) {
      char c = keyboard_getchar();
      if (c == '\n')
        break;
      if (c >= 32 && c < 127) {
        old_pass[i++] = c;
        vga_putchar('*');
      }
    }
    old_pass[i] = '\0';
    kprintf("\n");
  }

  kprintf("New password: ");
  int i = 0;
  while (i < 31) {
    char c = keyboard_getchar();
    if (c == '\n')
      break;
    if (c >= 32 && c < 127) {
      new_pass[i++] = c;
      vga_putchar('*');
    }
  }
  new_pass[i] = '\0';
  kprintf("\n");

  user_passwd(target, old_pass, new_pass);
}

/*
 * su command
 */
void cmd_su(const char *args) {
  const char *target = args[0] ? args : "root";

  if (user_is_root()) {
    user_switch(target, "");
  } else {
    char password[32];
    kprintf("Password: ");
    int i = 0;
    while (i < 31) {
      char c = keyboard_getchar();
      if (c == '\n')
        break;
      if (c >= 32 && c < 127) {
        password[i++] = c;
        vga_putchar('*');
      }
    }
    password[i] = '\0';
    kprintf("\n");

    if (user_switch(target, password) != 0) {
      kprintf_color("Authentication failed\n", VGA_COLOR_RED);
    }
  }
}

/*
 * users command - list all users
 */
void cmd_users(const char *args) {
  (void)args;

  kprintf("\nUser List:\n");
  kprintf("UID    Username        Home              Flags\n");
  kprintf("-----  --------------  ----------------  -----\n");

  for (int i = 0; i < user_count; i++) {
    if (users[i].flags & USER_FLAG_ACTIVE) {
      kprintf("%-5d  %-14s  %-16s  ", users[i].uid, users[i].username,
              users[i].home);

      if (users[i].flags & USER_FLAG_ADMIN)
        kprintf("admin ");
      if (users[i].flags & USER_FLAG_LOCKED)
        kprintf("locked ");
      kprintf("\n");
    }
  }
  kprintf("\n");
}
