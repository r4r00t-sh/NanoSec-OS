/*
 * NanoSec OS - Security Monitor
 */

#include "../kernel.h"

#define MAX_LOGS 64

typedef struct {
  uint32_t timestamp;
  int severity;
  char message[48];
} log_entry_t;

static log_entry_t logs[MAX_LOGS];
static int log_count = 0;
static int log_head = 0;
static int alert_count = 0;
static int secmon_enabled = 1;

int secmon_init(void) {
  log_count = 0;
  log_head = 0;
  alert_count = 0;
  secmon_enabled = 1;
  secmon_log("Security monitor initialized", 0);
  return 0;
}

void secmon_log(const char *event, int severity) {
  if (!secmon_enabled)
    return;

  log_entry_t *entry = &logs[log_head];
  entry->timestamp = timer_get_ticks();
  entry->severity = severity;
  strncpy(entry->message, event, 47);
  entry->message[47] = '\0';

  log_head = (log_head + 1) % MAX_LOGS;
  if (log_count < MAX_LOGS)
    log_count++;

  if (severity >= 2) {
    secmon_alert(event);
  }
}

void secmon_alert(const char *message) {
  alert_count++;
  kprintf_color("\n[ALERT] ", VGA_COLOR_RED);
  kprintf("%s\n", message);
}

int secmon_get_alert_count(void) { return alert_count; }

void secmon_acknowledge_alerts(void) { alert_count = 0; }

void secmon_status(void) {
  kprintf("\n=== Security Monitor ===\n");
  kprintf("Status: %s\n", secmon_enabled ? "ACTIVE" : "DISABLED");
  kprintf("Log entries: %d\n", log_count);
  kprintf("Alerts: %d\n", alert_count);
}

void secmon_show_logs(int count) {
  if (count > log_count)
    count = log_count;

  kprintf("\n=== Recent Events ===\n");

  int start = (log_head - count + MAX_LOGS) % MAX_LOGS;
  for (int i = 0; i < count; i++) {
    int idx = (start + i) % MAX_LOGS;
    log_entry_t *e = &logs[idx];

    const char *sev = e->severity == 0   ? "INFO"
                      : e->severity == 1 ? "WARN"
                                         : "CRIT";
    kprintf("[%s] %s\n", sev, e->message);
  }
}

void secmon_enable(int enable) {
  secmon_enabled = enable;
  kprintf("Security monitor %s\n", enable ? "enabled" : "disabled");
}
