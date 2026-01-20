/*
 * NanoSec OS - Manual Pages
 * ===========================
 * Built-in man page system
 */

#include "../kernel.h"

/* Man page entry */
typedef struct {
  const char *name;
  const char *section;
  const char *synopsis;
  const char *description;
} man_page_t;

/* Built-in man pages */
static const man_page_t man_pages[] = {
    /* Section 1: User Commands */
    {"ls", "1", "ls [directory]",
     "List directory contents.\n"
     "If no directory specified, lists current directory.\n"},
    {"cd", "1", "cd [directory]",
     "Change the current working directory.\n"
     "With no arguments, changes to /root.\n"
     "Use 'cd ..' to go up one level.\n"},
    {"pwd", "1", "pwd", "Print the current working directory path.\n"},
    {"mkdir", "1", "mkdir <directory>", "Create a new directory.\n"},
    {"rm", "1", "rm [-rf] <file|directory>",
     "Remove files or directories.\n"
     "Options:\n"
     "  -r  Remove directories recursively\n"
     "  -f  Force removal without confirmation\n"},
    {"cp", "1", "cp <source> <destination>",
     "Copy a file from source to destination.\n"},
    {"mv", "1", "mv <source> <destination>", "Move or rename a file.\n"},
    {"touch", "1", "touch <file>",
     "Create an empty file if it doesn't exist.\n"},
    {"cat", "1", "cat <file>", "Display the contents of a file.\n"},
    {"edit", "1", "edit <file>",
     "Open the built-in text editor.\n"
     "Commands in editor:\n"
     "  :w  Save file\n"
     "  :q  Quit\n"
     "  :wq Save and quit\n"},
    {"clear", "1", "clear", "Clear the terminal screen.\n"},
    {"echo", "1", "echo <text>", "Display text to the terminal.\n"},
    {"whoami", "1", "whoami", "Print the current username.\n"},
    {"ps", "1", "ps", "List running processes.\n"},
    {"help", "1", "help", "Display list of available commands.\n"},

    /* Section 8: System Administration */
    {"nping", "8", "nping <host>",
     "Send ICMP echo requests to a host.\n"
     "Use 127.0.0.1 for loopback test.\n"},
    {"nifconfig", "8", "nifconfig [ip <address>] [gateway <address>]",
     "Configure network interfaces.\n"
     "With no arguments, shows current configuration.\n"},
    {"firewall", "8", "firewall <enable|disable|status|add|remove>",
     "Manage the kernel firewall.\n"
     "  enable   - Enable firewall\n"
     "  disable  - Disable firewall\n"
     "  status   - Show firewall status\n"
     "  add      - Add a rule\n"
     "  remove   - Remove a rule\n"},
    {"reboot", "8", "reboot", "Reboot the system.\n"},
    {"shutdown", "8", "shutdown", "Power off the system.\n"},

    /* End marker */
    {NULL, NULL, NULL, NULL}};

/*
 * Display a man page
 */
void cmd_man(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: man <command>\n");
    kprintf("Example: man ls\n");
    return;
  }

  /* Search for the command */
  for (int i = 0; man_pages[i].name != NULL; i++) {
    if (strcmp(man_pages[i].name, args) == 0) {
      kprintf("\n");
      kprintf_color(man_pages[i].name, VGA_COLOR_LIGHT_CYAN);
      kprintf("(%s) - NanoSec Manual\n", man_pages[i].section);
      kprintf("\n");
      kprintf_color("SYNOPSIS\n", VGA_COLOR_YELLOW);
      kprintf("    %s\n\n", man_pages[i].synopsis);
      kprintf_color("DESCRIPTION\n", VGA_COLOR_YELLOW);
      kprintf("    %s\n", man_pages[i].description);
      return;
    }
  }

  kprintf("No manual entry for '%s'\n", args);
}

/*
 * List all available man pages
 */
void cmd_apropos(const char *args) {
  (void)args;

  kprintf("\nAvailable manual pages:\n");
  kprintf("=======================\n\n");

  kprintf_color("User Commands (1):\n", VGA_COLOR_YELLOW);
  for (int i = 0; man_pages[i].name != NULL; i++) {
    if (strcmp(man_pages[i].section, "1") == 0) {
      kprintf("  %-12s %s\n", man_pages[i].name, man_pages[i].synopsis);
    }
  }

  kprintf("\n");
  kprintf_color("System Administration (8):\n", VGA_COLOR_YELLOW);
  for (int i = 0; man_pages[i].name != NULL; i++) {
    if (strcmp(man_pages[i].section, "8") == 0) {
      kprintf("  %-12s %s\n", man_pages[i].name, man_pages[i].synopsis);
    }
  }

  kprintf("\n");
}
