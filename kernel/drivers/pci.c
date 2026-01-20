/*
 * NanoSec OS - PCI Bus Driver
 * =============================
 * PCI device enumeration and configuration
 */

#include "../kernel.h"

/* PCI Configuration ports */
#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

/* PCI configuration offsets */
#define PCI_VENDOR_ID 0x00
#define PCI_DEVICE_ID 0x02
#define PCI_COMMAND 0x04
#define PCI_STATUS 0x06
#define PCI_REVISION 0x08
#define PCI_PROG_IF 0x09
#define PCI_SUBCLASS 0x0A
#define PCI_CLASS 0x0B
#define PCI_HEADER_TYPE 0x0E
#define PCI_BAR0 0x10
#define PCI_BAR1 0x14
#define PCI_BAR2 0x18
#define PCI_BAR3 0x1C
#define PCI_BAR4 0x20
#define PCI_BAR5 0x24
#define PCI_INTERRUPT_LINE 0x3C
#define PCI_INTERRUPT_PIN 0x3D

/* PCI device structure */
typedef struct {
  uint8_t bus;
  uint8_t slot;
  uint8_t func;
  uint16_t vendor_id;
  uint16_t device_id;
  uint8_t class_code;
  uint8_t subclass;
  uint8_t prog_if;
  uint8_t irq;
  uint32_t bar[6];
} pci_device_t;

#define MAX_PCI_DEVICES 64
static pci_device_t pci_devices[MAX_PCI_DEVICES];
static int pci_device_count = 0;

/* Class names */
static const char *pci_class_names[] = {
    "Unclassified", "Mass Storage", "Network",        "Display", "Multimedia",
    "Memory",       "Bridge",       "Communications", "System",  "Input",
    "Docking",      "Processor",    "Serial Bus"};

/*
 * Build PCI config address
 */
static uint32_t pci_config_addr(uint8_t bus, uint8_t slot, uint8_t func,
                                uint8_t offset) {
  return (1 << 31) | /* Enable */
         (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
}

/*
 * Read from PCI config space
 */
uint32_t pci_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
  outb(PCI_CONFIG_ADDR, pci_config_addr(bus, slot, func, offset));
  outb(PCI_CONFIG_ADDR + 1, pci_config_addr(bus, slot, func, offset) >> 8);
  outb(PCI_CONFIG_ADDR + 2, pci_config_addr(bus, slot, func, offset) >> 16);
  outb(PCI_CONFIG_ADDR + 3, pci_config_addr(bus, slot, func, offset) >> 24);

  return inb(PCI_CONFIG_DATA) | (inb(PCI_CONFIG_DATA + 1) << 8) |
         (inb(PCI_CONFIG_DATA + 2) << 16) | (inb(PCI_CONFIG_DATA + 3) << 24);
}

uint16_t pci_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
  uint32_t val = pci_read32(bus, slot, func, offset & ~3);
  return (val >> ((offset & 2) * 8)) & 0xFFFF;
}

uint8_t pci_read8(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
  uint32_t val = pci_read32(bus, slot, func, offset & ~3);
  return (val >> ((offset & 3) * 8)) & 0xFF;
}

/*
 * Write to PCI config space
 */
void pci_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,
                 uint32_t value) {
  outb(PCI_CONFIG_ADDR, pci_config_addr(bus, slot, func, offset));
  outb(PCI_CONFIG_ADDR + 1, pci_config_addr(bus, slot, func, offset) >> 8);
  outb(PCI_CONFIG_ADDR + 2, pci_config_addr(bus, slot, func, offset) >> 16);
  outb(PCI_CONFIG_ADDR + 3, pci_config_addr(bus, slot, func, offset) >> 24);

  outb(PCI_CONFIG_DATA, value);
  outb(PCI_CONFIG_DATA + 1, value >> 8);
  outb(PCI_CONFIG_DATA + 2, value >> 16);
  outb(PCI_CONFIG_DATA + 3, value >> 24);
}

/*
 * Check for device at bus/slot/func
 */
static int pci_check_device(uint8_t bus, uint8_t slot, uint8_t func) {
  uint16_t vendor = pci_read16(bus, slot, func, PCI_VENDOR_ID);

  if (vendor == 0xFFFF) {
    return 0; /* No device */
  }

  if (pci_device_count >= MAX_PCI_DEVICES) {
    return 0;
  }

  pci_device_t *dev = &pci_devices[pci_device_count++];

  dev->bus = bus;
  dev->slot = slot;
  dev->func = func;
  dev->vendor_id = vendor;
  dev->device_id = pci_read16(bus, slot, func, PCI_DEVICE_ID);
  dev->class_code = pci_read8(bus, slot, func, PCI_CLASS);
  dev->subclass = pci_read8(bus, slot, func, PCI_SUBCLASS);
  dev->prog_if = pci_read8(bus, slot, func, PCI_PROG_IF);
  dev->irq = pci_read8(bus, slot, func, PCI_INTERRUPT_LINE);

  /* Read BARs */
  for (int i = 0; i < 6; i++) {
    dev->bar[i] = pci_read32(bus, slot, func, PCI_BAR0 + i * 4);
  }

  return 1;
}

/*
 * Enumerate all PCI devices
 */
int pci_init(void) {
  pci_device_count = 0;

  /* Scan all buses */
  for (int bus = 0; bus < 256; bus++) {
    for (int slot = 0; slot < 32; slot++) {
      uint16_t vendor = pci_read16(bus, slot, 0, PCI_VENDOR_ID);

      if (vendor == 0xFFFF)
        continue;

      pci_check_device(bus, slot, 0);

      /* Check for multi-function device */
      uint8_t header = pci_read8(bus, slot, 0, PCI_HEADER_TYPE);
      if (header & 0x80) {
        for (int func = 1; func < 8; func++) {
          pci_check_device(bus, slot, func);
        }
      }
    }
  }

  kprintf("  [OK] PCI (%d devices)\n", pci_device_count);

  return pci_device_count;
}

/*
 * Find device by class
 */
pci_device_t *pci_find_device_class(uint8_t class_code, uint8_t subclass) {
  for (int i = 0; i < pci_device_count; i++) {
    if (pci_devices[i].class_code == class_code &&
        pci_devices[i].subclass == subclass) {
      return &pci_devices[i];
    }
  }
  return NULL;
}

/*
 * Find device by vendor/device ID
 */
pci_device_t *pci_find_device(uint16_t vendor, uint16_t device) {
  for (int i = 0; i < pci_device_count; i++) {
    if (pci_devices[i].vendor_id == vendor &&
        pci_devices[i].device_id == device) {
      return &pci_devices[i];
    }
  }
  return NULL;
}

/*
 * List all PCI devices
 */
void pci_list_devices(void) {
  kprintf("\n=== PCI Devices ===\n");

  for (int i = 0; i < pci_device_count; i++) {
    pci_device_t *dev = &pci_devices[i];
    const char *class_name =
        (dev->class_code < 13) ? pci_class_names[dev->class_code] : "Unknown";

    kprintf("%02x:%02x.%d %04x:%04x %s\n", dev->bus, dev->slot, dev->func,
            dev->vendor_id, dev->device_id, class_name);
  }

  kprintf("\n");
}

/*
 * Enable bus mastering for device
 */
void pci_enable_bus_master(pci_device_t *dev) {
  uint16_t cmd = pci_read16(dev->bus, dev->slot, dev->func, PCI_COMMAND);
  cmd |= 0x04; /* Bus master bit */
  pci_write32(dev->bus, dev->slot, dev->func, PCI_COMMAND, cmd);
}

/*
 * Get BAR address (memory or I/O)
 */
uint32_t pci_get_bar_addr(pci_device_t *dev, int bar) {
  if (bar < 0 || bar > 5)
    return 0;

  uint32_t bar_val = dev->bar[bar];

  if (bar_val & 1) {
    /* I/O space */
    return bar_val & ~0x3;
  } else {
    /* Memory space */
    return bar_val & ~0xF;
  }
}
