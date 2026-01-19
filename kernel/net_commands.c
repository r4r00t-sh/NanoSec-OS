/*
 * NanoSec OS - Network Commands (Updated with Real Stack)
 * =========================================================
 */

#include "kernel.h"
#include "net/network.h"

/*
 * nifconfig - configure network interface
 */
void cmd_nifconfig(const char *args) {
  if (args[0] == '\0') {
    /* Show current config */
    uint8_t mac[6];
    net_get_mac(mac);
    uint32_t ip = net_get_ip();
    uint32_t gw = net_get_gateway();

    kprintf("\neth0:\n");
    kprintf("  MAC:     %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1],
            mac[2], mac[3], mac[4], mac[5]);
    kprintf("  IPv4:    %d.%d.%d.%d\n", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
            (ip >> 8) & 0xFF, ip & 0xFF);
    kprintf("  Gateway: %d.%d.%d.%d\n", (gw >> 24) & 0xFF, (gw >> 16) & 0xFF,
            (gw >> 8) & 0xFF, gw & 0xFF);
    kprintf("  DNS:     %d.%d.%d.%d\n", (dns_get_server() >> 24) & 0xFF,
            (dns_get_server() >> 16) & 0xFF, (dns_get_server() >> 8) & 0xFF,
            dns_get_server() & 0xFF);
    kprintf("\n");
    return;
  }

  /* Parse IP setting: nifconfig ip 10.0.0.2 */
  if (strncmp(args, "ip ", 3) == 0) {
    uint32_t ip = 0;
    int val = 0;
    const char *p = args + 3;

    while (*p) {
      if (*p >= '0' && *p <= '9') {
        val = val * 10 + (*p - '0');
      } else if (*p == '.') {
        ip = (ip << 8) | (val & 0xFF);
        val = 0;
      }
      p++;
    }
    ip = (ip << 8) | (val & 0xFF);

    net_set_ip(ip);
    kprintf("IP set to %d.%d.%d.%d\n", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
            (ip >> 8) & 0xFF, ip & 0xFF);
  } else if (strncmp(args, "gateway ", 8) == 0) {
    uint32_t gw = 0;
    int val = 0;
    const char *p = args + 8;

    while (*p) {
      if (*p >= '0' && *p <= '9') {
        val = val * 10 + (*p - '0');
      } else if (*p == '.') {
        gw = (gw << 8) | (val & 0xFF);
        val = 0;
      }
      p++;
    }
    gw = (gw << 8) | (val & 0xFF);

    net_set_gateway(gw);
    kprintf("Gateway set to %d.%d.%d.%d\n", (gw >> 24) & 0xFF,
            (gw >> 16) & 0xFF, (gw >> 8) & 0xFF, gw & 0xFF);
  } else {
    kprintf("Usage:\n");
    kprintf("  nifconfig              Show config\n");
    kprintf("  nifconfig ip X.X.X.X   Set IP\n");
    kprintf("  nifconfig gateway X.X.X.X\n");
  }
}

/*
 * nping - real ICMP ping
 */
void cmd_nping(const char *args) { cmd_ping_real(args); }

/*
 * narp - show/manage ARP cache
 */
void cmd_narp(const char *args) {
  if (args[0] == '\0') {
    arp_show_cache();
  } else {
    kprintf("Usage: narp (show ARP cache)\n");
  }
}

/*
 * ndns - DNS lookup
 */
void cmd_ndns(const char *args) { cmd_dns_real(args); }

/*
 * nroute - show routing table
 */
void cmd_nroute(const char *args) {
  (void)args;

  uint32_t ip = net_get_ip();
  uint32_t gw = net_get_gateway();

  kprintf("\nRouting Table:\n");
  kprintf("Destination     Gateway         Flags\n");
  kprintf("-----------     -------         -----\n");
  kprintf("%d.%d.%d.0/24   *               U\n", (ip >> 24) & 0xFF,
          (ip >> 16) & 0xFF, (ip >> 8) & 0xFF);
  kprintf("0.0.0.0         %d.%d.%d.%d   UG\n", (gw >> 24) & 0xFF,
          (gw >> 16) & 0xFF, (gw >> 8) & 0xFF, gw & 0xFF);
  kprintf("\n");
}

/*
 * nnetstat - network statistics
 */
void cmd_nnetstat(const char *args) {
  (void)args;

  kprintf("\nNetwork Statistics:\n");
  kprintf("==================\n");
  kprintf("Interface: eth0 (NE2000)\n");
  kprintf("Status:    UP\n");
  kprintf("\n");
}
