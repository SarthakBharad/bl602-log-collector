#ifndef __MQTT_H
#define __MQTT_H

#define MQTT_USER "suas"
#define MQTT_PW "J4auBDJYzcrL8s9TEZJt"

#define FALSE 0
#define TRUE 1
#define ENABLE_MQTTS TRUE

#define QUALITY_OF_SERVICE 1

#define MQTT_TOPIC_PREFIX "logs/"

void my_mqtt_connect();
void my_mqtt_disconnect();
void my_mqtt_publish();

#ifdef __cplusplus
extern "C" {
#endif

int is_mqtt_connected();
void mqtt_send_text(const char* text);
const char* get_device_id(void);

#ifdef __cplusplus
}
#endif

#endif