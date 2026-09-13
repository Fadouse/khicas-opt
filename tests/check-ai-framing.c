/* Exercise the actual C framing code with packet and clock fixtures. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ai_usb.h"
#include "usb_device.h"

enum { GOOD, WRONG_ID, OVERSIZE, BAD_TEXT, REMOTE_ERROR, CANCEL, TIMEOUT };
static int mode, closed, acknowledged, clock_calls;
static unsigned sent, read_offset, response_size;
static uint8_t request[1040], response[2065];
int RTC_GetTicks(void) {
  return mode == TIMEOUT && clock_calls++ ? 16000 : 1;
}
void usb_device_open(usb_device * d) {
  d->configured = 1;
  d->generation = 1;
  d->pending = 0;
}
void usb_device_close(usb_device * d) {
  (void)d;
  ++closed;
}
int usb_device_cancelled(void) {
  return mode == CANCEL;
}
int usb_device_poll(usb_device * d) {
  d->configured = !acknowledged;
  return d->configured;
}
int usb_device_write(usb_device * d, const uint8_t * data, unsigned size) {
  (void)d;
  if (sent == sizeof(request)) {
    assert(size == 16 && !memcmp(data, "KAI1", 4) && data[4] == 3);
    assert(!memcmp(data + 8, request + 8, 4));
    assert(data[12] == 0 && data[13] == 0 && data[14] == 0 && data[15] == 0);
    acknowledged = 1;
    return 1;
  }
  assert(size > 0 && size <= 64 && sent + size <= sizeof(request));
  memcpy(request + sent, data, size);
  sent += size;
  return 1;
}
int usb_device_read(usb_device * d, uint8_t * data) {
  (void)d;
  if (!read_offset) {
    memcpy(response, "KAI1\2\0\0\0", 8);
    memcpy(response + 8, request + 8, 4);
    response[12] = response[13] = 0;
    response[14] = 8;
    response[15] = 0;
    memset(response + 16, 'A', 2048);
    response_size = 2064;
    if (mode == WRONG_ID)
      response[11] ^= 1;
    if (mode == OVERSIZE)
      response[15] = 1;
    if (mode == BAD_TEXT)
      response[20] = 0;
    if (mode == REMOTE_ERROR)
      response[5] = 1;
  }
  unsigned count = response_size - read_offset;
  if (count > 64)
    count = 64;
  memcpy(data, response + read_offset, count);
  read_offset += count;
  return count;
}
int main(void) {
  char question[1024], answer[2049];
  memset(question, 'x', sizeof(question));
  for (mode = GOOD; mode <= TIMEOUT; ++mode) {
    sent = read_offset = 0;
    closed = acknowledged = clock_calls = 0;
    int status = khicas_ai_exchange(question, sizeof(question), answer);
    assert(closed == 1);
    if (mode == GOOD || mode == REMOTE_ERROR) {
      assert(status == (mode == REMOTE_ERROR));
      assert(acknowledged);
      assert(sent == 1040 && !memcmp(request, "KAI1\1\0\0\0", 8));
      assert(request[12] == 0 && request[13] == 0 && request[14] == 4 && request[15] == 0);
      assert(!memcmp(request + 16, question, 1024));
      assert(strlen(answer) == 2048);
      for (unsigned i = 0; i < 2048; ++i)
        assert(answer[i] == 'A');
    } else {
      assert(status == (mode == CANCEL ? KHICAS_AI_CANCELLED : -1) && !acknowledged &&
             !strncmp(answer, "AI:", 3));
    }
  }
  closed = 0;
  assert(khicas_ai_exchange(question, 1025, answer) == -1 && !closed);
  assert(khicas_ai_exchange(question, 0, answer) == -1 && !closed);
  puts("PASS: C framing 1024/2048-byte bounds, ACK, wrong ID, invalid lengths/text, remote error, "
       "cancel and timeout");
}
