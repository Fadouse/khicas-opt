#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "esp_netif.h"
#include "lwip/ip4_addr.h"

#define BRIDGE_REQUEST_MAX 1024
#define BRIDGE_REPLY_MAX 2048
#define BRIDGE_CA_MAX 4096

typedef struct {
  char ssid[33];
  char wifi_password[65];
  char api_url[257];
  char model[97];
  char api_key[513];
  char api_ca[BRIDGE_CA_MAX];
} bridge_config_t;

extern esp_netif_t * bridge_lan;
extern esp_netif_t * bridge_wan;
void bridge_config_init(void);
void bridge_config_get(bridge_config_t * out);
esp_err_t bridge_config_save(const bridge_config_t * config);
const char * bridge_password(void);
const char * bridge_ap_name(void);
void bridge_network_start(void);
void bridge_web_start(void);
void bridge_usb_start(void);
bool bridge_api_solve(const char * question, char * reply, size_t capacity);
void bridge_tls_identity(char ** certificate, char ** key);
