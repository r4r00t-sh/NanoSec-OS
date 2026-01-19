/*
 * NanoSec OS - NE2000 Network Driver
 * ====================================
 * Real driver for NE2000-compatible NICs (QEMU compatible)
 */

#include "../kernel.h"
#include "network.h"

/* NE2000 I/O Base (default ISA) */
#define NE2000_BASE 0x300

/* NE2000 Registers */
#define NE_CMD 0x00
#define NE_PSTART 0x01
#define NE_PSTOP 0x02
#define NE_BOUNDARY 0x03
#define NE_TPSR 0x04
#define NE_TBCR0 0x05
#define NE_TBCR1 0x06
#define NE_ISR 0x07
#define NE_RSAR0 0x08
#define NE_RSAR1 0x09
#define NE_RBCR0 0x0A
#define NE_RBCR1 0x0B
#define NE_RCR 0x0C
#define NE_TCR 0x0D
#define NE_DCR 0x0E
#define NE_IMR 0x0F
#define NE_DATA 0x10
#define NE_RESET 0x1F

/* Page 1 Registers */
#define NE_CURR 0x07 /* Current page (page 1) */
#define NE_PAR0 0x01 /* Physical Address */
#define NE_MAR0 0x08 /* Multicast Address */

/* Command register bits */
#define NE_CMD_STOP 0x01
#define NE_CMD_START 0x02
#define NE_CMD_TRANS 0x04
#define NE_CMD_RREAD 0x08
#define NE_CMD_RWRITE 0x10
#define NE_CMD_NODMA 0x20
#define NE_CMD_PAGE0 0x00
#define NE_CMD_PAGE1 0x40
#define NE_CMD_PAGE2 0x80

/* Receive buffer layout */
#define NE_RX_START 0x46
#define NE_RX_STOP 0x80
#define NE_TX_START 0x40

static uint16_t ne_base = NE2000_BASE;
static int ne_initialized = 0;
static uint8_t current_mac[6];

/* Word output for NE2000 */
static inline void outw(uint16_t port, uint16_t val) {
  asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

/* Read register */
static uint8_t ne_read(uint8_t reg) { return inb(ne_base + reg); }

/* Write register */
static void ne_write(uint8_t reg, uint8_t val) { outb(ne_base + reg, val); }

/*
 * Initialize NE2000
 */
int net_init(void) {
  /* Reset the card */
  uint8_t tmp = inb(ne_base + NE_RESET);
  outb(ne_base + NE_RESET, tmp);

  /* Wait for reset */
  for (volatile int i = 0; i < 10000; i++)
    ;

  /* Stop NIC */
  ne_write(NE_CMD, NE_CMD_STOP | NE_CMD_NODMA | NE_CMD_PAGE0);

  /* Wait for stop */
  int timeout = 1000;
  while ((ne_read(NE_ISR) & 0x80) == 0 && timeout-- > 0)
    ;

  /* Initialize Data Configuration */
  ne_write(NE_DCR, 0x49); /* Word mode, fifo threshold 8 */

  /* Clear counters */
  ne_write(NE_RBCR0, 0);
  ne_write(NE_RBCR1, 0);

  /* Accept broadcast and physical */
  ne_write(NE_RCR, 0x04);

  /* TCR: internal loopback */
  ne_write(NE_TCR, 0x02);

  /* Set up receive buffer */
  ne_write(NE_PSTART, NE_RX_START);
  ne_write(NE_PSTOP, NE_RX_STOP);
  ne_write(NE_BOUNDARY, NE_RX_START);

  /* Set tx buffer */
  ne_write(NE_TPSR, NE_TX_START);

  /* Clear ISR */
  ne_write(NE_ISR, 0xFF);

  /* Set IMR */
  ne_write(NE_IMR, 0x00); /* No interrupts for now */

  /* Switch to page 1 to set MAC and current page */
  ne_write(NE_CMD, NE_CMD_STOP | NE_CMD_NODMA | NE_CMD_PAGE1);

  /* Set current receive page */
  ne_write(NE_CURR, NE_RX_START + 1);

  /* Read MAC address from PROM */
  ne_write(NE_CMD, NE_CMD_STOP | NE_CMD_NODMA | NE_CMD_PAGE0);

  /* Read PROM to get MAC */
  ne_write(NE_RBCR0, 12);
  ne_write(NE_RBCR1, 0);
  ne_write(NE_RSAR0, 0);
  ne_write(NE_RSAR1, 0);
  ne_write(NE_CMD, NE_CMD_START | NE_CMD_RREAD | NE_CMD_PAGE0);

  for (int i = 0; i < 6; i++) {
    current_mac[i] = inb(ne_base + NE_DATA);
    inb(ne_base + NE_DATA); /* Skip duplicate */
  }

  /* Set MAC in page 1 */
  ne_write(NE_CMD, NE_CMD_STOP | NE_CMD_NODMA | NE_CMD_PAGE1);
  for (int i = 0; i < 6; i++) {
    ne_write(NE_PAR0 + i, current_mac[i]);
  }

  /* Accept all multicast */
  for (int i = 0; i < 8; i++) {
    ne_write(NE_MAR0 + i, 0xFF);
  }

  /* Back to page 0 and start */
  ne_write(NE_CMD, NE_CMD_START | NE_CMD_NODMA | NE_CMD_PAGE0);

  /* Enable normal mode */
  ne_write(NE_TCR, 0x00);
  ne_write(NE_RCR, 0x0C); /* Accept broadcast + physical */

  ne_initialized = 1;

  /* Initialize network config */
  net_set_mac(current_mac);
  net_set_ip(0x0A000002);      /* 10.0.0.2 */
  net_set_gateway(0x0A000001); /* 10.0.0.1 */
  net_set_netmask(0xFFFFFF00); /* 255.255.255.0 */

  /* Initialize protocols */
  arp_init();
  udp_init();

  kprintf("  [OK] NE2000 Network (MAC: %02x:%02x:%02x:%02x:%02x:%02x)\n",
          current_mac[0], current_mac[1], current_mac[2], current_mac[3],
          current_mac[4], current_mac[5]);

  return 0;
}

/*
 * Send packet
 */
int net_send(const uint8_t *packet, size_t len) {
  if (!ne_initialized)
    return -1;
  if (len < 14 || len > 1514)
    return -1;

  /* Pad to minimum */
  if (len < 60)
    len = 60;

  /* Wait for any pending transmit */
  int timeout = 1000;
  while ((ne_read(NE_CMD) & NE_CMD_TRANS) && timeout-- > 0)
    ;

  /* Set up DMA for write */
  ne_write(NE_ISR, 0x40); /* Clear remote DMA complete */
  ne_write(NE_RBCR0, len & 0xFF);
  ne_write(NE_RBCR1, (len >> 8) & 0xFF);
  ne_write(NE_RSAR0, 0);
  ne_write(NE_RSAR1, NE_TX_START);

  /* Start remote write */
  ne_write(NE_CMD, NE_CMD_START | NE_CMD_RWRITE | NE_CMD_PAGE0);

  /* Write data */
  for (size_t i = 0; i < len; i += 2) {
    uint16_t word = packet[i];
    if (i + 1 < len)
      word |= (packet[i + 1] << 8);
    outw(ne_base + NE_DATA, word);
  }

  /* Wait for completion */
  timeout = 1000;
  while ((ne_read(NE_ISR) & 0x40) == 0 && timeout-- > 0)
    ;

  /* Set transmit parameters */
  ne_write(NE_TPSR, NE_TX_START);
  ne_write(NE_TBCR0, len & 0xFF);
  ne_write(NE_TBCR1, (len >> 8) & 0xFF);

  /* Start transmit */
  ne_write(NE_CMD, NE_CMD_START | NE_CMD_TRANS | NE_CMD_NODMA | NE_CMD_PAGE0);

  return 0;
}

/*
 * Poll for incoming packets
 */
void net_poll(void) {
  if (!ne_initialized)
    return;

  uint8_t buffer[1514];
  int len;

  while ((len = net_receive(buffer, sizeof(buffer))) > 0) {
    if (len < 14)
      continue;

    /* Parse Ethernet header */
    uint16_t ethertype = (buffer[12] << 8) | buffer[13];

    if (ethertype == ETHERTYPE_ARP) {
      arp_handle(buffer + 14, len - 14);
    } else if (ethertype == ETHERTYPE_IP) {
      ip_handle(buffer + 14, len - 14);
    }
  }
}

/*
 * Receive packet
 */
int net_receive(uint8_t *buffer, size_t max_len) {
  if (!ne_initialized)
    return -1;

  /* Check for receive complete */
  uint8_t isr = ne_read(NE_ISR);
  if (!(isr & 0x01))
    return 0; /* No packet */

  /* Get current boundary and next page */
  uint8_t boundary = ne_read(NE_BOUNDARY);

  ne_write(NE_CMD, NE_CMD_START | NE_CMD_NODMA | NE_CMD_PAGE1);
  uint8_t current = ne_read(NE_CURR);
  ne_write(NE_CMD, NE_CMD_START | NE_CMD_NODMA | NE_CMD_PAGE0);

  if (boundary == current - 1 ||
      (current == NE_RX_START && boundary == NE_RX_STOP - 1)) {
    return 0; /* No packet */
  }

  uint8_t next_page = boundary + 1;
  if (next_page >= NE_RX_STOP)
    next_page = NE_RX_START;

  /* Read packet header */
  ne_write(NE_RBCR0, 4);
  ne_write(NE_RBCR1, 0);
  ne_write(NE_RSAR0, 0);
  ne_write(NE_RSAR1, next_page);
  ne_write(NE_CMD, NE_CMD_START | NE_CMD_RREAD | NE_CMD_PAGE0);

  uint8_t status = inb(ne_base + NE_DATA);
  uint8_t next = inb(ne_base + NE_DATA);
  uint16_t len = inb(ne_base + NE_DATA);
  len |= (inb(ne_base + NE_DATA) << 8);

  (void)status;

  if (len < 4)
    len = 4;
  len -= 4; /* Remove CRC */

  if (len > max_len)
    len = max_len;
  if (len > 1514)
    len = 1514;

  /* Read packet data */
  ne_write(NE_RBCR0, len & 0xFF);
  ne_write(NE_RBCR1, (len >> 8) & 0xFF);
  ne_write(NE_RSAR0, 4);
  ne_write(NE_RSAR1, next_page);
  ne_write(NE_CMD, NE_CMD_START | NE_CMD_RREAD | NE_CMD_PAGE0);

  for (size_t i = 0; i < len; i++) {
    buffer[i] = inb(ne_base + NE_DATA);
  }

  /* Update boundary */
  uint8_t new_boundary = next - 1;
  if (new_boundary < NE_RX_START)
    new_boundary = NE_RX_STOP - 1;
  ne_write(NE_BOUNDARY, new_boundary);

  /* Clear receive flag */
  ne_write(NE_ISR, 0x01);

  return len;
}
