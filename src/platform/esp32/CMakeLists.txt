set(bridge_sources bridge_main.c bridge_config.c bridge_web.c bridge_api.c bridge_usb.c)
if(CONFIG_BRIDGE_QEMU)
  list(APPEND bridge_sources bridge_qemu_net.c)
else()
  list(APPEND bridge_sources bridge_wifi.c)
endif()
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bridge_generated")
idf_component_register(SRCS ${bridge_sources}
  INCLUDE_DIRS "." "${CMAKE_BINARY_DIR}/bridge_generated"
  REQUIRES esp_wifi esp_netif esp_event esp_http_client esp_https_server nvs_flash
           json mbedtls usb esp_driver_gpio esp_timer bootloader_support)
# Keep one prompt for the Python host and the ESP32 host.
execute_process(COMMAND "${PYTHON}" -c
  "import sys,json,pathlib; sys.path.insert(0,sys.argv[1]); from khicas_ai import PROMPT; p=pathlib.Path(sys.argv[2]); p.parent.mkdir(parents=True,exist_ok=True); p.write_text('static const char bridge_prompt[] = '+json.dumps(PROMPT)+';\\n')"
  "${CMAKE_CURRENT_LIST_DIR}/../../runtime"
  "${CMAKE_BINARY_DIR}/bridge_generated/bridge_prompt.h"
  COMMAND_ERROR_IS_FATAL ANY)
