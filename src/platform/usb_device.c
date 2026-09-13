/* SPDX-License-Identifier: GPL-3.0-or-later
 * SH7305 USBHS register layout cross-checked with gint's mpu/usb.h.
 * One bounded ASCII request per full-speed bulk packet; one request in flight.
 */
#include <stdint.h>
#include "usb_device.h"

#define R16(a) (*(volatile uint16_t *)(a))
#define R32(a) (*(volatile uint32_t *)(a))
#define USB(n) R16(0xa4d80000u + (n))
#define BYTE(n) (*(volatile uint8_t *)(0xa4d80000u + (n)))
#define SYSCFG USB(0x00)
#define STATUS USB(0x40)
#define DCP USB(0x60)
#define OUTCTR USB(0x70)
#define INCTR USB(0x72)
#define IPRF R16(0xa4080014)
#define EXTPRI R32(0xa4140010)

/* FFFF:FFFF is an unassigned, VM-only test identity, not a shipping VID/PID. */
static const uint8_t device[] = {18, 1, 0, 2, 0, 0, 0, 64, 255, 255, 255, 255, 0, 1, 0, 0, 0, 1};
static const uint8_t configuration[] = {9,  2, 32, 0,    1, 1,    0, 0x80, 50, 9, 4,
                                        0,  0, 2,  0xff, 0, 0,    0, 7,    5,  1, 2,
                                        64, 0, 0,  7,    5, 0x82, 2, 64,   0,  0};

static void delay(void) {
  /* Bounded settling delay; physical-device clock timing is not yet accepted. */
  for (volatile unsigned i = 0; i < 200000; ++i)
    __asm__ volatile("nop");
}
static int fifo_select(unsigned offset, uint16_t selection) {
  USB(offset) = selection;
  for (unsigned i = 0; i < 10000; ++i)
    if (USB(offset) == selection && (USB(offset + 2) & 0x2000))
      return 1;
  return 0;
}
static void pipe_configure(unsigned pipe, uint16_t cfg, uint16_t buffer) {
  USB(0x6e + 2 * pipe) = 0;
  USB(0x64) = pipe;
  USB(0x68) = cfg;
  USB(0x6a) = buffer;
  USB(0x6c) = 64;
  USB(0x6e) = 0;
  USB(0x6e + 2 * pipe) = 0x200; /* Reset the data toggle/FIFO. */
  USB(0x6e + 2 * pipe) = 0;
}
static void configure(int enabled) {
  OUTCTR = INCTR = 0;
  pipe_configure(1, 0x4001, 8); /* bulk OUT endpoint 1 */
  pipe_configure(2, 0x4012, 9); /* bulk IN endpoint 2 */
  USB(0x46) = USB(0x4a) = 0;
  if (enabled)
    OUTCTR = 1;
}
static void control_send(const uint8_t * data, unsigned length, unsigned requested) {
  if (length > requested)
    length = requested;
  DCP = 0;
  if (!fifo_select(0x20, 0x120)) {
    DCP = 2;
    return;
  }
  USB(0x22) = 0x4000;
  for (unsigned i = 0; i < length; ++i)
    BYTE(0x14) = data[i];
  USB(0x22) = 0x8000;
  DCP = 1;
}
static int setup(int configured) {
  uint16_t request = USB(0x54), value = USB(0x56), index = USB(0x58), length = USB(0x5a);
  uint8_t reply[2] = {0, 0};
  STATUS = (uint16_t)~0x0808;
  DCP = 0;
  if (request == 0x0680 && !index && value == 0x100) {
    control_send(device, sizeof(device), length);
  } else if (request == 0x0680 && !index && value == 0x200) {
    control_send(configuration, sizeof(configuration), length);
  } else if (request == 0x0500 && value < 128 && !index && !length) {
    DCP = 5; /* SET_ADDRESS status; USBHS applies the address itself. */
  } else if (request == 0x0900 && value <= 1 && !index && !length) {
    configured = value;
    configure(configured);
    DCP = 5;
  } else if (request == 0x0880 && !value && !index && length == 1) {
    reply[0] = configured;
    control_send(reply, 1, length);
  } else if (request == 0x0080 && !value && !index && length == 2) {
    control_send(reply, 2, length);
  } else if (request == 0x0a81 && !value && !index && length == 1 && configured) {
    control_send(reply, 1, length);
  } else if (request == 0x0b01 && !value && !index && !length && configured) {
    DCP = 5;
  } else {
    DCP = 2;
  }
  return configured;
}
void usb_device_open(usb_device * state) {
  state->old_iprf = IPRF;
  state->old_extpri = EXTPRI;
  state->old_clock = R32(0xa4150014);
  state->old_stop = R32(0xa4150038);
  state->old_a = R16(0xa4050180);
  state->old_b = R16(0xa4050182);
  state->old_power = R16(0xa40501d4);
  state->configured = state->pending = 0;
  state->generation = 0;
  IPRF &= (uint16_t)~0x00f0; /* Own USB polling; preserve keyboard priority. */
  EXTPRI &= ~0x0f000000u;    /* Prevent the OS cable handler taking the device. */
  R16(0xa4050180) &= (uint16_t)~0x00c0;
  R16(0xa4050182) &= (uint16_t)~0xc000;
  R32(0xa4150014) &= ~0x100u;
  delay();
  R32(0xa4150038) &= ~0x00100000u;
  R16(0xa40501d4) = 0x0600;
  state->old_syscfg = SYSCFG;
  SYSCFG |= 0x400;
  delay();
  state->old_irq0 = USB(0x30);
  state->old_irq1 = USB(0x32);
  USB(0x30) = USB(0x32) = 0;
  USB(0x02) = 5;
  SYSCFG = 0x400;
  USB(0x5c) = 0;
  USB(0x5e) = 64;
  configure(0);
  SYSCFG = 0x411; /* Module/clock enable, full-speed D+ pull-up. */
  (void)USB(0x04);
}
void usb_device_close(usb_device * state) {
  SYSCFG &= (uint16_t)~0x10;
  configure(0);
  USB(0x30) = state->old_irq0;
  USB(0x32) = state->old_irq1;
  SYSCFG = state->old_syscfg;
  R16(0xa40501d4) = state->old_power;
  delay();
  R32(0xa4150038) = state->old_stop;
  R32(0xa4150014) = state->old_clock;
  R16(0xa4050180) = state->old_a;
  R16(0xa4050182) = state->old_b;
  EXTPRI = state->old_extpri;
  IPRF = state->old_iprf;
}
int usb_device_poll(usb_device * state) {
  uint16_t status = STATUS;
  if (status & 0x1000) {
    STATUS = (uint16_t)~0x1000;
    if ((status & 0x70) == 0x10) {
      state->configured = state->pending = 0;
      configure(0);
      USB(0x5e) = 64;
    }
  }
  if ((status & 0x8000) && !(status & 0x80)) {
    state->configured = state->pending = 0;
    configure(0);
    STATUS = (uint16_t)~0x8000;
  }
  if (STATUS & 8) {
    int before = state->configured;
    state->configured = setup(before);
    if (before != state->configured) {
      state->pending = 0;
      if (state->configured)
        ++state->generation;
    }
  } else if ((STATUS & 0x0807) == 0x0802) {
    STATUS = (uint16_t)~0x0800;
    DCP = 5;
  }
  if (state->pending && (USB(0x4a) & 4)) {
    INCTR = 0;
    USB(0x4a) = (uint16_t)~4;
    state->pending = 0;
    OUTCTR = 1;
  }
  return state->configured;
}
int usb_device_write(usb_device * state, const uint8_t * data, unsigned size) {
  if (!state->configured || state->pending)
    return 0;
  if (!size || size > 64 || !fifo_select(0x2c, 0x102))
    return -1;
  USB(0x2e) = 0x4000;
  USB(0x4a) = (uint16_t)~4;
  for (unsigned i = 0; i < size; ++i)
    BYTE(0x1c) = data[i];
  USB(0x2e) = 0x8000;
  INCTR = 1;
  state->pending = 1;
  return 1;
}
int usb_device_read(usb_device * state, uint8_t * data) {
  if (!state->configured || state->pending || !(USB(0x46) & 2))
    return 0;
  OUTCTR = 0;
  if (!fifo_select(0x28, 0x101))
    return -1;
  unsigned length = USB(0x2a) & 0xfff;
  if (length > 64)
    return -1;
  for (unsigned i = 0; i < length; ++i)
    data[i] = BYTE(0x18);
  USB(0x2a) = 0x4000;
  USB(0x46) = (uint16_t)~2;
  OUTCTR = 1;
  return length;
}
int usb_device_cancelled(void) {
  return !!(R16(0xa44b0006) & 0x800);
}
int usb_device_exe(void) {
  return !!(R16(0xa44b0000) & 0x400);
}
