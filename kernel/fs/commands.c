/*
 * NanoSec OS - Unix File Commands
 * =================================
 * cd, mkdir, man, apropos commands
 */

#include "../kernel.h"

/* External filesystem functions */
extern int fhs_chdir(const char *path);
extern int fs_mkdir(const char *name);
extern int fs_isdir(const char *name);

/*
 * cd - Change directory
 */
void cmd_cd(const char *args) {
  if (args[0] == '\0' || strcmp(args, "~") == 0) {
    fhs_chdir("/root");
    return;
  }

  if (fhs_chdir(args) != 0) {
    kprintf("cd: %s: No such directory\n", args);
  }
}

/*
 * mkdir - Create directory
 */
void cmd_mkdir(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: mkdir <directory>\n");
    return;
  }

  if (fs_mkdir(args) == 0) {
    kprintf("Created: %s/\n", args);
  } else {
    kprintf("mkdir: cannot create '%s'\n", args);
  }
}
