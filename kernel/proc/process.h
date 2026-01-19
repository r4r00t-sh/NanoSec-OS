/*
 * NanoSec OS - Process Management
 * =================================
 * Task structures and process control
 */

#ifndef _PROCESS_H
#define _PROCESS_H

#include <stdint.h>

/* Process states */
#define PROC_UNUSED 0
#define PROC_CREATED 1
#define PROC_READY 2
#define PROC_RUNNING 3
#define PROC_BLOCKED 4
#define PROC_ZOMBIE 5

/* Maximum processes */
#define MAX_PROCESSES 64

/* Stack size per process */
#define PROC_STACK_SIZE 4096

/* Task Control Block */
typedef struct process {
  uint32_t pid;  /* Process ID */
  uint32_t ppid; /* Parent PID */

  /* CPU state (saved during context switch) */
  uint32_t esp; /* Stack pointer */
  uint32_t ebp; /* Base pointer */
  uint32_t eip; /* Instruction pointer (for new processes) */

  /* Memory */
  uint32_t page_dir;     /* Page directory physical address */
  uint32_t stack_bottom; /* Bottom of kernel stack */

  /* State */
  uint8_t state;
  uint8_t priority;

  /* Scheduling */
  uint32_t time_slice; /* Remaining time slice */
  uint32_t total_time; /* Total CPU time used */

  /* Name */
  char name[32];

  /* Linked list for scheduler */
  struct process *next;
} process_t;

/* Current running process */
extern process_t *current_process;

/* Process functions */
void proc_init(void);
process_t *proc_create(const char *name, void (*entry)(void));
void proc_destroy(process_t *proc);
void proc_yield(void);
void proc_exit(int status);
process_t *proc_get_current(void);
uint32_t proc_get_pid(void);

/* Scheduler functions */
void scheduler_init(void);
void scheduler_add(process_t *proc);
void scheduler_remove(process_t *proc);
void schedule(void);

/* Context switch (assembly) */
extern void switch_context(uint32_t *old_esp, uint32_t new_esp);

#endif /* _PROCESS_H */
