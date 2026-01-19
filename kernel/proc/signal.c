/*
 * NanoSec OS - Signals
 * ======================
 * POSIX-like signal handling
 */

#include "../kernel.h"
#include "../proc/process.h"

/* Signal numbers */
#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGFPE 8
#define SIGKILL 9
#define SIGSEGV 11
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGUSR1 10
#define SIGUSR2 12
#define SIGCHLD 17
#define SIGCONT 18
#define SIGSTOP 19

#define MAX_SIGNALS 32

/* Signal handler type */
typedef void (*signal_handler_t)(int sig);

/* Signal actions */
#define SIG_DFL ((signal_handler_t)0)
#define SIG_IGN ((signal_handler_t)1)

/* Per-process signal state */
typedef struct {
  uint32_t pending; /* Pending signals bitmap */
  uint32_t blocked; /* Blocked signals bitmap */
  signal_handler_t handlers[MAX_SIGNALS];
} signal_state_t;

/* Signal states for all processes */
#define MAX_PROCESSES 64
static signal_state_t signal_states[MAX_PROCESSES];

/* External process functions */
extern process_t *proc_get_current(void);
extern uint32_t proc_get_pid(void);

/*
 * Initialize signal subsystem
 */
void signal_init(void) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    signal_states[i].pending = 0;
    signal_states[i].blocked = 0;
    for (int j = 0; j < MAX_SIGNALS; j++) {
      signal_states[i].handlers[j] = SIG_DFL;
    }
  }
}

/*
 * Set signal handler
 */
signal_handler_t signal_set(int sig, signal_handler_t handler) {
  if (sig < 1 || sig >= MAX_SIGNALS)
    return SIG_DFL;
  if (sig == SIGKILL || sig == SIGSTOP)
    return SIG_DFL; /* Cannot catch */

  uint32_t pid = proc_get_pid();
  if (pid >= MAX_PROCESSES)
    return SIG_DFL;

  signal_handler_t old = signal_states[pid].handlers[sig];
  signal_states[pid].handlers[sig] = handler;

  return old;
}

/*
 * Send signal to process
 */
int signal_send(uint32_t pid, int sig) {
  if (pid >= MAX_PROCESSES)
    return -1;
  if (sig < 1 || sig >= MAX_SIGNALS)
    return -1;

  signal_states[pid].pending |= (1 << sig);

  return 0;
}

/*
 * Block signals
 */
uint32_t signal_block(uint32_t mask) {
  uint32_t pid = proc_get_pid();
  if (pid >= MAX_PROCESSES)
    return 0;

  uint32_t old = signal_states[pid].blocked;
  signal_states[pid].blocked |= mask;

  /* Cannot block SIGKILL or SIGSTOP */
  signal_states[pid].blocked &= ~((1 << SIGKILL) | (1 << SIGSTOP));

  return old;
}

/*
 * Unblock signals
 */
uint32_t signal_unblock(uint32_t mask) {
  uint32_t pid = proc_get_pid();
  if (pid >= MAX_PROCESSES)
    return 0;

  uint32_t old = signal_states[pid].blocked;
  signal_states[pid].blocked &= ~mask;

  return old;
}

/*
 * Check and deliver pending signals
 * Called from scheduler or syscall return
 */
void signal_check(void) {
  uint32_t pid = proc_get_pid();
  if (pid >= MAX_PROCESSES)
    return;

  signal_state_t *state = &signal_states[pid];

  /* Find unblocked pending signal */
  uint32_t deliverable = state->pending & ~state->blocked;

  if (deliverable == 0)
    return;

  /* Find highest priority signal */
  for (int sig = 1; sig < MAX_SIGNALS; sig++) {
    if (!(deliverable & (1 << sig)))
      continue;

    /* Clear pending bit */
    state->pending &= ~(1 << sig);

    /* Get handler */
    signal_handler_t handler = state->handlers[sig];

    if (handler == SIG_IGN) {
      continue; /* Ignored */
    } else if (handler == SIG_DFL) {
      /* Default action */
      switch (sig) {
      case SIGCHLD:
      case SIGCONT:
        /* Ignore by default */
        break;

      case SIGSTOP:
        /* Stop process */
        /* TODO: Implement process stop */
        break;

      default:
        /* Terminate */
        proc_exit(128 + sig);
        break;
      }
    } else {
      /* Custom handler */
      handler(sig);
    }

    break; /* Only deliver one signal at a time */
  }
}

/*
 * Raise signal in current process
 */
int signal_raise(int sig) { return signal_send(proc_get_pid(), sig); }

/*
 * Kill command - send signal to process
 */
void cmd_kill(const char *args) {
  int sig = SIGTERM;
  uint32_t pid = 0;

  /* Parse arguments */
  const char *p = args;

  /* Check for signal number */
  if (*p == '-') {
    p++;
    sig = 0;
    while (*p >= '0' && *p <= '9') {
      sig = sig * 10 + (*p++ - '0');
    }
    while (*p == ' ')
      p++;
  }

  /* Parse PID */
  while (*p >= '0' && *p <= '9') {
    pid = pid * 10 + (*p++ - '0');
  }

  if (pid == 0) {
    kprintf("Usage: kill [-signal] pid\n");
    return;
  }

  if (signal_send(pid, sig) == 0) {
    kprintf("Sent signal %d to PID %d\n", sig, pid);
  } else {
    kprintf("Failed to send signal\n");
  }
}
