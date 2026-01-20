/*
 * NanoSec OS - ARP Protocol
 * ==========================
 * Address Resolution Protocol - IP to MAC mapping
 */

#include "../kernel.h"
#include "network.h"

/* ARP cache */
#define ARP_CACHE_SIZE 16
#define ARP_TIMEOUT 300 /* 5 minutes in seconds */

typedef struct {
  uint32_t ip;
  uint8_t mac[6];
  uint32_t timestamp;
  uint8_t valid;
} arp_entry_t;

static arp_entry_t arp_cache[ARP_CACHE_SIZE];

/* Our network config */
static uint32_t our_ip = 0x0A000001; /* 10.0.0.1 */
static uint8_t our_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
static uint32_t gateway_ip = 0x0A000002; /* 10.0.0.2 */
static uint32_t netmask = 0xFFFFFF00;    /* 255.255.255.0 */

/*
 * Initialize ARP
 */
void arp_init(void) { memset(arp_cache, 0, sizeof(arp_cache)); }

/*
 * Set network configuration
 */
void net_set_ip(uint32_t ip) { our_ip = ip; }
void net_set_gateway(uint32_t gw) { gateway_ip = gw; }
void net_set_netmask(uint32_t mask) { netmask = mask; }
uint32_t net_get_ip(void) { return our_ip; }
uint32_t net_get_gateway(void) { return gateway_ip; }

void net_set_mac(const uint8_t *mac) { memcpy(our_mac, mac, 6); }

void net_get_mac(uint8_t *mac) { memcpy(mac, our_mac, 6); }

/*
 * Look up MAC address in ARP cache
 */
int arp_lookup(uint32_t ip, uint8_t *mac) {
  for (int i = 0; i < ARP_CACHE_SIZE; i++) {
    if (arp_cache[i].valid && arp_cache[i].ip == ip) {
      memcpy(mac, arp_cache[i].mac, 6);
      return 0;
    }
  }
  return -1; /* Not found */
}

/*
 * Add entry to ARP cache
 */
void arp_add(uint32_t ip, const uint8_t *mac) {
  /* Check if already exists */
  for (int i = 0; i < ARP_CACHE_SIZE; i++) {
    if (arp_cache[i].valid && arp_cache[i].ip == ip) {
      memcpy(arp_cache[i].mac, mac, 6);
      arp_cache[i].timestamp = timer_get_ticks();
      return;
    }
  }

  /* Find empty or oldest slot */
  int oldest = 0;
  uint32_t oldest_time = 0xFFFFFFFF;

  for (int i = 0; i < ARP_CACHE_SIZE; i++) {
    if (!arp_cache[i].valid) {
      oldest = i;
      break;
    }
    if (arp_cache[i].timestamp < oldest_time) {
      oldest_time = arp_cache[i].timestamp;
      oldest = i;
    }
  }

  arp_cache[oldest].ip = ip;
  memcpy(arp_cache[oldest].mac, mac, 6);
  arp_cache[oldest].timestamp = timer_get_ticks();
  arp_cache[oldest].valid = 1;
}

/*
 * Build and send ARP request
 */
void arp_request(uint32_t target_ip) {
  uint8_t packet[42]; /* Ethernet header (14) + ARP (28) */

  /* Ethernet header - broadcast */
  memset(packet, 0xFF, 6);        /* Dest MAC: broadcast */
  memcpy(packet + 6, our_mac, 6); /* Src MAC */
  packet[12] = 0x08;
  packet[13] = 0x06; /* EtherType: ARP */

  /* ARP header */
  uint8_t *arp = packet + 14;
  arp[0] = 0x00;
  arp[1] = 0x01; /* Hardware type: Ethernet */
  arp[2] = 0x08;
  arp[3] = 0x00; /* Protocol type: IPv4 */
  arp[4] = 6;    /* Hardware size */
  arp[5] = 4;    /* Protocol size */
  arp[6] = 0x00;
  arp[7] = 0x01; /* Opcode: Request */

  memcpy(arp + 8, our_mac, 6);     /* Sender MAC */
  arp[14] = (our_ip >> 24) & 0xFF; /* Sender IP */
  arp[15] = (our_ip >> 16) & 0xFF;
  arp[16] = (our_ip >> 8) & 0xFF;
  arp[17] = our_ip & 0xFF;

  memset(arp + 18, 0, 6);             /* Target MAC: unknown */
  arp[24] = (target_ip >> 24) & 0xFF; /* Target IP */
  arp[25] = (target_ip >> 16) & 0xFF;
  arp[26] = (target_ip >> 8) & 0xFF;
  arp[27] = target_ip & 0xFF;

  /* Send packet */
  net_send(packet, 42);
}

/*
 * Handle incoming ARP packet
 */
void arp_handle(const uint8_t *packet, size_t len) {
  if (len < 28)
    return;

  uint16_t opcode = (packet[6] << 8) | packet[7];

  /* Extract sender info */
  uint8_t sender_mac[6];
  memcpy(sender_mac, packet + 8, 6);

  uint32_t sender_ip = ((uint32_t)packet[14] << 24) |
                       ((uint32_t)packet[15] << 16) |
                       ((uint32_t)packet[16] << 8) | packet[17];

  uint32_t target_ip = ((uint32_t)packet[24] << 24) |
                       ((uint32_t)packet[25] << 16) |
                       ((uint32_t)packet[26] << 8) | packet[27];

  /* Add sender to cache */
  arp_add(sender_ip, sender_mac);

  if (opcode == 1 && target_ip == our_ip) {
    /* ARP Request for us - send reply */
    uint8_t reply[42];

    /* Ethernet header */
    memcpy(reply, sender_mac, 6);  /* Dest MAC */
    memcpy(reply + 6, our_mac, 6); /* Src MAC */
    reply[12] = 0x08;
    reply[13] = 0x06; /* EtherType: ARP */

    /* ARP reply */
    uint8_t *arp = reply + 14;
    arp[0] = 0x00;
    arp[1] = 0x01;
    arp[2] = 0x08;
    arp[3] = 0x00;
    arp[4] = 6;
    arp[5] = 4;
    arp[6] = 0x00;
    arp[7] = 0x02; /* Opcode: Reply */

    memcpy(arp + 8, our_mac, 6); /* Sender MAC (us) */
    arp[14] = (our_ip >> 24) & 0xFF;
    arp[15] = (our_ip >> 16) & 0xFF;
    arp[16] = (our_ip >> 8) & 0xFF;
    arp[17] = our_ip & 0xFF;

    memcpy(arp + 18, sender_mac, 6); /* Target MAC */
    arp[24] = (sender_ip >> 24) & 0xFF;
    arp[25] = (sender_ip >> 16) & 0xFF;
    arp[26] = (sender_ip >> 8) & 0xFF;
    arp[27] = sender_ip & 0xFF;

    net_send(reply, 42);
  }
}

/*
 * Resolve IP to MAC (with ARP request if needed)
 */
int arp_resolve(uint32_t ip, uint8_t *mac, int timeout_ms) {
  /* Check if on same subnet */
  if ((ip & netmask) != (our_ip & netmask)) {
    /* Use gateway */
    ip = gateway_ip;
  }

  /* Check cache first */
  if (arp_lookup(ip, mac) == 0) {
    return 0;
  }

  /* Send ARP request */
  arp_request(ip);

  /* Wait for reply */
  for (int i = 0; i < timeout_ms / 10; i++) {
    net_poll(); /* Process incoming packets */

    if (arp_lookup(ip, mac) == 0) {
      return 0;
    }

    /* Small delay */
    for (volatile int j = 0; j < 10000; j++)
      ;
  }

  return -1; /* Timeout */
}

/*
 * Show ARP cache
 */
void arp_show_cache(void) {
  kprintf("\n=== ARP Cache ===\n");
  kprintf("IP Address       MAC Address\n");
  kprintf("---------------  -----------------\n");

  int count = 0;
  for (int i = 0; i < ARP_CACHE_SIZE; i++) {
    if (arp_cache[i].valid) {
      uint32_t ip = arp_cache[i].ip;
      uint8_t *mac = arp_cache[i].mac;

      kprintf("%d.%d.%d.%d    %02x:%02x:%02x:%02x:%02x:%02x\n",
              (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF,
              mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      count++;
    }
  }

  if (count == 0) {
    kprintf("(empty)\n");
  }
  kprintf("\n");
}
