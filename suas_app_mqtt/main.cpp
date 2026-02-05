extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard input/output
#include <stdio.h>
#include "log.h"
// lwip TCP stack
#include <lwip/tcpip.h>
}

#include <etl/string.h>

// Own header
#include "wifi.h"

/* main function, execution starts here */
extern "C" void bfl_main(void) {
  /* Define information containers for tasks */
  constexpr uint16_t WIFI_STACK_SIZE = 1024;
  constinit static StackType_t wifi_stack[WIFI_STACK_SIZE]{};
  constinit static StaticTask_t wifi_task{};

  /* Initialize system */
  vInitializeBL602();

  // === ADD THIS LINE HERE ===
  smart_log("Booting...\r\n");

  /* Start tasks */
  /* WiFi task */
  smart_log("[SYSTEM] Starting WiFi task\r\n");
  xTaskCreateStatic(task_wifi, etl::string_view("wifi").data(), WIFI_STACK_SIZE,
                    nullptr, 16, wifi_stack, &wifi_task);

  /* Start TCP/IP stack */
  smart_log("[SYSTEM] Starting TCP/IP stack\r\n");
  tcpip_init(nullptr, nullptr);

  /* Start scheduler */
  smart_log("[SYSTEM] Starting scheduler\r\n");
  vTaskStartScheduler();
}
