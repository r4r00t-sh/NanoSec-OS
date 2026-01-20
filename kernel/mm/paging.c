/*
 * NanoSec OS - Paging (Virtual Memory)
 * ======================================
 * Page directory, page tables, and memory protection
 */

#include "../kernel.h"

/* Page flags */
#define PAGE_PRESENT 0x001
#define PAGE_WRITE 0x002
#define PAGE_USER 0x004
#define PAGE_ACCESSED 0x020
#define PAGE_DIRTY 0x040
#define PAGE_SIZE_4MB 0x080

/* Page size */
#define PAGE_SIZE 4096
#define PAGES_PER_TABLE 1024
#define TABLES_PER_DIR 1024

/* Kernel page directory and tables */
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t first_page_table[1024] __attribute__((aligned(4096)));

/* Physical memory bitmap */
#define PHYS_MEMORY_SIZE (32 * 1024 * 1024) /* 32 MB */
#define TOTAL_PAGES (PHYS_MEMORY_SIZE / PAGE_SIZE)
#define BITMAP_SIZE (TOTAL_PAGES / 32)

static uint32_t page_bitmap[BITMAP_SIZE];
static uint32_t free_pages = 0;

/*
 * Set a bit in the bitmap (mark page as used)
 */
static void bitmap_set(uint32_t page) {
  page_bitmap[page / 32] |= (1 << (page % 32));
}

/*
 * Clear a bit in the bitmap (mark page as free)
 */
static void bitmap_clear(uint32_t page) {
  page_bitmap[page / 32] &= ~(1 << (page % 32));
}

/*
 * Test a bit in the bitmap
 */
static int bitmap_test(uint32_t page) {
  return page_bitmap[page / 32] & (1 << (page % 32));
}

/*
 * Find first free page
 */
static uint32_t find_free_page(void) {
  for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
    if (page_bitmap[i] != 0xFFFFFFFF) {
      for (int j = 0; j < 32; j++) {
        if (!(page_bitmap[i] & (1 << j))) {
          return i * 32 + j;
        }
      }
    }
  }
  return 0xFFFFFFFF; /* No free pages */
}

/*
 * Allocate a physical page
 */
uint32_t page_alloc(void) {
  uint32_t page = find_free_page();
  if (page == 0xFFFFFFFF) {
    return 0;
  }

  bitmap_set(page);
  free_pages--;

  return page * PAGE_SIZE;
}

/*
 * Free a physical page
 */
void page_free(uint32_t phys_addr) {
  uint32_t page = phys_addr / PAGE_SIZE;
  if (page < TOTAL_PAGES && bitmap_test(page)) {
    bitmap_clear(page);
    free_pages++;
  }
}

/*
 * Initialize paging
 */
void paging_init(void) {
  /* Initialize bitmap - mark all as free first */
  memset(page_bitmap, 0, sizeof(page_bitmap));
  free_pages = TOTAL_PAGES;

  /* Reserve first 1MB (low memory, BIOS, etc.) */
  for (uint32_t i = 0; i < (1024 * 1024) / PAGE_SIZE; i++) {
    bitmap_set(i);
    free_pages--;
  }

  /* Reserve kernel memory (1MB - 4MB) */
  for (uint32_t i = 256; i < 1024; i++) {
    bitmap_set(i);
    free_pages--;
  }

  /* Clear page directory */
  memset(page_directory, 0, sizeof(page_directory));

  /* Identity map first 4MB using 4KB pages */
  for (int i = 0; i < 1024; i++) {
    first_page_table[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE;
  }

  /* Set first entry in page directory */
  page_directory[0] = ((uint32_t)first_page_table) | PAGE_PRESENT | PAGE_WRITE;

  /* Load page directory into CR3 */
  asm volatile("mov %0, %%cr3" : : "r"(page_directory));

  /* Enable paging in CR0 */
  uint32_t cr0;
  asm volatile("mov %%cr0, %0" : "=r"(cr0));
  cr0 |= 0x80000000; /* Set PG bit */
  asm volatile("mov %0, %%cr0" : : "r"(cr0));

  kprintf("  [OK] Paging (%d KB free)\n", (free_pages * 4));
}

/*
 * Map a virtual address to physical address
 */
void page_map(uint32_t virt, uint32_t phys, uint32_t flags) {
  uint32_t pd_index = virt >> 22;
  uint32_t pt_index = (virt >> 12) & 0x3FF;

  /* Check if page table exists */
  if (!(page_directory[pd_index] & PAGE_PRESENT)) {
    /* Allocate new page table */
    uint32_t pt_phys = page_alloc();
    if (!pt_phys)
      return;

    memset((void *)pt_phys, 0, PAGE_SIZE);
    page_directory[pd_index] = pt_phys | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
  }

  /* Get page table */
  uint32_t *pt = (uint32_t *)(page_directory[pd_index] & 0xFFFFF000);

  /* Set page table entry */
  pt[pt_index] = (phys & 0xFFFFF000) | (flags & 0xFFF) | PAGE_PRESENT;

  /* Invalidate TLB entry */
  asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

/*
 * Unmap a virtual address
 */
void page_unmap(uint32_t virt) {
  uint32_t pd_index = virt >> 22;
  uint32_t pt_index = (virt >> 12) & 0x3FF;

  if (!(page_directory[pd_index] & PAGE_PRESENT)) {
    return;
  }

  uint32_t *pt = (uint32_t *)(page_directory[pd_index] & 0xFFFFF000);
  pt[pt_index] = 0;

  asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

/*
 * Get physical address for virtual address
 */
uint32_t page_get_phys(uint32_t virt) {
  uint32_t pd_index = virt >> 22;
  uint32_t pt_index = (virt >> 12) & 0x3FF;

  if (!(page_directory[pd_index] & PAGE_PRESENT)) {
    return 0;
  }

  uint32_t *pt = (uint32_t *)(page_directory[pd_index] & 0xFFFFF000);

  if (!(pt[pt_index] & PAGE_PRESENT)) {
    return 0;
  }

  return (pt[pt_index] & 0xFFFFF000) | (virt & 0xFFF);
}

/*
 * Get free memory
 */
uint32_t page_get_free(void) { return free_pages * PAGE_SIZE; }
