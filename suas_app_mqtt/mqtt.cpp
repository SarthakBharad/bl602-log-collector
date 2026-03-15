extern "C" {
#include <bl_sec.h>
#include <bl_wifi.h>
#include <lwip/apps/mqtt.h>
#include <lwip/apps/mqtt_opts.h>
#include <lwip/apps/mqtt_priv.h>
#include <lwip/ip4_addr.h>
#include <lwip/ip_addr.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include "log.h" 
}
#include <etl/string.h>
#include <etl/to_string.h>
#include "mqtt.h"

#if ENABLE_MQTTS == TRUE
extern "C" {
#include <lwip/altcp_tls.h>
}
#include "keys.hpp"
#endif

void my_mqtt_incoming_topic_cb(void *arg, const char *topic, u32_t total_len);
void my_mqtt_incoming_payload_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
void my_mqtt_sub_request_cb(void *arg, err_t result);

extern "C" void flush_logs_to_mqtt();
extern "C" void check_memory_status();

mqtt_client_t mqttClient;
ip_addr_t mqttBrokerIp;
static uint8_t topicNr;
auto mqttConnected = false;
auto subscribedTopic = etl::string_view("test");

extern "C" const char* get_device_id(void) {
    static char device_id[16] = {0};
    
    if (device_id[0] == '\0') {
        uint8_t mac[6];
        bl_wifi_mac_addr_get(mac);
        
        snprintf(device_id, sizeof(device_id), "BL602_%02X%02X", mac[4], mac[5]);
        
        printf("[MQTT] Device ID: %s\r\n", device_id);
    }
    
    return device_id;
}

void my_mqtt_disconnect() {
  mqtt_unsubscribe(&mqttClient, subscribedTopic.data(), nullptr, 0);
  mqtt_disconnect(&mqttClient);
  mqttConnected = false;
  printf("[MQTT] Disconnected manually\r\n");
}

void my_mqtt_publish_cb([[gnu::unused]] void *arg, err_t result) {
  if (result == ERR_OK) {
    printf("[MQTT_PUB_CB] OK\r\n");
  } else if (result == ERR_MEM) {
    printf("[MQTT_PUB_CB] FAILED: OUT OF MEMORY (ERR_MEM)\r\n");
    check_memory_status();
  } else if (result == ERR_CONN) {
    printf("[MQTT_PUB_CB] FAILED: Connection error (ERR_CONN)\r\n");
    mqttConnected = false;
  } else {
    printf("[MQTT_PUB_CB] FAILED: Error %d\r\n", result);
  }
}

void my_mqtt_connected_cb([[gnu::unused]] mqtt_client_t *client, void *arg,
                          mqtt_connection_status_t status) {
  if (status == MQTT_CONNECT_ACCEPTED) {
    printf("[MQTT] Connected!\r\n");
    mqttConnected = true;
    
    mqtt_set_inpub_callback(&mqttClient, my_mqtt_incoming_topic_cb,
                            my_mqtt_incoming_payload_cb, arg);

    mqtt_subscribe(&mqttClient, subscribedTopic.data(),
                             QUALITY_OF_SERVICE, my_mqtt_sub_request_cb, arg);
    
    printf("[MQTT] Waiting for connection to stabilize...\r\n");
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    check_memory_status();
    
    printf("[MQTT] Starting batched log flush...\r\n");
    flush_logs_to_mqtt(); 

  } else {
     printf("[MQTT] Disconnected/Failed: %d\r\n", status);
     mqttConnected = false;
     if (status != MQTT_CONNECT_ACCEPTED) {
         printf("[MQTT] Attempting reconnect in 5 seconds...\r\n");
         vTaskDelay(pdMS_TO_TICKS(5000));
         my_mqtt_connect(); 
     }
  }
}

void my_mqtt_incoming_topic_cb([[gnu::unused]] void *arg, const char *topic,
                               u32_t total_len) {
  auto messageTopic = etl::string_view(topic);
  
  printf("[MQTT] Msg on topic: %s (len: %u)\r\n", 
         messageTopic.data(), (unsigned int)total_len);

  if (messageTopic == subscribedTopic) {
    topicNr = 0;
  } else {
    topicNr = 1;
  }
}

void my_mqtt_incoming_payload_cb([[gnu::unused]] void *arg, const u8_t *data,
                                 u16_t len, u8_t flags) {
  (void)data; 
  (void)flags;
  printf("[MQTT] Payload len: %d\r\n", len);
}

void my_mqtt_sub_request_cb([[gnu::unused]] void *arg, err_t result) {
  if (result == ERR_OK) {
    printf("[MQTT] Subscribed successfully\r\n");
  } else {
    printf("[MQTT] Subscribe failed: %d\r\n", result);
  }
}

void my_mqtt_connect() {
  struct mqtt_connect_client_info_t client_info;
  
  IP_ADDR4(&mqttBrokerIp, 10, 25, 13, 186);

  auto randomId = bl_rand();
  auto clientId = etl::string<16>();
  clientId.resize(16);
  snprintf(clientId.data(), clientId.size() + 1, "%02x", randomId);
  
  printf("[MQTT] Connecting with Client ID: %s\r\n", clientId.data());
  check_memory_status();

  memset(&client_info, 0, sizeof(client_info));
  client_info.client_id = clientId.data();
  client_info.client_user = MQTT_USER;
  client_info.client_pass = MQTT_PW;
  client_info.keep_alive = 60;

#if ENABLE_MQTTS == TRUE
  client_info.tls_config = altcp_tls_create_config_client_2wayauth(
      CA_CERT.data(), CA_CERT_LEN, PRIV_KEY.data(), PRIV_KEY_LEN, nullptr, 0,
      CERT.data(), CERT_LEN);
  mqtt_client_connect(&mqttClient, &mqttBrokerIp, MQTT_TLS_PORT,
                                  my_mqtt_connected_cb, 0, &client_info);
#else
  mqtt_client_connect(&mqttClient, &mqttBrokerIp, MQTT_PORT,
                                  my_mqtt_connected_cb, 0, &client_info);
#endif
} 

void my_mqtt_publish() {
   if(mqttConnected) {
   }
}

extern "C" int is_mqtt_connected() {
  if (!mqttConnected) return 0;
  if (mqtt_client_is_connected(&mqttClient) == 0) {
    mqttConnected = false;
    return 0;
  }
  return 1;
}

extern "C" void mqtt_send_text(const char* text) {
  if (!mqttConnected) {
    printf("[MQTT_SEND] Not connected (flag)\r\n");
    return;
  }
  
  if (mqtt_client_is_connected(&mqttClient) == 0) {
    printf("[MQTT_SEND] Not connected (client)\r\n");
    mqttConnected = false;
    return;
  }

  char topic[32];
  snprintf(topic, sizeof(topic), "%s%s", MQTT_TOPIC_PREFIX, get_device_id());

  int msg_len = strlen(text);
  printf("[MQTT_SEND] Sending %d bytes to %s\r\n", msg_len, topic);
  
  err_t err = mqtt_publish(&mqttClient, topic, text,
                           msg_len, 0, 0,
                           my_mqtt_publish_cb, 0);
  
  if (err != ERR_OK) {
    if (err == ERR_MEM) {
      printf("[MQTT_SEND] OUT OF MEMORY! Need to wait longer between messages\r\n");
      check_memory_status();
    } else if (err == ERR_CONN) {
      printf("[MQTT_SEND] Connection error (ERR_CONN)\r\n");
      mqttConnected = false;
      printf("[MQTT_SEND] Attempting reconnect...\r\n");
      vTaskDelay(pdMS_TO_TICKS(2000));
      my_mqtt_connect();
    } else if (err == ERR_TIMEOUT) {
      printf("[MQTT_SEND] Timeout error\r\n");
    } else {
      printf("[MQTT_SEND] publish() failed: %d\r\n", err);
    }
  }
}