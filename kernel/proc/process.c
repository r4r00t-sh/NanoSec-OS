/*
 * NanoSec OS - Process Management
 * =================================
 * Process creation, destruction, and scheduling
 */

#include "process.h"
#include "../cpu/idt.h"
#include "../kernel.h"


/* Process table */
static process_t proc_table[MAX_PROCESSES];
static uint32_t next_pid = 1;

/* Current and idle processes */
process_t *current_process = NULL;
static process_t *idle_process = NULL;

/* Ready queue (simple linked list) */
static process_t *ready_queue_head = NULL;
static process_t *ready_queue_tail = NULL;

/* Kernel stacks for processes */
static uint8_t proc_stacks[MAX_PROCESSES][PROC_STACK_SIZE]
    __attribute__((aligned(16)));

/*
 * Initialize process subsystem
 */
void proc_init(void) {
  /* Clear process table */
  for (int i = 0; i < MAX_PROCESSES; i++) {
    proc_table[i].state = PROC_UNUSED;
    proc_table[i].pid = 0;
  }

  /* Create the kernel/idle process (PID 0) */
  idle_process = &proc_table[0];
  idle_process->pid = 0;
  idle_process->ppid = 0;
  idle_process->state = PROC_RUNNING;
  idle_process->priority = 0;
  idle_process->stack_bottom = (uint32_t)&proc_stacks[0][PROC_STACK_SIZE];
  idle_process->esp = idle_process->stack_bottom;
  strcpy(idle_process->name, "kernel");
  idle_process->next = NULL;

  current_process = idle_process;

  kprintf("  [OK] Process Manager\n");
}

/*
 * Find a free process slot
 */
static process_t *proc_alloc(void) {
  for (int i = 1; i < MAX_PROCESSES; i++) {
    if (proc_table[i].state == PROC_UNUSED) {
      return &proc_table[i];
    }
  }
  return NULL;
}

/*
 * Create a new process
 */
process_t *proc_create(const char *name, void (*entry)(void)) {
  process_t *proc = proc_alloc();
  if (!proc) {
    return NULL;
  }

  int index = proc - proc_table;

  proc->pid = next_pid++;
  proc->ppid = current_process ? current_process->pid : 0;
  proc->state = PROC_CREATED;
  proc->priority = 1;
  proc->time_slice = 10; /* 10 timer ticks */
  proc->total_time = 0;
  proc->page_dir = 0; /* Use kernel page directory for now */

  strncpy(proc->name, name, 31);
  proc->name[31] = '\0';

  /* Set up stack */
  proc->stack_bottom = (uint32_t)&proc_stacks[index][PROC_STACK_SIZE];
  uint32_t *stack = (uint32_t *)proc->stack_bottom;

  /* Push initial context onto stack */
  /* This mimics what happens during an interrupt */
  *(--stack) = 0x202;           /* EFLAGS (IF set) */
  *(--stack) = 0x08;            /* CS */
  *(--stack) = (uint32_t)entry; /* EIP - entry point */
  *(--stack) = 0;               /* Error code */
  *(--stack) = 0;               /* Interrupt number */
  *(--stack) = 0;               /* EAX */
  *(--stack) = 0;               /* ECX */
  *(--stack) = 0;               /* EDX */
  *(--stack) = 0;               /* EBX */
  *(--stack) = 0;               /* Dummy ESP */
  *(--stack) = 0;               /* EBP */
  *(--stack) = 0;               /* ESI */
  *(--stack) = 0;               /* EDI */
  *(--stack) = 0x10;            /* DS */

  proc->esp = (uint32_t)stack;

  /* Add to ready queue */
  proc->state = PROC_READY;
  scheduler_add(proc);

  return proc;
}

/*
 * Destroy a process
 */
void proc_destroy(process_t *proc) {
  if (!proc || proc == idle_process) {
    return;
  }

  scheduler_remove(proc);
  proc->state = PROC_UNUSED;
  proc->pid = 0;
}

/*
 * Voluntarily yield CPU
 */
void proc_yield(void) { schedule(); }

/*
 * Exit current process
 */
void proc_exit(int status) {
  (void)status;

  if (current_process && current_process != idle_process) {
    current_process->state = PROC_ZOMBIE;
    schedule();
  }
}

/*
 * Get current process
 */
process_t *proc_get_current(void) { return current_process; }

/*
 * Get current PID
 */
uint32_t proc_get_pid(void) {
  return current_process ? current_process->pid : 0;
}

/*
 * Add process to ready queue
 */
void scheduler_add(process_t *proc) {
  proc->next = NULL;

  if (!ready_queue_head) {
    ready_queue_head = proc;
    ready_queue_tail = proc;
  } else {
    ready_queue_tail->next = proc;
    ready_queue_tail = proc;
  }
}

/*
 * Remove process from ready queue
 */
void scheduler_remove(process_t *proc) {
  process_t *prev = NULL;
  process_t *curr = ready_queue_head;

  while (curr) {
    if (curr == proc) {
      if (prev) {
        prev->next = curr->next;
      } else {
        ready_queue_head = curr->next;
      }

      if (curr == ready_queue_tail) {
        ready_queue_tail = prev;
      }

      curr->next = NULL;
      return;
    }
    prev = curr;
    curr = curr->next;
  }
}

/*
 * Select next process and switch
 */
void schedule(void) {
  process_t *next = NULL;

  /* Find next ready process */
  if (ready_queue_head) {
    next = ready_queue_head;
    ready_queue_head = next->next;

    if (!ready_queue_head) {
      ready_queue_tail = NULL;
    }
    next->next = NULL;
  }

  /* If no ready process, run idle */
  if (!next) {
    next = idle_process;
  }

  /* If same process, no switch needed */
  if (next == current_process) {
    return;
  }

  /* Put current back on queue if still runnable */
  if (current_process && current_process->state == PROC_RUNNING) {
    current_process->state = PROC_READY;
    scheduler_add(current_process);
  }

  /* Switch to new process */
  process_t *old = current_process;
  current_process = next;
  current_process->state = PROC_RUNNING;

  /* Perform context switch */
  if (old) {
    switch_context(&old->esp, next->esp);
  }
}

/*
 * Timer interrupt handler - preemptive scheduling
 */
static void timer_handler(interrupt_frame_t *frame) {
  (void)frame;

  /* Decrement time slice */
  if (current_process && current_process->time_slice > 0) {
    current_process->time_slice--;
    current_process->total_time++;
  }

  /* If time slice expired, reschedule */
  if (current_process && current_process->time_slice == 0) {
    current_process->time_slice = 10; /* Reset time slice */
    schedule();
  }
}

/*
 * Initialize scheduler
 */
void scheduler_init(void) {
  /* Register timer interrupt handler */
  isr_register_handler(IRQ0, timer_handler);

  kprintf("  [OK] Scheduler (10ms quantum)\n");
}
