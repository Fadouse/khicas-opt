/* SPDX-License-Identifier: GPL-2.0-or-later
 * Forward control and bulk USB tokens to the independent CG50 USBHS model.
 */
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "hw/usb.h"
#include "hw/usb/hcd-dwc2.h"
#include "hw/usb/dwc2-regs.h"
#include "hw/qdev-properties.h"
#include <sys/socket.h>
#include <sys/un.h>

#define TYPE_CG50_PROXY "usb-cg50"
typedef struct CG50Proxy {
  USBDevice parent;
  char * path;
  FILE * socket;
  QEMUTimer * timer;
  bool powered;
  uint8_t configuration[4096];
  unsigned configuration_length;
} CG50Proxy;
OBJECT_DECLARE_SIMPLE_TYPE(CG50Proxy, CG50_PROXY)

static bool command(CG50Proxy * s, const char * request, char * reply, size_t size) {
  if (!s->socket || fprintf(s->socket, "%s\n", request) < 0 || fflush(s->socket) ||
      !fgets(reply, size, s->socket))
    return false;
  size_t length = strlen(reply);
  if (!length || reply[length - 1] != '\n')
    return false;
  reply[length - 1] = 0;
  return true;
}

static int token(CG50Proxy * s, int pid, unsigned endpoint, uint8_t * data, unsigned length) {
  if (length > 64)
    return USB_RET_BABBLE;
  char request[160], reply[160];
  if (pid == USB_TOKEN_IN) {
    snprintf(request, sizeof(request), "in %u", endpoint);
  } else {
    if (pid == USB_TOKEN_SETUP)
      strcpy(request, "setup ");
    else
      snprintf(request, sizeof(request), "out %u ", endpoint);
    unsigned at = strlen(request);
    for (unsigned i = 0; i < length; ++i)
      snprintf(request + at + i * 2, 3, "%02x", data[i]);
  }
  if (!command(s, request, reply, sizeof(reply)))
    return USB_RET_IOERROR;
  if (!strcmp(reply, "NAK"))
    return USB_RET_NAK;
  if (pid != USB_TOKEN_IN)
    return !strcmp(reply, "OK") ? length : USB_RET_STALL;
  if (strncmp(reply, "DATA ", 5))
    return USB_RET_STALL;
  unsigned size = strlen(reply + 5);
  if ((size & 1) || size / 2 > length)
    return USB_RET_BABBLE;
  for (unsigned i = 0; i < size / 2; ++i) {
    int a = g_ascii_xdigit_value(reply[5 + i * 2]);
    int b = g_ascii_xdigit_value(reply[6 + i * 2]);
    if (a < 0 || b < 0)
      return USB_RET_IOERROR;
    data[i] = a * 16 + b;
  }
  return size / 2;
}

static int wait_token(CG50Proxy * s, int pid, uint8_t * data, unsigned length) {
  int result;
  int64_t deadline = g_get_monotonic_time() + 1000000;
  do {
    result = token(s, pid, 0, data, length);
    if (result != USB_RET_NAK)
      break;
    g_usleep(1000);
  } while (g_get_monotonic_time() < deadline);
  return result;
}

static void control(USBDevice * dev, USBPacket * packet, int request, int value, int index,
                    int length, uint8_t * data) {
  CG50Proxy * s = CG50_PROXY(dev);
  uint8_t setup[8] = {request >> 8, request,    value,  value >> 8,
                      index,        index >> 8, length, length >> 8};
  bool input = setup[0] & 128;
  fprintf(stderr, "CG50 control request=%04x value=%04x length=%d\n", request, value, length);
  if (length > 4096) {
    packet->status = USB_RET_STALL;
    return;
  }
  int result = wait_token(s, USB_TOKEN_SETUP, setup, 8);
  unsigned used = 0;
  if (result < 0)
    goto failed;
  while (used < length) {
    unsigned size = MIN(64, length - used);
    result = wait_token(s, input ? USB_TOKEN_IN : USB_TOKEN_OUT, data + used, size);
    if (result < 0)
      goto failed;
    used += result;
    if (result < size)
      break;
  }
  uint8_t status[64];
  result = wait_token(s, input ? USB_TOKEN_OUT : USB_TOKEN_IN, status, 0);
  if (result < 0)
    goto failed;
  if (request == 0x0005)
    dev->addr = value;
  if (request == 0x8006 && (value >> 8) == 2 && used >= 9) {
    memcpy(s->configuration, data, used);
    s->configuration_length = used;
  }
  if (request == 0x0009) {
    usb_ep_reset(dev);
    for (unsigned at = 0; at + 2 <= s->configuration_length;) {
      const uint8_t * d = s->configuration + at;
      unsigned n = d[0];
      if (n < 2 || at + n > s->configuration_length)
        break;
      if (d[1] == 5 && n >= 7) {
        int pid = d[2] & 128 ? USB_TOKEN_IN : USB_TOKEN_OUT;
        usb_ep_set_type(dev, pid, d[2] & 15, d[3] & 3);
        usb_ep_set_max_packet_size(dev, pid, d[2] & 15, d[4] | d[5] << 8);
      }
      at += n;
    }
  }
  packet->actual_length = used;
  return;
failed:
  fprintf(stderr, "CG50 control failed request=%04x result=%d transferred=%u\n", request, result,
          used);
  packet->status = result == USB_RET_NAK ? USB_RET_IOERROR : result;
}

static void bulk(USBDevice * dev, USBPacket * packet) {
  CG50Proxy * s = CG50_PROXY(dev);
  uint8_t bytes[64];
  unsigned length = packet->iov.size;
  if (length > 64) {
    packet->status = USB_RET_BABBLE;
    return;
  }
  if (packet->pid == USB_TOKEN_OUT)
    usb_packet_copy(packet, bytes, length);
  int result = token(s, packet->pid, packet->ep->nr, bytes, length);
  if (result < 0) {
    packet->actual_length = 0;
    packet->status = result;
    return;
  }
  if (packet->pid == USB_TOKEN_IN)
    usb_packet_copy(packet, bytes, result);
}

static void reset(USBDevice * dev) {
  char reply[160];
  CG50Proxy * s = CG50_PROXY(dev);
  s->configuration_length = 0;
  command(s, "reset", reply, sizeof(reply));
  /* Allow the independently scheduled CG50 to observe the reset interrupt. */
  g_usleep(20000);
}

static void poll_link(void * opaque) {
  CG50Proxy * s = opaque;
  USBDevice * dev = USB_DEVICE(s);
  DWC2State * host = dev->port->opaque;
  bool power = host->hprt0 & HPRT0_PWR;
  char reply[160];
  if (power != s->powered) {
    if (command(s, power ? "attach" : "detach", reply, sizeof(reply)))
      s->powered = power;
  }
  unsigned syscfg = 0;
  bool connected = power && command(s, "state", reply, sizeof(reply)) &&
                   sscanf(reply, "STATE %x", &syscfg) == 1 && (syscfg & 0x11) == 0x11;
  if (connected && !dev->attached)
    usb_device_attach(dev, &error_abort);
  if (!connected && dev->attached)
    usb_device_detach(dev);
  timer_mod(s->timer, qemu_clock_get_ms(QEMU_CLOCK_REALTIME) + 20);
}

static void realize(USBDevice * dev, Error ** errp) {
  CG50Proxy * s = CG50_PROXY(dev);
  if (dev->port->hubcount ||
      !object_dynamic_cast(OBJECT(usb_bus_from_device(dev)->qbus.parent), TYPE_DWC2_USB)) {
    error_setg(errp, "usb-cg50 requires a DWC2 root port; specify port=1");
    return;
  }
  struct sockaddr_un address = {.sun_family = AF_UNIX};
  if (!s->path || strlen(s->path) >= sizeof(address.sun_path)) {
    error_setg(errp, "usb-cg50 requires a valid socket path");
    return;
  }
  strcpy(address.sun_path, s->path);
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0 || connect(fd, (struct sockaddr *)&address, sizeof(address))) {
    error_setg_errno(errp, errno, "CG50 USB socket connection failed");
    if (fd >= 0)
      close(fd);
    return;
  }
  struct timeval timeout = {.tv_sec = 2};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  s->socket = fdopen(fd, "r+");
  if (!s->socket) {
    close(fd);
    error_setg(errp, "CG50 socket stream failed");
    return;
  }
  setvbuf(s->socket, NULL, _IONBF, 0);
  dev->speed = USB_SPEED_FULL;
  dev->speedmask = USB_SPEED_MASK_FULL;
  dev->auto_attach = false;
  s->timer = timer_new_ms(QEMU_CLOCK_REALTIME, poll_link, s);
  timer_mod(s->timer, qemu_clock_get_ms(QEMU_CLOCK_REALTIME) + 20);
}

static void unrealize(USBDevice * dev) {
  CG50Proxy * s = CG50_PROXY(dev);
  if (s->timer) {
    timer_del(s->timer);
    timer_free(s->timer);
  }
  if (s->socket)
    fclose(s->socket);
}

static Property properties[] = {
    DEFINE_PROP_STRING("socket", CG50Proxy, path),
    DEFINE_PROP_END_OF_LIST(),
};
static void class_init(ObjectClass * klass, void * data) {
  USBDeviceClass * usb = USB_DEVICE_CLASS(klass);
  usb->product_desc = "CG50 USBHS token connection";
  usb->realize = realize;
  usb->unrealize = unrealize;
  usb->handle_reset = reset;
  usb->handle_control = control;
  usb->handle_data = bulk;
  device_class_set_props(DEVICE_CLASS(klass), properties);
}
static const TypeInfo info = {
    .name = TYPE_CG50_PROXY,
    .parent = TYPE_USB_DEVICE,
    .instance_size = sizeof(CG50Proxy),
    .class_init = class_init,
};
static void register_types(void) {
  type_register_static(&info);
}
type_init(register_types)
