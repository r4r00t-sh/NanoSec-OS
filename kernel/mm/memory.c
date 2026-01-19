/*
 * NanoSec OS - Memory Manager
 */

#include "../kernel.h"

#define HEAP_START 0x100000
#define HEAP_SIZE 0x100000 /* 1MB heap */

typedef struct block {
  uint32_t size;
  uint8_t free;
  struct block *next;
} block_t;

static block_t *heap_start = NULL;
static size_t total_allocated = 0;
static size_t total_free = 0;

void mm_init(void) {
  heap_start = (block_t *)HEAP_START;
  heap_start->size = HEAP_SIZE - sizeof(block_t);
  heap_start->free = 1;
  heap_start->next = NULL;
  total_free = heap_start->size;
  total_allocated = 0;
  kprintf("  Memory: %d KB heap at 0x%x\n", HEAP_SIZE / 1024, HEAP_START);
}

void *kmalloc(size_t size) {
  block_t *curr = heap_start;
  while (curr) {
    if (curr->free && curr->size >= size) {
      if (curr->size > size + sizeof(block_t) + 16) {
        block_t *new_block =
            (block_t *)((uint8_t *)curr + sizeof(block_t) + size);
        new_block->size = curr->size - size - sizeof(block_t);
        new_block->free = 1;
        new_block->next = curr->next;
        curr->next = new_block;
        curr->size = size;
      }
      curr->free = 0;
      total_allocated += curr->size;
      total_free -= curr->size;
      return (void *)((uint8_t *)curr + sizeof(block_t));
    }
    curr = curr->next;
  }
  return NULL;
}

void kfree(void *ptr) {
  if (!ptr)
    return;
  block_t *block = (block_t *)((uint8_t *)ptr - sizeof(block_t));
  block->free = 1;
  total_allocated -= block->size;
  total_free += block->size;
}

void mm_stats(size_t *allocated, size_t *free) {
  if (allocated)
    *allocated = total_allocated;
  if (free)
    *free = total_free;
}

void mm_status(void) {
  kprintf("\n=== Memory Status ===\n");
  kprintf("Heap: 0x%x\n", HEAP_START);
  kprintf("Allocated: %d bytes\n", (int)total_allocated);
  kprintf("Free: %d bytes\n", (int)total_free);
}

/* String functions */
void *memset(void *ptr, int value, size_t num) {
  uint8_t *p = (uint8_t *)ptr;
  while (num--)
    *p++ = (uint8_t)value;
  return ptr;
}

void *memcpy(void *dest, const void *src, size_t num) {
  uint8_t *d = (uint8_t *)dest;
  const uint8_t *s = (const uint8_t *)src;
  while (num--)
    *d++ = *s++;
  return dest;
}

int memcmp(const void *p1, const void *p2, size_t num) {
  const uint8_t *a = (const uint8_t *)p1;
  const uint8_t *b = (const uint8_t *)p2;
  while (num--) {
    if (*a != *b)
      return *a - *b;
    a++;
    b++;
  }
  return 0;
}

size_t strlen(const char *str) {
  size_t len = 0;
  while (*str++)
    len++;
  return len;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

char *strcpy(char *dest, const char *src) {
  char *d = dest;
  while ((*d++ = *src++))
    ;
  return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
  char *d = dest;
  while (n && (*d++ = *src++))
    n--;
  while (n--)
    *d++ = '\0';
  return dest;
}

char *strcat(char *dest, const char *src) {
  char *d = dest;
  while (*d)
    d++;
  while ((*d++ = *src++))
    ;
  return dest;
}
