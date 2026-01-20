/*
 * NanoSec OS - FAT32 Filesystem
 * ===============================
 * FAT32 read/write support
 */

#include "../kernel.h"

/* FAT32 structures */
typedef struct __attribute__((packed)) {
  uint8_t jmp[3];
  char oem[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t num_fats;
  uint16_t root_entries;     /* 0 for FAT32 */
  uint16_t total_sectors_16; /* 0 for FAT32 */
  uint8_t media_type;
  uint16_t fat_size_16; /* 0 for FAT32 */
  uint16_t sectors_per_track;
  uint16_t num_heads;
  uint32_t hidden_sectors;
  uint32_t total_sectors_32;
  /* FAT32 specific */
  uint32_t fat_size_32;
  uint16_t ext_flags;
  uint16_t fs_version;
  uint32_t root_cluster;
  uint16_t fs_info_sector;
  uint16_t backup_boot_sector;
  uint8_t reserved[12];
  uint8_t drive_number;
  uint8_t reserved1;
  uint8_t boot_sig;
  uint32_t volume_id;
  char volume_label[11];
  char fs_type[8];
} fat32_bpb_t;

/* Directory entry */
typedef struct __attribute__((packed)) {
  char name[8];
  char ext[3];
  uint8_t attr;
  uint8_t reserved;
  uint8_t create_time_tenths;
  uint16_t create_time;
  uint16_t create_date;
  uint16_t access_date;
  uint16_t cluster_hi;
  uint16_t mod_time;
  uint16_t mod_date;
  uint16_t cluster_lo;
  uint32_t size;
} fat32_dir_entry_t;

/* Long filename entry */
typedef struct __attribute__((packed)) {
  uint8_t order;
  uint16_t name1[5];
  uint8_t attr;
  uint8_t type;
  uint8_t checksum;
  uint16_t name2[6];
  uint16_t cluster;
  uint16_t name3[2];
} fat32_lfn_entry_t;

/* File attributes */
#define FAT_ATTR_READ_ONLY 0x01
#define FAT_ATTR_HIDDEN 0x02
#define FAT_ATTR_SYSTEM 0x04
#define FAT_ATTR_VOLUME_ID 0x08
#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_ARCHIVE 0x20
#define FAT_ATTR_LFN 0x0F

/* FAT32 state */
static struct {
  int mounted;
  int drive;
  uint32_t fat_start;
  uint32_t data_start;
  uint32_t root_cluster;
  uint32_t sectors_per_cluster;
  uint32_t bytes_per_cluster;
  uint32_t fat_size;
  uint8_t *fat_cache; /* One sector of FAT */
  uint32_t fat_cache_sector;
  uint8_t *cluster_buf; /* One cluster buffer */
} fat32;

/* Sector buffer */
static uint8_t sector_buf[512];

/* External IDE functions */
extern int ide_read(int drive, uint32_t lba, uint8_t count, void *buffer);
extern int ide_write(int drive, uint32_t lba, uint8_t count,
                     const void *buffer);

/*
 * Read FAT entry
 */
static uint32_t fat32_read_fat(uint32_t cluster) {
  uint32_t offset = cluster * 4;
  uint32_t sector = fat32.fat_start + (offset / 512);
  uint32_t entry_offset = offset % 512;

  /* Read FAT sector if not cached */
  if (fat32.fat_cache_sector != sector) {
    ide_read(fat32.drive, sector, 1, fat32.fat_cache);
    fat32.fat_cache_sector = sector;
  }

  return *(uint32_t *)(fat32.fat_cache + entry_offset) & 0x0FFFFFFF;
}

/*
 * Write FAT entry
 */
static void fat32_write_fat(uint32_t cluster, uint32_t value) {
  uint32_t offset = cluster * 4;
  uint32_t sector = fat32.fat_start + (offset / 512);
  uint32_t entry_offset = offset % 512;

  /* Read FAT sector if not cached */
  if (fat32.fat_cache_sector != sector) {
    ide_read(fat32.drive, sector, 1, fat32.fat_cache);
    fat32.fat_cache_sector = sector;
  }

  *(uint32_t *)(fat32.fat_cache + entry_offset) =
      (*(uint32_t *)(fat32.fat_cache + entry_offset) & 0xF0000000) |
      (value & 0x0FFFFFFF);

  ide_write(fat32.drive, sector, 1, fat32.fat_cache);
}

/*
 * Read cluster
 */
static int fat32_read_cluster(uint32_t cluster, void *buffer) {
  uint32_t lba = fat32.data_start + (cluster - 2) * fat32.sectors_per_cluster;
  return ide_read(fat32.drive, lba, fat32.sectors_per_cluster, buffer);
}

/*
 * Mount FAT32 filesystem
 */
int fat32_mount(int drive) {
  fat32_bpb_t *bpb = (fat32_bpb_t *)sector_buf;

  /* Read boot sector */
  if (ide_read(drive, 0, 1, sector_buf) < 0) {
    return -1;
  }

  /* Verify FAT32 signature */
  if (bpb->boot_sig != 0x29) {
    return -2;
  }

  /* Check for FAT32 */
  if (bpb->fat_size_16 != 0 || bpb->root_entries != 0) {
    return -3; /* Not FAT32 */
  }

  /* Store parameters */
  fat32.drive = drive;
  fat32.sectors_per_cluster = bpb->sectors_per_cluster;
  fat32.bytes_per_cluster = bpb->sectors_per_cluster * bpb->bytes_per_sector;
  fat32.fat_start = bpb->reserved_sectors;
  fat32.fat_size = bpb->fat_size_32;
  fat32.data_start = bpb->reserved_sectors + (bpb->num_fats * bpb->fat_size_32);
  fat32.root_cluster = bpb->root_cluster;

  /* Allocate buffers */
  fat32.fat_cache = kmalloc(512);
  fat32.cluster_buf = kmalloc(fat32.bytes_per_cluster);
  fat32.fat_cache_sector = 0xFFFFFFFF;

  if (!fat32.fat_cache || !fat32.cluster_buf) {
    return -4;
  }

  fat32.mounted = 1;

  kprintf("  [OK] FAT32 (cluster size: %d bytes)\n", fat32.bytes_per_cluster);

  return 0;
}

/*
 * Convert 8.3 name to normal name
 */
static void fat32_get_name(fat32_dir_entry_t *entry, char *name) {
  int i, j = 0;

  /* Copy name (remove trailing spaces) */
  for (i = 0; i < 8 && entry->name[i] != ' '; i++) {
    name[j++] = entry->name[i];
  }

  /* Add extension if present */
  if (entry->ext[0] != ' ') {
    name[j++] = '.';
    for (i = 0; i < 3 && entry->ext[i] != ' '; i++) {
      name[j++] = entry->ext[i];
    }
  }

  name[j] = '\0';
}

/*
 * List directory
 */
int fat32_list_dir(uint32_t cluster,
                   void (*callback)(const char *name, uint32_t size,
                                    int is_dir)) {
  if (!fat32.mounted)
    return -1;

  while (cluster < 0x0FFFFFF8) {
    if (fat32_read_cluster(cluster, fat32.cluster_buf) < 0) {
      return -1;
    }

    fat32_dir_entry_t *entries = (fat32_dir_entry_t *)fat32.cluster_buf;
    int num_entries = fat32.bytes_per_cluster / sizeof(fat32_dir_entry_t);

    for (int i = 0; i < num_entries; i++) {
      /* End of directory */
      if (entries[i].name[0] == 0x00) {
        return 0;
      }

      /* Skip deleted entries */
      if ((uint8_t)entries[i].name[0] == 0xE5) {
        continue;
      }

      /* Skip LFN and volume label */
      if (entries[i].attr == FAT_ATTR_LFN ||
          (entries[i].attr & FAT_ATTR_VOLUME_ID)) {
        continue;
      }

      char name[13];
      fat32_get_name(&entries[i], name);

      callback(name, entries[i].size,
               (entries[i].attr & FAT_ATTR_DIRECTORY) ? 1 : 0);
    }

    cluster = fat32_read_fat(cluster);
  }

  return 0;
}

/*
 * Read file
 */
int fat32_read_file(uint32_t start_cluster, void *buffer, uint32_t size) {
  if (!fat32.mounted)
    return -1;

  uint8_t *buf = (uint8_t *)buffer;
  uint32_t bytes_read = 0;
  uint32_t cluster = start_cluster;

  while (bytes_read < size && cluster < 0x0FFFFFF8) {
    if (fat32_read_cluster(cluster, fat32.cluster_buf) < 0) {
      return -1;
    }

    uint32_t to_copy = fat32.bytes_per_cluster;
    if (bytes_read + to_copy > size) {
      to_copy = size - bytes_read;
    }

    memcpy(buf + bytes_read, fat32.cluster_buf, to_copy);
    bytes_read += to_copy;

    cluster = fat32_read_fat(cluster);
  }

  return bytes_read;
}
