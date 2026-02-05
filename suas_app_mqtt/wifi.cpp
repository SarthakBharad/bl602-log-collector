extern "C" {
#include <aos/yloop.h>
#include <bl_wifi.h>
#include <blog.h>
#include <event_device.h>
#include <hal_board.h>
#include <hal_button.h>
#include <hal_gpio.h>
#include <hal_sys.h>
#include <hal_uart.h>
#include <hal_wifi.h>
#include <libfdt.h>
#include <looprt.h>
#include <loopset.h>
#include <stdio.h>
#include "log.h"
#include <vfs.h>
#include <wifi_mgmr_ext.h>
}
#include <etl/array.h>
#include <etl/string.h>

#include "mqtt.h"
#include "wifi.h"

/* WiFi configuration */
constinit static wifi_conf_t conf{
    .country_code = "EU",
    .channel_nums = {},
};

/* Helper function to read device tree */
static int get_dts_addr(etl::string_view name, uint32_t &start, uint32_t &off) {
  /* Check we get valid data*/
  if (name.empty()) {
    return -1;
  }

  /* Compute device tree data */
  auto fdt = reinterpret_cast<const void *>(hal_board_get_factory_addr());
  auto offset = fdt_subnode_offset(fdt, 0, name.data());

  /* Check if offset is valid*/
  if (offset <= 0) {
    log_error("%s is invalid\r\n", name);
    return -1;
  }

  /* Return final data */
  start = reinterpret_cast<uint32_t>(fdt);
  off = offset;
  return 0;
}

// Helper function to ensure the wifi manager
// is started only once after initialization
static void _configure_wifi(void) {
  static auto is_initialized = false;
  if (!is_initialized) {
    // Start wifi manager
    smart_log("[WIFI] Initialized\r\n");
    wifi_mgmr_start_background(&conf);
    smart_log("[WIFI] MGMR now running in background\r\n");
    is_initialized = true;
  }
}

/* Connect to a WiFi access point */
static void _connect_sta_wifi() {
  // Enable station mode
  auto wifi_interface = wifi_mgmr_sta_enable();

  // Connect to access point
  wifi_mgmr_sta_connect(&wifi_interface, etl::string<32>(WIFI_SSID).data(),
                        etl::string<32>(WIFI_PW).data(), nullptr, 0, 0, 0);
  smart_log("[WIFI] Connected to a network\r\n");

  /* Enable automatic reconnect */
  wifi_mgmr_sta_autoconnect_enable();
}

/* React to WiFi events */
static void event_cb_wifi_event(input_event_t *event,
                                [[gnu::unused]] void *private_data) {
  switch (event->code) {
    // --- INITIALIZATION ---
    case CODE_WIFI_ON_INIT_DONE:
      _configure_wifi();
      break;
    case CODE_WIFI_ON_MGMR_DONE:
      smart_log("[WIFI] MGMR done\r\n");
      smart_log("[WIFI] Connecting to a network\r\n");
      _connect_sta_wifi();
      break;

    // --- CONNECTION LIFECYCLE ---
    case CODE_WIFI_ON_CONNECTING:
      smart_log("[WIFI] Connecting to a WiFi network\r\n");
      break;
    case CODE_WIFI_ON_CONNECTED:
      smart_log("[WIFI] Connected to a network\r\n");
      break;
    case CODE_WIFI_ON_PRE_GOT_IP:
      smart_log("[WIFI] About to receive IP address\r\n");
      break;
    /*case CODE_WIFI_ON_GOT_IP:
      smart_log("[WIFI] Received an IP address\r\n");
      // Initialize MQTT connection
      vTaskDelay(pdMS_TO_TICKS(80));
      my_mqtt_connect();
      break;*/
    case CODE_WIFI_ON_GOT_IP: {
      uint32_t ip, gateway, mask;
      wifi_mgmr_sta_ip_get(&ip, &gateway, &mask);
      smart_log("[WIFI] Received IP: %d.%d.%d.%d\r\n",
              (ip >> 0) & 0xFF,
              (ip >> 8) & 0xFF,
              (ip >> 16) & 0xFF,
              (ip >> 24) & 0xFF);
      vTaskDelay(pdMS_TO_TICKS(50));
      my_mqtt_connect();
      break;
      }
     
    case CODE_WIFI_ON_DISCONNECT:
      smart_log("[WIFI] Disconnected from network\r\n");
      break;
    case CODE_WIFI_CMD_RECONNECT:
      smart_log("[WIFI] Reconnecting...\r\n");
      break;

    // --- SCAN ---
    case CODE_WIFI_ON_SCAN_DONE:
      smart_log("[WIFI] Scan done\r\n");
      wifi_mgmr_cli_scanlist();
      break;

    // --- ERROR ---
    case CODE_WIFI_ON_EMERGENCY_MAC:
      smart_log("[WIFI] Emergency MAC - rebooting\r\n");
      hal_reboot();
      break;

    // --- IGNORED EVENTS (provisioning, AP mode, etc.) ---
    case CODE_WIFI_ON_PROV_CONNECT:
    case CODE_WIFI_ON_PROV_SSID:
    case CODE_WIFI_ON_PROV_BSSID:
    case CODE_WIFI_ON_PROV_PASSWD:
    case CODE_WIFI_ON_PROV_DISCONNECT:
    case CODE_WIFI_ON_SCAN_DONE_ONJOIN:
    case CODE_WIFI_ON_MGMR_DENOISE:
    case CODE_WIFI_ON_AP_STA_ADD:
    case CODE_WIFI_ON_AP_STA_DEL:
      // Not used in station mode with hardcoded credentials
      break;

    default:
      smart_log("[WIFI] Unknown event: %d\r\n", event->code);
      break;
  }
}

/* Listener for key events */
void event_cb_key_event(input_event_t *event,
                        [[gnu::unused]] void *private_data) {
  if (event->code == KEY_3) {
    // Disconnect on long button press
    smart_log("[MQTT] Disconnecting\r\n");
    my_mqtt_disconnect();
  } else {
    // Publish MQTT message
    smart_log("[MQTT] Publishing message\r\n");
    my_mqtt_publish();
  }
}

/* WiFi task */
void task_wifi([[gnu::unused]] void *pvParameters) {
  // Setup looprt task
  uint32_t fdt = 0, offset = 0;  // these must be uint32_t
  constexpr uint16_t LOOPRT_STACK_SIZE = 512;
  constinit static StackType_t task_looprt_stack[LOOPRT_STACK_SIZE]{};
  constinit static StaticTask_t task_looprt_task{};

  /* Init looprt */
  looprt_start(task_looprt_stack, LOOPRT_STACK_SIZE, &task_looprt_task);
  loopset_led_hook_on_looprt();

  /* Setup virtual file system */
  vfs_init();
  vfs_device_init();

  /* Setup UART */
  if (get_dts_addr(etl::string_view("uart"), fdt, offset) == 0) {
    vfs_uart_init(fdt, offset);
  }

  /* Setup GPIO */
  if (get_dts_addr(etl::string_view("gpio"), fdt, offset) == 0) {
    fdt_button_module_init(reinterpret_cast<const void *>(fdt),
                           static_cast<int>(offset));
  }

  /* Initialize command line */
#ifdef CONF_USER_ENABLE_VFS_ROMFS
  romfs_register();
#endif

  /* Initialize aos loop */
  aos_loop_init();

  /* Register event filter for key events */
  aos_register_event_filter(EV_KEY, event_cb_key_event, nullptr);

  /* Register event filter for WiFi events*/
  aos_register_event_filter(EV_WIFI, event_cb_wifi_event, nullptr);

  /* Start wifi firmware */
  smart_log("[WIFI] Starting WiFi stack\r\n");
  hal_wifi_start_firmware_task();
  aos_post_event(EV_WIFI, CODE_WIFI_ON_INIT_DONE, 0);
  smart_log("[WIFI] Firmware loaded\r\n");

  /*  Start aos loop */
  aos_loop_run();

  /* Will hopefully never happen */
  smart_log("[WIFI] Exiting WiFi task - should never happen\r\n");
  vTaskDelete(nullptr);
}
