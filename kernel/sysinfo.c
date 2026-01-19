/*
 * NanoSec OS - System Information
 * =================================
 * CPU detection and system info
 */

#include "kernel.h"

/* CPUID instruction wrapper */
static inline void cpuid(uint32_t code, uint32_t *eax, uint32_t *ebx,
                         uint32_t *ecx, uint32_t *edx) {
  asm volatile("cpuid"
               : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
               : "a"(code));
}

/* CPU info structure */
typedef struct {
  char vendor[13];
  char brand[49];
  uint32_t family;
  uint32_t model;
  uint32_t stepping;
  int has_fpu;
  int has_mmx;
  int has_sse;
  int has_sse2;
} cpu_info_t;

static cpu_info_t cpu_info;

/*
 * Detect CPU information
 */
void cpu_detect(void) {
  uint32_t eax, ebx, ecx, edx;

  /* Get vendor string */
  cpuid(0, &eax, &ebx, &ecx, &edx);
  *((uint32_t *)&cpu_info.vendor[0]) = ebx;
  *((uint32_t *)&cpu_info.vendor[4]) = edx;
  *((uint32_t *)&cpu_info.vendor[8]) = ecx;
  cpu_info.vendor[12] = '\0';

  /* Get CPU features */
  cpuid(1, &eax, &ebx, &ecx, &edx);
  cpu_info.stepping = eax & 0xF;
  cpu_info.model = (eax >> 4) & 0xF;
  cpu_info.family = (eax >> 8) & 0xF;

  cpu_info.has_fpu = (edx >> 0) & 1;
  cpu_info.has_mmx = (edx >> 23) & 1;
  cpu_info.has_sse = (edx >> 25) & 1;
  cpu_info.has_sse2 = (edx >> 26) & 1;

  /* Get brand string if available */
  cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
  if (eax >= 0x80000004) {
    cpuid(0x80000002, &eax, &ebx, &ecx, &edx);
    *((uint32_t *)&cpu_info.brand[0]) = eax;
    *((uint32_t *)&cpu_info.brand[4]) = ebx;
    *((uint32_t *)&cpu_info.brand[8]) = ecx;
    *((uint32_t *)&cpu_info.brand[12]) = edx;

    cpuid(0x80000003, &eax, &ebx, &ecx, &edx);
    *((uint32_t *)&cpu_info.brand[16]) = eax;
    *((uint32_t *)&cpu_info.brand[20]) = ebx;
    *((uint32_t *)&cpu_info.brand[24]) = ecx;
    *((uint32_t *)&cpu_info.brand[28]) = edx;

    cpuid(0x80000004, &eax, &ebx, &ecx, &edx);
    *((uint32_t *)&cpu_info.brand[32]) = eax;
    *((uint32_t *)&cpu_info.brand[36]) = ebx;
    *((uint32_t *)&cpu_info.brand[40]) = ecx;
    *((uint32_t *)&cpu_info.brand[44]) = edx;
    cpu_info.brand[48] = '\0';
  } else {
    strcpy(cpu_info.brand, "Unknown");
  }
}

/*
 * Print system information
 */
void cmd_sysinfo(const char *args) {
  (void)args;

  cpu_detect();

  kprintf("\n");
  kprintf_color("=== NanoSec System Information ===\n\n", VGA_COLOR_CYAN);

  /* OS Info */
  kprintf_color("Operating System:\n", VGA_COLOR_YELLOW);
  kprintf("  Name:      NanoSec OS\n");
  kprintf("  Version:   1.0.0 \"Sentinel\"\n");
  kprintf("  Type:      Custom (Not Linux!)\n");
  kprintf("  Arch:      x86 (i386)\n");
  kprintf("\n");

  /* CPU Info */
  kprintf_color("Processor:\n", VGA_COLOR_YELLOW);
  kprintf("  Vendor:    %s\n", cpu_info.vendor);
  kprintf("  Model:     %s\n", cpu_info.brand);
  kprintf("  Family:    %d  Model: %d  Stepping: %d\n", cpu_info.family,
          cpu_info.model, cpu_info.stepping);
  kprintf("  Features:  ");
  if (cpu_info.has_fpu)
    kprintf("FPU ");
  if (cpu_info.has_mmx)
    kprintf("MMX ");
  if (cpu_info.has_sse)
    kprintf("SSE ");
  if (cpu_info.has_sse2)
    kprintf("SSE2 ");
  kprintf("\n\n");

  /* Memory Info */
  size_t alloc, free_mem;
  mm_stats(&alloc, &free_mem);
  kprintf_color("Memory:\n", VGA_COLOR_YELLOW);
  kprintf("  Allocated: %d bytes\n", (int)alloc);
  kprintf("  Free:      %d bytes\n", (int)free_mem);
  kprintf("\n");

  /* Uptime */
  uint32_t ticks = timer_get_ticks();
  uint32_t secs = ticks / 100;
  uint32_t mins = secs / 60;
  kprintf_color("Uptime:\n", VGA_COLOR_YELLOW);
  kprintf("  %d minutes, %d seconds (%d ticks)\n", mins, secs % 60, ticks);
  kprintf("\n");
}

/*
 * Print process list (simplified - our kernel is single-tasking)
 */
void cmd_ps(const char *args) {
  (void)args;

  kprintf("\n");
  kprintf_color("=== Process List ===\n\n", VGA_COLOR_CYAN);
  kprintf("  PID  STATE    NAME\n");
  kprintf("  ---  -----    ----\n");
  kprintf("    0  running  kernel\n");
  kprintf("    1  running  shell\n");
  kprintf("\n");
  kprintf("Total: 2 processes\n");
  kprintf("(NanoSec is single-tasking; no preemptive multitasking yet)\n");
  kprintf("\n");
}

/*
 * Uptime command
 */
void cmd_uptime(const char *args) {
  (void)args;

  uint32_t ticks = timer_get_ticks();
  uint32_t secs = ticks / 100;
  uint32_t mins = secs / 60;
  uint32_t hours = mins / 60;

  kprintf("up ");
  if (hours > 0)
    kprintf("%d hour(s), ", hours);
  kprintf("%d min, %d sec\n", mins % 60, secs % 60);
}

/*
 * Date/time command (simulated - no RTC driver yet)
 */
void cmd_date(const char *args) {
  (void)args;
  kprintf("Sun Jan 19 15:45:00 IST 2025\n");
  kprintf_color("(RTC not implemented - showing static date)\n",
                VGA_COLOR_YELLOW);
}

/*
 * Who am I
 */
void cmd_whoami(const char *args) {
  (void)args;
  kprintf("root\n");
}

/*
 * Hostname
 */
void cmd_hostname(const char *args) {
  (void)args;
  kprintf("nanosec\n");
}

/*
 * uname - system info
 */
void cmd_uname(const char *args) {
  if (args[0] == '-' && args[1] == 'a') {
    kprintf("NanoSec nanosec 1.0.0 #1 SMP x86 Custom_Kernel\n");
  } else {
    kprintf("NanoSec\n");
  }
}
