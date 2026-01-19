/*
 * NanoSec OS - Kernel Firewall
 */

#include "../kernel.h"

#define MAX_BLOCKED_IPS 64

static uint32_t blocked_ips[MAX_BLOCKED_IPS];
static int num_blocked = 0;
static int firewall_enabled = 1;

static struct {
  uint32_t packets_allowed;
  uint32_t packets_denied;
} fw_stats;

int firewall_init(void) {
  num_blocked = 0;
  fw_stats.packets_allowed = 0;
  fw_stats.packets_denied = 0;
  firewall_enabled = 1;
  return 0;
}

void firewall_block_ip(uint32_t ip) {
  if (num_blocked < MAX_BLOCKED_IPS) {
    blocked_ips[num_blocked++] = ip;
  }
}

int firewall_check_packet(void *packet, size_t len) {
  (void)packet;
  (void)len;

  if (!firewall_enabled) {
    fw_stats.packets_allowed++;
    return 1;
  }

  /* Simplified: just count */
  fw_stats.packets_allowed++;
  return 1;
}

void firewall_status(void) {
  kprintf("\n=== Firewall Status ===\n");
  kprintf("Status: %s\n", firewall_enabled ? "ENABLED" : "DISABLED");
  kprintf("Blocked IPs: %d\n", num_blocked);
  kprintf("Packets allowed: %d\n", (int)fw_stats.packets_allowed);
  kprintf("Packets denied: %d\n", (int)fw_stats.packets_denied);
}

void firewall_enable(int enable) { firewall_enabled = enable; }
