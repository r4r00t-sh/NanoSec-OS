/*
 * NanoSec OS - System Calls
 * ==========================
 * User/kernel interface via INT 0x80
 */

#include "../cpu/idt.h"
#include "../kernel.h"
#include "../proc/process.h"


/* Syscall numbers */
#define SYS_EXIT 0
#define SYS_FORK 1
#define SYS_READ 2
#define SYS_WRITE 3
#define SYS_OPEN 4
#define SYS_CLOSE 5
#define SYS_EXEC 6
#define SYS_GETPID 7
#define SYS_YIELD 8
#define SYS_SLEEP 9
#define SYS_PS 10

/* Maximum syscalls */
#define MAX_SYSCALLS 32

/* Syscall handler type */
typedef int (*syscall_fn)(uint32_t arg1, uint32_t arg2, uint32_t arg3);

/* Syscall table */
static syscall_fn syscall_table[MAX_SYSCALLS];

/*
 * Syscall implementations
 */

static int sys_exit(uint32_t status, uint32_t unused1, uint32_t unused2) {
  (void)unused1;
  (void)unused2;
  proc_exit(status);
  return 0;
}

static int sys_getpid(uint32_t unused1, uint32_t unused2, uint32_t unused3) {
  (void)unused1;
  (void)unused2;
  (void)unused3;
  return proc_get_pid();
}

static int sys_yield(uint32_t unused1, uint32_t unused2, uint32_t unused3) {
  (void)unused1;
  (void)unused2;
  (void)unused3;
  proc_yield();
  return 0;
}

static int sys_write(uint32_t fd, uint32_t buf, uint32_t count) {
  if (fd == 1 || fd == 2) { /* stdout or stderr */
    char *str = (char *)buf;
    for (uint32_t i = 0; i < count && str[i]; i++) {
      vga_putchar(str[i]);
    }
    return count;
  }
  return -1;
}

static int sys_read(uint32_t fd, uint32_t buf, uint32_t count) {
  if (fd == 0) { /* stdin */
    char *str = (char *)buf;
    for (uint32_t i = 0; i < count; i++) {
      str[i] = keyboard_getchar();
      if (str[i] == '\n') {
        return i + 1;
      }
    }
    return count;
  }
  return -1;
}

static int sys_ps(uint32_t unused1, uint32_t unused2, uint32_t unused3) {
  (void)unused1;
  (void)unused2;
  (void)unused3;
  cmd_ps("");
  return 0;
}

/*
 * Syscall interrupt handler
 */
static void syscall_handler(interrupt_frame_t *frame) {
  /* Syscall number in EAX, args in EBX, ECX, EDX */
  uint32_t syscall_num = frame->eax;
  uint32_t arg1 = frame->ebx;
  uint32_t arg2 = frame->ecx;
  uint32_t arg3 = frame->edx;

  if (syscall_num >= MAX_SYSCALLS || !syscall_table[syscall_num]) {
    frame->eax = -1; /* Invalid syscall */
    return;
  }

  /* Call handler and store result in EAX */
  frame->eax = syscall_table[syscall_num](arg1, arg2, arg3);
}

/*
 * Initialize syscall subsystem
 */
void syscall_init(void) {
  /* Clear table */
  for (int i = 0; i < MAX_SYSCALLS; i++) {
    syscall_table[i] = NULL;
  }

  /* Register syscalls */
  syscall_table[SYS_EXIT] = sys_exit;
  syscall_table[SYS_READ] = sys_read;
  syscall_table[SYS_WRITE] = sys_write;
  syscall_table[SYS_GETPID] = sys_getpid;
  syscall_table[SYS_YIELD] = sys_yield;
  syscall_table[SYS_PS] = sys_ps;

  /* Register INT 0x80 handler */
  isr_register_handler(0x80, syscall_handler);

  kprintf("  [OK] Syscalls (INT 0x80)\n");
}
