#include "bridge.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"

void app_main(void) {
  /* Never erase saved credentials automatically on an NVS error. */
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  bridge_config_init();
  bridge_network_start();
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
#if CONFIG_BRIDGE_QEMU
  esp_sntp_setservername(0, "10.0.2.2");
#else
  esp_sntp_setservername(0, "pool.ntp.org");
#endif
  esp_sntp_init();
  bridge_web_start();
  bridge_usb_start();
  ESP_LOGI("bridge", "ready; AP %s; HTTPS https://192.168.50.1/", bridge_ap_name());
  ESP_LOGI("bridge", "local setup password (Wi-Fi and HTTPS admin): %s", bridge_password());
}
