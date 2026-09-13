#include "bridge.h"
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "esp_https_server.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "lwip/sockets.h"
#include "mbedtls/base64.h"
#include "mbedtls/x509_crt.h"

static const char page[] =
    "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
    "<title>KhiCAS bridge</title><style>body{font:16px system-ui;background:#eef3f8;color:#19334f;"
    "max-width:620px;margin:5vh "
    "auto;padding:24px}form{background:white;padding:28px;border-radius:16px}"
    "label{display:block;margin-top:18px}input,textarea,button{box-sizing:border-box;width:100%;"
    "padding:12px;"
    "border:1px solid "
    "#b7c6d6;border-radius:6px;font:inherit}button{background:#175dac;color:white;margin-top:24px}"
    "small{color:#536779}#status{white-space:pre-wrap}</style><h1>KhiCAS bridge</h1>"
    "<p>Connect your calculator to your AI service.</p><form id=f>"
    "<label>Upstream Wi-Fi SSID<input name=ssid maxlength=32></label>"
    "<label>Wi-Fi password<input name=wifi_password type=password maxlength=64 "
    "autocomplete=new-password></label>"
    "<label>API base URL<input name=api_url type=url placeholder='https://api.example.com/v1' "
    "maxlength=256 required></label>"
    "<label>Model<input name=model maxlength=96 required></label>"
    "<label>API key<input name=api_key type=password maxlength=512 "
    "autocomplete=new-password></label>"
    "<small>Leave saved passwords blank to keep them.</small>"
    "<details><summary>Custom API certificate authority</summary>"
    "<textarea name=api_ca rows=5 maxlength=4095 placeholder='Optional PEM CA "
    "certificate'></textarea></details>"
    "<button>Save</button><button type=button id=reboot>Restart bridge</button>"
    "<p id=status role=status></p></form>"
    "<script>const f=document.querySelector('form'),s=document.querySelector('#status');"
    "fetch('/config').then(r=>r.json()).then(c=>{for(const k of "
    "['ssid','api_url','model','api_ca'])f.elements[k].value=c[k]||'';"
    "s.textContent=c.has_key?'API key saved.':'Configure your API "
    "key.'}).catch(()=>s.textContent='Unable to load settings.');"
    "f.onsubmit=async e=>{e.preventDefault();try{const r=await "
    "fetch('/config',{method:'POST',headers:{'Content-Type':'application/json',"
    "'X-KhiCAS-Config':'1'},body:JSON.stringify(Object.fromEntries(new "
    "FormData(f)))});s.textContent=await r.text();"
    "if(r.ok){f.elements.api_key.value='';f.elements.wifi_password.value=''}}catch(e){s."
    "textContent='Save failed.'}};"
    "document.querySelector('#reboot').onclick=()=>fetch('/"
    "restart',{method:'POST',headers:{'X-KhiCAS-Config':'1'}})"
    ".then(()=>s.textContent='Restarting. Reconnect to the "
    "AP.').catch(()=>s.textContent='Reconnect to the AP.');</script>";

static bool authorized(httpd_req_t * request) {
  struct sockaddr_storage local;
  socklen_t size = sizeof(local);
  esp_netif_ip_info_t lan;
  uint32_t address = 0;
  if (!getsockname(httpd_req_to_sockfd(request), (struct sockaddr *)&local, &size)) {
    if (local.ss_family == AF_INET)
      address = ((struct sockaddr_in *)&local)->sin_addr.s_addr;
    else if (local.ss_family == AF_INET6) {
      struct sockaddr_in6 * v6 = (struct sockaddr_in6 *)&local;
      if (IN6_IS_ADDR_V4MAPPED(&v6->sin6_addr))
        memcpy(&address, &v6->sin6_addr.s6_addr[12], 4);
    }
  }
  if (!address || esp_netif_get_ip_info(bridge_lan, &lan) != ESP_OK || address != lan.ip.addr) {
    httpd_resp_send_err(request, HTTPD_403_FORBIDDEN, "Management is available on the AP only");
    return false;
  }
  char supplied[128], expected[128] = "Basic ", plain[64];
  size_t used;
  snprintf(plain, sizeof(plain), "admin:%s", bridge_password());
  mbedtls_base64_encode((unsigned char *)expected + 6, sizeof(expected) - 6, &used,
                        (unsigned char *)plain, strlen(plain));
  if (httpd_req_get_hdr_value_str(request, "Authorization", supplied, sizeof(supplied)) != ESP_OK ||
      strcmp(supplied, expected)) {
    httpd_resp_set_status(request, "401 Unauthorized");
    httpd_resp_set_hdr(request, "WWW-Authenticate", "Basic realm=\"KhiCAS bridge\"");
    httpd_resp_sendstr(request,
                       "Use admin and the setup password shown on the local serial console.");
    return false;
  }
  httpd_resp_set_hdr(request, "Cache-Control", "no-store");
  httpd_resp_set_hdr(request, "X-Content-Type-Options", "nosniff");
  httpd_resp_set_hdr(request, "X-Frame-Options", "DENY");
  if (request->method == HTTP_POST &&
      (httpd_req_get_hdr_value_str(request, "X-KhiCAS-Config", supplied, sizeof(supplied)) !=
           ESP_OK ||
       strcmp(supplied, "1"))) {
    httpd_resp_send_err(request, HTTPD_403_FORBIDDEN, "Missing configuration header");
    return false;
  }
  return true;
}

static bool field(cJSON * body, const char * name, char * out, size_t capacity, bool secret) {
  cJSON * value = cJSON_GetObjectItemCaseSensitive(body, name);
  if (!value)
    return true;
  if (!cJSON_IsString(value) || strlen(value->valuestring) >= capacity)
    return false;
  if (secret && !value->valuestring[0])
    return true;
  for (const unsigned char * p = (unsigned char *)value->valuestring; *p; ++p) {
    if (*p < 32 && (strcmp(name, "api_ca") || (*p != 10 && *p != 13)))
      return false;
  }
  strlcpy(out, value->valuestring, capacity);
  return true;
}

static esp_err_t settings(httpd_req_t * request) {
  if (!authorized(request))
    return ESP_OK;
  bridge_config_t * config = malloc(sizeof(*config));
  if (!config)
    return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
  bridge_config_get(config);
  cJSON * body = NULL;
  char * text = NULL;
  if (request->method == HTTP_GET) {
    body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "ssid", config->ssid);
    cJSON_AddStringToObject(body, "api_url", config->api_url);
    cJSON_AddStringToObject(body, "model", config->model);
    cJSON_AddStringToObject(body, "api_ca", config->api_ca);
    cJSON_AddBoolToObject(body, "has_key", config->api_key[0] != 0);
    text = cJSON_PrintUnformatted(body);
    httpd_resp_set_type(request, "application/json");
    if (text)
      httpd_resp_sendstr(request, text);
    else
      httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
    goto done;
  }
  if (request->content_len <= 0 || request->content_len > 7168)
    goto invalid;
  text = malloc(request->content_len + 1);
  if (!text)
    goto invalid;
  size_t got = 0;
  while (got < request->content_len) {
    int n = httpd_req_recv(request, text + got, request->content_len - got);
    if (n <= 0)
      goto invalid;
    got += n;
  }
  text[got] = 0;
  body = cJSON_ParseWithLength(text, got + 1);
  if (!cJSON_IsObject(body) || !field(body, "ssid", config->ssid, sizeof(config->ssid), false) ||
      !field(body, "wifi_password", config->wifi_password, sizeof(config->wifi_password), true) ||
      !field(body, "api_url", config->api_url, sizeof(config->api_url), false) ||
      !field(body, "model", config->model, sizeof(config->model), false) ||
      !field(body, "api_key", config->api_key, sizeof(config->api_key), true) ||
      !field(body, "api_ca", config->api_ca, sizeof(config->api_ca), false))
    goto invalid;
  size_t length = strlen(config->api_url);
  while (length && config->api_url[length - 1] == '/')
    config->api_url[--length] = 0;
  if (strncmp(config->api_url, "https://", 8) || length <= 8 || strpbrk(config->api_url, "@?# ") ||
      !config->model[0])
    goto invalid;
  length = strlen(config->wifi_password);
  if (length && length < 8)
    goto invalid;
  if (config->api_ca[0]) {
    mbedtls_x509_crt ca;
    mbedtls_x509_crt_init(&ca);
    int error =
        mbedtls_x509_crt_parse(&ca, (unsigned char *)config->api_ca, strlen(config->api_ca) + 1);
    mbedtls_x509_crt_free(&ca);
    if (error)
      goto invalid;
  }
  if (bridge_config_save(config) != ESP_OK) {
    httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to save settings");
  } else {
    httpd_resp_sendstr(
        request, "Saved. API settings apply to the next request. Restart to apply Wi-Fi changes.");
  }
  goto done;
invalid:
  httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST,
                      "Invalid settings: use HTTPS and check field lengths");
done:
  cJSON_Delete(body);
  free(text);
  memset(config, 0, sizeof(*config));
  free(config);
  return ESP_OK;
}

static void restart(void * unused) {
  (void)unused;
  esp_restart();
}
static esp_timer_handle_t restart_timer;
static esp_err_t restart_page(httpd_req_t * request) {
  if (!authorized(request))
    return ESP_OK;
  httpd_resp_sendstr(request, "Restarting");
  esp_timer_start_once(restart_timer, 1000000);
  return ESP_OK;
}

static esp_err_t index_page(httpd_req_t * request) {
  if (!authorized(request))
    return ESP_OK;
  httpd_resp_set_type(request, "text/html; charset=utf-8");
  return httpd_resp_send(request, page, sizeof(page) - 1);
}

void bridge_web_start(void) {
  char *certificate, *key;
  bridge_tls_identity(&certificate, &key);
  httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
  config.httpd.stack_size = 10240;
  config.httpd.max_open_sockets = 3;
  config.httpd.lru_purge_enable = true;
  config.servercert = (unsigned char *)certificate;
  config.servercert_len = strlen(certificate) + 1;
  config.prvtkey_pem = (unsigned char *)key;
  config.prvtkey_len = strlen(key) + 1;
  httpd_handle_t server;
  ESP_ERROR_CHECK(httpd_ssl_start(&server, &config));
  const httpd_uri_t routes[] = {
      {.uri = "/", .method = HTTP_GET, .handler = index_page},
      {.uri = "/config", .method = HTTP_GET, .handler = settings},
      {.uri = "/config", .method = HTTP_POST, .handler = settings},
      {.uri = "/restart", .method = HTTP_POST, .handler = restart_page},
  };
  for (unsigned i = 0; i < sizeof(routes) / sizeof(routes[0]); ++i) {
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &routes[i]));
  }
  esp_timer_create_args_t timer = {.callback = restart, .name = "restart"};
  ESP_ERROR_CHECK(esp_timer_create(&timer, &restart_timer));
  /* The HTTPS server owns references to the PEM buffers for its lifetime. */
}
