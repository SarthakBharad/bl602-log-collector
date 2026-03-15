extern "C" {
#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include "log.h"
#include <lwip/tcpip.h>
}

#include <etl/string.h>

#include "wifi.h"

extern "C" void bfl_main(void) {
  constexpr uint16_t WIFI_STACK_SIZE = 1024;
  constinit static StackType_t wifi_stack[WIFI_STACK_SIZE]{};
  constinit static StaticTask_t wifi_task{};

  vInitializeBL602();

  smart_log("Booting...\r\n");

  smart_log("[SYSTEM] Starting WiFi task\r\n");
  xTaskCreateStatic(task_wifi, etl::string_view("wifi").data(), WIFI_STACK_SIZE,
                    nullptr, 16, wifi_stack, &wifi_task);

  smart_log("[SYSTEM] Starting TCP/IP stack\r\n");
  tcpip_init(nullptr, nullptr);

  smart_log("[SYSTEM] Starting scheduler\r\n");
  vTaskStartScheduler();
}