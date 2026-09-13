#include "bridge.h"
#include <stdlib.h>
#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"

esp_netif_t *bridge_lan, *bridge_wan;

static void network_event(void * arg, esp_event_base_t base, int32_t id, void * data) {
  (void)arg;
  if (base == WIFI_EVENT && (id == WIFI_EVENT_STA_START || id == WIFI_EVENT_STA_DISCONNECTED)) {
    wifi_config_t station;
    if (esp_wifi_get_config(WIFI_IF_STA, &station) == ESP_OK && station.sta.ssid[0]) {
      esp_wifi_connect();
    }
  } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t * event = data;
    esp_netif_ip_info_t lan;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(bridge_lan, &lan));
    if ((event->ip_info.ip.addr & lan.netmask.addr) == (lan.ip.addr & lan.netmask.addr)) {
      esp_netif_napt_disable(bridge_lan);
      ESP_LOGE("bridge", "uplink overlaps AP subnet 192.168.50.0/24; change upstream subnet");
      return;
    }
    esp_netif_dns_info_t dns;
    if (esp_netif_get_dns_info(bridge_wan, ESP_NETIF_DNS_MAIN, &dns) == ESP_OK) {
      esp_netif_set_dns_info(bridge_lan, ESP_NETIF_DNS_MAIN, &dns);
    }
    ESP_ERROR_CHECK(esp_netif_napt_enable(bridge_lan));
    ESP_LOGI("bridge", "uplink connected; guest lwIP NAPT enabled");
  }
}

void bridge_network_start(void) {
  bridge_wan = esp_netif_create_default_wifi_sta();
  bridge_lan = esp_netif_create_default_wifi_ap();
  assert(bridge_wan && bridge_lan);
  ESP_ERROR_CHECK(esp_netif_dhcps_stop(bridge_lan));
  esp_netif_ip_info_t ip;
  IP4_ADDR(&ip.ip, 192, 168, 50, 1);
  IP4_ADDR(&ip.gw, 192, 168, 50, 1);
  IP4_ADDR(&ip.netmask, 255, 255, 255, 0);
  ESP_ERROR_CHECK(esp_netif_set_ip_info(bridge_lan, &ip));
  ESP_ERROR_CHECK(esp_netif_dhcps_start(bridge_lan));
  wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&init));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, network_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, network_event, NULL));
  wifi_config_t ap = {
      .ap = {.ssid_hidden = 1, .channel = 6, .max_connection = 4, .authmode = WIFI_AUTH_WPA2_PSK}};
  strlcpy((char *)ap.ap.ssid, bridge_ap_name(), sizeof(ap.ap.ssid));
  strlcpy((char *)ap.ap.password, bridge_password(), sizeof(ap.ap.password));
  ap.ap.ssid_len = strlen(bridge_ap_name());
  bridge_config_t * saved = malloc(sizeof(*saved));
  assert(saved);
  bridge_config_get(saved);
  wifi_config_t station = {0};
  memcpy(station.sta.ssid, saved->ssid, strlen(saved->ssid));
  memcpy(station.sta.password, saved->wifi_password, strlen(saved->wifi_password));
  free(saved);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &station));
  ESP_ERROR_CHECK(esp_wifi_start());
}
