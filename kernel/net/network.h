/*
 * NanoSec OS - Network Header
 * ============================
 * Network protocol definitions and function declarations
 */

#ifndef _NETWORK_H
#define _NETWORK_H

#include <stdint.h>

/* ============================================
 * Network Configuration
 * ============================================ */

void net_set_ip(uint32_t ip);
void net_set_gateway(uint32_t gw);
void net_set_netmask(uint32_t mask);
void net_set_mac(const uint8_t *mac);

uint32_t net_get_ip(void);
uint32_t net_get_gateway(void);
void net_get_mac(uint8_t *mac);

/* ============================================
 * Low-level Network Driver (NE2000)
 * ============================================ */

int net_init(void);
int net_send(const uint8_t *packet, size_t len);
void net_poll(void);
int net_receive(uint8_t *buffer, size_t max_len);

/* ============================================
 * ARP Protocol
 * ============================================ */

void arp_init(void);
int arp_lookup(uint32_t ip, uint8_t *mac);
void arp_add(uint32_t ip, const uint8_t *mac);
void arp_request(uint32_t target_ip);
int arp_resolve(uint32_t ip, uint8_t *mac, int timeout_ms);
void arp_handle(const uint8_t *packet, size_t len);
void arp_show_cache(void);

/* ============================================
 * IP Layer
 * ============================================ */

void ip_handle(const uint8_t *packet, size_t len);
int ip_send(uint32_t dest_ip, uint8_t protocol, const uint8_t *data,
            size_t len);

/* ============================================
 * ICMP Protocol
 * ============================================ */

int icmp_ping(uint32_t dest_ip, uint16_t seq, uint32_t *rtt);
void icmp_handle(const uint8_t *packet, size_t len, uint32_t src_ip);
void icmp_send_reply(uint32_t dest_ip, const uint8_t *request, size_t req_len);

/* ============================================
 * UDP Protocol
 * ============================================ */

void udp_init(void);
int udp_socket(uint16_t local_port);
void udp_close(int sock);
int udp_send(int sock, uint32_t dest_ip, uint16_t dest_port,
             const uint8_t *data, size_t len);
int udp_recv(int sock, uint8_t *buffer, size_t max_len, uint32_t *from_ip,
             uint16_t *from_port, int timeout_ms);
void udp_handle(const uint8_t *packet, size_t len, uint32_t src_ip);

/* ============================================
 * DNS Client
 * ============================================ */

void dns_set_server(uint32_t ip);
uint32_t dns_get_server(void);
int dns_lookup(const char *hostname, uint32_t *ip);

/* ============================================
 * Shell Commands
 * ============================================ */

void cmd_ping_real(const char *args);
void cmd_dns_real(const char *args);
int strncmp(const char *s1, const char *s2, size_t n);

/* ============================================
 * Ethernet Frame Structure
 * ============================================ */

typedef struct __attribute__((packed)) {
  uint8_t dest_mac[6];
  uint8_t src_mac[6];
  uint16_t ethertype;
} ethernet_header_t;

#define ETHERTYPE_IP 0x0800
#define ETHERTYPE_ARP 0x0806

/* ============================================
 * IP Header Structure
 * ============================================ */

typedef struct __attribute__((packed)) {
  uint8_t version_ihl;
  uint8_t dscp_ecn;
  uint16_t total_length;
  uint16_t identification;
  uint16_t flags_fragment;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t checksum;
  uint32_t src_ip;
  uint32_t dest_ip;
} ip_header_t;

#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

/* ============================================
 * ARP Header Structure
 * ============================================ */

typedef struct __attribute__((packed)) {
  uint16_t hw_type;
  uint16_t proto_type;
  uint8_t hw_size;
  uint8_t proto_size;
  uint16_t opcode;
  uint8_t sender_mac[6];
  uint8_t sender_ip[4];
  uint8_t target_mac[6];
  uint8_t target_ip[4];
} arp_header_t;

#define ARP_REQUEST 1
#define ARP_REPLY 2

#endif /* _NETWORK_H */
