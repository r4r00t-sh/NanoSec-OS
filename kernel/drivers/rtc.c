/*
 * NanoSec OS - RTC (Real Time Clock) Driver
 * ==========================================
 * Read date/time from CMOS RTC
 */

#include "../kernel.h"

/* CMOS ports */
#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

/* RTC registers */
#define RTC_SECONDS 0x00
#define RTC_MINUTES 0x02
#define RTC_HOURS 0x04
#define RTC_DAY 0x07
#define RTC_MONTH 0x08
#define RTC_YEAR 0x09
#define RTC_STATUS_A 0x0A
#define RTC_STATUS_B 0x0B

/* Time structure */
typedef struct {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t day;
  uint8_t month;
  uint16_t year;
} rtc_time_t;

static rtc_time_t current_time;

/*
 * Read CMOS register
 */
static uint8_t cmos_read(uint8_t reg) {
  outb(CMOS_ADDR, reg);
  return inb(CMOS_DATA);
}

/*
 * Check if RTC update in progress
 */
static int rtc_update_in_progress(void) {
  return cmos_read(RTC_STATUS_A) & 0x80;
}

/*
 * Convert BCD to binary
 */
static uint8_t bcd_to_bin(uint8_t bcd) {
  return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/*
 * Read current time from RTC
 */
void rtc_read_time(rtc_time_t *time) {
  /* Wait for update to complete */
  while (rtc_update_in_progress())
    ;

  time->second = cmos_read(RTC_SECONDS);
  time->minute = cmos_read(RTC_MINUTES);
  time->hour = cmos_read(RTC_HOURS);
  time->day = cmos_read(RTC_DAY);
  time->month = cmos_read(RTC_MONTH);
  time->year = cmos_read(RTC_YEAR);

  /* Check if BCD mode (bit 2 of status B) */
  uint8_t status_b = cmos_read(RTC_STATUS_B);
  if (!(status_b & 0x04)) {
    /* Convert from BCD */
    time->second = bcd_to_bin(time->second);
    time->minute = bcd_to_bin(time->minute);
    time->hour = bcd_to_bin(time->hour & 0x7F) | (time->hour & 0x80);
    time->day = bcd_to_bin(time->day);
    time->month = bcd_to_bin(time->month);
    time->year = bcd_to_bin(time->year);
  }

  /* Handle 12-hour format */
  if (!(status_b & 0x02) && (time->hour & 0x80)) {
    time->hour = ((time->hour & 0x7F) + 12) % 24;
  }

  /* Assume 21st century */
  time->year += 2000;
}

/*
 * Get day of week name
 */
static const char *day_name(int day) {
  const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  return days[day % 7];
}

/*
 * Get month name
 */
static const char *month_name(int month) {
  const char *months[] = {"",    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  if (month < 1 || month > 12)
    return "???";
  return months[month];
}

/*
 * Calculate day of week (Zeller's formula simplified)
 */
static int calc_day_of_week(int day, int month, int year) {
  if (month < 3) {
    month += 12;
    year--;
  }
  int k = year % 100;
  int j = year / 100;
  int h = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;
  return ((h + 6) % 7);
}

/* ============ Shell Commands ============ */

/*
 * date command - show current date/time
 */
void cmd_date_rtc(const char *args) {
  (void)args;

  rtc_read_time(&current_time);

  int dow =
      calc_day_of_week(current_time.day, current_time.month, current_time.year);

  kprintf("%s %s %d %02d:%02d:%02d %d\n", day_name(dow),
          month_name(current_time.month), current_time.day, current_time.hour,
          current_time.minute, current_time.second, current_time.year);
}

/*
 * time command - show just the time
 */
void cmd_time(const char *args) {
  (void)args;

  rtc_read_time(&current_time);
  kprintf("%02d:%02d:%02d\n", current_time.hour, current_time.minute,
          current_time.second);
}

/*
 * cal command - show simple calendar
 */
void cmd_cal(const char *args) {
  (void)args;

  rtc_read_time(&current_time);

  kprintf("\n    %s %d\n", month_name(current_time.month), current_time.year);
  kprintf("Su Mo Tu We Th Fr Sa\n");

  /* Get first day of month */
  int first_dow = calc_day_of_week(1, current_time.month, current_time.year);

  /* Days in month */
  int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int max_day = days[current_time.month];

  /* Leap year check */
  if (current_time.month == 2) {
    int y = current_time.year;
    if ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0) {
      max_day = 29;
    }
  }

  /* Print calendar */
  for (int i = 0; i < first_dow; i++) {
    kprintf("   ");
  }

  for (int d = 1; d <= max_day; d++) {
    if (d == current_time.day) {
      kprintf_color("", VGA_COLOR_BLACK);
      vga_set_color(VGA_COLOR_WHITE);
      kprintf("%2d", d);
      vga_set_color(VGA_COLOR_LIGHT_GREY);
    } else {
      kprintf("%2d", d);
    }
    kprintf(" ");

    if ((first_dow + d) % 7 == 0) {
      kprintf("\n");
    }
  }
  kprintf("\n\n");
}
