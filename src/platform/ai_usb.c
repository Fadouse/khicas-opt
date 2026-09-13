/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ai_usb.h"
#include "usb_device.h"
#include <fxcg/rtc.h>

static uint32_t read32(const uint8_t * p) {
  return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
static void write32(uint8_t * p, uint32_t value) {
  for (int i = 3; i >= 0; --i) {
    p[i] = value;
    value >>= 8;
  }
}
static void copy_text(char * destination, const char * text) {
  while (*text)
    *destination++ = *text++;
  *destination = 0;
}
int khicas_ai_exchange(const char * request, unsigned size, char * reply) {
  usb_device device;
  uint8_t header[16], packet[64];
  unsigned sent = 0, received = 0, expected = 0, generation = 0;
  uint32_t start = (uint32_t)RTC_GetTicks(), id = start ? start : 1;
  int remote_error = 0, result = -1, complete = 0, acknowledged = 0;
  const char * error = "AI: USB timeout";
  reply[0] = 0;
  if (!size || size > KHICAS_AI_REQUEST_MAX) {
    copy_text(reply, "AI: request must be 1..1024 bytes");
    return -1;
  }
  header[0] = 'K';
  header[1] = 'A';
  header[2] = 'I';
  header[3] = '1';
  header[4] = 1;
  header[5] = header[6] = header[7] = 0;
  write32(header + 8, id);
  write32(header + 12, size);
  usb_device_open(&device);
  while ((uint32_t)((uint32_t)RTC_GetTicks() - start) < 120 * 128u) {
    if (usb_device_cancelled()) {
      result = KHICAS_AI_CANCELLED;
      error = "AI: cancelled";
      break;
    }
    int configured = usb_device_poll(&device);
    if (complete && acknowledged && !configured) {
      result = remote_error;
      break;
    }
    if (generation && (!configured || generation != device.generation)) {
      error = "AI: USB disconnected or reset";
      break;
    }
    if (!configured)
      continue;
    generation = device.generation;
    if (complete) {
      if (!acknowledged) {
        header[4] = 3;
        write32(header + 12, 0);
        int status = usb_device_write(&device, header, 16);
        if (status < 0) {
          error = "AI: ACK failed";
          break;
        }
        acknowledged = status;
      }
      continue;
    }
    if (sent < size + 16) {
      unsigned count = size + 16 - sent;
      if (count > 64)
        count = 64;
      for (unsigned i = 0; i < count; ++i)
        packet[i] = sent + i < 16 ? header[sent + i] : (uint8_t)request[sent + i - 16];
      int status = usb_device_write(&device, packet, count);
      if (status < 0) {
        error = "AI: USB write failed";
        break;
      }
      if (status)
        sent += count;
      continue;
    }
    int count = usb_device_read(&device, packet);
    if (count < 0) {
      error = "AI: USB read failed";
      break;
    }
    if (!count)
      continue;
    unsigned offset = 0;
    if (!received && !expected) {
      if (count < 16 || packet[0] != 'K' || packet[1] != 'A' || packet[2] != 'I' ||
          packet[3] != '1' || packet[4] != 2 || packet[5] > 1 || packet[6] || packet[7] ||
          read32(packet + 8) != id) {
        error = "AI: invalid response header";
        break;
      }
      expected = read32(packet + 12);
      remote_error = packet[5];
      offset = 16;
      if (!expected || expected > KHICAS_AI_REPLY_MAX) {
        error = "AI: invalid response length";
        break;
      }
    }
    if ((unsigned)count - offset > expected - received) {
      error = "AI: excess response data";
      break;
    }
    int valid = 1;
    for (unsigned i = offset; i < (unsigned)count; ++i) {
      uint8_t c = packet[i];
      if ((c < 32 && c != '\n' && c != '\r' && c != '\t') || c > 126) {
        valid = 0;
        break;
      }
      reply[received++] = c;
    }
    if (!valid) {
      error = "AI: response must be ASCII text";
      break;
    }
    if (received == expected) {
      reply[received] = 0;
      complete = 1;
    }
  }
  usb_device_close(&device);
  if (result < 0)
    copy_text(reply, error);
  return result;
}
