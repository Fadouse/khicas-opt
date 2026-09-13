/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdint.h>
#include <fxcg/display.h>
#include "usb_device.h"
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

/* The OS supplies the add-in stack. No mutable globals or heap. */
int __attribute__((section(".pretext"))) usb_poc_start(void) {
  usb_device device;
  unsigned generation = 0, sequence = 1;
  int awaiting = 0, send_next = 0, exe_down = 1;
  char received[65], response[16], sent[16];
  usb_device_open(&device);
  display("Waiting for host", "", "");
  while (!usb_device_cancelled()) {
    if (!usb_device_poll(&device)) {
      awaiting = send_next = 0;
      continue;
    }
    if (generation != device.generation) {
      generation = device.generation;
      sequence = 1;
      awaiting = 0;
      send_next = 1;
    }
    int exe = usb_device_exe();
    if (!awaiting && !device.pending && exe && !exe_down) {
      sequence = sequence == UINT32_MAX ? 1 : sequence + 1;
      send_next = 1;
    }
    exe_down = exe;
    if (send_next) {
      unsigned size = message("send", sequence, sent);
      message("recv", sequence, response);
      int status = usb_device_write(&device, (const uint8_t *)sent, size);
      if (status < 0)
        break;
      if (status) {
        awaiting = 1;
        send_next = 0;
        sent[size - 1] = 0;
        display("Waiting for reply", sent, "");
      }
    }
    int size = usb_device_read(&device, (uint8_t *)received);
    if (size < 0)
      break;
    if (size) {
      received[size] = 0;
      int valid = awaiting && same(received, response, size);
      if (valid)
        awaiting = 0;
      if (received[size - 1] == '\n')
        received[size - 1] = 0;
      display(valid ? "Reply OK; EXE: next" : "Unexpected reply", sent,
              size < 20 ? received : "(long reply)");
    }
  }
  usb_device_close(&device);
  return 1;
}
