/*
 * NanoSec OS - DNS Client
 * ========================
 * Domain Name System resolver
 */

#include "../kernel.h"
#include "network.h"

/* DNS settings */
static uint32_t dns_server = 0x08080808; /* 8.8.8.8 Google DNS */

/* DNS header structure */
#define DNS_PORT 53
#define DNS_FLAG_QR 0x8000
#define DNS_FLAG_RD 0x0100
#define DNS_FLAG_RA 0x0080

void dns_set_server(uint32_t ip) { dns_server = ip; }

uint32_t dns_get_server(void) { return dns_server; }

/*
 * Encode domain name for DNS query
 */
static size_t dns_encode_name(const char *name, uint8_t *buffer) {
  size_t pos = 0;
  const char *p = name;

  while (*p) {
    /* Find end of label */
    const char *dot = p;
    while (*dot && *dot != '.')
      dot++;

    size_t label_len = dot - p;
    if (label_len > 63)
      label_len = 63;

    buffer[pos++] = label_len;
    while (p < dot) {
      buffer[pos++] = *p++;
    }

    if (*p == '.')
      p++;
  }

  buffer[pos++] = 0; /* Terminating zero */
  return pos;
}

/*
 * DNS lookup
 */
int dns_lookup(const char *hostname, uint32_t *ip) {
  uint8_t query[512];
  uint8_t response[512];

  /* Build DNS query */
  static uint16_t dns_id = 1;
  uint16_t id = dns_id++;

  /* Header */
  query[0] = (id >> 8) & 0xFF;
  query[1] = id & 0xFF;
  query[2] = 0x01; /* RD=1 (recursion desired) */
  query[3] = 0x00;
  query[4] = 0x00;
  query[5] = 0x01; /* QDCOUNT = 1 */
  query[6] = 0x00;
  query[7] = 0x00; /* ANCOUNT = 0 */
  query[8] = 0x00;
  query[9] = 0x00; /* NSCOUNT = 0 */
  query[10] = 0x00;
  query[11] = 0x00; /* ARCOUNT = 0 */

  /* Question */
  size_t name_len = dns_encode_name(hostname, query + 12);
  size_t qpos = 12 + name_len;

  query[qpos++] = 0x00;
  query[qpos++] = 0x01; /* QTYPE = A */
  query[qpos++] = 0x00;
  query[qpos++] = 0x01; /* QCLASS = IN */

  /* Open UDP socket */
  int sock = udp_socket(1024 + (id % 1000));
  if (sock < 0) {
    return -1;
  }

  /* Send query */
  if (udp_send(sock, dns_server, DNS_PORT, query, qpos) < 0) {
    udp_close(sock);
    return -2;
  }

  /* Wait for response */
  uint32_t from_ip;
  uint16_t from_port;
  int len =
      udp_recv(sock, response, sizeof(response), &from_ip, &from_port, 3000);

  udp_close(sock);

  if (len < 12) {
    return -3; /* No response */
  }

  /* Check response ID */
  uint16_t resp_id = (response[0] << 8) | response[1];
  if (resp_id != id) {
    return -4;
  }

  /* Check flags */
  uint16_t flags = (response[2] << 8) | response[3];
  if (!(flags & DNS_FLAG_QR)) {
    return -5; /* Not a response */
  }

  /* Get answer count */
  uint16_t ancount = (response[6] << 8) | response[7];
  if (ancount == 0) {
    return -6; /* No answers */
  }

  /* Skip question section */
  size_t pos = 12;
  while (pos < (size_t)len && response[pos] != 0) {
    if ((response[pos] & 0xC0) == 0xC0) {
      pos += 2;
      break;
    }
    pos += response[pos] + 1;
  }
  if (response[pos] == 0)
    pos++;
  pos += 4; /* Skip QTYPE and QCLASS */

  /* Parse answer */
  while (ancount-- > 0 && pos < (size_t)len - 10) {
    /* Skip name (may be compressed) */
    if ((response[pos] & 0xC0) == 0xC0) {
      pos += 2;
    } else {
      while (pos < (size_t)len && response[pos] != 0) {
        pos += response[pos] + 1;
      }
      pos++;
    }

    uint16_t rtype = (response[pos] << 8) | response[pos + 1];
    pos += 2;
    pos += 2; /* Skip class */
    pos += 4; /* Skip TTL */

    uint16_t rdlen = (response[pos] << 8) | response[pos + 1];
    pos += 2;

    if (rtype == 1 && rdlen == 4) { /* A record */
      *ip = ((uint32_t)response[pos] << 24) |
            ((uint32_t)response[pos + 1] << 16) |
            ((uint32_t)response[pos + 2] << 8) | response[pos + 3];
      return 0;
    }

    pos += rdlen;
  }

  return -7; /* No A record found */
}

/*
 * DNS command
 */
void cmd_dns_real(const char *args) {
  if (args[0] == '\0') {
    kprintf("DNS Server: %d.%d.%d.%d\n", (dns_server >> 24) & 0xFF,
            (dns_server >> 16) & 0xFF, (dns_server >> 8) & 0xFF,
            dns_server & 0xFF);
    kprintf("Usage: dns <hostname>\n");
    return;
  }

  /* Check if setting DNS server */
  if (strncmp(args, "server ", 7) == 0) {
    /* Parse IP */
    uint32_t ip = 0;
    int val = 0;
    const char *p = args + 7;

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

    dns_set_server(ip);
    kprintf("DNS server set to %d.%d.%d.%d\n", (ip >> 24) & 0xFF,
            (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
    return;
  }

  /* Lookup hostname */
  kprintf("Looking up %s...\n", args);

  uint32_t ip;
  int result = dns_lookup(args, &ip);

  if (result == 0) {
    kprintf("%s -> %d.%d.%d.%d\n", args, (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
            (ip >> 8) & 0xFF, ip & 0xFF);
  } else {
    kprintf("DNS lookup failed (error %d)\n", result);
  }
}

/*
 * strncmp implementation
 */
int strncmp(const char *s1, const char *s2, size_t n) {
  while (n && *s1 && *s1 == *s2) {
    s1++;
    s2++;
    n--;
  }
  if (n == 0)
    return 0;
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}
