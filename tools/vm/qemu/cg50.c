/*
 * Experimental fx-CG50 board for QEMU 10.1 (SH-4A, big endian).
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Peripheral addresses and BCD behavior cross-checked against the MIT licensed
 * hexbinoct/casio-cg50 model; see LICENSE.reference and ../README.md. Timing is approximate.
 */
#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qemu/log.h"
#include "qemu/timer.h"
#include "cpu.h"
#include "exec/cputlb.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "hw/irq.h"
#include "hw/sh4/sh_intc.h"
#include "system/address-spaces.h"
#include "system/reset.h"
#include "ui/console.h"
#include "cg50_usb.h"
#include "cg50_rtc.h"

#define FLASH_SIZE (32 * MiB)
#define LCD_WIDTH 384
#define LCD_HEIGHT 216

typedef struct CG50State CG50State;
typedef struct CG50Window {
  MemoryRegion mr;
  CG50State * s;
  uint32_t base;
  uint32_t size;
  uint8_t * regs;
} CG50Window;
struct CG50State {
  SuperHCPU * cpu;
  MemoryRegion ram, flash, erased, ilram, ocram, xram, yram, smallram;
  CG50Window windows[8];
  QemuConsole * con;
  QEMUTimer *timer, *flash_timer;
  uint32_t erase_base, erase_size, flash_toggle;
  struct intc_desc intc;
  struct intc_source irq[7];
  CG50USB usb;
  CG50RTC rtc;
  uint8_t rtc_mask;
  bool usb_masked, cable_masked, cable_pending, cable_attached;
  uint64_t ticks;
  uint16_t adc_flags[2];
  uint16_t keys[6], key_flags, key_enable;
  bool key_masked;
  uint32_t bcd_a, bcd_b, bcd_result, bcd_flag;
  uint32_t flash_state, flash_mode, buffer_left, buffer_count;
  struct {
    uint32_t addr, size, val;
  } buffer[256];
  uint32_t frame_addr;
  unsigned flash_trace_left;
};

static void update_key_irq(CG50State * s) {
  unsigned priority = lduw_be_p(s->windows[0].regs + 0x080014) >> 12;
  bool asserted = priority && !s->key_masked && (s->key_flags & s->key_enable & 0xff);
  if (asserted != !!s->irq[1].asserted) {
    sh_intc_toggle_source(&s->irq[1], 0, asserted ? 1 : -1);
  }
}

static void update_rtc_irq(void * opaque) {
  CG50State * s = opaque;
  unsigned priority = lduw_be_p(s->windows[0].regs + 0x080028) >> 12;
  unsigned flags = cg50_rtc_irqs(&s->rtc) & ~s->rtc_mask;
  static const unsigned masks[] = {2, 1, 4};
  for (unsigned i = 0; i < 3; i++) {
    bool asserted = priority && (flags & masks[i]);
    if (asserted != !!s->irq[4 + i].asserted) {
      sh_intc_toggle_source(&s->irq[4 + i], 0, asserted ? 1 : -1);
    }
  }
}

static void update_cable_irq(CG50State * s) {
  unsigned priority = (ldl_be_p(s->windows[0].regs + 0x140010) >> 24) & 15;
  unsigned sense = (lduw_be_p(s->windows[0].regs + 0x14001c) >> 12) & 3;
  if (sense >= 2) {
    s->cable_pending = s->usb.attached == (sense == 3);
  }
  bool asserted = priority && !s->cable_masked && s->cable_pending;
  if (asserted != !!s->irq[3].asserted) {
    sh_intc_toggle_source(&s->irq[3], 0, asserted ? 1 : -1);
  }
}

static void update_usb_irq(void * opaque) {
  CG50State * s = opaque;
  if (s->cable_attached != s->usb.attached) {
    unsigned sense = (lduw_be_p(s->windows[0].regs + 0x14001c) >> 12) & 3;
    s->cable_attached = s->usb.attached;
    if (sense < 2 && s->usb.attached == (sense == 1)) {
      s->cable_pending = true;
    }
    update_cable_irq(s);
  }
  unsigned priority = lduw_be_p(s->windows[0].regs + 0x080014) & 0xf0;
  bool asserted = priority && !s->usb_masked && cg50_usb_irq(&s->usb);
  if (asserted != !!s->irq[2].asserted) {
    sh_intc_toggle_source(&s->irq[2], 0, asserted ? 1 : -1);
  }
}

static void flash_program(CG50State * s, uint32_t addr, unsigned size, uint32_t value) {
  uint8_t * p = memory_region_get_ram_ptr(&s->flash);
  if (addr > FLASH_SIZE - size) {
    return;
  }
  for (unsigned i = 0; i < size; i++) {
    p[addr + i] &= value >> (8 * (size - i - 1));
  }
  memory_region_flush_rom_device(&s->flash, addr, size);
}

static void flash_complete(void * opaque) {
  CG50State * s = opaque;
  uint8_t * p = memory_region_get_ram_ptr(&s->flash);
  memset(p + s->erase_base, 0xff, s->erase_size);
  s->flash_mode = 0;
  memory_region_rom_device_set_romd(&s->flash, true);
  memory_region_flush_rom_device(&s->flash, s->erase_base, s->erase_size);
}

/* CFI AMD command-set model: 32 MiB, 256 uniform 128 KiB sectors. */
static uint64_t flash_read(void * opaque, hwaddr addr, unsigned size) {
  CG50State * s = opaque;
  static const uint8_t cfi[0x50] = {
      [0x10] = 'Q',  [0x11] = 'R', [0x12] = 'Y', [0x13] = 2,   [0x15] = 0x40, [0x1b] = 0x27,
      [0x1c] = 0x36, [0x1f] = 7,   [0x20] = 7,   [0x21] = 9,   [0x22] = 12,   [0x23] = 1,
      [0x24] = 1,    [0x25] = 10,  [0x26] = 13,  [0x27] = 25,  [0x28] = 2,    [0x2a] = 6,
      [0x2c] = 1,    [0x2d] = 255, [0x30] = 2,   [0x40] = 'P', [0x41] = 'R',  [0x42] = 'I',
      [0x43] = '1',  [0x44] = '3', [0x46] = 2,
  };
  unsigned word = (addr & 0xffff) >> 1;
  uint32_t value = 0;
  if (s->flash_mode == 3) {
    s->flash_toggle ^= 0x40;
    return 8 | s->flash_toggle; /* erase active: DQ7=0, DQ3=1, DQ6 toggles */
  }
  if (s->flash_mode == 1) {
    switch (word) {
    case 0:
      value = 1;
      break;
    case 1:
      value = 0x227e;
      break;
    case 0xe:
      value = 0x2222;
      break;
    case 0xf:
      value = 0x2201;
      break;
    }
  } else if (s->flash_mode == 2) {
    value = word < sizeof(cfi) ? cfi[word] : 0;
  } else {
    uint8_t * p = memory_region_get_ram_ptr(&s->flash);
    for (unsigned i = 0; i < size; i++) {
      value = (value << 8) | p[addr + i];
    }
  }
  return value;
}

static void flash_write(void * opaque, hwaddr addr, uint64_t val, unsigned size) {
  CG50State * s = opaque;
  unsigned a = addr & 0xfff, c = val & 0xff;
  if (s->flash_trace_left) {
    s->flash_trace_left--;
    fprintf(stderr, "flash write %08" HWADDR_PRIx "/%u %08" PRIx64 " state=%u pc=%08x\n", addr,
            size, val, s->flash_state, s->cpu->env.pc);
  }
  if (s->flash_state != 3 && s->flash_state != 8 && c == 0xf0) {
    s->flash_mode = s->flash_state = 0;
    timer_del(s->flash_timer);
    memory_region_rom_device_set_romd(&s->flash, true);
    return;
  }
  if (s->flash_state == 0 && a == 0xaa && c == 0x98) {
    s->flash_mode = 2;
    memory_region_rom_device_set_romd(&s->flash, false);
    return;
  }
  switch (s->flash_state) {
  case 0:
    s->flash_state = a == 0xaaa && c == 0xaa ? 1 : 0;
    break;
  case 1:
    s->flash_state = a == 0x554 && c == 0x55 ? 2 : 0;
    break;
  case 2:
    if (a == 0xaaa && c == 0x90) {
      s->flash_mode = 1;
      s->flash_state = 0;
      memory_region_rom_device_set_romd(&s->flash, false);
    } else if (a == 0xaaa && c == 0xa0) {
      s->flash_state = 3;
    } else if (a == 0xaaa && c == 0x80) {
      s->flash_state = 4;
    } else if (c == 0x25) {
      s->flash_state = 7;
    } else {
      s->flash_state = 0;
    }
    break;
  case 3:
    flash_program(s, addr, size, val);
    s->flash_state = 0;
    break;
  case 4:
    s->flash_state = a == 0xaaa && c == 0xaa ? 5 : 0;
    break;
  case 5:
    s->flash_state = a == 0x554 && c == 0x55 ? 6 : 0;
    break;
  case 6:
    if (c == 0x30) {
      s->erase_base = addr & ~0x1ffff;
      s->erase_size = 0x20000;
    } else if (c == 0x10) {
      s->erase_base = 0;
      s->erase_size = FLASH_SIZE;
    }
    if (c == 0x30 || c == 0x10) {
      s->flash_mode = 3;
      memory_region_rom_device_set_romd(&s->flash, false);
      timer_mod(s->flash_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + 2000000);
    }
    s->flash_state = 0;
    break;
  case 7:
    s->buffer_left = (val & 0xffff) + 1;
    s->buffer_count = 0;
    s->flash_state = s->buffer_left <= ARRAY_SIZE(s->buffer) ? 8 : 0;
    break;
  case 8:
    if (s->buffer_left) {
      unsigned i = s->buffer_count++;
      s->buffer[i].addr = addr;
      s->buffer[i].size = size;
      s->buffer[i].val = val;
      s->buffer_left--;
    } else {
      if (c == 0x29) {
        for (unsigned i = 0; i < s->buffer_count; i++) {
          flash_program(s, s->buffer[i].addr, s->buffer[i].size, s->buffer[i].val);
        }
      }
      s->flash_state = 0;
    }
    break;
  }
}
static const MemoryRegionOps flash_ops = {
    .read = flash_read,
    .write = flash_write,
    .endianness = DEVICE_BIG_ENDIAN,
    .valid = {.min_access_size = 1, .max_access_size = 4},
};

static void bcd_command(CG50State * s, uint32_t cmd) {
  int flag = (cmd & 4) ? 1 : (cmd & 2) ? s->bcd_flag : 0;
  uint32_t result = 0;
  for (unsigned i = 0; i < 8; i++) {
    int a = (s->bcd_a >> (4 * i)) & 15;
    int b = (s->bcd_b >> (4 * i)) & 15;
    int digit;
    if (cmd & 1) {
      digit = a + b + flag;
      flag = digit > 9;
      if (flag) {
        digit -= 10;
      }
    } else {
      digit = a - b - flag;
      flag = digit < 0;
      if (flag) {
        digit += 10;
      }
    }
    result |= (digit & 15) << (4 * i);
  }
  s->bcd_flag = flag;
  s->bcd_result = result;
}

static uint64_t mmio_read(void * opaque, hwaddr offset, unsigned size) {
  CG50Window * w = opaque;
  CG50State * s = w->s;
  CPUSH4State * e = &s->cpu->env;
  uint32_t a = w->base + offset;
  uint32_t value = 0;
  switch (a) {
  case 0xff000000:
    return e->pteh;
  case 0xff000004:
    return e->ptel;
  case 0xff000008:
    return e->ttb;
  case 0xff00000c:
    return e->tea;
  case 0xff000010:
    return e->mmucr;
  case 0xff000020:
    return e->tra;
  case 0xff000024:
    return e->expevt ? e->expevt : 0x0a02; /* fx-CG50 model strap */
  case 0xff000028:
    return e->intevt;
  case 0xff000030:
    return 0x10300b00; /* SH7305 processor family */
  case 0xff000044:
    return 0x00002c00; /* SH7305 product, also distinguishes native add-in loaders. */
  case 0xff000034:
    return e->ptea;
  case 0x04150020:
    return ldl_be_p(w->regs + offset) & ~0x80;
  case 0x04150000:
    return ldl_be_p(w->regs + offset) & 0x7fffffff;
  case 0x04150060:
    return 0; /* CPG transition complete */
  case 0x044a0060:
    return 0x8000; /* ETMU underflow */
  case 0x044d00d8:
    return -(qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) / 32) & 0xffffff;
  case 0x04610082:
  case 0x04610084:
    return 0x7140; /* normal battery ADC */
  case 0x04610088:
  case 0x04610089:
  case 0x0461008a:
  case 0x0461008b:
    return size == 1 ? (s->adc_flags[(a - 0x04610088) / 2] >> (8 * !(a & 1))) & 0xff
                     : s->adc_flags[(a - 0x04610088) / 2];
  case 0x04140024:
    return s->cable_pending ? 0x40 : 0;
  case 0x04050162:
    return (w->regs[offset] & ~2) | (s->usb.attached ? 2 : 0);
  }
  if (a >= 0x044b0000 && a < 0x044b000c) {
    unsigned word = (a - 0x044b0000) / 2;
    unsigned columns = lduw_be_p(w->regs + 0x4b001a);
    unsigned rows = lduw_be_p(w->regs + 0x4b001c) & 0xff;
    unsigned mask = ((columns & (1 << (2 * word))) ? rows : 0) |
                    ((columns & (2 << (2 * word))) ? rows << 8 : 0);
    uint16_t value = s->keys[word] & mask;
    return size == 1 ? (value >> (8 * !(a & 1))) & 0xff : value;
  }
  if (a == 0x044b0014) {
    return (s->key_enable << 8) | s->key_flags;
  }
  if ((a & 0xfffff000) == 0x04cb0000) {
    switch (a & 15) {
    case 0:
      return s->bcd_flag ? 0x10040000 : 0x00010000;
    case 4:
      return s->bcd_a;
    case 8:
      return s->bcd_b;
    case 12:
      return s->bcd_result;
    }
  }
  if (a >= 0xf2000000 && a < 0xf8000000) {
    switch (a & 0xff000000) {
    case 0xf2000000:
      return cpu_sh4_read_mmaped_itlb_addr(e, a);
    case 0xf3000000:
      return cpu_sh4_read_mmaped_itlb_data(e, a);
    case 0xf6000000:
      return cpu_sh4_read_mmaped_utlb_addr(e, a);
    case 0xf7000000:
      return cpu_sh4_read_mmaped_utlb_data(e, a);
    }
  }
  if (w->regs) {
    for (unsigned i = 0; i < size; i++) {
      value = (value << 8) | w->regs[offset + i];
    }
  }
  if ((a & 0xfffff00f) == 0xfe00800c) {
    value |= 2;
  }
  qemu_log_mask(LOG_UNIMP, "cg50 read %08x/%u = %08x pc=%08x\n", a, size, value, e->pc);
  return value;
}

static void mmio_write(void * opaque, hwaddr offset, uint64_t val, unsigned size) {
  CG50Window * w = opaque;
  CG50State * s = w->s;
  CPUSH4State * e = &s->cpu->env;
  uint32_t a = w->base + offset;
  if (w->regs) {
    for (unsigned i = 0; i < size; i++) {
      w->regs[offset + i] = val >> (8 * (size - i - 1));
    }
  }
  switch (a) {
  case 0x04080028:
    update_rtc_irq(s);
    return;
  case 0x040800a8:
    s->rtc_mask |= val & 7;
    update_rtc_irq(s);
    return;
  case 0x040800e8:
    s->rtc_mask &= ~(val & 7);
    update_rtc_irq(s);
    return;
  case 0x04140010:
  case 0x0414001c:
    update_cable_irq(s);
    return;
  case 0x04140024:
    if (!(val & 0x40)) {
      s->cable_pending = false;
      update_cable_irq(s);
    }
    return;
  case 0x04140044:
    if (val & 0x40) {
      s->cable_masked = true;
      update_cable_irq(s);
    }
    return;
  case 0x04140064:
    if (val & 0x40) {
      s->cable_masked = false;
      update_cable_irq(s);
    }
    return;
  case 0x04080014:
    update_usb_irq(s);
    update_key_irq(s);
    return;
  case 0x040800a4:
    if (val & 2) {
      s->usb_masked = true;
      update_usb_irq(s);
    }
    return;
  case 0x040800e4:
    if (val & 2) {
      s->usb_masked = false;
      update_usb_irq(s);
    }
    return;
  case 0x04080094:
    if (val & 0x80) {
      s->key_masked = true;
      update_key_irq(s);
    }
    return;
  case 0x040800d4:
    if (val & 0x80) {
      s->key_masked = false;
      update_key_irq(s);
    }
    return;
  case 0x044b0014:
    s->key_flags &= ~(val & 0xff);
    s->key_enable = (val >> 8) & 0xff;
    update_key_irq(s);
    return;
  case 0xff000000:
    e->pteh = val;
    tlb_flush(CPU(s->cpu));
    return;
  case 0xff000004:
    e->ptel = val;
    return;
  case 0xff000008:
    e->ttb = val;
    return;
  case 0xff00000c:
    e->tea = val;
    return;
  case 0xff000010:
    if (val & 4) {
      cpu_sh4_invalidate_tlb(e);
    }
    e->mmucr = val & ~4;
    tlb_flush(CPU(s->cpu));
    return;
  case 0xff000020:
    e->tra = val & 0x7ff;
    return;
  case 0xff000024:
    e->expevt = val & 0x7ff;
    return;
  case 0xff000028:
    e->intevt = val & 0x7ff;
    return;
  case 0xff000034:
    e->ptea = val & 15;
    return;
  case 0x04610088:
  case 0x04610089:
  case 0x0461008a:
  case 0x0461008b: {
    unsigned index = (a - 0x04610088) / 2;
    if (size == 1) {
      unsigned shift = 8 * !(a & 1);
      s->adc_flags[index] = (s->adc_flags[index] & ~(0xff << shift)) | ((val & 0xff) << shift);
    } else {
      s->adc_flags[index] = val;
    }
    if (!((s->adc_flags[0] | s->adc_flags[1]) & 0x8000) && s->irq[0].asserted) {
      sh_intc_toggle_source(&s->irq[0], 0, -1);
    }
    return;
  }
  }
  if ((a & 0xfffff000) == 0x04cb0000) {
    switch (a & 15) {
    case 0:
      bcd_command(s, val);
      break;
    case 4:
      s->bcd_a = val;
      break;
    case 8:
      s->bcd_b = val;
      break;
    }
    return;
  }
  if ((a & 0xfffff00f) == 0xfe00800c && (val & 1)) {
    uint32_t off = offset & ~15;
    uint32_t src = ldl_be_p(w->regs + off);
    uint32_t dest = ldl_be_p(w->regs + off + 4);
    if ((dest & 0x1fffffff) == 0x14000000) {
      s->frame_addr = src & 0x1fffffff;
    }
  }
  if (a >= 0xf2000000 && a < 0xf8000000) {
    switch (a & 0xff000000) {
    case 0xf2000000:
      cpu_sh4_write_mmaped_itlb_addr(e, a, val);
      return;
    case 0xf3000000:
      cpu_sh4_write_mmaped_itlb_data(e, a, val);
      return;
    case 0xf6000000:
      cpu_sh4_write_mmaped_utlb_addr(e, a, val);
      return;
    case 0xf7000000:
      cpu_sh4_write_mmaped_utlb_data(e, a, val);
      return;
    }
  }
  qemu_log_mask(LOG_UNIMP, "cg50 write %08x/%u = %08" PRIx64 " pc=%08x\n", a, size, val, e->pc);
}
static const MemoryRegionOps mmio_ops = {
    .read = mmio_read,
    .write = mmio_write,
    .endianness = DEVICE_BIG_ENDIAN,
    .valid = {.min_access_size = 1, .max_access_size = 4, .unaligned = true},
};

static void timer_tick(void * opaque) {
  CG50State * s = opaque;
  cg50_rtc_update(&s->rtc);
  s->ticks++;
  /* Completion is latched even when its interrupt is disabled: the OS also polls scans. */
  if (!(s->ticks & 7) && (lduw_be_p(s->windows[0].regs + 0x4b000c) & 0x8000)) {
    s->key_flags |= 2;
    update_key_irq(s);
  }
  s->adc_flags[0] |= 0xc000;
  s->adc_flags[1] |= 0xc000;
  if (!s->irq[0].asserted) {
    sh_intc_toggle_source(&s->irq[0], 0, 1);
  }
  timer_mod(s->timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + 1000000);
}

/* Matrix coordinates use the hardware KEYSC layout, not OS keycode injection. */
static const struct {
  QKeyCode code;
  unsigned row, col;
} keymap[] = {
    {Q_KEY_CODE_F1, 6, 9},
    {Q_KEY_CODE_F2, 5, 9},
    {Q_KEY_CODE_F3, 4, 9},
    {Q_KEY_CODE_F4, 3, 9},
    {Q_KEY_CODE_F5, 2, 9},
    {Q_KEY_CODE_F6, 1, 9},
    {Q_KEY_CODE_RET, 2, 1},
    {Q_KEY_CODE_ESC, 3, 7},
    {Q_KEY_CODE_BACKSPACE, 3, 4},
    {Q_KEY_CODE_DELETE, 3, 4},
    {Q_KEY_CODE_HOME, 3, 8},
    {Q_KEY_CODE_END, 0, 0},
    {Q_KEY_CODE_UP, 1, 8},
    {Q_KEY_CODE_DOWN, 2, 7},
    {Q_KEY_CODE_LEFT, 2, 8},
    {Q_KEY_CODE_RIGHT, 1, 7},
    {Q_KEY_CODE_SHIFT, 6, 8},
    {Q_KEY_CODE_CTRL, 6, 7},
    {Q_KEY_CODE_0, 6, 1},
    {Q_KEY_CODE_1, 6, 2},
    {Q_KEY_CODE_2, 5, 2},
    {Q_KEY_CODE_3, 4, 2},
    {Q_KEY_CODE_4, 6, 3},
    {Q_KEY_CODE_5, 5, 3},
    {Q_KEY_CODE_6, 4, 3},
    {Q_KEY_CODE_7, 6, 4},
    {Q_KEY_CODE_8, 5, 4},
    {Q_KEY_CODE_9, 4, 4},
    {Q_KEY_CODE_KP_ADD, 3, 2},
    {Q_KEY_CODE_MINUS, 2, 2},
    {Q_KEY_CODE_KP_MULTIPLY, 3, 3},
    {Q_KEY_CODE_SLASH, 2, 3},
    {Q_KEY_CODE_DOT, 5, 1},
    {Q_KEY_CODE_X, 6, 6},
    {Q_KEY_CODE_L, 5, 6},
    {Q_KEY_CODE_N, 4, 6},
    {Q_KEY_CODE_S, 3, 6},
    {Q_KEY_CODE_C, 2, 6},
    {Q_KEY_CODE_T, 1, 6},
    {Q_KEY_CODE_P, 4, 7},
    {Q_KEY_CODE_Q, 5, 7},
    {Q_KEY_CODE_F, 6, 5},
    {Q_KEY_CODE_D, 5, 5},
    {Q_KEY_CODE_BRACKET_LEFT, 4, 5},
    {Q_KEY_CODE_BRACKET_RIGHT, 3, 5},
    {Q_KEY_CODE_COMMA, 2, 5},
    {Q_KEY_CODE_EQUAL, 1, 5},
    {Q_KEY_CODE_O, 5, 8},
    {Q_KEY_CODE_V, 4, 8},
    {Q_KEY_CODE_KP_SUBTRACT, 3, 1},
    {Q_KEY_CODE_E, 4, 1},
};
static CG50State * keyboard_board;
static void key_event(DeviceState * dev, QemuConsole * src, InputEvent * evt) {
  CG50State * s = keyboard_board;
  InputKeyEvent * key = evt->u.key.data;
  QKeyCode code = qemu_input_key_value_to_qcode(key->key);
  for (unsigned i = 0; i < ARRAY_SIZE(keymap); i++) {
    if (keymap[i].code == code) {
      unsigned word = keymap[i].col / 2;
      unsigned bit = keymap[i].row + 8 * (keymap[i].col & 1);
      if (key->down) {
        s->keys[word] |= 1 << bit;
        s->key_flags |= 8;
      } else {
        s->keys[word] &= ~(1 << bit);
        s->key_flags |= 2;
      }
      update_key_irq(s);
      return;
    }
  }
}
static const QemuInputHandler keyboard_handler = {
    .name = "CG50 key matrix",
    .mask = INPUT_EVENT_MASK_KEY,
    .event = key_event,
};

static void update_display(void * opaque) {
  CG50State * s = opaque;
  DisplaySurface * surface = qemu_console_surface(s->con);
  uint32_t addr = s->frame_addr;
  uint8_t * ram = memory_region_get_ram_ptr(&s->ram);
  if (addr < 0x0c000000 || addr + LCD_WIDTH * LCD_HEIGHT * 2 > 0x0c800000) {
    return;
  }
  ram += addr - 0x0c000000;
  for (unsigned y = 0; y < LCD_HEIGHT; y++) {
    uint32_t * row = (uint32_t *)(surface_data(surface) + y * surface_stride(surface));
    for (unsigned x = 0; x < LCD_WIDTH; x++) {
      uint16_t c = lduw_be_p(ram + (y * LCD_WIDTH + x) * 2);
      unsigned r = c >> 11, g = (c >> 5) & 63, b = c & 31;
      row[x] = (((r << 3) | (r >> 2)) << 16) | (((g << 2) | (g >> 4)) << 8) | (b << 3) | (b >> 2);
    }
  }
  dpy_gfx_update(s->con, 0, 0, LCD_WIDTH, LCD_HEIGHT);
}
static const GraphicHwOps graphic_ops = {.gfx_update = update_display};

static void map_ram(MemoryRegion * mr, const char * name, hwaddr base, uint64_t size) {
  memory_region_init_ram(mr, NULL, name, size, &error_fatal);
  memory_region_add_subregion(get_system_memory(), base, mr);
}
static void map_io(CG50State * s, unsigned i, const char * name, uint32_t base, uint32_t size,
                   bool backing) {
  CG50Window * w = &s->windows[i];
  w->s = s;
  w->base = base;
  w->size = size;
  w->regs = backing ? g_malloc0(size) : NULL;
  memory_region_init_io(&w->mr, NULL, &mmio_ops, w, name, size);
  memory_region_add_subregion(get_system_memory(), base, &w->mr);
}
static void cg50_cpu_reset(void * opaque) {
  CG50State * s = opaque;
  cpu_reset(CPU(s->cpu));
  s->cpu->env.intc_handle = &s->intc;
}

static void cg50_init(MachineState * machine) {
  CG50State * s = g_new0(CG50State, 1);
  int64_t rom_size;
  s->cpu = SUPERH_CPU(cpu_create(machine->cpu_type));
  s->flash_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, flash_complete, s);
  s->flash_trace_left = getenv("CG50_TRACE_FLASH") ? 512 : 0;
  s->intc.sources = s->irq;
  s->intc.nr_sources = ARRAY_SIZE(s->irq);
  for (unsigned i = 0; i < ARRAY_SIZE(s->irq); i++) {
    s->irq[i].parent = &s->intc;
    s->irq[i].enable_count = s->irq[i].enable_max = 1;
  }
  s->irq[0].vect = 0x560;
  s->irq[1].vect = 0xbe0;
  s->irq[2].vect = 0xa20;
  s->irq[3].vect = 0x620;
  s->irq[4].vect = 0xaa0;
  s->irq[5].vect = 0xac0;
  s->irq[6].vect = 0xa80;
  s->cpu->env.intc_handle = &s->intc;
  map_ram(&s->ram, "cg50.dram", 0x0c000000, 8 * MiB);
  map_ram(&s->ilram, "cg50.ilram", 0xfd800000, 64 * KiB);
  map_ram(&s->ocram, "cg50.onchip", 0xfe200000, 2 * MiB);
  map_ram(&s->xram, "cg50.xram", 0xe5007000, 8 * KiB);
  map_ram(&s->yram, "cg50.yram", 0xe5017000, 8 * KiB);
  map_ram(&s->smallram, "cg50.smallram", 0xe5200000, 4 * KiB);
  memory_region_init_rom_device(&s->flash, NULL, &flash_ops, s, "cg50.flash", FLASH_SIZE,
                                &error_fatal);
  memory_region_add_subregion(get_system_memory(), 0, &s->flash);
  memset(memory_region_get_ram_ptr(&s->flash), 0xff, FLASH_SIZE);
  if (!machine->firmware) {
    error_report("cg50 requires -bios <local firmware dump>");
    exit(1);
  }
  rom_size = load_image_size(machine->firmware, memory_region_get_ram_ptr(&s->flash), FLASH_SIZE);
  if (rom_size <= 0) {
    error_report("cannot load firmware %s", machine->firmware);
    exit(1);
  }
  if (rom_size < FLASH_SIZE) {
    warn_report("flash image covers %" PRId64 " of %lu bytes; remaining bytes are erased FF",
                rom_size, FLASH_SIZE);
  }
  map_ram(&s->erased, "cg50.flash-unpopulated", FLASH_SIZE, FLASH_SIZE);
  memset(memory_region_get_ram_ptr(&s->erased), 0xff, FLASH_SIZE);
  memory_region_set_readonly(&s->erased, true);
  map_io(s, 0, "cg50.peripherals", 0x04000000, 16 * MiB, true);
  map_io(s, 1, "cg50.lcd", 0x14000000, 512 * KiB, true);
  map_io(s, 2, "cg50.ccn", 0xff000000, 4096, true);
  map_io(s, 3, "cg50.dmac", 0xfe008000, 4096, true);
  map_io(s, 4, "cg50.bsc", 0xfec10000, 4096, true);
  map_io(s, 5, "cg50.cache-tlb", 0xf0000000, 128 * MiB, false);
  cg50_usb_init(&s->usb, update_usb_irq, s);
  cg50_rtc_init(&s->rtc, update_rtc_irq, s);
  s->frame_addr = 0x0c000000;
  keyboard_board = s;
  qemu_input_handler_register(NULL, &keyboard_handler);
  s->con = graphic_console_init(NULL, 0, &graphic_ops, s);
  qemu_console_resize(s->con, LCD_WIDTH, LCD_HEIGHT);
  s->timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, timer_tick, s);
  timer_mod(s->timer, 1000000);
  qemu_register_reset(cg50_cpu_reset, s);
}
static void cg50_machine_init(MachineClass * mc) {
  mc->desc = "Casio fx-CG50 (experimental SH7305 board)";
  mc->init = cg50_init;
  mc->default_cpu_type = TYPE_SH7785_CPU;
  mc->default_ram_size = 8 * MiB;
  mc->default_ram_id = NULL;
}
DEFINE_MACHINE("cg50", cg50_machine_init)
