/*
 * NanoSec OS - IDE/ATA Hard Disk Driver
 * ========================================
 * Primary IDE controller with PIO mode support
 */

#include "../kernel.h"

/* IDE controller I/O ports (Primary) */
#define IDE_DATA 0x1F0
#define IDE_ERROR 0x1F1
#define IDE_FEATURES 0x1F1
#define IDE_SECTOR_CNT 0x1F2
#define IDE_LBA_LO 0x1F3
#define IDE_LBA_MID 0x1F4
#define IDE_LBA_HI 0x1F5
#define IDE_DEVICE 0x1F6
#define IDE_STATUS 0x1F7
#define IDE_COMMAND 0x1F7
#define IDE_CTRL 0x3F6

/* IDE Status Register Bits */
#define IDE_STATUS_BSY 0x80  /* Busy */
#define IDE_STATUS_DRDY 0x40 /* Drive ready */
#define IDE_STATUS_DF 0x20   /* Drive fault */
#define IDE_STATUS_DRQ 0x08  /* Data request */
#define IDE_STATUS_ERR 0x01  /* Error */

/* IDE Commands */
#define IDE_CMD_READ_PIO 0x20
#define IDE_CMD_WRITE_PIO 0x30
#define IDE_CMD_IDENTIFY 0xEC
#define IDE_CMD_FLUSH 0xE7

/* Sector size */
#define SECTOR_SIZE 512

/* Drive info */
typedef struct {
  int present;
  int is_ata;
  uint32_t sectors;
  char model[41];
  char serial[21];
} ide_drive_t;

static ide_drive_t ide_drives[4]; /* Master/slave on primary/secondary */

/*
 * Wait for IDE to be ready
 */
static int ide_wait(int check_error) {
  uint8_t status;

  /* Wait until not busy */
  int timeout = 100000;
  while ((status = inb(IDE_STATUS)) & IDE_STATUS_BSY) {
    if (--timeout == 0)
      return -1;
  }

  if (check_error && (status & (IDE_STATUS_DF | IDE_STATUS_ERR))) {
    return -1;
  }

  return 0;
}

/*
 * Select drive
 */
static void ide_select_drive(int drive) {
  /* Drive 0 = 0xA0, Drive 1 = 0xB0 (for master/slave) */
  outb(IDE_DEVICE, 0xA0 | ((drive & 1) << 4));
  io_wait();
  io_wait();
  io_wait();
  io_wait();
}

/*
 * Identify drive
 */
static int ide_identify(int drive, ide_drive_t *info) {
  ide_select_drive(drive);

  /* Send IDENTIFY command */
  outb(IDE_SECTOR_CNT, 0);
  outb(IDE_LBA_LO, 0);
  outb(IDE_LBA_MID, 0);
  outb(IDE_LBA_HI, 0);
  outb(IDE_COMMAND, IDE_CMD_IDENTIFY);

  /* Check if drive exists */
  uint8_t status = inb(IDE_STATUS);
  if (status == 0) {
    info->present = 0;
    return -1;
  }

  /* Wait for response */
  if (ide_wait(0) < 0) {
    info->present = 0;
    return -1;
  }

  /* Check if it's ATA (not ATAPI) */
  uint8_t mid = inb(IDE_LBA_MID);
  uint8_t hi = inb(IDE_LBA_HI);

  if (mid != 0 || hi != 0) {
    info->present = 0;
    return -1; /* ATAPI or SATA */
  }

  /* Wait for DRQ */
  while (!(inb(IDE_STATUS) & IDE_STATUS_DRQ))
    ;

  /* Read identify data */
  uint16_t data[256];
  for (int i = 0; i < 256; i++) {
    data[i] = inb(IDE_DATA) | (inb(IDE_DATA) << 8);
  }

  info->present = 1;
  info->is_ata = 1;

  /* Get sector count (LBA28) */
  info->sectors = data[60] | (data[61] << 16);

  /* Get model string (swap bytes) */
  for (int i = 0; i < 20; i++) {
    info->model[i * 2] = data[27 + i] >> 8;
    info->model[i * 2 + 1] = data[27 + i] & 0xFF;
  }
  info->model[40] = '\0';

  /* Trim trailing spaces */
  for (int i = 39; i >= 0 && info->model[i] == ' '; i--) {
    info->model[i] = '\0';
  }

  return 0;
}

/*
 * Initialize IDE controller
 */
int ide_init(void) {
  int found = 0;

  /* Detect drives on primary controller */
  for (int i = 0; i < 2; i++) {
    memset(&ide_drives[i], 0, sizeof(ide_drive_t));

    if (ide_identify(i, &ide_drives[i]) == 0) {
      kprintf("  [OK] IDE%d: %s (%d MB)\n", i, ide_drives[i].model,
              ide_drives[i].sectors / 2048);
      found++;
    }
  }

  return found;
}

/*
 * Read sectors from disk
 */
int ide_read(int drive, uint32_t lba, uint8_t count, void *buffer) {
  if (drive < 0 || drive > 1 || !ide_drives[drive].present) {
    return -1;
  }

  ide_wait(0);

  /* Select drive with LBA mode */
  outb(IDE_DEVICE, 0xE0 | ((drive & 1) << 4) | ((lba >> 24) & 0x0F));

  /* Set sector count and LBA */
  outb(IDE_SECTOR_CNT, count);
  outb(IDE_LBA_LO, lba & 0xFF);
  outb(IDE_LBA_MID, (lba >> 8) & 0xFF);
  outb(IDE_LBA_HI, (lba >> 16) & 0xFF);

  /* Send read command */
  outb(IDE_COMMAND, IDE_CMD_READ_PIO);

  /* Read sectors */
  uint16_t *buf = (uint16_t *)buffer;

  for (int sector = 0; sector < count; sector++) {
    if (ide_wait(1) < 0) {
      return -1;
    }

    /* Wait for DRQ */
    while (!(inb(IDE_STATUS) & IDE_STATUS_DRQ))
      ;

    /* Read 256 words (512 bytes) */
    for (int i = 0; i < 256; i++) {
      *buf++ = inb(IDE_DATA) | (inb(IDE_DATA) << 8);
    }
  }

  return 0;
}

/*
 * Write sectors to disk
 */
int ide_write(int drive, uint32_t lba, uint8_t count, const void *buffer) {
  if (drive < 0 || drive > 1 || !ide_drives[drive].present) {
    return -1;
  }

  ide_wait(0);

  /* Select drive with LBA mode */
  outb(IDE_DEVICE, 0xE0 | ((drive & 1) << 4) | ((lba >> 24) & 0x0F));

  /* Set sector count and LBA */
  outb(IDE_SECTOR_CNT, count);
  outb(IDE_LBA_LO, lba & 0xFF);
  outb(IDE_LBA_MID, (lba >> 8) & 0xFF);
  outb(IDE_LBA_HI, (lba >> 16) & 0xFF);

  /* Send write command */
  outb(IDE_COMMAND, IDE_CMD_WRITE_PIO);

  /* Write sectors */
  const uint16_t *buf = (const uint16_t *)buffer;

  for (int sector = 0; sector < count; sector++) {
    if (ide_wait(0) < 0) {
      return -1;
    }

    /* Wait for DRQ */
    while (!(inb(IDE_STATUS) & IDE_STATUS_DRQ))
      ;

    /* Write 256 words (512 bytes) */
    for (int i = 0; i < 256; i++) {
      outb(IDE_DATA, *buf & 0xFF);
      outb(IDE_DATA, (*buf >> 8) & 0xFF);
      buf++;
    }
  }

  /* Flush cache */
  outb(IDE_COMMAND, IDE_CMD_FLUSH);
  ide_wait(0);

  return 0;
}

/*
 * Get drive info
 */
int ide_get_info(int drive, uint32_t *sectors) {
  if (drive < 0 || drive > 1 || !ide_drives[drive].present) {
    return -1;
  }

  *sectors = ide_drives[drive].sectors;
  return 0;
}
