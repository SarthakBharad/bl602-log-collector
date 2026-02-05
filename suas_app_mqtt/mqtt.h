/*#ifndef __MQTT_H
#define __MQTT_H

// Credentials
// Do not hardcode credentials in production code
#define MQTT_USER "suas"
#define MQTT_PW "J4auBDJYzcrL8s9TEZJt"

// Security
#define FALSE 0
#define TRUE 1
#define ENABLE_MQTTS TRUE

// Quality of Service for messages
#define QUALITY_OF_SERVICE 1

// --- Function Prototypes ---

// These are regular C++ functions (internal use)
void my_mqtt_connect();
void my_mqtt_disconnect();
void my_mqtt_publish();

// These functions are defined with extern "C" in mqtt.cpp, 
// so they must be declared inside extern "C" here too.
#ifdef __cplusplus
extern "C" {
#endif

int is_mqtt_connected();
void mqtt_send_text(const char* text); // Added this so other files can use it

#ifdef __cplusplus
}
#endif

#endif*/ /*__MQTT_H */


//Multidevice support code:::

#ifndef __MQTT_H
#define __MQTT_H

// Credentials
// Do not hardcode credentials in production code
#define MQTT_USER "suas"
#define MQTT_PW "J4auBDJYzcrL8s9TEZJt"

// Security
#define FALSE 0
#define TRUE 1
#define ENABLE_MQTTS TRUE

// Quality of Service for messages
#define QUALITY_OF_SERVICE 1

// MQTT Topic prefix for multi-device support
// Each device will publish to: logs/<DEVICE_ID>
// Example: logs/BL602_6C21
#define MQTT_TOPIC_PREFIX "logs/"

// --- Function Prototypes ---

// These are regular C++ functions (internal use)
void my_mqtt_connect();
void my_mqtt_disconnect();
void my_mqtt_publish();

// These functions are defined with extern "C" in mqtt.cpp, 
// so they must be declared inside extern "C" here too.
#ifdef __cplusplus
extern "C" {
#endif

int is_mqtt_connected();
void mqtt_send_text(const char* text);

// NEW: Get device ID based on MAC address
// Returns string like "BL602_6C21"
const char* get_device_id(void);

#ifdef __cplusplus
}
#endif

#endif /*__MQTT_H */
