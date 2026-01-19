/*
 * NanoSec OS - Network Driver (NE2000 compatible)
 * ================================================
 * Provides network card access for QEMU's ne2000 emulation
 */

#include "network.h"
#include "../kernel.h"


/* NE2000 I/O Ports (base = 0x300 for ISA) */
#define NE2K_BASE 0x300
#define NE2K_DATA (NE2K_BASE + 0x10)
#define NE2K_RESET (NE2K_BASE + 0x1F)

/* NE2000 Registers */
#define NE2K_CR 0x00     /* Command Register */
#define NE2K_PSTART 0x01 /* Page Start */
#define NE2K_PSTOP 0x02  /* Page Stop */
#define NE2K_BNRY 0x03   /* Boundary */
#define NE2K_TSR 0x04    /* Transmit Status */
#define NE2K_TPSR 0x04   /* Transmit Page Start */
#define NE2K_NCR 0x05    /* Number of Collisions */
#define NE2K_TBCR0 0x05  /* Transmit Byte Count 0 */
#define NE2K_TBCR1 0x06  /* Transmit Byte Count 1 */
#define NE2K_ISR 0x07    /* Interrupt Status */
#define NE2K_RSAR0 0x08  /* Remote Start Address 0 */
#define NE2K_RSAR1 0x09  /* Remote Start Address 1 */
#define NE2K_RBCR0 0x0A  /* Remote Byte Count 0 */
#define NE2K_RBCR1 0x0B  /* Remote Byte Count 1 */
#define NE2K_RCR 0x0C    /* Receive Configuration */
#define NE2K_TCR 0x0D    /* Transmit Configuration */
#define NE2K_DCR 0x0E    /* Data Configuration */
#define NE2K_IMR 0x0F    /* Interrupt Mask */

/* Page 1 registers */
#define NE2K_PAR0 0x01 /* Physical Address 0-5 */
#define NE2K_CURR 0x07 /* Current Page */

/* Command bits */
#define CR_STP 0x01 /* Stop */
#define CR_STA 0x02 /* Start */
#define CR_TXP 0x04 /* Transmit Packet */
#define CR_RD0 0x08 /* Remote DMA Read */
#define CR_RD1 0x10 /* Remote DMA Write */
#define CR_RD2 0x20 /* Remote DMA Abort */
#define CR_PS0 0x40 /* Page Select 0 */
#define CR_PS1 0x80 /* Page Select 1 */

/* Network interface state */
static net_interface_t net_if;
static int net_initialized = 0;

/* Statistics */
static struct {
  uint32_t tx_packets;
  uint32_t rx_packets;
  uint32_t tx_bytes;
  uint32_t rx_bytes;
  uint32_t errors;
} net_stats;

/*
 * Read from NE2000 register
 */
static uint8_t ne2k_read(uint8_t reg) { return inb(NE2K_BASE + reg); }

/*
 * Write to NE2000 register
 */
static void ne2k_write(uint8_t reg, uint8_t val) { outb(NE2K_BASE + reg, val); }

/*
 * Select register page
 */
static void ne2k_page(int page) {
  uint8_t cr = ne2k_read(NE2K_CR) & ~(CR_PS0 | CR_PS1);
  if (page == 1)
    cr |= CR_PS0;
  else if (page == 2)
    cr |= CR_PS1;
  ne2k_write(NE2K_CR, cr);
}

/*
 * Reset NE2000
 */
static void ne2k_reset(void) {
  uint8_t tmp = inb(NE2K_RESET);
  outb(NE2K_RESET, tmp);

  /* Wait for reset */
  while ((ne2k_read(NE2K_ISR) & 0x80) == 0)
    ;
  ne2k_write(NE2K_ISR, 0xFF); /* Clear interrupts */
}

/*
 * Initialize network driver
 */
int net_init(void) {
  memset(&net_if, 0, sizeof(net_if));
  memset(&net_stats, 0, sizeof(net_stats));

  /* Try to detect NE2000 */
  ne2k_reset();

  /* Check if card is present */
  ne2k_write(NE2K_CR, CR_STP | CR_RD2); /* Stop, abort DMA */

  if (ne2k_read(NE2K_CR) != (CR_STP | CR_RD2)) {
    /* No NE2000 found - use virtual interface */
    net_if.up = 0;
    strcpy(net_if.name, "lo0");
    net_if.ip = 0x7F000001;      /* 127.0.0.1 */
    net_if.netmask = 0xFF000000; /* 255.0.0.0 */
    net_initialized = 1;
    return 0;
  }

  /* Configure NE2000 */
  ne2k_write(NE2K_DCR, 0x49); /* Word transfer, 8-bit DMA */
  ne2k_write(NE2K_RBCR0, 0);
  ne2k_write(NE2K_RBCR1, 0);
  ne2k_write(NE2K_RCR, 0x20); /* Monitor mode initially */
  ne2k_write(NE2K_TCR, 0x02); /* Loopback */

  /* Read MAC address from PROM */
  ne2k_write(NE2K_RSAR0, 0);
  ne2k_write(NE2K_RSAR1, 0);
  ne2k_write(NE2K_RBCR0, 12);
  ne2k_write(NE2K_RBCR1, 0);
  ne2k_write(NE2K_CR, CR_STA | CR_RD0);

  for (int i = 0; i < 6; i++) {
    net_if.mac[i] = inb(NE2K_DATA);
    inb(NE2K_DATA); /* Skip duplicate */
  }

  /* Set up receive buffer ring */
  ne2k_write(NE2K_PSTART, 0x46);
  ne2k_write(NE2K_PSTOP, 0x80);
  ne2k_write(NE2K_BNRY, 0x46);

  /* Set MAC address in registers */
  ne2k_page(1);
  for (int i = 0; i < 6; i++) {
    ne2k_write(NE2K_PAR0 + i, net_if.mac[i]);
  }
  ne2k_write(NE2K_CURR, 0x47);
  ne2k_page(0);

  /* Enable receive */
  ne2k_write(NE2K_RCR, 0x04);           /* Accept broadcast */
  ne2k_write(NE2K_TCR, 0x00);           /* Normal operation */
  ne2k_write(NE2K_ISR, 0xFF);           /* Clear interrupts */
  ne2k_write(NE2K_IMR, 0x00);           /* Disable interrupts for now */
  ne2k_write(NE2K_CR, CR_STA | CR_RD2); /* Start */

  strcpy(net_if.name, "eth0");
  net_if.up = 1;
  net_if.ip = 0; /* No IP yet */
  net_if.netmask = 0;
  net_if.gateway = 0;

  net_initialized = 1;

  return 0;
}

/*
 * Get network interface info
 */
net_interface_t *net_get_interface(void) { return &net_if; }

/*
 * Configure IP address
 */
int net_set_ip(uint32_t ip, uint32_t netmask) {
  net_if.ip = ip;
  net_if.netmask = netmask;
  return 0;
}

/*
 * Set gateway
 */
int net_set_gateway(uint32_t gateway) {
  net_if.gateway = gateway;
  return 0;
}

/*
 * Bring interface up/down
 */
int net_set_up(int up) {
  net_if.up = up;
  if (up) {
    ne2k_write(NE2K_CR, CR_STA | CR_RD2);
  } else {
    ne2k_write(NE2K_CR, CR_STP | CR_RD2);
  }
  return 0;
}

/*
 * Send packet
 */
int net_send(uint8_t *data, size_t len) {
  if (!net_if.up || len > 1500)
    return -1;

  /* Copy to transmit buffer */
  ne2k_write(NE2K_RSAR0, 0);
  ne2k_write(NE2K_RSAR1, 0x40); /* TX buffer at page 0x40 */
  ne2k_write(NE2K_RBCR0, len & 0xFF);
  ne2k_write(NE2K_RBCR1, len >> 8);
  ne2k_write(NE2K_CR, CR_STA | CR_RD1); /* Start, remote write */

  for (size_t i = 0; i < len; i++) {
    outb(NE2K_DATA, data[i]);
  }

  /* Wait for DMA complete */
  while ((ne2k_read(NE2K_ISR) & 0x40) == 0)
    ;
  ne2k_write(NE2K_ISR, 0x40);

  /* Transmit */
  ne2k_write(NE2K_TPSR, 0x40);
  ne2k_write(NE2K_TBCR0, len & 0xFF);
  ne2k_write(NE2K_TBCR1, len >> 8);
  ne2k_write(NE2K_CR, CR_STA | CR_TXP | CR_RD2);

  /* Wait for transmit complete */
  while ((ne2k_read(NE2K_ISR) & 0x02) == 0)
    ;
  ne2k_write(NE2K_ISR, 0x02);

  net_stats.tx_packets++;
  net_stats.tx_bytes += len;

  return 0;
}

/*
 * Get network statistics
 */
void net_get_stats(uint32_t *tx_pkts, uint32_t *rx_pkts, uint32_t *tx_bytes,
                   uint32_t *rx_bytes) {
  if (tx_pkts)
    *tx_pkts = net_stats.tx_packets;
  if (rx_pkts)
    *rx_pkts = net_stats.rx_packets;
  if (tx_bytes)
    *tx_bytes = net_stats.tx_bytes;
  if (rx_bytes)
    *rx_bytes = net_stats.rx_bytes;
}

/*
 * IP to string
 */
void ip_to_str(uint32_t ip, char *buf) {
  int i = 0;
  for (int b = 3; b >= 0; b--) {
    uint8_t octet = (ip >> (b * 8)) & 0xFF;
    if (octet >= 100)
      buf[i++] = '0' + octet / 100;
    if (octet >= 10)
      buf[i++] = '0' + (octet / 10) % 10;
    buf[i++] = '0' + octet % 10;
    if (b > 0)
      buf[i++] = '.';
  }
  buf[i] = '\0';
}

/*
 * String to IP
 */
uint32_t str_to_ip(const char *str) {
  uint32_t ip = 0;
  uint32_t octet = 0;

  while (*str) {
    if (*str == '.') {
      ip = (ip << 8) | (octet & 0xFF);
      octet = 0;
    } else if (*str >= '0' && *str <= '9') {
      octet = octet * 10 + (*str - '0');
    }
    str++;
  }
  ip = (ip << 8) | (octet & 0xFF);

  return ip;
}
