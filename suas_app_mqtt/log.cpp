#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include "log.h"
#include "mqtt.h" 

extern "C" void mqtt_send_text(const char* text);
extern "C" int is_mqtt_connected();
extern "C" const char* get_device_id(void);

#define MAX_BATCH_SIZE 660            
#define LINES_PER_BATCH 5             
#define BATCH_DELAY_MS 1650           
#define MAX_LOGS_TO_KEEP 31           

static char log_buffer[4096];
static int buffer_index = 0;
static int mqtt_fully_ready = 0;
static int log_count = 0;

extern "C" void flush_logs_to_mqtt();
extern "C" void set_mqtt_ready(int ready);
extern "C" void check_memory_status();
extern "C" int is_mqtt_ready();

extern "C" void set_mqtt_ready(int ready) {
    mqtt_fully_ready = ready;
    if (ready) {
        printf("[LOG] MQTT ready, starting flush\r\n");
    }
}

extern "C" int is_mqtt_ready() {
    return mqtt_fully_ready;
}

extern "C" void check_memory_status() {
    size_t free_heap = xPortGetFreeHeapSize();
    printf("[MEMORY] Free heap: %u bytes, Buffer: %d/%d bytes, Logs: %d\r\n", 
           (unsigned int)free_heap, buffer_index, (int)sizeof(log_buffer), log_count);
}

extern "C" void smart_log(const char* fmt, ...) {
    va_list args;
    char raw_msg[256];      
    char log_line[280];     

    va_start(args, fmt);
    vsnprintf(raw_msg, sizeof(raw_msg), fmt, args);
    va_end(args);

    printf("%s", raw_msg);

    int raw_len = strlen(raw_msg);
    while (raw_len > 0 && (raw_msg[raw_len-1] == '\n' || raw_msg[raw_len-1] == '\r')) {
        raw_msg[raw_len-1] = '\0';
        raw_len--;
    }
    
    if (raw_len == 0) return;

    snprintf(log_line, sizeof(log_line), "%s|%lu|%s\n", 
             get_device_id(),
             (unsigned long)xTaskGetTickCount(),
             raw_msg);

    int len = strlen(log_line);
    
    if (buffer_index + len >= (int)sizeof(log_buffer) - 100) {
        printf("[LOG] Buffer nearly full (%d bytes), keeping recent logs\r\n", buffer_index);
        
        int lines_to_keep = MAX_LOGS_TO_KEEP;
        int keep_from = buffer_index;
        
        for (int i = buffer_index - 1; i >= 0 && lines_to_keep > 0; i--) {
            if (log_buffer[i] == '\n') {
                lines_to_keep--;
                if (lines_to_keep == 0) {
                    keep_from = i + 1;
                    break;
                }
            }
        }
        
        if (keep_from > 0 && keep_from < buffer_index) {
            int kept_size = buffer_index - keep_from;
            memmove(log_buffer, &log_buffer[keep_from], kept_size);
            buffer_index = kept_size;
            log_count = MAX_LOGS_TO_KEEP;
            printf("[LOG] Kept last %d logs (%d bytes)\r\n", MAX_LOGS_TO_KEEP, buffer_index);
        } else {
            memset(log_buffer, 0, sizeof(log_buffer));
            buffer_index = 0;
            log_count = 0;
            printf("[LOG] Buffer cleared\r\n");
        }
    }
    
    if (buffer_index + len < (int)sizeof(log_buffer) - 1) {
        strcpy(&log_buffer[buffer_index], log_line);
        buffer_index += len;
        log_count++;
    } else {
        printf("[LOG] No space, dropping message\r\n");
    }
}

extern "C" void flush_logs_to_mqtt() {
    if (buffer_index == 0) {
        printf("[FLUSH] Buffer empty\r\n");
        return;
    }
    
    if (is_mqtt_connected() != 1) {
        printf("[FLUSH] MQTT not connected, keeping logs in buffer\r\n");
        return;
    }

    printf("[FLUSH] Starting flush of %d bytes (%d logs)\r\n", buffer_index, log_count);
    check_memory_status();
    
    char batch_buf[MAX_BATCH_SIZE];
    int batch_len = 0;
    int read_offset = 0;
    int total_sent = 0;
    int lines_in_batch = 0;
    int batch_count = 0;

    while (read_offset < buffer_index) {
        if (is_mqtt_connected() != 1) {
            printf("[FLUSH] Connection lost after %d messages\r\n", total_sent);
            int remaining = buffer_index - read_offset;
            if (read_offset > 0 && remaining > 0) {
                memmove(log_buffer, &log_buffer[read_offset], remaining);
                buffer_index = remaining;
                log_count = 0;
            }
            return;
        }

        vTaskDelay(pdMS_TO_TICKS(50));

        int line_start = read_offset;
        int line_end = -1;
        
        for (int i = read_offset; i < buffer_index; i++) {
            if (log_buffer[i] == '\n') {
                line_end = i;
                break;
            }
        }

        if (line_end == -1) {
            printf("[FLUSH] No complete line at offset %d\r\n", read_offset);
            int remaining = buffer_index - read_offset;
            if (read_offset > 0 && remaining > 0) {
                memmove(log_buffer, &log_buffer[read_offset], remaining);
                buffer_index = remaining;
            }
            break;
        }

        int line_len = line_end - line_start + 1;
        
        if (batch_len + line_len >= MAX_BATCH_SIZE) {
            if (batch_len > 0) {
                batch_buf[batch_len] = '\0';
                batch_count++;
                printf("[FLUSH] Batch #%d (%d bytes, %d lines)\r\n", 
                       batch_count, batch_len, lines_in_batch);
                
                mqtt_send_text(batch_buf);
                total_sent += lines_in_batch;
                
                vTaskDelay(pdMS_TO_TICKS(BATCH_DELAY_MS));
                check_memory_status();
                
                batch_len = 0;
                lines_in_batch = 0;
            }
        }

        if (line_len > 0 && line_len < MAX_BATCH_SIZE) {
            memcpy(&batch_buf[batch_len], &log_buffer[line_start], line_len);
            batch_len += line_len;
            lines_in_batch++;
            read_offset = line_end + 1;

            if (lines_in_batch >= LINES_PER_BATCH) {
                batch_buf[batch_len] = '\0';
                batch_count++;
                printf("[FLUSH] Batch #%d (%d bytes, %d lines)\r\n", 
                       batch_count, batch_len, lines_in_batch);
                
                mqtt_send_text(batch_buf);
                total_sent += lines_in_batch;
                
                vTaskDelay(pdMS_TO_TICKS(BATCH_DELAY_MS));
                check_memory_status();
                
                batch_len = 0;
                lines_in_batch = 0;
            }
        } else {
            printf("[FLUSH] Skipping oversized line (%d bytes)\r\n", line_len);
            read_offset = line_end + 1;
        }
    }

    if (batch_len > 0 && lines_in_batch > 0) {
        batch_buf[batch_len] = '\0';
        batch_count++;
        printf("[FLUSH] Final batch #%d (%d bytes, %d lines)\r\n", 
               batch_count, batch_len, lines_in_batch);
        mqtt_send_text(batch_buf);
        total_sent += lines_in_batch;
    }

    printf("[FLUSH] Complete: %d messages in %d batches\r\n", total_sent, batch_count);
    memset(log_buffer, 0, sizeof(log_buffer));
    buffer_index = 0;
    log_count = 0;
    
    set_mqtt_ready(1);
    check_memory_status();
}