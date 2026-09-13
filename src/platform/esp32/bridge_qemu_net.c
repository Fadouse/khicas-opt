/* OpenCores MMIO adapter for two QEMU NICs; not a Wi-Fi radio model. */
#include "bridge.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_netif_defaults.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RX_COUNT 8
#define FRAME_MAX 1600
typedef struct {
  esp_netif_driver_base_t base;
  volatile uint32_t * registers;
  unsigned rx_index;
  unsigned char tx[FRAME_MAX];
  unsigned char rx[RX_COUNT][FRAME_MAX];
} virtual_mac_t;

esp_netif_t *bridge_lan, *bridge_wan;
static virtual_mac_t interfaces[2];

static esp_err_t transmit(void * handle, void * buffer, size_t length) {
  virtual_mac_t * mac = handle;
  if (length > FRAME_MAX)
    return ESP_ERR_INVALID_SIZE;
  memcpy(mac->tx, buffer, length);
  /* QEMU completes TX synchronously before the descriptor MMIO write returns. */
  mac->registers[0x400 / 4] = (length << 16) | 0xa800;
  return ESP_OK;
}

static void free_received(void * handle, void * buffer) {
  (void)handle;
  free(buffer);
}

static esp_err_t attach(esp_netif_t * netif, void * handle) {
  virtual_mac_t * mac = handle;
  mac->base.netif = netif;
  esp_netif_driver_ifconfig_t driver = {
      .handle = mac,
      .transmit = transmit,
      .driver_free_rx_buffer = free_received,
  };
  return esp_netif_set_driver_config(netif, &driver);
}

static void poll_rx(void * unused) {
  (void)unused;
  for (;;) {
    for (unsigned n = 0; n < 2; ++n) {
      virtual_mac_t * mac = &interfaces[n];
      for (unsigned count = 0; count < RX_COUNT; ++count) {
        unsigned index = mac->rx_index;
        volatile uint32_t * desc = mac->registers + 0x400 / 4 + 2 * (index + 1);
        uint32_t flags = desc[0];
        if (flags & 0x8000)
          break;
        unsigned length = flags >> 16;
        if (length >= 18 && length <= FRAME_MAX) {
          /* OpenCores includes the four-byte Ethernet FCS in length. */
          void * copy = malloc(length - 4);
          if (copy) {
            memcpy(copy, mac->rx[index], length - 4);
            esp_netif_receive(mac->base.netif, copy, length - 4, NULL);
          }
        }
        desc[0] = 0x8000 | (index == RX_COUNT - 1 ? 0x2000 : 0);
        mac->rx_index = (index + 1) % RX_COUNT;
      }
      mac->registers[1] = 0xffffffff;
    }
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

void bridge_network_start(void) {
  for (unsigned n = 0; n < 2; ++n) {
    bool lan = n == 1;
    virtual_mac_t * mac = &interfaces[n];
    mac->base.post_attach = attach;
    mac->registers = (volatile uint32_t *)(0x600cd000 + n * 0x1000);
    assert(mac->registers[0] == 0xa000);
    mac->registers[0x20 / 4] = 1;
    mac->registers[0x404 / 4] = (uint32_t)mac->tx;
    mac->registers[0x400 / 4] = 0x2800;
    for (unsigned i = 0; i < RX_COUNT; ++i) {
      unsigned d = 0x400 / 4 + 2 * (i + 1);
      mac->registers[d + 1] = (uint32_t)mac->rx[i];
      mac->registers[d] = 0x8000 | (i == RX_COUNT - 1 ? 0x2000 : 0);
    }
    unsigned char address[6] = {2, 0x50, 0, 0, 0, n + 1};
    mac->registers[0x40 / 4] = n + 1;
    mac->registers[0x44 / 4] = 0x0250;
    mac->registers[0] = 0xa003;
    esp_netif_ip_info_t ip = {0};
    if (lan) {
      IP4_ADDR(&ip.ip, 192, 168, 50, 1);
      IP4_ADDR(&ip.gw, 192, 168, 50, 1);
      IP4_ADDR(&ip.netmask, 255, 255, 255, 0);
    }
    esp_netif_inherent_config_t base = ESP_NETIF_INHERENT_DEFAULT_ETH();
    base.if_key = lan ? "BRIDGE_LAN" : "BRIDGE_WAN";
    base.if_desc = lan ? "lan" : "wan";
    base.route_prio = lan ? 10 : 100;
    if (lan) {
      base.flags = ESP_NETIF_DHCP_SERVER | ESP_NETIF_FLAG_AUTOUP;
      base.ip_info = &ip;
    }
    esp_netif_config_t config = {.base = &base, .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};
    esp_netif_t * netif = esp_netif_new(&config);
    assert(netif);
    ESP_ERROR_CHECK(esp_netif_attach(netif, mac));
    ESP_ERROR_CHECK(esp_netif_set_mac(netif, address));
    esp_netif_action_start(netif, NULL, 0, NULL);
    esp_netif_action_connected(netif, NULL, 0, NULL);
    if (lan)
      bridge_lan = netif;
    else
      bridge_wan = netif;
  }
  esp_netif_dns_info_t dns = {.ip.type = ESP_IPADDR_TYPE_V4};
  IP4_ADDR(&dns.ip.u_addr.ip4, 10, 0, 2, 3);
  ESP_ERROR_CHECK(esp_netif_set_dns_info(bridge_lan, ESP_NETIF_DNS_MAIN, &dns));
  ESP_ERROR_CHECK(esp_netif_napt_enable(bridge_lan));
  if (xTaskCreate(poll_rx, "ethernet", 3072, NULL, 8, NULL) != pdPASS)
    abort();
  ESP_LOGI("bridge", "QEMU dual Ethernet; guest lwIP NAPT enabled on LAN");
}
