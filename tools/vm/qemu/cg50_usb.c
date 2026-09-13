/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SH7305 USBHS device-side register model with a local transaction-level host.
 * Register layout: gint/include/gint/mpu/usb.h and Renesas USBHS documentation.
 * This transport exposes USB tokens, not filesystem or guest syscall hooks.
 */
#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/timer.h"
#include "qapi/error.h"
#include "system/address-spaces.h"
#include "cg50_usb.h"

#define REG(u, n) ((u)->regs[(n) / 2])
#define INTSTS 0x40
static const unsigned fifo_sel[3] = {0x20, 0x28, 0x2c};

static void usb_update(CG50USB * u) {
  uint16_t status = REG(u, INTSTS) & ~0x0700;
  if (REG(u, 0x46) & REG(u, 0x36)) {
    status |= 0x0100;
  }
  if (REG(u, 0x48) & REG(u, 0x38)) {
    status |= 0x0200;
  }
  if (REG(u, 0x4a) & REG(u, 0x3a)) {
    status |= 0x0400;
  }
  REG(u, INTSTS) = status;
  u->irq_changed(u->opaque);
}
bool cg50_usb_irq(CG50USB * u) {
  return (REG(u, 0x40) & REG(u, 0x30) & 0xff00) || (REG(u, 0x42) & REG(u, 0x32) & 0xfff0);
}
static unsigned selected(CG50USB * u, unsigned fifo) {
  unsigned n = REG(u, fifo_sel[fifo]) & 15;
  return n < 10 ? n : 0;
}
static unsigned packet_size(CG50USB * u, unsigned n) {
  unsigned maxp = n ? u->pipe[n].maxp & 0x7ff : REG(u, 0x5e) & 0x7f;
  return maxp ? MIN(maxp, 4096) : 64;
}
static int fifo_index(unsigned a) {
  if (a >= 0x14 && a < 0x20) {
    return (a - 0x14) / 4;
  }
  return -1;
}
static int ctr_index(unsigned a) {
  for (unsigned i = 0; i < 3; i++) {
    if (a == fifo_sel[i] + 2) {
      return i;
    }
  }
  return -1;
}
static void control_stage(CG50USB * u, unsigned stage) {
  REG(u, INTSTS) = (REG(u, INTSTS) & ~7) | 0x0800 | stage;
}
static uint64_t usb_read(void * opaque, hwaddr a, unsigned size) {
  CG50USB * u = opaque;
  int fifo = fifo_index(a), ctr = ctr_index(a);
  uint32_t value = 0;
  if (fifo >= 0) {
    CG50USBPipe * p = &u->pipe[selected(u, fifo)];
    bool big = REG(u, fifo_sel[fifo]) & 0x0100;
    for (unsigned i = 0; i < size; i++) {
      uint8_t byte = p->cursor < p->length ? p->data[p->cursor++] : 0;
      value |= (uint32_t)byte << (8 * (big ? size - i - 1 : i));
    }
    return value;
  }
  if (size == 1) {
    return (usb_read(u, a & ~1, 2) >> ((a & 1) ? 0 : 8)) & 255;
  }
  if (size == 4) {
    return (usb_read(u, a, 2) << 16) | usb_read(u, a + 2, 2);
  }
  if (ctr >= 0) {
    CG50USBPipe * p = &u->pipe[selected(u, ctr)];
    return 0x2000 | (p->ready ? 0x8000 : 0) | (p->length - p->cursor);
  }
  if (a == 4) {
    return u->attached ? 1 : 0;
  }
  if (a == 0x4c) {
    return (qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) / 1000000) & 0x7ff;
  }
  if (a >= 0x68 && a <= 0x6e) {
    unsigned n = REG(u, 0x64) & 15;
    if (!n || n >= 10) {
      return 0;
    }
    CG50USBPipe * p = &u->pipe[n];
    return a == 0x68 ? p->cfg : a == 0x6a ? p->buf : a == 0x6c ? p->maxp : p->peri;
  }
  if (a >= 0x70 && a < 0x82) {
    return u->pipe[1 + (a - 0x70) / 2].ctr | 0x8000;
  }
  if (a == 0x60) {
    return u->pipe[0].ctr | 0x8000;
  }
  if (a + size > sizeof(u->regs)) {
    return 0;
  }
  value = REG(u, a);
  return value;
}
static void usb_write(void * opaque, hwaddr a, uint64_t value, unsigned size) {
  CG50USB * u = opaque;
  int fifo = fifo_index(a), ctr = ctr_index(a);
  if (u->trace_left && fifo < 0) {
    u->trace_left--;
    qemu_log("CG50 USB W %02" HWADDR_PRIx " size=%u value=%04" PRIx64 "\n", a, size, value);
  }
  if (fifo >= 0) {
    unsigned n = selected(u, fifo);
    CG50USBPipe * p = &u->pipe[n];
    bool big = REG(u, fifo_sel[fifo]) & 0x0100;
    for (unsigned i = 0; i < size && p->length < sizeof(p->data); i++) {
      p->data[p->length++] = value >> (8 * (big ? size - i - 1 : i));
    }
    if (p->length >= packet_size(u, n)) {
      p->ready = true;
    }
    return;
  }
  if (size == 4) {
    usb_write(u, a, value >> 16, 2);
    usb_write(u, a + 2, value & 0xffff, 2);
    return;
  }
  if (size == 1) {
    unsigned shift = (a & 1) ? 0 : 8;
    value = (REG(u, a) & ~(255 << shift)) | ((value & 255) << shift);
    a &= ~1;
    ctr = ctr_index(a);
  }
  if (ctr >= 0) {
    CG50USBPipe * p = &u->pipe[selected(u, ctr)];
    if (value & 0x4000) {
      p->length = p->cursor = 0;
      p->ready = false;
    }
    if (value & 0x8000) {
      p->ready = true;
    }
  } else if (a == 0x40) {
    REG(u, a) &= value | 0x00f7; /* W0C events and setup VALID; state is read-only. */
  } else if (a == 0x42 || a == 0x46 || a == 0x48 || a == 0x4a) {
    REG(u, a) &= value;
  } else if (a >= 0x68 && a <= 0x6e) {
    unsigned n = REG(u, 0x64) & 15;
    if (n && n < 10) {
      CG50USBPipe * p = &u->pipe[n];
      if (a == 0x68) {
        p->cfg = value;
      }
      if (a == 0x6a) {
        p->buf = value;
      }
      if (a == 0x6c) {
        p->maxp = value;
      }
      if (a == 0x6e) {
        p->peri = value;
      }
    }
  } else if (a == 0x60 || (a >= 0x70 && a < 0x82)) {
    unsigned n = a == 0x60 ? 0 : 1 + (a - 0x70) / 2;
    CG50USBPipe * p = &u->pipe[n];
    p->ctr = value & ~0x8320;
    if (value & 0x0200) {
      p->length = p->cursor = 0;
      p->ready = false;
    }
    if (!n && (value & 4)) {
      p->ready = true; /* Controller completes the zero-length status packet. */
    }
  } else if (a < sizeof(u->regs)) {
    REG(u, a) = value;
  }
  usb_update(u);
}
static const MemoryRegionOps usb_ops = {
    .read = usb_read,
    .write = usb_write,
    .endianness = DEVICE_BIG_ENDIAN,
    .valid = {.min_access_size = 1, .max_access_size = 4},
    .impl = {.min_access_size = 1, .max_access_size = 4},
};
static void reply(CG50USB * u, const char * text) {
  qemu_chr_fe_write_all(&u->host, (const uint8_t *)text, strlen(text));
}
static int decode_hex(const char * text, uint8_t * out, unsigned capacity) {
  unsigned length = strlen(text);
  if ((length & 1) || length / 2 > capacity) {
    return -1;
  }
  for (unsigned i = 0; i < length; i += 2) {
    int a = g_ascii_xdigit_value(text[i]), b = g_ascii_xdigit_value(text[i + 1]);
    if (a < 0 || b < 0) {
      return -1;
    }
    out[i / 2] = a * 16 + b;
  }
  return length / 2;
}
static int endpoint_pipe(CG50USB * u, unsigned endpoint, bool in) {
  if (!endpoint) {
    return 0;
  }
  for (unsigned i = 1; i < 10; i++) {
    if ((u->pipe[i].cfg & 15) == endpoint && !!(u->pipe[i].cfg & 16) == in) {
      return i;
    }
  }
  return -1;
}
static void host_command(CG50USB * u, char * line) {
  char * arg = strchr(line, ' ');
  if (arg) {
    *arg++ = 0;
  }
  if (!strcmp(line, "attach") || !strcmp(line, "detach")) {
    u->attached = !strcmp(line, "attach");
    REG(u, INTSTS) = 0x8000 | (u->attached ? 0x80 : 0);
    reply(u, "OK\n");
  } else if (!strcmp(line, "state")) {
    char text[128];
    snprintf(text, sizeof(text), "STATE %04x %04x %04x %04x %04x\n", REG(u, 0), REG(u, 0x30),
             REG(u, 0x40), REG(u, 0x5e), u->pipe[0].ctr);
    reply(u, text);
  } else if (!u->attached) {
    reply(u, "DISCONNECTED\n");
  } else if (!strcmp(line, "reset")) {
    memset(u->pipe, 0, sizeof(u->pipe));
    REG(u, 0x08) = 3;
    REG(u, 0x50) = 0;
    REG(u, INTSTS) = 0x1090;
    reply(u, "OK\n");
  } else if (!strcmp(line, "setup") && arg) {
    if (decode_hex(arg, u->setup, 8) != 8) {
      reply(u, "ERROR setup\n");
      return;
    }
    for (unsigned i = 0; i < 4; i++) {
      REG(u, 0x54 + 2 * i) = lduw_le_p(u->setup + 2 * i);
    }
    u->pipe[0].length = u->pipe[0].cursor = 0;
    u->pipe[0].ready = false;
    /* A new SETUP ends a control-pipe STALL; NAK until firmware handles it. */
    u->pipe[0].ctr &= (uint16_t)~3;
    u->transferred = 0;
    control_stage(u, u->setup[0] & 128 ? 1 : REG(u, 0x5a) ? 3 : 5);
    REG(u, INTSTS) |= 8;
    reply(u, "OK\n");
  } else if ((!strcmp(line, "in") || !strcmp(line, "out")) && arg) {
    bool in = !strcmp(line, "in");
    char * end;
    unsigned endpoint = strtoul(arg, &end, 10);
    if (endpoint > 15 || (*end && *end != ' ')) {
      reply(u, "ERROR endpoint\n");
      return;
    }
    int n = endpoint_pipe(u, endpoint, in);
    if (n < 0 || !(u->pipe[n].ctr & 3)) {
      reply(u, "NAK\n");
      return;
    }
    CG50USBPipe * p = &u->pipe[n];
    if ((p->ctr & 3) >= 2) {
      reply(u, "STALL\n");
      return;
    }
    if (in) {
      if (!p->ready) {
        reply(u, "NAK\n");
        return;
      }
      if (u->trace_left) {
        u->trace_left--;
        qemu_log("CG50 USB IN pipe=%d length=%u cursor=%u\n", n, p->length, p->cursor);
      }
      char text[8200];
      unsigned take = MIN(p->length - p->cursor, packet_size(u, n));
      strcpy(text, "DATA ");
      for (unsigned i = 0; i < take; i++) {
        sprintf(text + 5 + 2 * i, "%02x", p->data[p->cursor++]);
      }
      strcpy(text + 5 + 2 * take, "\n");
      reply(u, text);
      u->transferred += take;
      if (p->cursor == p->length) {
        p->length = p->cursor = 0;
        p->ready = false;
        REG(u, 0x4a) |= 1 << n;
        REG(u, 0x46) |= 1 << n;
      }
      if (!n) {
        if ((u->setup[0] & 128) && (take < packet_size(u, n) || u->transferred >= REG(u, 0x5a))) {
          control_stage(u, 2);
        } else if (!(u->setup[0] & 128)) {
          control_stage(u, 0);
          if (u->setup[1] == 5) {
            REG(u, 0x50) = REG(u, 0x56);
            REG(u, INTSTS) = 0x18a0;
          }
          if (u->setup[1] == 9) {
            REG(u, INTSTS) = 0x18b0;
          }
        }
      }
    } else {
      if (p->length != p->cursor) {
        reply(u, "NAK\n");
        return;
      }
      int length = decode_hex(*end == ' ' ? end + 1 : "", p->data, packet_size(u, n));
      if (length < 0) {
        reply(u, "ERROR packet\n");
        return;
      }
      p->length = length;
      p->cursor = 0;
      p->ready = false;
      REG(u, 0x46) |= 1 << n;
      if (!n && (u->setup[0] & 128) && !length) {
        control_stage(u, 0);
      }
      reply(u, "OK\n");
    }
  } else {
    reply(u, "ERROR command\n");
  }
  usb_update(u);
}
static int can_read(void * opaque) {
  return 4096;
}
static void host_read(void * opaque, const uint8_t * data, int size) {
  CG50USB * u = opaque;
  for (int i = 0; i < size; i++) {
    if (u->discard_line) {
      if (data[i] == '\n') {
        u->discard_line = false;
      }
      continue;
    }
    if (data[i] == '\n') {
      u->command[u->command_length] = 0;
      host_command(u, u->command);
      u->command_length = 0;
    } else if (u->command_length + 1 < sizeof(u->command)) {
      u->command[u->command_length++] = data[i];
    } else {
      u->command_length = 0;
      u->discard_line = true;
      reply(u, "ERROR length\n");
    }
  }
}
void cg50_usb_init(CG50USB * u, void (*irq_changed)(void *), void * opaque) {
  u->trace_left = getenv("CG50_TRACE_USB") ? 4096 : 0;
  u->irq_changed = irq_changed;
  u->opaque = opaque;
  memory_region_init_io(&u->mr, NULL, &usb_ops, u, "cg50.usbhs", 0x100);
  memory_region_add_subregion_overlap(get_system_memory(), 0x04d80000, &u->mr, 1);
  Chardev * chr = qemu_chr_find("cg50-usb");
  if (chr) {
    qemu_chr_fe_init(&u->host, chr, &error_fatal);
    qemu_chr_fe_set_handlers(&u->host, can_read, host_read, NULL, NULL, u, NULL, true);
  }
}
