/*#ifndef __LOG_H
#define __LOG_H

#ifdef __cplusplus
extern "C" {
#endif

void smart_log(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // __LOG_H


// working code -- current

#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C" {
#endif

// Main logging function
void smart_log(const char* fmt, ...);

// Flush logs to MQTT (called after connection established)
void flush_logs_to_mqtt(void);

// Set MQTT ready flag
void set_mqtt_ready(int ready);

// Check if MQTT is ready to send
int is_mqtt_ready(void);

// Check and display memory status
void check_memory_status(void);

#ifdef __cplusplus
}
#endif

#endif // LOG_H

*/

/*
 * log.h - Log Collector Header for PineCone BL602
 */



/*#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C" {
#endif

// Main logging function - use like printf
// Example: smart_log("[WIFI] Connected to %s\r\n", ssid);
void smart_log(const char* fmt, ...);

// Flush buffered logs to MQTT (called after connection established)
void flush_logs_to_mqtt(void);

// Set MQTT ready flag (called by mqtt.cpp after connection)
void set_mqtt_ready(int ready);

// Check if MQTT is ready to send
int is_mqtt_ready(void);

// Check and display memory status (for debugging)
void check_memory_status(void);

#ifdef __cplusplus
}
#endif

#endif // LOG_H
*/






//Multidevice support:::::

/*
 * log.h - Log Collector Header for PineCone BL602
 */

#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C" {
#endif

// Main logging function - use like printf
// Example: smart_log("[WIFI] Connected to %s\r\n", ssid);
void smart_log(const char* fmt, ...);

// Flush buffered logs to MQTT (called after connection established)
void flush_logs_to_mqtt(void);

// Set MQTT ready flag (called by mqtt.cpp after connection)
void set_mqtt_ready(int ready);

// Check if MQTT is ready to send
int is_mqtt_ready(void);

// Check and display memory status (for debugging)
void check_memory_status(void);

#ifdef __cplusplus
}
#endif

#endif // LOG_H
