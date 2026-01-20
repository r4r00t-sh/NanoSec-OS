/*
 * NanoSec OS - IP Layer
 * =======================
 * IPv4 packet handling
 */

#include "../kernel.h"
#include "network.h"

/*
 * Handle incoming IP packet
 */
void ip_handle(const uint8_t *packet, size_t len) {
  if (len < 20)
    return;

  uint8_t version = (packet[0] >> 4) & 0x0F;
  uint8_t ihl = packet[0] & 0x0F;

  if (version != 4)
    return; /* Only IPv4 */

  size_t header_len = ihl * 4;
  if (len < header_len)
    return;

  uint8_t protocol = packet[9];

  uint32_t src_ip = ((uint32_t)packet[12] << 24) |
                    ((uint32_t)packet[13] << 16) | ((uint32_t)packet[14] << 8) |
                    packet[15];

  uint32_t dest_ip = ((uint32_t)packet[16] << 24) |
                     ((uint32_t)packet[17] << 16) |
                     ((uint32_t)packet[18] << 8) | packet[19];

  /* Check if packet is for us */
  uint32_t our_ip = net_get_ip();
  if (dest_ip != our_ip && dest_ip != 0xFFFFFFFF) {
    return; /* Not for us */
  }

  const uint8_t *payload = packet + header_len;
  size_t payload_len = len - header_len;

  switch (protocol) {
  case 1: /* ICMP */
    icmp_handle(payload, payload_len, src_ip);
    break;
  case 17: /* UDP */
    udp_handle(payload, payload_len, src_ip);
    break;
  case 6: /* TCP - not implemented */
    break;
  }
}

/*
 * Send IP packet
 */
int ip_send(uint32_t dest_ip, uint8_t protocol, const uint8_t *data,
            size_t len) {
  if (len > 1480)
    return -1; /* Too large */

  uint8_t packet[1514];
  uint8_t dest_mac[6];

  /* Resolve destination MAC */
  if (arp_resolve(dest_ip, dest_mac, 1000) != 0) {
    return -1;
  }

  uint8_t our_mac[6];
  net_get_mac(our_mac);
  uint32_t our_ip = net_get_ip();

  static uint16_t ip_id = 0;

  /* Ethernet header */
  memcpy(packet, dest_mac, 6);
  memcpy(packet + 6, our_mac, 6);
  packet[12] = 0x08;
  packet[13] = 0x00;

  /* IP header */
  uint8_t *ip = packet + 14;
  uint16_t total_len = 20 + len;

  ip[0] = 0x45;
  ip[1] = 0x00;
  ip[2] = (total_len >> 8) & 0xFF;
  ip[3] = total_len & 0xFF;
  ip[4] = (ip_id >> 8) & 0xFF;
  ip[5] = ip_id & 0xFF;
  ip_id++;
  ip[6] = 0x00;
  ip[7] = 0x00;
  ip[8] = 64; /* TTL */
  ip[9] = protocol;
  ip[10] = 0;
  ip[11] = 0; /* Checksum */

  ip[12] = (our_ip >> 24) & 0xFF;
  ip[13] = (our_ip >> 16) & 0xFF;
  ip[14] = (our_ip >> 8) & 0xFF;
  ip[15] = our_ip & 0xFF;

  ip[16] = (dest_ip >> 24) & 0xFF;
  ip[17] = (dest_ip >> 16) & 0xFF;
  ip[18] = (dest_ip >> 8) & 0xFF;
  ip[19] = dest_ip & 0xFF;

  /* Calculate checksum */
  uint32_t sum = 0;
  for (int i = 0; i < 20; i += 2) {
    sum += (ip[i] << 8) | ip[i + 1];
  }
  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }
  uint16_t csum = ~sum;
  ip[10] = (csum >> 8) & 0xFF;
  ip[11] = csum & 0xFF;

  /* Copy payload */
  memcpy(packet + 34, data, len);

  return net_send(packet, 14 + 20 + len);
}
