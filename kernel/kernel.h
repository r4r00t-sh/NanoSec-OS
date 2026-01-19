/*
 * NanoSec OS - Kernel Header
 * ===========================
 */

#ifndef _KERNEL_H
#define _KERNEL_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Kernel state structure */
typedef struct {
  uint8_t initialized;
  uint8_t firewall_active;
  uint8_t secmon_active;
  uint8_t fim_active;
  uint32_t uptime_seconds;
  uint32_t alert_count;
} kernel_state_t;

/* VGA Colors */
typedef enum {
  VGA_COLOR_BLACK = 0,
  VGA_COLOR_BLUE = 1,
  VGA_COLOR_GREEN = 2,
  VGA_COLOR_CYAN = 3,
  VGA_COLOR_RED = 4,
  VGA_COLOR_MAGENTA = 5,
  VGA_COLOR_BROWN = 6,
  VGA_COLOR_LIGHT_GREY = 7,
  VGA_COLOR_DARK_GREY = 8,
  VGA_COLOR_LIGHT_BLUE = 9,
  VGA_COLOR_LIGHT_GREEN = 10,
  VGA_COLOR_LIGHT_CYAN = 11,
  VGA_COLOR_LIGHT_RED = 12,
  VGA_COLOR_LIGHT_MAGENTA = 13,
  VGA_COLOR_YELLOW = 14,
  VGA_COLOR_WHITE = 15,
} vga_color_t;

/* ============================================
 * Core Functions
 * ============================================ */

void kernel_main(void);
void kernel_panic(const char *message);

void kprintf(const char *fmt, ...);
void kprintf_color(const char *str, vga_color_t color);

/* Memory functions */
void *memset(void *ptr, int value, size_t num);
void *memcpy(void *dest, const void *src, size_t num);
int memcmp(const void *ptr1, const void *ptr2, size_t num);
size_t strlen(const char *str);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);

/* ============================================
 * I/O Port Access (inline assembly)
 * ============================================ */

static inline void outb(uint16_t port, uint8_t val) {
  asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline void io_wait(void) { outb(0x80, 0); }

/* ============================================
 * VGA Driver
 * ============================================ */

void vga_init(void);
void vga_clear(void);
void vga_putchar(char c);
void vga_puts(const char *str);
void vga_set_color(vga_color_t color);
vga_color_t vga_get_color(void);

/* ============================================
 * Keyboard Driver
 * ============================================ */

int keyboard_init(void);
char keyboard_getchar(void);
int keyboard_available(void);

/* ============================================
 * Timer
 * ============================================ */

int timer_init(uint32_t freq);
uint32_t timer_get_ticks(void);
uint32_t timer_get_uptime(void);
void timer_delay_ms(uint32_t ms);
void delay(uint32_t count);

/* ============================================
 * Memory Management
 * ============================================ */

void mm_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
void mm_status(void);
void mm_stats(size_t *allocated, size_t *free);

/* String functions */
void *memset(void *ptr, int value, size_t num);
void *memcpy(void *dest, const void *src, size_t num);
int memcmp(const void *p1, const void *p2, size_t num);
size_t strlen(const char *str);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);

/* ============================================
 * Security - Firewall
 * ============================================ */

int firewall_init(void);
int firewall_check_packet(void *packet, size_t len);
void firewall_block_ip(uint32_t ip);
void firewall_status(void);
void firewall_enable(int enable);

/* ============================================
 * Security - Monitor
 * ============================================ */

int secmon_init(void);
void secmon_log(const char *event, int severity);
void secmon_alert(const char *message);
void secmon_status(void);
void secmon_enable(int enable);
int secmon_get_alert_count(void);
void secmon_acknowledge_alerts(void);
void secmon_show_logs(int count);

/* ============================================
 * Shell
 * ============================================ */

void shell_execute(const char *cmd);

/* ============================================
 * Network Commands (replaces net-tools)
 * ============================================ */

int net_init(void);
void cmd_nifconfig(const char *args);
void cmd_nroute(const char *args);
void cmd_nnetstat(const char *args);
void cmd_nping(const char *args);
void cmd_narp(const char *args);
void cmd_ndns(const char *args);

/* ============================================
 * System Commands
 * ============================================ */

void cmd_sysinfo(const char *args);
void cmd_ps(const char *args);
void cmd_uptime(const char *args);
void cmd_date(const char *args);
void cmd_whoami(const char *args);
void cmd_hostname(const char *args);
void cmd_uname(const char *args);
void cpu_detect(void);

/* ============================================
 * Filesystem
 * ============================================ */

int fs_init(void);
int fs_write(const char *name, const char *data, size_t len);
int fs_read(const char *name, char *buf, size_t max);

void cmd_ls(const char *args);
void cmd_cat(const char *args);
void cmd_touch(const char *args);
void cmd_rm(const char *args);
void cmd_pwd(const char *args);
void cmd_nedit(const char *args);
void cmd_hexdump(const char *args);

/* ============================================
 * User Authentication
 * ============================================ */

int user_init(void);
int user_login(const char *username, const char *password);
void user_logout(void);
int user_is_root(void);
uint16_t user_get_uid(void);
const char *user_get_username(void);
int user_add(const char *username, const char *password, int is_admin);
int user_check_permission(uint16_t file_uid, uint16_t file_gid,
                          uint16_t file_mode, int access_type);

void cmd_login(const char *args);
void cmd_logout(const char *args);
void cmd_whoami_user(const char *args);
void cmd_id(const char *args);
void cmd_adduser(const char *args);
void cmd_deluser(const char *args);
void cmd_passwd_user(const char *args);
void cmd_su(const char *args);
void cmd_users(const char *args);

/* ============================================
 * PC Speaker
 * ============================================ */

void speaker_beep(uint32_t freq, uint32_t duration_ms);
void speaker_startup(void);
void speaker_error(void);
void speaker_alert(void);
void cmd_beep(const char *args);
void cmd_play(const char *args);

/* ============================================
 * RTC (Real Time Clock)
 * ============================================ */

void cmd_date_rtc(const char *args);
void cmd_time(const char *args);
void cmd_cal(const char *args);

/* ============================================
 * Extended File Commands
 * ============================================ */

void cmd_cp(const char *args);
void cmd_mv(const char *args);
void cmd_head(const char *args);
void cmd_tail(const char *args);
void cmd_wc(const char *args);
void cmd_grep(const char *args);
void cmd_chmod(const char *args);
void cmd_chown(const char *args);
void cmd_ls_long(const char *args);
void perms_init(void);

/* ============================================
 * Environment Variables
 * ============================================ */

void env_init(void);
int env_set(const char *name, const char *value);
const char *env_get(const char *name);
int env_unset(const char *name);
void env_expand(const char *src, char *dst, size_t max);
void cmd_export(const char *args);
void cmd_env(const char *args);
void cmd_unset(const char *args);

/* ============================================
 * Serial Console
 * ============================================ */

int serial_init(uint16_t base, int baud_divisor);
void serial_putchar(char c);
void serial_puts(const char *str);
void klog(const char *msg);
void cmd_dmesg(const char *args);

/* ============================================
 * Command History & Aliases
 * ============================================ */

void history_add(const char *cmd);
const char *history_prev(void);
const char *history_next(void);
void cmd_history(const char *args);
void alias_init(void);
int alias_set(const char *name, const char *command);
const char *alias_get(const char *name);
void cmd_alias(const char *args);
void cmd_unalias(const char *args);

/* ============================================
 * Advanced Security
 * ============================================ */

uint32_t password_hash(const char *password);
void audit_init(void);
void audit_log_cmd(const char *command);
void cmd_audit(const char *args);
int sudo_check(void);
void cmd_sudo(const char *args);
void cmd_lock(const char *args);

#endif /* _KERNEL_H */
