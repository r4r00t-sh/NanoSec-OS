/*
 * NanoSec OS - Kernel Entry Point
 * =================================
 * Main kernel written in C
 */

#include "kernel.h"

/* Kernel version */
#define NANOSEC_VERSION "1.0.0"
#define NANOSEC_CODENAME "Sentinel"

/* Global kernel state */
static kernel_state_t kernel_state;

/* Forward declarations */
static void kernel_early_init(void);
static void kernel_init_security(void);
static void kernel_main_loop(void);
static void kernel_login_prompt(void);
static void print_banner(void);

/*
 * Kernel entry point - called from bootloader
 */
void kernel_main(void) {
  /* Early initialization */
  kernel_early_init();

  /* Print boot banner */
  print_banner();

  kprintf("[BOOT] Initializing drivers...\n");
  kprintf("  [OK] VGA driver\n");
  kprintf("  [OK] Keyboard driver\n");

  /* Initialize memory management */
  kprintf("[BOOT] Setting up memory...\n");
  mm_init();

  /* Initialize filesystem */
  kprintf("[BOOT] Initializing filesystem...\n");
  if (fs_init() == 0) {
    kprintf("  [OK] RAM Filesystem\n");
  }
  perms_init();

  /* Initialize user system */
  kprintf("[BOOT] Initializing users...\n");
  if (user_init() == 0) {
    kprintf("  [OK] User System\n");
  }

  /* Initialize environment and shell features */
  env_init();
  alias_init();
  audit_init();

  /* Initialize serial console */
  serial_init(0x3F8, 1); /* COM1, 115200 baud */
  klog("NanoSec OS booting...");

  /* Initialize network stack */
  kprintf("[BOOT] Initializing network...\n");
  net_init();

  /* Initialize security subsystem */
  kprintf("[BOOT] Initializing security...\n");
  kernel_init_security();

  kprintf("\n");
  kprintf_color("NanoSec OS ready.\n\n", VGA_COLOR_GREEN);

  /* Login prompt */
  kernel_login_prompt();

  /* Enter main kernel loop */
  kernel_main_loop();

  /* Should never reach here */
  kprintf_color("KERNEL PANIC: Main loop exited!\n", VGA_COLOR_RED);
  for (;;) {
    asm volatile("hlt");
  }
}

/*
 * Early kernel initialization
 */
static void kernel_early_init(void) {
  /* Clear kernel state */
  memset(&kernel_state, 0, sizeof(kernel_state_t));

  /* Initialize VGA text mode */
  vga_init();
  vga_clear();

  kernel_state.initialized = 1;
}

/*
 * Initialize security subsystem
 */
static void kernel_init_security(void) {
  /* Initialize firewall */
  if (firewall_init() == 0) {
    kprintf("  [OK] Firewall\n");
    kernel_state.firewall_active = 1;
  }

  /* Initialize security monitor */
  if (secmon_init() == 0) {
    kprintf("  [OK] Security Monitor\n");
    kernel_state.secmon_active = 1;
  }

  /* Print security status */
  kprintf("\n[SECURITY] Status: ");
  if (kernel_state.firewall_active && kernel_state.secmon_active) {
    kprintf_color("PROTECTED\n", VGA_COLOR_GREEN);
  } else {
    kprintf_color("DEGRADED\n", VGA_COLOR_YELLOW);
  }
}

/*
 * Login prompt - requires user to authenticate
 */
static void kernel_login_prompt(void) {
  char username[32], password[32];
  int logged_in = 0;

  while (!logged_in) {
    /* Username */
    kprintf("nanosec login: ");
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

    /* Password (hidden) */
    kprintf("Password: ");
    i = 0;
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
        /* Don't echo password */
      }
    }
    password[i] = '\0';
    kprintf("\n");

    /* Try to login */
    if (user_login(username, password) == 0) {
      kprintf("\n");
      kprintf_color("Welcome to NanoSec OS!\n", VGA_COLOR_GREEN);
      kprintf("Type 'help' for commands.\n\n");
      logged_in = 1;
    } else {
      kprintf_color("Login incorrect\n\n", VGA_COLOR_RED);
    }
  }
}

/*
 * Main kernel loop - command processing
 */
static void kernel_main_loop(void) {
  char cmd_buffer[256];
  int pos = 0;

  kprintf("nanosec# ");

  while (1) {
    /* Wait for keypress */
    char c = keyboard_getchar();

    if (c == '\n') {
      kprintf("\n");
      cmd_buffer[pos] = '\0';

      if (pos > 0) {
        shell_execute(cmd_buffer);
      }

      pos = 0;
      kprintf("nanosec# ");
    } else if (c == '\b') {
      if (pos > 0) {
        pos--;
        vga_putchar('\b');
        vga_putchar(' ');
        vga_putchar('\b');
      }
    } else if (c >= 32 && c < 127 && pos < 255) {
      cmd_buffer[pos++] = c;
      vga_putchar(c);
    }
  }
}

/*
 * Print boot banner
 */
static void print_banner(void) {
  vga_set_color(VGA_COLOR_CYAN);
  kprintf("\n");
  kprintf("  _   _                  ____            \n");
  kprintf(" | \\ | | __ _ _ __   ___/ ___|  ___  ___ \n");
  kprintf(" |  \\| |/ _` | '_ \\ / _ \\___ \\ / _ \\/ __|\n");
  kprintf(" | |\\  | (_| | | | | (_) |__) |  __/ (__ \n");
  kprintf(" |_| \\_|\\__,_|_| |_|\\___/____/ \\___|\\___|\n");
  kprintf("\n");
  vga_set_color(VGA_COLOR_WHITE);
  kprintf("  NanoSec OS v%s \"%s\"\n", NANOSEC_VERSION, NANOSEC_CODENAME);
  kprintf("  Security-First Operating System\n");
  kprintf("\n");
  vga_set_color(VGA_COLOR_LIGHT_GREY);
}

/*
 * Kernel panic
 */
void kernel_panic(const char *message) {
  asm volatile("cli");

  vga_set_color(VGA_COLOR_RED);
  kprintf("\n\n!!! KERNEL PANIC !!!\n");
  kprintf("Error: %s\n", message);
  kprintf("\nSystem halted.\n");

  for (;;) {
    asm volatile("hlt");
  }
}

/*
 * Printf implementation
 */
void kprintf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  while (*fmt) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt) {
      case 's': {
        const char *s = va_arg(args, const char *);
        while (*s)
          vga_putchar(*s++);
        break;
      }
      case 'd': {
        int n = va_arg(args, int);
        char buf[16];
        int i = 0;
        int neg = 0;

        if (n < 0) {
          neg = 1;
          n = -n;
        }
        if (n == 0)
          buf[i++] = '0';
        while (n > 0) {
          buf[i++] = '0' + (n % 10);
          n /= 10;
        }
        if (neg)
          vga_putchar('-');
        while (i > 0)
          vga_putchar(buf[--i]);
        break;
      }
      case 'x': {
        unsigned int n = va_arg(args, unsigned int);
        char hex[16];
        int i = 0;
        if (n == 0) {
          vga_putchar('0');
          break;
        }
        while (n > 0) {
          int d = n & 0xF;
          hex[i++] = d < 10 ? '0' + d : 'a' + d - 10;
          n >>= 4;
        }
        while (i > 0)
          vga_putchar(hex[--i]);
        break;
      }
      case 'c':
        vga_putchar((char)va_arg(args, int));
        break;
      case '%':
        vga_putchar('%');
        break;
      }
      fmt++;
    } else {
      vga_putchar(*fmt++);
    }
  }

  va_end(args);
}

void kprintf_color(const char *str, vga_color_t color) {
  vga_color_t old = vga_get_color();
  vga_set_color(color);
  kprintf("%s", str);
  vga_set_color(old);
}
