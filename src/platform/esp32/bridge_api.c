#include "bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "bridge_prompt.h"

bool bridge_api_solve(const char * question, char * reply, size_t capacity) {
  bool success = false;
  const char * error = "AI API request failed";
  bridge_config_t * config = malloc(sizeof(*config));
  char *raw = malloc(16385), *payload = NULL;
  cJSON *body = NULL, *response = NULL;
  esp_http_client_handle_t client = NULL;
  if (!config || !raw) {
    error = "AI host out of memory";
    goto finish;
  }
  bridge_config_get(config);
  if (!config->api_url[0] || !config->model[0] || !config->api_key[0]) {
    error = "Configure API URL, model and key on the bridge";
    goto finish;
  }
  char url[288], authorization[522];
  snprintf(url, sizeof(url), "%s/chat/completions", config->api_url);
  snprintf(authorization, sizeof(authorization), "Bearer %s", config->api_key);
  body = cJSON_CreateObject();
  if (!body)
    goto finish;
  cJSON_AddStringToObject(body, "model", config->model);
  cJSON_AddBoolToObject(body, "stream", false);
  cJSON * messages = cJSON_AddArrayToObject(body, "messages");
  if (!messages)
    goto finish;
  const char * roles[] = {"system", "user"};
  const char * contents[] = {bridge_prompt, question};
  for (unsigned i = 0; i < 2; ++i) {
    cJSON * message = cJSON_CreateObject();
    if (!message)
      goto finish;
    cJSON_AddItemToArray(messages, message);
    if (!cJSON_AddStringToObject(message, "role", roles[i]) ||
        !cJSON_AddStringToObject(message, "content", contents[i]))
      goto finish;
  }
  payload = cJSON_PrintUnformatted(body);
  if (!payload)
    goto finish;
  esp_http_client_config_t options = {
      .url = url,
      .method = HTTP_METHOD_POST,
      .timeout_ms = 45000,
      .disable_auto_redirect = true,
      .buffer_size = 2048,
      .cert_pem = config->api_ca[0] ? config->api_ca : NULL,
      .crt_bundle_attach = config->api_ca[0] ? NULL : esp_crt_bundle_attach,
  };
  client = esp_http_client_init(&options);
  if (!client)
    goto finish;
  if (esp_http_client_set_header(client, "Authorization", authorization) != ESP_OK ||
      esp_http_client_set_header(client, "Content-Type", "application/json") != ESP_OK ||
      esp_http_client_open(client, strlen(payload)) != ESP_OK)
    goto finish;
  size_t sent = 0, length = strlen(payload);
  while (sent < length) {
    int n = esp_http_client_write(client, payload + sent, length - sent);
    if (n <= 0)
      goto finish;
    sent += n;
  }
  if (esp_http_client_fetch_headers(client) < 0)
    goto finish;
  int status = esp_http_client_get_status_code(client);
  if (status != 200) {
    snprintf(reply, capacity, "AI API HTTP %d", status);
    error = NULL;
    goto finish;
  }
  size_t used = 0;
  while (used < 16384) {
    int n = esp_http_client_read(client, raw + used, 16384 - used);
    if (n < 0)
      goto finish;
    if (!n)
      break;
    used += n;
  }
  if (used == 16384 || !esp_http_client_is_complete_data_received(client)) {
    error = "AI API response is too large or incomplete";
    goto finish;
  }
  raw[used] = 0;
  response = cJSON_ParseWithLength(raw, used + 1);
  cJSON * choices = cJSON_GetObjectItemCaseSensitive(response, "choices");
  cJSON * choice = cJSON_GetArrayItem(choices, 0);
  cJSON * reason = cJSON_GetObjectItemCaseSensitive(choice, "finish_reason");
  cJSON * message = cJSON_GetObjectItemCaseSensitive(choice, "message");
  cJSON * content = cJSON_GetObjectItemCaseSensitive(message, "content");
  error = "AI API returned invalid or truncated text";
  if (!cJSON_IsString(reason) || strcmp(reason->valuestring, "stop") || !cJSON_IsString(content))
    goto finish;
  length = strlen(content->valuestring);
  if (!length || length >= capacity || length > BRIDGE_REPLY_MAX)
    goto finish;
  bool printable = false;
  for (size_t i = 0; i < length; ++i) {
    unsigned char c = content->valuestring[i];
    if ((c < 32 && c != 9 && c != 10 && c != 13) || c > 126)
      goto finish;
    if (c > 32)
      printable = true;
  }
  if (!printable)
    goto finish;
  memcpy(reply, content->valuestring, length + 1);
  success = true;
finish:
  if (!success && error)
    strlcpy(reply, error, capacity);
  if (client)
    esp_http_client_cleanup(client);
  cJSON_Delete(body);
  cJSON_Delete(response);
  free(payload);
  free(raw);
  if (config) {
    memset(config, 0, sizeof(*config));
    free(config);
  }
  return success;
}
