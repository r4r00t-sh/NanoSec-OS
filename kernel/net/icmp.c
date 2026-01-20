/*
 * NanoSec OS - ICMP Protocol
 * ===========================
 * Internet Control Message Protocol - ping support
 */

#include "../kernel.h"
#include "network.h"

/* ICMP types */
#define ICMP_ECHO_REPLY 0
#define ICMP_ECHO_REQUEST 8

/* Ping statistics */
static volatile int ping_received = 0;
static volatile uint32_t ping_rtt = 0;
static volatile uint16_t ping_seq = 0;

/*
 * Calculate IP checksum
 */
static uint16_t ip_checksum(const uint8_t *data, size_t len) {
  uint32_t sum = 0;

  for (size_t i = 0; i < len - 1; i += 2) {
    sum += (data[i] << 8) | data[i + 1];
  }

  if (len & 1) {
    sum += data[len - 1] << 8;
  }

  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  return ~sum;
}

/*
 * Send ICMP echo request (ping)
 */
int icmp_ping(uint32_t dest_ip, uint16_t seq, uint32_t *rtt) {
  uint8_t packet[74]; /* Ethernet(14) + IP(20) + ICMP(8) + Data(32) */
  uint8_t dest_mac[6];

  /* Resolve MAC address */
  if (arp_resolve(dest_ip, dest_mac, 1000) != 0) {
    return -1; /* ARP failed */
  }

  /* Get our MAC */
  uint8_t our_mac[6];
  net_get_mac(our_mac);
  uint32_t our_ip = net_get_ip();

  /* Ethernet header */
  memcpy(packet, dest_mac, 6);
  memcpy(packet + 6, our_mac, 6);
  packet[12] = 0x08;
  packet[13] = 0x00; /* IPv4 */

  /* IP header */
  uint8_t *ip = packet + 14;
  ip[0] = 0x45; /* Version 4, IHL 5 */
  ip[1] = 0x00; /* DSCP/ECN */
  ip[2] = 0x00;
  ip[3] = 60; /* Total length */
  ip[4] = 0x00;
  ip[5] = seq & 0xFF; /* Identification */
  ip[6] = 0x00;
  ip[7] = 0x00; /* Flags/Fragment */
  ip[8] = 64;   /* TTL */
  ip[9] = 1;    /* Protocol: ICMP */
  ip[10] = 0x00;
  ip[11] = 0x00; /* Checksum (calculated below) */

  ip[12] = (our_ip >> 24) & 0xFF; /* Source IP */
  ip[13] = (our_ip >> 16) & 0xFF;
  ip[14] = (our_ip >> 8) & 0xFF;
  ip[15] = our_ip & 0xFF;

  ip[16] = (dest_ip >> 24) & 0xFF; /* Dest IP */
  ip[17] = (dest_ip >> 16) & 0xFF;
  ip[18] = (dest_ip >> 8) & 0xFF;
  ip[19] = dest_ip & 0xFF;

  /* IP checksum */
  uint16_t ip_csum = ip_checksum(ip, 20);
  ip[10] = (ip_csum >> 8) & 0xFF;
  ip[11] = ip_csum & 0xFF;

  /* ICMP header */
  uint8_t *icmp = packet + 34;
  icmp[0] = ICMP_ECHO_REQUEST; /* Type */
  icmp[1] = 0;                 /* Code */
  icmp[2] = 0;
  icmp[3] = 0; /* Checksum (calculated below) */
  icmp[4] = 0x12;
  icmp[5] = 0x34;              /* Identifier */
  icmp[6] = (seq >> 8) & 0xFF; /* Sequence */
  icmp[7] = seq & 0xFF;

  /* ICMP data - timestamp */
  uint32_t timestamp = timer_get_ticks();
  for (int i = 0; i < 32; i++) {
    icmp[8 + i] =
        (i < 4) ? ((timestamp >> (8 * (3 - i))) & 0xFF) : 'A' + (i % 26);
  }

  /* ICMP checksum */
  uint16_t icmp_csum = ip_checksum(icmp, 40);
  icmp[2] = (icmp_csum >> 8) & 0xFF;
  icmp[3] = icmp_csum & 0xFF;

  /* Send packet */
  ping_received = 0;
  ping_seq = seq;

  net_send(packet, 74);

  /* Wait for reply */
  uint32_t start = timer_get_ticks();
  uint32_t timeout = start + 3000; /* 3 second timeout */

  while (timer_get_ticks() < timeout) {
    net_poll();

    if (ping_received && ping_seq == seq) {
      *rtt = ping_rtt;
      return 0;
    }
  }

  return -2; /* Timeout */
}

/*
 * Handle incoming ICMP packet
 */
void icmp_handle(const uint8_t *packet, size_t len, uint32_t src_ip) {
  if (len < 8)
    return;

  uint8_t type = packet[0];
  uint8_t code = packet[1];
  uint16_t seq = (packet[6] << 8) | packet[7];

  if (type == ICMP_ECHO_REPLY && code == 0) {
    /* Echo reply - calculate RTT */
    if (len >= 12) {
      uint32_t sent_time = ((uint32_t)packet[8] << 24) |
                           ((uint32_t)packet[9] << 16) |
                           ((uint32_t)packet[10] << 8) | packet[11];
      ping_rtt = timer_get_ticks() - sent_time;
    } else {
      ping_rtt = 0;
    }
    ping_seq = seq;
    ping_received = 1;
  } else if (type == ICMP_ECHO_REQUEST && code == 0) {
    /* Echo request - send reply */
    icmp_send_reply(src_ip, packet, len);
  }
}

/*
 * Send ICMP echo reply
 */
void icmp_send_reply(uint32_t dest_ip, const uint8_t *request, size_t req_len) {
  size_t total_len = 14 + 20 + req_len;
  if (total_len > 1500)
    return;

  uint8_t packet[1500];
  uint8_t dest_mac[6];

  /* Resolve MAC */
  if (arp_resolve(dest_ip, dest_mac, 500) != 0) {
    return;
  }

  uint8_t our_mac[6];
  net_get_mac(our_mac);
  uint32_t our_ip = net_get_ip();

  /* Ethernet header */
  memcpy(packet, dest_mac, 6);
  memcpy(packet + 6, our_mac, 6);
  packet[12] = 0x08;
  packet[13] = 0x00;

  /* IP header */
  uint8_t *ip = packet + 14;
  ip[0] = 0x45;
  ip[1] = 0x00;
  uint16_t ip_len = 20 + req_len;
  ip[2] = (ip_len >> 8) & 0xFF;
  ip[3] = ip_len & 0xFF;
  ip[4] = 0x00;
  ip[5] = 0x00;
  ip[6] = 0x00;
  ip[7] = 0x00;
  ip[8] = 64;
  ip[9] = 1; /* ICMP */
  ip[10] = 0;
  ip[11] = 0;

  ip[12] = (our_ip >> 24) & 0xFF;
  ip[13] = (our_ip >> 16) & 0xFF;
  ip[14] = (our_ip >> 8) & 0xFF;
  ip[15] = our_ip & 0xFF;

  ip[16] = (dest_ip >> 24) & 0xFF;
  ip[17] = (dest_ip >> 16) & 0xFF;
  ip[18] = (dest_ip >> 8) & 0xFF;
  ip[19] = dest_ip & 0xFF;

  uint16_t csum = ip_checksum(ip, 20);
  ip[10] = (csum >> 8) & 0xFF;
  ip[11] = csum & 0xFF;

  /* ICMP reply - copy request but change type */
  uint8_t *icmp = packet + 34;
  memcpy(icmp, request, req_len);
  icmp[0] = ICMP_ECHO_REPLY;
  icmp[2] = 0;
  icmp[3] = 0; /* Clear checksum */

  csum = ip_checksum(icmp, req_len);
  icmp[2] = (csum >> 8) & 0xFF;
  icmp[3] = csum & 0xFF;

  net_send(packet, total_len);
}

/*
 * Ping command implementation
 */
void cmd_ping_real(const char *args) {
  if (args[0] == '\0') {
    kprintf("Usage: ping <ip>\n");
    kprintf("Example: ping 127.0.0.1 (loopback)\n");
    kprintf("         ping 10.0.2.2 (QEMU gateway)\n");
    return;
  }

  /* Parse IP */
  uint32_t ip = 0;
  int val = 0;
  const char *p = args;

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

  kprintf("PING %d.%d.%d.%d\n", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
          (ip >> 8) & 0xFF, ip & 0xFF);

  int sent = 0, received = 0;
  uint32_t total_rtt = 0;

  /* Check for loopback address */
  int is_loopback = ((ip >> 24) == 127); /* 127.x.x.x */
  int is_self = (ip == net_get_ip());    /* Our own IP */

  for (int i = 0; i < 4; i++) {
    uint32_t rtt;
    sent++;

    if (is_loopback || is_self) {
      /* Loopback - simulate instant response */
      rtt = 0;
      kprintf("Reply: seq=%d loopback\n", i + 1);
      received++;
    } else {
      /* Real network ping */
      int result = icmp_ping(ip, i + 1, &rtt);

      if (result == 0) {
        kprintf("Reply: seq=%d time=%dms\n", i + 1, rtt);
        received++;
        total_rtt += rtt;
      } else if (result == -1) {
        kprintf("ARP failed\n");
        break;
      } else {
        kprintf("Timeout\n");
      }
    }

    /* Delay between pings */
    for (volatile int j = 0; j < 100000; j++)
      ;
  }

  kprintf("\n--- statistics ---\n");
  kprintf("sent=%d recv=%d\n", sent, received);
}
