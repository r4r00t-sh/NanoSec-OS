/*
 * NanoSec OS - Filesystem Hierarchy Standard (FHS)
 * ==================================================
 * Initializes Linux-like directories at boot
 */

#include "../kernel.h"

/* FHS directories to create (for display purposes) */
static const char *fhs_dirs[] = {"/bin",  "/sbin", "/etc", "/var", "/tmp",
                                 "/home", "/root", "/usr", "/lib", "/dev",
                                 "/proc", "/mnt",  "/opt", NULL};

/*
 * Initialize FHS - display directory structure
 */
int fhs_init(void) {
  int count = 0;

  while (fhs_dirs[count] != NULL) {
    count++;
  }

  kprintf("  [OK] FHS (%d virtual directories)\n", count);
  return 0;
}
