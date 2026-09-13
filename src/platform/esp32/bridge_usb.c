#include "bridge.h"
#include <stdlib.h>
#include <string.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "usb/usb_host.h"

static usb_host_client_handle_t client;
static usb_device_handle_t device;
static uint8_t pending_address, ep_in, ep_out, interface;
static bool disconnected, completed;
static usb_transfer_t * transfer;

static void events(const usb_host_client_event_msg_t * event, void * arg) {
  (void)arg;
  if (event->event == USB_HOST_CLIENT_EVENT_NEW_DEV)
    pending_address = event->new_dev.address;
  else if (event->event == USB_HOST_CLIENT_EVENT_DEV_GONE)
    disconnected = true;
}

static void transfer_done(usb_transfer_t * result) {
  (void)result;
  completed = true;
}

static int packet(uint8_t endpoint, const void * out, unsigned length) {
  if (disconnected || length > 64)
    return -1;
  transfer->device_handle = device;
  transfer->bEndpointAddress = endpoint;
  transfer->num_bytes = length;
  transfer->callback = transfer_done;
  completed = false;
  if (out && length)
    memcpy(transfer->data_buffer, out, length);
  if (usb_host_transfer_submit(transfer) != ESP_OK)
    return -1;
  int64_t deadline = esp_timer_get_time() + 90000000;
  bool cancelled = false;
  while (!completed) {
    usb_host_client_handle_events(client, pdMS_TO_TICKS(20));
    if (!cancelled && (disconnected || esp_timer_get_time() > deadline)) {
      usb_host_endpoint_halt(device, endpoint);
      usb_host_endpoint_flush(device, endpoint);
      cancelled = true;
    }
  }
  if (cancelled || transfer->status != USB_TRANSFER_STATUS_COMPLETED)
    return -1;
  return transfer->actual_num_bytes;
}

static uint32_t big32(const unsigned char * p) {
  return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

static bool exchange(void) {
  unsigned char frame[16 + BRIDGE_REQUEST_MAX + 1];
  unsigned used = 0, expected = 16;
  while (used < expected) {
    int n = packet(ep_in, NULL, 64);
    if (n <= 0 || used + n > sizeof(frame) - 1)
      return false;
    memcpy(frame + used, transfer->data_buffer, n);
    used += n;
    if (used >= 16) {
      if (memcmp(frame, "KAI1\x01\0\0\0", 8) || !big32(frame + 8))
        return false;
      unsigned length = big32(frame + 12);
      if (!length || length > BRIDGE_REQUEST_MAX)
        return false;
      expected = 16 + length;
    }
    if (used > expected)
      return false;
  }
  if (memchr(frame + 16, 0, used - 16))
    return false;
  frame[used] = 0;
  char * reply = malloc(BRIDGE_REPLY_MAX + 17);
  if (!reply)
    return false;
  bool ok = bridge_api_solve((char *)frame + 16, reply + 16, BRIDGE_REPLY_MAX + 1);
  unsigned length = strlen(reply + 16);
  memcpy(reply, frame, 16);
  reply[4] = 2;
  reply[5] = ok ? 0 : 1;
  reply[12] = reply[13] = 0;
  reply[14] = length >> 8;
  reply[15] = length & 255;
  ESP_LOGI("bridge", "AI response: request=%lu status=%u bytes=%u", (unsigned long)big32(frame + 8),
           (unsigned)reply[5], length);
  bool sent = true;
  for (unsigned offset = 0; offset < length + 16; offset += 64) {
    unsigned n = length + 16 - offset;
    if (n > 64)
      n = 64;
    if (packet(ep_out, reply + offset, n) != n) {
      sent = false;
      break;
    }
  }
  free(reply);
  if (!sent || packet(ep_in, NULL, 64) != 16)
    return false;
  unsigned char * ack = transfer->data_buffer;
  return !memcmp(ack, "KAI1\x03\0\0\0", 8) && !memcmp(ack + 8, frame + 8, 4) &&
         big32(ack + 12) == 0;
}

static bool claim(void) {
  const usb_device_desc_t * descriptor;
  const usb_config_desc_t * config;
  if (usb_host_get_device_descriptor(device, &descriptor) != ESP_OK ||
      descriptor->idVendor != 0xffff || descriptor->idProduct != 0xffff ||
      usb_host_get_active_config_descriptor(device, &config) != ESP_OK)
    return false;
  unsigned total = config->wTotalLength;
  const unsigned char * bytes = (const unsigned char *)config;
  bool selected = false;
  ep_in = ep_out = 0;
  for (unsigned at = 0; at + 2 <= total;) {
    unsigned n = bytes[at];
    if (n < 2 || at + n > total)
      return false;
    const unsigned char * d = bytes + at;
    if (d[1] == 4 && n >= 9) {
      if (ep_in && ep_out)
        break;
      selected = d[3] == 0 && d[5] == 255 && d[6] == 0 && d[7] == 0;
      interface = d[2];
      ep_in = ep_out = 0;
    } else if (d[1] == 5 && n >= 7 && selected && (d[3] & 3) == 2 && d[4] == 64 && d[5] == 0 &&
               (d[2] & 15)) {
      if (d[2] & 128)
        ep_in = d[2];
      else
        ep_out = d[2];
    }
    at += n;
  }
  return ep_in && ep_out && usb_host_interface_claim(client, device, interface, 0) == ESP_OK;
}

static void host_events(void * unused) {
  (void)unused;
  for (;;) {
    uint32_t flags;
    usb_host_lib_handle_events(portMAX_DELAY, &flags);
  }
}

static void vbus(bool on) {
#if CONFIG_BRIDGE_VBUS_GPIO >= 0
  gpio_set_level(CONFIG_BRIDGE_VBUS_GPIO, on);
#endif
  esp_err_t result = usb_host_lib_set_root_port_power(on);
  ESP_LOGI("bridge", "USB port power %s: %s", on ? "on" : "off", esp_err_to_name(result));
  if (result != ESP_OK && result != ESP_ERR_INVALID_STATE)
    ESP_ERROR_CHECK(result);
}

static void usb_worker(void * unused) {
  (void)unused;
  usb_host_client_config_t options = {
      .max_num_event_msg = 8,
      .async = {.client_event_callback = events},
  };
  ESP_ERROR_CHECK(usb_host_client_register(&options, &client));
  ESP_ERROR_CHECK(usb_host_transfer_alloc(64, 0, &transfer));
  for (;;) {
    usb_host_client_handle_events(client, pdMS_TO_TICKS(20));
    if (!pending_address)
      continue;
    unsigned address = pending_address;
    pending_address = 0;
    disconnected = false;
    if (usb_host_device_open(client, address, &device) != ESP_OK)
      continue;
    if (claim()) {
      ESP_LOGI("bridge", "CG50 vendor interface claimed");
      bool acknowledged = exchange();
      ESP_LOGI("bridge", "USB exchange %s",
               acknowledged ? "acknowledged" : "failed or disconnected");
      ESP_LOGI("bridge", "heap free=%lu minimum=%lu; USB task stack remaining=%u",
               (unsigned long)esp_get_free_heap_size(),
               (unsigned long)esp_get_minimum_free_heap_size(),
               (unsigned)uxTaskGetStackHighWaterMark(NULL));
      usb_host_interface_release(client, device, interface);
      usb_host_device_close(client, device);
      device = NULL;
      /* Calculator restores its native driver only after the host detaches. */
      vbus(false);
      vTaskDelay(pdMS_TO_TICKS(1000));
      vbus(true);
    } else {
      usb_host_device_close(client, device);
      device = NULL;
    }
  }
}

void bridge_usb_start(void) {
#if CONFIG_BRIDGE_VBUS_GPIO >= 0
  gpio_config_t pin = {.pin_bit_mask = 1ULL << CONFIG_BRIDGE_VBUS_GPIO, .mode = GPIO_MODE_OUTPUT};
  ESP_ERROR_CHECK(gpio_config(&pin));
  gpio_set_level(CONFIG_BRIDGE_VBUS_GPIO, 1);
#endif
  usb_host_config_t config = {.intr_flags = ESP_INTR_FLAG_LEVEL1};
  ESP_ERROR_CHECK(usb_host_install(&config));
  if (xTaskCreate(host_events, "usb_events", 4096, NULL, 10, NULL) != pdPASS)
    abort();
  if (xTaskCreate(usb_worker, "usb_ai", 10240, NULL, 5, NULL) != pdPASS)
    abort();
}
