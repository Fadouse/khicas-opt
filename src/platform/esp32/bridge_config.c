#include "bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_random.h"
#include "esp_mac.h"
#include "bootloader_random.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs.h"
#include "mbedtls/pk.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/sha256.h"

static bridge_config_t current;
static SemaphoreHandle_t lock;
static nvs_handle_t storage;
static char password[25], ap_name[33];

static void check_crypto(int result) {
  if (result) {
    ESP_LOGE("bridge", "TLS identity operation failed: %d", result);
    abort();
  }
}

static int random_bytes(void * unused, unsigned char * out, size_t length) {
  (void)unused;
  esp_fill_random(out, length);
  return 0;
}

void bridge_config_init(void) {
  lock = xSemaphoreCreateMutex();
  assert(lock);
  ESP_ERROR_CHECK(nvs_open("bridge", NVS_READWRITE, &storage));
  size_t length = sizeof(current);
  esp_err_t error = nvs_get_blob(storage, "config1", &current, &length);
  if (error != ESP_ERR_NVS_NOT_FOUND) {
    ESP_ERROR_CHECK(error);
    assert(length == sizeof(current));
  }
  length = sizeof(password);
  error = nvs_get_str(storage, "password", password, &length);
  if (error == ESP_ERR_NVS_NOT_FOUND) {
    unsigned char bytes[12];
    /* Configuration is initialized before the Wi-Fi/ADC subsystems. */
    bootloader_random_enable();
    esp_fill_random(bytes, sizeof(bytes));
    bootloader_random_disable();
    for (size_t i = 0; i < sizeof(bytes); ++i) {
      snprintf(password + i * 2, 3, "%02x", bytes[i]);
    }
    ESP_ERROR_CHECK(nvs_set_str(storage, "password", password));
    ESP_ERROR_CHECK(nvs_commit(storage));
  } else {
    ESP_ERROR_CHECK(error);
  }
  unsigned char mac[6];
  ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac));
  snprintf(ap_name, sizeof(ap_name), "KhiCAS-%02X%02X%02X", mac[3], mac[4], mac[5]);
}

void bridge_config_get(bridge_config_t * out) {
  xSemaphoreTake(lock, portMAX_DELAY);
  *out = current;
  xSemaphoreGive(lock);
}

esp_err_t bridge_config_save(const bridge_config_t * config) {
  xSemaphoreTake(lock, portMAX_DELAY);
  esp_err_t error = nvs_set_blob(storage, "config1", config, sizeof(*config));
  if (error == ESP_OK)
    error = nvs_commit(storage);
  if (error == ESP_OK)
    current = *config;
  xSemaphoreGive(lock);
  return error;
}

const char * bridge_password(void) {
  return password;
}
const char * bridge_ap_name(void) {
  return ap_name;
}

void bridge_tls_identity(char ** certificate, char ** key) {
  *certificate = calloc(1, 2048);
  *key = calloc(1, 2048);
  assert(*certificate && *key);
  size_t csize = 2048, ksize = 2048;
  esp_err_t ce = nvs_get_str(storage, "tls_cert", *certificate, &csize);
  esp_err_t ke = nvs_get_str(storage, "tls_key", *key, &ksize);
  if (ce != ESP_OK || ke != ESP_OK) {
    assert((ce == ESP_OK || ce == ESP_ERR_NVS_NOT_FOUND) &&
           (ke == ESP_OK || ke == ESP_ERR_NVS_NOT_FOUND));
    mbedtls_pk_context pk;
    mbedtls_x509write_cert crt;
    unsigned char serial[16];
    mbedtls_pk_init(&pk);
    mbedtls_x509write_crt_init(&crt);
    check_crypto(mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)));
    check_crypto(
        mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(pk), random_bytes, NULL));
    esp_fill_random(serial, sizeof(serial));
    serial[0] &= 0x7f;
    mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
    mbedtls_x509write_crt_set_subject_key(&crt, &pk);
    mbedtls_x509write_crt_set_issuer_key(&crt, &pk);
    check_crypto(mbedtls_x509write_crt_set_serial_raw(&crt, serial, sizeof(serial)));
    check_crypto(mbedtls_x509write_crt_set_subject_name(&crt, "CN=192.168.50.1,O=KhiCAS"));
    check_crypto(mbedtls_x509write_crt_set_issuer_name(&crt, "CN=192.168.50.1,O=KhiCAS"));
    check_crypto(mbedtls_x509write_crt_set_validity(&crt, "20200101000000", "20491231235959"));
    check_crypto(mbedtls_x509write_crt_set_basic_constraints(&crt, 0, -1));
    /* DER GeneralNames: one IP address, 192.168.50.1. */
    const unsigned char san[] = {0x30, 6, 0x87, 4, 192, 168, 50, 1};
    check_crypto(mbedtls_x509write_crt_set_extension(&crt, "\x55\x1d\x11", 3, 0, san, sizeof(san)));
    check_crypto(
        mbedtls_x509write_crt_pem(&crt, (unsigned char *)*certificate, 2048, random_bytes, NULL));
    check_crypto(mbedtls_pk_write_key_pem(&pk, (unsigned char *)*key, 2048));
    mbedtls_x509write_crt_free(&crt);
    mbedtls_pk_free(&pk);
    ESP_ERROR_CHECK(nvs_set_str(storage, "tls_cert", *certificate));
    ESP_ERROR_CHECK(nvs_set_str(storage, "tls_key", *key));
    ESP_ERROR_CHECK(nvs_commit(storage));
  }
  mbedtls_x509_crt parsed;
  mbedtls_x509_crt_init(&parsed);
  check_crypto(
      mbedtls_x509_crt_parse(&parsed, (unsigned char *)*certificate, strlen(*certificate) + 1));
  unsigned char hash[32];
  char hex[65];
  check_crypto(mbedtls_sha256(parsed.raw.p, parsed.raw.len, hash, 0));
  for (int i = 0; i < 32; ++i)
    snprintf(hex + i * 2, 3, "%02x", hash[i]);
  ESP_LOGI("bridge", "HTTPS certificate SHA256: %s", hex);
  mbedtls_x509_crt_free(&parsed);
}
