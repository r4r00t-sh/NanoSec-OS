/*
 * NanoSec OS - USB Host Controller
 * ==================================
 * UHCI (USB 1.1) controller driver framework
 */

#include "../kernel.h"

/* UHCI registers */
#define UHCI_CMD 0x00
#define UHCI_STS 0x02
#define UHCI_INTR 0x04
#define UHCI_FRNUM 0x06
#define UHCI_FRBASEADD 0x08
#define UHCI_SOFMOD 0x0C
#define UHCI_PORTSC1 0x10
#define UHCI_PORTSC2 0x12

/* Command register bits */
#define UHCI_CMD_RS 0x0001      /* Run/Stop */
#define UHCI_CMD_HCRESET 0x0002 /* Host controller reset */
#define UHCI_CMD_GRESET 0x0004  /* Global reset */
#define UHCI_CMD_MAXP 0x0080    /* Max packet (64 bytes) */

/* Status register bits */
#define UHCI_STS_HCHALTED 0x0020

/* Port status bits */
#define UHCI_PORTSC_CCS 0x0001  /* Current connect status */
#define UHCI_PORTSC_CSC 0x0002  /* Connect status change */
#define UHCI_PORTSC_PED 0x0004  /* Port enabled */
#define UHCI_PORTSC_PEDC 0x0008 /* Port enable change */
#define UHCI_PORTSC_LSDA 0x0100 /* Low speed device attached */
#define UHCI_PORTSC_PR 0x0200   /* Port reset */

/* USB device structure */
typedef struct {
  int address;
  int speed; /* 0 = full, 1 = low */
  int port;
} usb_device_t;

#define MAX_USB_DEVICES 8
static usb_device_t usb_devices[MAX_USB_DEVICES];
static int usb_device_count = 0;

/* Controller state */
static uint16_t uhci_base = 0;
static int uhci_initialized = 0;

/* Frame list (1024 entries, 4KB aligned) */
static uint32_t frame_list[1024] __attribute__((aligned(4096)));

/* External PCI functions */
extern void *pci_find_device_class(uint8_t class_code, uint8_t subclass);
extern uint32_t pci_get_bar_addr(void *dev, int bar);
extern void pci_enable_bus_master(void *dev);

/*
 * Read UHCI register
 */
static uint16_t uhci_read16(uint16_t reg) {
  return inb(uhci_base + reg) | (inb(uhci_base + reg + 1) << 8);
}

/*
 * Write UHCI register
 */
static void uhci_write16(uint16_t reg, uint16_t value) {
  outb(uhci_base + reg, value & 0xFF);
  outb(uhci_base + reg + 1, (value >> 8) & 0xFF);
}

static void uhci_write32(uint16_t reg, uint32_t value) {
  outb(uhci_base + reg, value & 0xFF);
  outb(uhci_base + reg + 1, (value >> 8) & 0xFF);
  outb(uhci_base + reg + 2, (value >> 16) & 0xFF);
  outb(uhci_base + reg + 3, (value >> 24) & 0xFF);
}

/*
 * Reset port
 */
static void uhci_reset_port(int port) {
  uint16_t reg = (port == 0) ? UHCI_PORTSC1 : UHCI_PORTSC2;

  /* Set port reset */
  uhci_write16(reg, UHCI_PORTSC_PR);

  /* Wait 50ms */
  for (volatile int i = 0; i < 500000; i++)
    ;

  /* Clear reset */
  uhci_write16(reg, 0);

  /* Wait for device */
  for (volatile int i = 0; i < 100000; i++)
    ;

  /* Enable port */
  uhci_write16(reg, UHCI_PORTSC_PED);
}

/*
 * Initialize UHCI controller
 */
int usb_init(void) {
  /* Find UHCI controller (class 0x0C, subclass 0x03) */
  void *pci_dev = pci_find_device_class(0x0C, 0x03);

  if (!pci_dev) {
    kprintf("  [--] No USB controller found\n");
    return -1;
  }

  /* Get I/O base address (BAR4) */
  uhci_base = pci_get_bar_addr(pci_dev, 4) & ~0x3;

  if (uhci_base == 0) {
    return -1;
  }

  /* Enable bus mastering */
  pci_enable_bus_master(pci_dev);

  /* Reset controller */
  uhci_write16(UHCI_CMD, UHCI_CMD_GRESET);
  for (volatile int i = 0; i < 1000000; i++)
    ;
  uhci_write16(UHCI_CMD, 0);

  uhci_write16(UHCI_CMD, UHCI_CMD_HCRESET);
  for (volatile int i = 0; i < 100000; i++)
    ;

  /* Wait for reset to complete */
  while (uhci_read16(UHCI_CMD) & UHCI_CMD_HCRESET)
    ;

  /* Initialize frame list (all point to terminate) */
  for (int i = 0; i < 1024; i++) {
    frame_list[i] = 0x00000001; /* Terminate */
  }

  /* Set frame list base address */
  uhci_write32(UHCI_FRBASEADD, (uint32_t)frame_list);

  /* Start frame number */
  uhci_write16(UHCI_FRNUM, 0);

  /* Enable interrupts (for now, disable all) */
  uhci_write16(UHCI_INTR, 0);

  /* Start controller */
  uhci_write16(UHCI_CMD, UHCI_CMD_RS | UHCI_CMD_MAXP);

  uhci_initialized = 1;

  /* Check ports for devices */
  usb_device_count = 0;

  for (int port = 0; port < 2; port++) {
    uint16_t reg = (port == 0) ? UHCI_PORTSC1 : UHCI_PORTSC2;
    uint16_t status = uhci_read16(reg);

    if (status & UHCI_PORTSC_CCS) {
      uhci_reset_port(port);

      /* Check if device still connected */
      status = uhci_read16(reg);
      if (status & UHCI_PORTSC_CCS) {
        usb_devices[usb_device_count].port = port;
        usb_devices[usb_device_count].speed =
            (status & UHCI_PORTSC_LSDA) ? 1 : 0;
        usb_devices[usb_device_count].address = 0;
        usb_device_count++;
      }
    }
  }

  kprintf("  [OK] USB UHCI (%d devices)\n", usb_device_count);

  return 0;
}

/*
 * Get USB device count
 */
int usb_get_device_count(void) { return usb_device_count; }
