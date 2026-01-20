/*
 * NanoSec OS - UDP Protocol
 * ==========================
 * User Datagram Protocol - connectionless transport
 */

#include "../kernel.h"
#include "network.h"

/* UDP socket structure */
#define MAX_UDP_SOCKETS 8

typedef struct {
  uint16_t local_port;
  uint8_t in_use;
  uint8_t *recv_buffer;
  size_t recv_len;
  uint32_t recv_from_ip;
  uint16_t recv_from_port;
  volatile int has_data;
} udp_socket_t;

static udp_socket_t udp_sockets[MAX_UDP_SOCKETS];
static uint8_t udp_recv_buffers[MAX_UDP_SOCKETS][512];

/*
 * Initialize UDP
 */
void udp_init(void) {
  memset(udp_sockets, 0, sizeof(udp_sockets));
  for (int i = 0; i < MAX_UDP_SOCKETS; i++) {
    udp_sockets[i].recv_buffer = udp_recv_buffers[i];
  }
}

/*
 * Open UDP socket
 */
int udp_socket(uint16_t local_port) {
  for (int i = 0; i < MAX_UDP_SOCKETS; i++) {
    if (!udp_sockets[i].in_use) {
      udp_sockets[i].local_port = local_port;
      udp_sockets[i].in_use = 1;
      udp_sockets[i].has_data = 0;
      return i;
    }
  }
  return -1;
}

/*
 * Close UDP socket
 */
void udp_close(int sock) {
  if (sock >= 0 && sock < MAX_UDP_SOCKETS) {
    udp_sockets[sock].in_use = 0;
  }
}

/*
 * Send UDP packet
 */
int udp_send(int sock, uint32_t dest_ip, uint16_t dest_port,
             const uint8_t *data, size_t len) {
  if (sock < 0 || sock >= MAX_UDP_SOCKETS || !udp_sockets[sock].in_use) {
    return -1;
  }

  if (len > 1472)
    return -1; /* Too large */

  uint8_t udp_packet[1480];
  uint16_t src_port = udp_sockets[sock].local_port;

  /* UDP header */
  udp_packet[0] = (src_port >> 8) & 0xFF;
  udp_packet[1] = src_port & 0xFF;
  udp_packet[2] = (dest_port >> 8) & 0xFF;
  udp_packet[3] = dest_port & 0xFF;

  uint16_t udp_len = 8 + len;
  udp_packet[4] = (udp_len >> 8) & 0xFF;
  udp_packet[5] = udp_len & 0xFF;
  udp_packet[6] = 0; /* Checksum (optional for IPv4) */
  udp_packet[7] = 0;

  /* Copy data */
  memcpy(udp_packet + 8, data, len);

  return ip_send(dest_ip, 17, udp_packet, udp_len);
}

/*
 * Handle incoming UDP packet
 */
void udp_handle(const uint8_t *packet, size_t len, uint32_t src_ip) {
  if (len < 8)
    return;

  uint16_t src_port = (packet[0] << 8) | packet[1];
  uint16_t dest_port = (packet[2] << 8) | packet[3];
  uint16_t udp_len = (packet[4] << 8) | packet[5];

  if (udp_len > len)
    return;

  size_t data_len = udp_len - 8;
  const uint8_t *data = packet + 8;

  /* Find matching socket */
  for (int i = 0; i < MAX_UDP_SOCKETS; i++) {
    if (udp_sockets[i].in_use && udp_sockets[i].local_port == dest_port) {
      /* Store data */
      if (data_len > 512)
        data_len = 512;
      memcpy(udp_sockets[i].recv_buffer, data, data_len);
      udp_sockets[i].recv_len = data_len;
      udp_sockets[i].recv_from_ip = src_ip;
      udp_sockets[i].recv_from_port = src_port;
      udp_sockets[i].has_data = 1;
      return;
    }
  }
}

/*
 * Receive UDP data
 */
int udp_recv(int sock, uint8_t *buffer, size_t max_len, uint32_t *from_ip,
             uint16_t *from_port, int timeout_ms) {
  if (sock < 0 || sock >= MAX_UDP_SOCKETS || !udp_sockets[sock].in_use) {
    return -1;
  }

  uint32_t start = timer_get_ticks();

  while ((int)(timer_get_ticks() - start) < timeout_ms) {
    net_poll();

    if (udp_sockets[sock].has_data) {
      size_t len = udp_sockets[sock].recv_len;
      if (len > max_len)
        len = max_len;

      memcpy(buffer, udp_sockets[sock].recv_buffer, len);
      if (from_ip)
        *from_ip = udp_sockets[sock].recv_from_ip;
      if (from_port)
        *from_port = udp_sockets[sock].recv_from_port;

      udp_sockets[sock].has_data = 0;
      return len;
    }
  }

  return -1; /* Timeout */
}
