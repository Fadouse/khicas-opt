#ifndef KHICAS_USB_DEVICE_H
#define KHICAS_USB_DEVICE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Single owner; all saved hardware and transfer state belongs to the caller. */
typedef struct {
  uint16_t old_iprf, old_syscfg, old_irq0, old_irq1, old_a, old_b, old_power;
  uint32_t old_extpri, old_clock, old_stop;
  unsigned generation;
  int configured, pending;
} usb_device;
void usb_device_open(usb_device * state);
void usb_device_close(usb_device * state);
int usb_device_poll(usb_device * state);
/* Write: 1 queued, 0 busy, -1 error. Read: byte count, 0 empty, -1 error.
   Read requires a 64-byte destination. A generation change invalidates transfers. */
int usb_device_write(usb_device * state, const uint8_t * data, unsigned size);
int usb_device_read(usb_device * state, uint8_t * data);
int usb_device_cancelled(void);
int usb_device_exe(void);
#ifdef __cplusplus
}
#endif
#endif
