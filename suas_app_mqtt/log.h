#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C" {
#endif

void smart_log(const char* fmt, ...);
void flush_logs_to_mqtt(void);
void set_mqtt_ready(int ready);
int is_mqtt_ready(void);
void check_memory_status(void);

#ifdef __cplusplus
}
#endif

#endif