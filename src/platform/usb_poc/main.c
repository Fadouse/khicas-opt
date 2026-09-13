/* SPDX-License-Identifier: GPL-3.0-or-later
 * SH7305 USBHS register layout cross-checked with gint's mpu/usb.h.
 * One bounded ASCII request per full-speed bulk packet; one request in flight.
 */
#include <stdint.h>
#include <fxcg/display.h>

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
static unsigned message(const char * prefix, uint32_t value, char * buffer) {
  char digits[10];
  unsigned count = 0, length = 0;
  while (*prefix)
    buffer[length++] = *prefix++;
  do {
    digits[count++] = '0' + value % 10;
    value /= 10;
  } while (value);
  while (count)
    buffer[length++] = digits[--count];
  buffer[length++] = '\n';
  buffer[length] = 0;
  return length;
}
static int same(const char * a, const char * b, unsigned length) {
  for (unsigned i = 0; i < length; ++i)
    if (!b[i] || a[i] != b[i])
      return 0;
  return b[length] == 0;
}
static void line(int row, const char * text) {
  /* PrintXY consumes the two leading characters, as in the main UI's mPrintXY. */
  char buffer[24];
  unsigned i = 2;
  buffer[0] = buffer[1] = ' ';
  while (*text && i < sizeof(buffer) - 1)
    buffer[i++] = *text++;
  buffer[i] = 0;
  PrintXY(1, row, buffer, 0, 0);
}
static void display(const char * state, const char * request, const char * reply) {
  Bdisp_AllClr_VRAM();
  line(1, "USB protocol PoC");
  line(3, state);
  line(5, request);
  line(6, reply);
  line(8, "EXIT: close USB");
  Bdisp_PutDisp_DD();
}

/* The OS supplies the add-in stack. No runtime globals or heap are required. */
int __attribute__((section(".pretext"))) usb_poc_start(void) {
  uint16_t old_iprf = IPRF, old_syscfg, old_irq0, old_irq1;
  uint32_t old_extpri = EXTPRI, old_clock = R32(0xa4150014), old_stop = R32(0xa4150038);
  uint16_t old_a = R16(0xa4050180), old_b = R16(0xa4050182), old_power = R16(0xa40501d4);
  int configured = 0, pending = 0, awaiting_reply = 0, send_next = 0;
  int exe_down = 1;
  uint32_t sequence = 1;
  char received[65], response[65], sent[16];
  received[0] = response[0] = sent[0] = 0;
  IPRF &= (uint16_t)~0x00f0; /* Own USB polling; preserve keyboard priority. */
  EXTPRI &= ~0x0f000000u;    /* Prevent the OS cable handler taking the device. */
  R16(0xa4050180) &= (uint16_t)~0x00c0;
  R16(0xa4050182) &= (uint16_t)~0xc000;
  R32(0xa4150014) &= ~0x100u;
  delay();
  R32(0xa4150038) &= ~0x00100000u;
  R16(0xa40501d4) = 0x0600;
  old_syscfg = SYSCFG;
  SYSCFG |= 0x400;
  delay();
  old_irq0 = USB(0x30);
  old_irq1 = USB(0x32);
  USB(0x30) = USB(0x32) = 0;
  USB(0x02) = 5;
  SYSCFG = 0x400;
  USB(0x5c) = 0;
  USB(0x5e) = 64;
  configure(0);
  SYSCFG = 0x411; /* Module/clock enable, full-speed D+ pull-up. */
  (void)USB(0x04);
  display("Waiting for host", received, response);
  for (;;) {
    uint16_t status = STATUS;
    if (R16(0xa44b0006) & 0x800)
      break; /* EXIT matrix key */
    if (status & 0x1000) {
      STATUS = (uint16_t)~0x1000;
      if ((status & 0x70) == 0x10) {
        configured = pending = awaiting_reply = send_next = 0;
        configure(0);
        USB(0x5e) = 64;
      }
    }
    if ((status & 0x8000) && !(status & 0x80)) {
      configured = pending = awaiting_reply = send_next = 0;
      configure(0);
      STATUS = (uint16_t)~0x8000;
    }
    if (STATUS & 8) {
      int before = configured;
      configured = setup(configured);
      if (before != configured) {
        display(configured ? "Connected: bulk" : "Waiting for host", "", "");
        sequence = 1;
        send_next = configured;
        pending = awaiting_reply = 0;
      }
    } else if ((STATUS & 0x0807) == 0x0802) {
      STATUS = (uint16_t)~0x0800;
      DCP = 5;
    }
    if (pending && (USB(0x4a) & 4)) {
      INCTR = 0;
      USB(0x4a) = (uint16_t)~4;
      pending = 0;
      OUTCTR = 1;
    }
    int exe = !!(R16(0xa44b0000) & 0x400);
    if (configured && !awaiting_reply && !pending && exe && !exe_down) {
      sequence = sequence == UINT32_MAX ? 1 : sequence + 1;
      send_next = 1;
    }
    exe_down = exe;
    if (configured && send_next && !pending) {
      unsigned count = message("send", sequence, sent);
      message("recv", sequence, response);
      if (!fifo_select(0x2c, 0x102))
        break;
      USB(0x2e) = 0x4000;
      USB(0x4a) = (uint16_t)~4;
      for (unsigned i = 0; i < count; ++i)
        BYTE(0x1c) = sent[i];
      USB(0x2e) = 0x8000;
      INCTR = 1;
      pending = awaiting_reply = 1;
      send_next = 0;
      sent[count - 1] = 0;
      display("Waiting for reply", sent, "");
    }
    if (configured && !pending && (USB(0x46) & 2)) {
      OUTCTR = 0;
      if (!fifo_select(0x28, 0x101))
        break;
      unsigned length = USB(0x2a) & 0xfff;
      if (length > 64)
        break;
      for (unsigned i = 0; i < length; ++i)
        received[i] = (char)BYTE(0x18);
      USB(0x2a) = 0x4000;
      USB(0x46) = (uint16_t)~2;
      received[length] = 0;
      int valid = awaiting_reply && same(received, response, length);
      if (valid)
        awaiting_reply = 0;
      if (length && received[length - 1] == '\n')
        received[length - 1] = 0;
      display(valid ? "Reply OK; EXE: next" : "Unexpected reply", sent,
              length < 20 ? received : "(long reply)");
      OUTCTR = 1;
    }
  }
  SYSCFG &= (uint16_t)~0x10;
  configure(0);
  USB(0x30) = old_irq0;
  USB(0x32) = old_irq1;
  SYSCFG = old_syscfg;
  R16(0xa40501d4) = old_power;
  delay();
  R32(0xa4150038) = old_stop;
  R32(0xa4150014) = old_clock;
  R16(0xa4050180) = old_a;
  R16(0xa4050182) = old_b;
  EXTPRI = old_extpri;
  IPRF = old_iprf;
  return 1;
}
