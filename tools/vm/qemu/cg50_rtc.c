/* SPDX-License-Identifier: GPL-2.0-or-later
 * SH7305 calendar RTC. Register layout and periods follow Renesas SH7724
 * chapter 28 and gint/include/gint/mpu/rtc.h.
 */
#include "qemu/osdep.h"
#include "qemu/cutils.h"
#include "qemu/timer.h"
#include "system/address-spaces.h"
#include "system/rtc.h"
#include "cg50_rtc.h"

#define RCR1 0x1c
#define RCR2 0x1e
#define SECOND 1000000000LL
static const int64_t periods[] = {0,          SECOND / 256, SECOND / 64, SECOND / 16,
                                  SECOND / 4, SECOND / 2,   SECOND,      2 * SECOND};

static unsigned bcd(unsigned n) {
  return (n / 10) * 16 + n % 10;
}
static unsigned unbcd(unsigned n) {
  return (n >> 4) * 10 + (n & 15);
}
static int64_t elapsed(CG50RTC * r) {
  return r->elapsed + ((r->regs[RCR2] & 1) ? qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) - r->anchor : 0);
}
static void calendar(CG50RTC * r, struct tm * tm) {
  time_t seconds = r->epoch + elapsed(r) / SECOND;
  gmtime_r(&seconds, tm);
}
unsigned cg50_rtc_irqs(CG50RTC * r) {
  unsigned flags = 0;
  if ((r->regs[RCR2] & 0x80) && (r->regs[RCR2] & 0x70))
    flags |= 2;
  if ((r->regs[RCR1] & 0x90) == 0x90)
    flags |= 1;
  if ((r->regs[RCR1] & 9) == 9)
    flags |= 4;
  return flags;
}
void cg50_rtc_update(CG50RTC * r) {
  int64_t ticks = elapsed(r), second = ticks / SECOND;
  if (second != r->seen_second) {
    struct tm tm;
    r->seen_second = second;
    r->regs[RCR1] |= 0x80;
    calendar(r, &tm);
    unsigned values[] = {bcd(tm.tm_sec), bcd(tm.tm_min),  bcd(tm.tm_hour),
                         tm.tm_wday,     bcd(tm.tm_mday), bcd(tm.tm_mon + 1)};
    bool enabled = false, match = true;
    for (unsigned i = 0; i < 6; i++) {
      uint8_t alarm = r->regs[0x10 + 2 * i];
      if (alarm & 0x80) {
        enabled = true;
        match &= (alarm & 0x7f) == values[i];
      }
    }
    if (enabled && match)
      r->regs[RCR1] |= 1;
  }
  unsigned selection = (r->regs[RCR2] >> 4) & 7;
  if (selection) {
    int64_t period = ticks / periods[selection];
    if (period != r->seen_period)
      r->regs[RCR2] |= 0x80;
    r->seen_period = period;
  }
  r->irq_changed(r->opaque);
}
static uint64_t rtc_read(void * opaque, hwaddr offset, unsigned size) {
  CG50RTC * r = opaque;
  if (offset + size > sizeof(r->regs))
    return 0;
  cg50_rtc_update(r);
  if (size > 1) {
    uint64_t result = 0;
    for (unsigned i = 0; i < size; i++)
      result = (result << 8) | rtc_read(r, offset + i, 1);
    return result;
  }
  struct tm tm;
  calendar(r, &tm);
  unsigned year = tm.tm_year + 1900;
  switch (offset) {
  case 0:
    return (elapsed(r) % SECOND) * 128 / SECOND;
  case 2:
    return bcd(tm.tm_sec);
  case 4:
    return bcd(tm.tm_min);
  case 6:
    return bcd(tm.tm_hour);
  case 8:
    return tm.tm_wday;
  case 10:
    return bcd(tm.tm_mday);
  case 12:
    return bcd(tm.tm_mon + 1);
  case 14:
    return bcd(year / 100);
  case 15:
    return bcd(year % 100);
  default:
    return r->regs[offset];
  }
}
static void rtc_write(void * opaque, hwaddr offset, uint64_t value, unsigned size) {
  CG50RTC * r = opaque;
  if (offset + size > sizeof(r->regs))
    return;
  cg50_rtc_update(r);
  if (size > 1) {
    for (unsigned i = 0; i < size; i++)
      rtc_write(r, offset + i, value >> (8 * (size - 1 - i)), 1);
    return;
  }
  value &= 255;
  if (offset == RCR1) {
    r->regs[RCR1] = (r->regs[RCR1] & value & 0x81) | (value & 0x18);
  } else if (offset == RCR2) {
    int64_t ticks = elapsed(r);
    r->elapsed = (value & 2) ? ticks / SECOND * SECOND : ticks;
    r->anchor = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    if (value & 4) {
      struct tm tm;
      calendar(r, &tm);
      tm.tm_sec = tm.tm_sec < 30 ? 0 : 60;
      r->epoch = mktimegm(&tm) - r->elapsed / SECOND;
    }
    r->regs[RCR2] = (r->regs[RCR2] & value & 0x80) | (value & 0x71);
    unsigned selection = (value >> 4) & 7;
    r->seen_period = selection ? r->elapsed / periods[selection] : 0;
  } else if (offset >= 2 && offset <= 15 && (!(offset & 1) || offset == 15)) {
    struct tm tm;
    calendar(r, &tm);
    unsigned v = unbcd(value), year = tm.tm_year + 1900;
    switch (offset) {
    case 2:
      if (v < 60)
        tm.tm_sec = v;
      break;
    case 4:
      if (v < 60)
        tm.tm_min = v;
      break;
    case 6:
      if (v < 24)
        tm.tm_hour = v;
      break;
    case 8:
      break; /* Weekday follows the calendar date. */
    case 10:
      if (v >= 1 && v <= 31)
        tm.tm_mday = v;
      break;
    case 12:
      if (v >= 1 && v <= 12)
        tm.tm_mon = v - 1;
      break;
    case 14:
      tm.tm_year = v * 100 + year % 100 - 1900;
      break;
    case 15:
      tm.tm_year = year / 100 * 100 + v - 1900;
      break;
    }
    r->epoch = mktimegm(&tm) - elapsed(r) / SECOND;
  } else if (offset != 0 && offset < sizeof(r->regs)) {
    r->regs[offset] = value;
  }
  r->irq_changed(r->opaque);
}
static const MemoryRegionOps rtc_ops = {
    .read = rtc_read,
    .write = rtc_write,
    .endianness = DEVICE_BIG_ENDIAN,
    .valid = {.min_access_size = 1, .max_access_size = 4},
    .impl = {.min_access_size = 1, .max_access_size = 4},
};
void cg50_rtc_init(CG50RTC * r, void (*irq_changed)(void *), void * opaque) {
  struct tm tm;
  qemu_get_timedate(&tm, 0);
  r->epoch = mktimegm(&tm);
  r->anchor = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
  r->regs[RCR2] = 1;
  r->irq_changed = irq_changed;
  r->opaque = opaque;
  memory_region_init_io(&r->mr, NULL, &rtc_ops, r, "cg50.rtc", 32);
  memory_region_add_subregion_overlap(get_system_memory(), 0x0413fec0, &r->mr, 1);
}
