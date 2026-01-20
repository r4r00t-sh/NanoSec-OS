/*
 * NanoSec OS - Advanced Security
 * ================================
 * ASLR, Memory Protection, Encryption
 */

#include "../kernel.h"

/* =============================================
 * ASLR - Address Space Layout Randomization
 * ============================================= */

static uint32_t aslr_seed = 0x12345678;

/* Simple PRNG for ASLR */
static uint32_t aslr_random(void) {
  aslr_seed = aslr_seed * 1103515245 + 12345;
  return (aslr_seed >> 16) & 0x7fff;
}

/* Initialize ASLR with entropy from timer */
void aslr_init(void) {
  aslr_seed = timer_get_ticks() ^ 0xDEADBEEF;
  /* Mix in some additional entropy */
  for (int i = 0; i < 10; i++) {
    aslr_seed ^= (aslr_random() << 16);
  }
  kprintf("  [OK] ASLR\n");
}

/* Get randomized stack base */
uint32_t aslr_get_stack_base(void) {
  /* Random offset within a 16MB range, page-aligned */
  uint32_t offset = (aslr_random() & 0x3FF) * 4096;
  return 0xBF000000 - offset;
}

/* Get randomized heap base */
uint32_t aslr_get_heap_base(void) {
  uint32_t offset = (aslr_random() & 0x1FF) * 4096;
  return 0x10000000 + offset;
}

/* Get randomized mmap base */
uint32_t aslr_get_mmap_base(void) {
  uint32_t offset = (aslr_random() & 0x7FF) * 4096;
  return 0x40000000 + offset;
}

/* =============================================
 * Stack Canary / Stack Smashing Protection
 * ============================================= */

static uint32_t stack_canary = 0;

void ssp_init(void) {
  /* Generate random canary */
  stack_canary = aslr_random() << 16;
  stack_canary |= aslr_random();
  stack_canary ^=
      0x00000A0D; /* Include newline chars to detect string overflows */
  kprintf("  [OK] Stack Protection\n");
}

uint32_t ssp_get_canary(void) { return stack_canary; }

void __attribute__((noreturn)) __stack_chk_fail(void) {
  kprintf_color("\n!!! STACK SMASHING DETECTED !!!\n", VGA_COLOR_RED);
  kprintf("Process terminated.\n");
  proc_exit(-1);
  for (;;)
    asm volatile("hlt");
}

/* =============================================
 * Simple XOR Encryption (for demo purposes)
 * ============================================= */

static uint8_t crypto_key[32] = {0};
static int crypto_initialized = 0;

void crypto_init(const uint8_t *key, size_t key_len) {
  if (key_len > 32)
    key_len = 32;
  memcpy(crypto_key, key, key_len);
  crypto_initialized = 1;
  kprintf("  [OK] Crypto Engine\n");
}

void crypto_encrypt(uint8_t *data, size_t len) {
  if (!crypto_initialized)
    return;

  for (size_t i = 0; i < len; i++) {
    data[i] ^= crypto_key[i % 32];
    /* Simple substitution for extra obfuscation */
    data[i] = (data[i] << 4) | (data[i] >> 4);
  }
}

void crypto_decrypt(uint8_t *data, size_t len) {
  if (!crypto_initialized)
    return;

  for (size_t i = 0; i < len; i++) {
    /* Reverse substitution */
    data[i] = (data[i] << 4) | (data[i] >> 4);
    data[i] ^= crypto_key[i % 32];
  }
}

/* =============================================
 * Memory Protection
 * ============================================= */

/* Check if address is in kernel space */
int mem_is_kernel(uint32_t addr) { return addr >= 0xC0000000; }

/* Check if address is valid user address */
int mem_is_user_valid(uint32_t addr, size_t len) {
  /* Check bounds */
  if (addr < 0x00400000)
    return 0; /* Below user space */
  if (addr >= 0xC0000000)
    return 0; /* Kernel space */
  if (addr + len < addr)
    return 0; /* Overflow */
  if (addr + len >= 0xC0000000)
    return 0;

  return 1;
}

/* Validate user pointer before kernel use */
int mem_verify_user_ptr(const void *ptr, size_t len, int write) {
  uint32_t addr = (uint32_t)ptr;
  (void)write; /* TODO: Check page permissions */

  return mem_is_user_valid(addr, len);
}

/* =============================================
 * Secure Memory Operations
 * ============================================= */

/* Zero memory securely (prevent compiler optimization) */
void secure_zero(void *ptr, size_t len) {
  volatile uint8_t *p = (volatile uint8_t *)ptr;
  while (len--) {
    *p++ = 0;
  }
}

/* Compare memory in constant time (timing-safe) */
int secure_compare(const void *a, const void *b, size_t len) {
  const volatile uint8_t *pa = (const volatile uint8_t *)a;
  const volatile uint8_t *pb = (const volatile uint8_t *)b;
  uint8_t diff = 0;

  for (size_t i = 0; i < len; i++) {
    diff |= pa[i] ^ pb[i];
  }

  return diff == 0;
}

/* =============================================
 * Security Initialization
 * ============================================= */

void security_tier7_init(void) {
  kprintf("[BOOT] Initializing advanced security...\n");

  aslr_init();
  ssp_init();

  /* Initialize crypto with a demo key */
  uint8_t demo_key[] = "NanoSecOS-SecurityKey-2026";
  crypto_init(demo_key, sizeof(demo_key) - 1);
}
