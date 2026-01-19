/*
 * NanoSec OS - Network Header
 */

#ifndef _NETWORK_H
#define _NETWORK_H

#include <stddef.h>
#include <stdint.h>


/* Network interface */
typedef struct {
  char name[8];
  uint8_t mac[6];
  uint32_t ip;
  uint32_t netmask;
  uint32_t gateway;
  uint32_t dns;
  uint8_t up;
} net_interface_t;

/* Ethernet header */
typedef struct __attribute__((packed)) {
  uint8_t dest[6];
  uint8_t src[6];
  uint16_t type;
} eth_header_t;

#define ETH_TYPE_IP 0x0800
#define ETH_TYPE_ARP 0x0806

/* IP header */
typedef struct __attribute__((packed)) {
  uint8_t version_ihl;
  uint8_t tos;
  uint16_t length;
  uint16_t id;
  uint16_t flags_frag;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t checksum;
  uint32_t src;
  uint32_t dest;
} ip_header_t;

#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

/* ICMP header */
typedef struct __attribute__((packed)) {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint16_t id;
  uint16_t seq;
} icmp_header_t;

#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY 0

/* ARP header */
typedef struct __attribute__((packed)) {
  uint16_t hw_type;
  uint16_t proto_type;
  uint8_t hw_len;
  uint8_t proto_len;
  uint16_t operation;
  uint8_t sender_mac[6];
  uint32_t sender_ip;
  uint8_t target_mac[6];
  uint32_t target_ip;
} arp_header_t;

#define ARP_REQUEST 1
#define ARP_REPLY 2

/* UDP header */
typedef struct __attribute__((packed)) {
  uint16_t src_port;
  uint16_t dest_port;
  uint16_t length;
  uint16_t checksum;
} udp_header_t;

/* TCP header */
typedef struct __attribute__((packed)) {
  uint16_t src_port;
  uint16_t dest_port;
  uint32_t seq;
  uint32_t ack;
  uint8_t offset;
  uint8_t flags;
  uint16_t window;
  uint16_t checksum;
  uint16_t urgent;
} tcp_header_t;

#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20

/* Functions */
int net_init(void);
net_interface_t *net_get_interface(void);
int net_set_ip(uint32_t ip, uint32_t netmask);
int net_set_gateway(uint32_t gateway);
int net_set_up(int up);
int net_send(uint8_t *data, size_t len);
void net_get_stats(uint32_t *tx_pkts, uint32_t *rx_pkts, uint32_t *tx_bytes,
                   uint32_t *rx_bytes);

/* Utilities */
void ip_to_str(uint32_t ip, char *buf);
uint32_t str_to_ip(const char *str);

#endif
