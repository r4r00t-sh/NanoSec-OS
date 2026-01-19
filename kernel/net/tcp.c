/*
 * NanoSec OS - TCP Protocol
 * ===========================
 * Transmission Control Protocol with full state machine
 */

#include "../kernel.h"
#include "network.h"

/* TCP states */
#define TCP_CLOSED 0
#define TCP_LISTEN 1
#define TCP_SYN_SENT 2
#define TCP_SYN_RCVD 3
#define TCP_ESTABLISHED 4
#define TCP_FIN_WAIT1 5
#define TCP_FIN_WAIT2 6
#define TCP_CLOSE_WAIT 7
#define TCP_CLOSING 8
#define TCP_LAST_ACK 9
#define TCP_TIME_WAIT 10

/* TCP flags */
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20

/* TCP header */
typedef struct __attribute__((packed)) {
  uint16_t src_port;
  uint16_t dst_port;
  uint32_t seq_num;
  uint32_t ack_num;
  uint8_t data_offset; /* Upper 4 bits */
  uint8_t flags;
  uint16_t window;
  uint16_t checksum;
  uint16_t urgent_ptr;
} tcp_header_t;

/* TCP connection */
typedef struct {
  int state;
  uint16_t local_port;
  uint16_t remote_port;
  uint32_t remote_ip;
  uint32_t seq_num; /* Our sequence number */
  uint32_t ack_num; /* Expected sequence from remote */
  uint32_t send_window;
  uint32_t recv_window;
  uint8_t recv_buf[4096];
  uint16_t recv_len;
  uint8_t send_buf[4096];
  uint16_t send_len;
  int in_use;
} tcp_socket_t;

#define MAX_TCP_SOCKETS 16
static tcp_socket_t tcp_sockets[MAX_TCP_SOCKETS];

/* Simple pseudo-random for ISN */
static uint32_t tcp_isn = 0x12345678;

/* Network byte order helpers */
static inline uint16_t htons(uint16_t n) {
  return ((n & 0xFF) << 8) | ((n >> 8) & 0xFF);
}
static inline uint16_t ntohs(uint16_t n) { return htons(n); }
static inline uint32_t htonl(uint32_t n) {
  return ((n & 0xFF) << 24) | ((n & 0xFF00) << 8) | ((n >> 8) & 0xFF00) |
         ((n >> 24) & 0xFF);
}
static inline uint32_t ntohl(uint32_t n) { return htonl(n); }

/* Forward declaration */
int tcp_send_segment(int sock, uint8_t flags, const uint8_t *data,
                     uint16_t len);

/* Simple memmove implementation */
static void *tcp_memmove(void *dest, const void *src, size_t n) {
  uint8_t *d = (uint8_t *)dest;
  const uint8_t *s = (const uint8_t *)src;
  if (d < s) {
    for (size_t i = 0; i < n; i++)
      d[i] = s[i];
  } else {
    for (size_t i = n; i > 0; i--)
      d[i - 1] = s[i - 1];
  }
  return dest;
}

/*
 * Calculate TCP checksum
 */
static uint16_t tcp_checksum(uint32_t src_ip, uint32_t dst_ip,
                             const uint8_t *tcp_data, uint16_t tcp_len) {
  uint32_t sum = 0;

  /* Pseudo header */
  sum += (src_ip >> 16) & 0xFFFF;
  sum += src_ip & 0xFFFF;
  sum += (dst_ip >> 16) & 0xFFFF;
  sum += dst_ip & 0xFFFF;
  sum += 6; /* Protocol: TCP */
  sum += tcp_len;

  /* TCP header and data */
  const uint16_t *ptr = (const uint16_t *)tcp_data;
  for (int i = 0; i < tcp_len / 2; i++) {
    sum += ntohs(ptr[i]);
  }

  if (tcp_len & 1) {
    sum += tcp_data[tcp_len - 1] << 8;
  }

  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  return ~sum;
}

/*
 * Initialize TCP
 */
void tcp_init(void) {
  for (int i = 0; i < MAX_TCP_SOCKETS; i++) {
    tcp_sockets[i].in_use = 0;
    tcp_sockets[i].state = TCP_CLOSED;
  }
  tcp_isn = timer_get_ticks();
}

/*
 * Create TCP socket
 */
int tcp_socket(void) {
  for (int i = 0; i < MAX_TCP_SOCKETS; i++) {
    if (!tcp_sockets[i].in_use) {
      tcp_sockets[i].in_use = 1;
      tcp_sockets[i].state = TCP_CLOSED;
      tcp_sockets[i].local_port = 0;
      tcp_sockets[i].remote_port = 0;
      tcp_sockets[i].remote_ip = 0;
      tcp_sockets[i].seq_num = tcp_isn++;
      tcp_sockets[i].recv_len = 0;
      tcp_sockets[i].send_len = 0;
      tcp_sockets[i].recv_window = 4096;
      return i;
    }
  }
  return -1;
}

/*
 * Close TCP socket
 */
void tcp_close(int sock) {
  if (sock < 0 || sock >= MAX_TCP_SOCKETS)
    return;

  tcp_socket_t *s = &tcp_sockets[sock];

  if (s->state == TCP_ESTABLISHED) {
    /* Send FIN */
    tcp_send_segment(sock, TCP_FIN | TCP_ACK, NULL, 0);
    s->seq_num++;
    s->state = TCP_FIN_WAIT1;
    /* TODO: Wait for close to complete */
  }

  s->in_use = 0;
  s->state = TCP_CLOSED;
}

/*
 * Bind socket to port
 */
int tcp_bind(int sock, uint16_t port) {
  if (sock < 0 || sock >= MAX_TCP_SOCKETS)
    return -1;

  tcp_sockets[sock].local_port = port;
  return 0;
}

/*
 * Listen for connections
 */
int tcp_listen(int sock) {
  if (sock < 0 || sock >= MAX_TCP_SOCKETS)
    return -1;

  tcp_sockets[sock].state = TCP_LISTEN;
  return 0;
}

/*
 * Send TCP segment
 */
int tcp_send_segment(int sock, uint8_t flags, const uint8_t *data,
                     uint16_t len) {
  if (sock < 0 || sock >= MAX_TCP_SOCKETS)
    return -1;

  tcp_socket_t *s = &tcp_sockets[sock];

  /* Build packet: Ethernet + IP + TCP + data */
  uint8_t packet[1500];
  size_t total_len = 14 + 20 + 20 + len;

  /* Get our info */
  uint8_t our_mac[6], dest_mac[6];
  net_get_mac(our_mac);
  uint32_t our_ip = net_get_ip();

  /* Resolve MAC */
  if (arp_resolve(s->remote_ip, dest_mac, 1000) != 0) {
    return -1;
  }

  /* Ethernet header */
  memcpy(packet, dest_mac, 6);
  memcpy(packet + 6, our_mac, 6);
  packet[12] = 0x08;
  packet[13] = 0x00; /* IPv4 */

  /* IP header */
  uint8_t *ip = packet + 14;
  ip[0] = 0x45; /* IPv4, IHL=5 */
  ip[1] = 0x00;
  uint16_t ip_len = 20 + 20 + len;
  ip[2] = (ip_len >> 8) & 0xFF;
  ip[3] = ip_len & 0xFF;
  ip[4] = 0x00;
  ip[5] = 0x00; /* ID */
  ip[6] = 0x40;
  ip[7] = 0x00; /* Don't fragment */
  ip[8] = 64;   /* TTL */
  ip[9] = 6;    /* Protocol: TCP */
  ip[10] = 0;
  ip[11] = 0; /* Checksum (calculated below) */
  ip[12] = (our_ip >> 24) & 0xFF;
  ip[13] = (our_ip >> 16) & 0xFF;
  ip[14] = (our_ip >> 8) & 0xFF;
  ip[15] = our_ip & 0xFF;
  ip[16] = (s->remote_ip >> 24) & 0xFF;
  ip[17] = (s->remote_ip >> 16) & 0xFF;
  ip[18] = (s->remote_ip >> 8) & 0xFF;
  ip[19] = s->remote_ip & 0xFF;

  /* IP checksum */
  uint32_t sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += (ip[i * 2] << 8) | ip[i * 2 + 1];
  }
  while (sum >> 16)
    sum = (sum & 0xFFFF) + (sum >> 16);
  ip[10] = (~sum >> 8) & 0xFF;
  ip[11] = ~sum & 0xFF;

  /* TCP header */
  uint8_t *tcp = packet + 34;
  tcp[0] = (s->local_port >> 8) & 0xFF;
  tcp[1] = s->local_port & 0xFF;
  tcp[2] = (s->remote_port >> 8) & 0xFF;
  tcp[3] = s->remote_port & 0xFF;
  tcp[4] = (s->seq_num >> 24) & 0xFF;
  tcp[5] = (s->seq_num >> 16) & 0xFF;
  tcp[6] = (s->seq_num >> 8) & 0xFF;
  tcp[7] = s->seq_num & 0xFF;
  tcp[8] = (s->ack_num >> 24) & 0xFF;
  tcp[9] = (s->ack_num >> 16) & 0xFF;
  tcp[10] = (s->ack_num >> 8) & 0xFF;
  tcp[11] = s->ack_num & 0xFF;
  tcp[12] = 0x50; /* Data offset: 5 (20 bytes) */
  tcp[13] = flags;
  tcp[14] = (s->recv_window >> 8) & 0xFF;
  tcp[15] = s->recv_window & 0xFF;
  tcp[16] = 0;
  tcp[17] = 0; /* Checksum */
  tcp[18] = 0;
  tcp[19] = 0; /* Urgent pointer */

  /* Copy data */
  if (data && len > 0) {
    memcpy(tcp + 20, data, len);
  }

  /* TCP checksum */
  uint16_t csum = tcp_checksum(our_ip, s->remote_ip, tcp, 20 + len);
  tcp[16] = (csum >> 8) & 0xFF;
  tcp[17] = csum & 0xFF;

  return net_send(packet, total_len);
}

/*
 * Connect to remote host
 */
int tcp_connect(int sock, uint32_t ip, uint16_t port) {
  if (sock < 0 || sock >= MAX_TCP_SOCKETS)
    return -1;

  tcp_socket_t *s = &tcp_sockets[sock];

  s->remote_ip = ip;
  s->remote_port = port;

  /* Assign ephemeral port if not bound */
  if (s->local_port == 0) {
    s->local_port = 49152 + (tcp_isn % 16384);
  }

  /* Send SYN */
  s->state = TCP_SYN_SENT;
  tcp_send_segment(sock, TCP_SYN, NULL, 0);
  s->seq_num++;

  /* Wait for SYN-ACK */
  uint32_t timeout = timer_get_ticks() + 5000;
  while (timer_get_ticks() < timeout) {
    net_poll();
    if (s->state == TCP_ESTABLISHED) {
      return 0;
    }
  }

  s->state = TCP_CLOSED;
  return -1;
}

/*
 * Handle incoming TCP packet
 */
void tcp_handle(const uint8_t *packet, size_t len, uint32_t src_ip) {
  if (len < 20)
    return;

  tcp_header_t *tcp = (tcp_header_t *)packet;

  uint16_t src_port = (packet[0] << 8) | packet[1];
  uint16_t dst_port = (packet[2] << 8) | packet[3];
  uint32_t seq =
      (packet[4] << 24) | (packet[5] << 16) | (packet[6] << 8) | packet[7];
  uint32_t ack =
      (packet[8] << 24) | (packet[9] << 16) | (packet[10] << 8) | packet[11];
  uint8_t flags = packet[13];
  uint8_t data_offset = (packet[12] >> 4) * 4;

  /* Find matching socket */
  int sock = -1;
  for (int i = 0; i < MAX_TCP_SOCKETS; i++) {
    if (!tcp_sockets[i].in_use)
      continue;

    tcp_socket_t *s = &tcp_sockets[i];

    if (s->local_port == dst_port) {
      if (s->state == TCP_LISTEN ||
          (s->remote_port == src_port && s->remote_ip == src_ip)) {
        sock = i;
        break;
      }
    }
  }

  if (sock < 0) {
    /* No matching socket - send RST */
    return;
  }

  tcp_socket_t *s = &tcp_sockets[sock];

  /* State machine */
  switch (s->state) {
  case TCP_LISTEN:
    if (flags & TCP_SYN) {
      s->remote_ip = src_ip;
      s->remote_port = src_port;
      s->ack_num = seq + 1;
      s->state = TCP_SYN_RCVD;
      tcp_send_segment(sock, TCP_SYN | TCP_ACK, NULL, 0);
      s->seq_num++;
    }
    break;

  case TCP_SYN_SENT:
    if ((flags & (TCP_SYN | TCP_ACK)) == (TCP_SYN | TCP_ACK)) {
      s->ack_num = seq + 1;
      s->state = TCP_ESTABLISHED;
      tcp_send_segment(sock, TCP_ACK, NULL, 0);
    }
    break;

  case TCP_SYN_RCVD:
    if (flags & TCP_ACK) {
      s->state = TCP_ESTABLISHED;
    }
    break;

  case TCP_ESTABLISHED:
    if (flags & TCP_FIN) {
      s->ack_num = seq + 1;
      s->state = TCP_CLOSE_WAIT;
      tcp_send_segment(sock, TCP_ACK, NULL, 0);
      /* Application should close */
    } else if (flags & TCP_ACK) {
      /* Handle data */
      uint16_t data_len = len - data_offset;
      if (data_len > 0) {
        const uint8_t *data = packet + data_offset;
        uint16_t copy = (data_len < sizeof(s->recv_buf) - s->recv_len)
                            ? data_len
                            : sizeof(s->recv_buf) - s->recv_len;
        memcpy(s->recv_buf + s->recv_len, data, copy);
        s->recv_len += copy;
        s->ack_num += data_len;
        tcp_send_segment(sock, TCP_ACK, NULL, 0);
      }
    }
    break;

  case TCP_FIN_WAIT1:
    if (flags & TCP_ACK) {
      s->state = TCP_FIN_WAIT2;
    }
    if (flags & TCP_FIN) {
      s->ack_num = seq + 1;
      tcp_send_segment(sock, TCP_ACK, NULL, 0);
      s->state = TCP_TIME_WAIT;
    }
    break;

  case TCP_FIN_WAIT2:
    if (flags & TCP_FIN) {
      s->ack_num = seq + 1;
      tcp_send_segment(sock, TCP_ACK, NULL, 0);
      s->state = TCP_TIME_WAIT;
    }
    break;

  case TCP_CLOSE_WAIT:
    /* Wait for application to close */
    break;

  case TCP_LAST_ACK:
    if (flags & TCP_ACK) {
      s->state = TCP_CLOSED;
      s->in_use = 0;
    }
    break;
  }
}

/*
 * Send data on connected socket
 */
int tcp_send(int sock, const uint8_t *data, uint16_t len) {
  if (sock < 0 || sock >= MAX_TCP_SOCKETS)
    return -1;

  tcp_socket_t *s = &tcp_sockets[sock];

  if (s->state != TCP_ESTABLISHED)
    return -1;

  int ret = tcp_send_segment(sock, TCP_ACK | TCP_PSH, data, len);
  if (ret == 0) {
    s->seq_num += len;
  }
  return ret == 0 ? len : -1;
}

/*
 * Receive data from socket
 */
int tcp_recv(int sock, uint8_t *buffer, uint16_t max_len) {
  if (sock < 0 || sock >= MAX_TCP_SOCKETS)
    return -1;

  tcp_socket_t *s = &tcp_sockets[sock];

  /* Poll for data */
  net_poll();

  if (s->recv_len == 0)
    return 0;

  uint16_t copy = (s->recv_len < max_len) ? s->recv_len : max_len;
  memcpy(buffer, s->recv_buf, copy);

  /* Shift remaining data */
  if (copy < s->recv_len) {
    tcp_memmove(s->recv_buf, s->recv_buf + copy, s->recv_len - copy);
  }
  s->recv_len -= copy;

  return copy;
}
