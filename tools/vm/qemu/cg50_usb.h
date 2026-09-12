/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef CG50_USB_H
#define CG50_USB_H
#include "system/memory.h"
#include "chardev/char-fe.h"

typedef struct CG50USBPipe {
  uint16_t cfg, buf, maxp, peri, ctr;
  uint8_t data[4096];
  unsigned length, cursor;
  bool ready;
} CG50USBPipe;
typedef struct CG50USB {
  MemoryRegion mr;
  CharBackend host;
  uint16_t regs[128];
  CG50USBPipe pipe[10];
  bool attached;
  uint8_t setup[8];
  unsigned transferred;
  char command[8300];
  unsigned command_length;
  bool discard_line;
  unsigned trace_left;
  void (*irq_changed)(void *);
  void * opaque;
} CG50USB;
void cg50_usb_init(CG50USB * u, void (*irq_changed)(void *), void * opaque);
bool cg50_usb_irq(CG50USB * u);
#endif
