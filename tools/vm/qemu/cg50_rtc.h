/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef CG50_RTC_H
#define CG50_RTC_H
#include "system/memory.h"

typedef struct CG50RTC {
  MemoryRegion mr;
  uint8_t regs[32];
  int64_t epoch, anchor, elapsed;
  int64_t seen_second, seen_period;
  void (*irq_changed)(void *);
  void * opaque;
} CG50RTC;
void cg50_rtc_init(CG50RTC * r, void (*irq_changed)(void *), void * opaque);
void cg50_rtc_update(CG50RTC * r);
unsigned cg50_rtc_irqs(CG50RTC * r);
#endif
