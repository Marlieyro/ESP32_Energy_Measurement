// ----------------- Custom includes ---------------
#include "esp_log.h"
#include "uart_conf.h"
#include "appModBusConfig/modbus_conf.h"
#include "appWiFiConfig/wifi_conf.h"
#include "appHTTPClient/app_http_client.h"

// ----------------- ESPIDF includes ---------------
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ----------------- DEFINES -----------------------
#define MODBUS_TASK_STACK_DEPTH_BYTES (3072)
#define MODBUS_TASK_PRIORITY (12) // Range 0-25 where 0 is lowest
#define HTTP_POST_TASK_PRIORITY (8)

static const char *TAG = "MAIN_INIT_THREAD";

// Це звичайна таска не idle
void app_main(void){
    // 1. WiFi Initialization
    wifi_init_general();

    // 2. ModBus Init
    modbus_conf_init();

    // 3. MsgBuffer Init
    app_MsgBuffer_create();

    // 4. HTTP Client Init
    app_http_client_init();

    // 5. Start ModBus task
    if (xTaskCreate(vTask_Modbus_Start_Communication, "modbus_communication_task", MODBUS_TASK_STACK_DEPTH_BYTES, (void*)1, MODBUS_TASK_PRIORITY, NULL) != pdPASS) {
        ESP_LOGI(TAG, "Failed to create modbus communication task");
    }

    // 6. Start Test HTTP POST request
    if (xTaskCreate(vTask_HTTP_POST_request, "test_http_post_request", MODBUS_TASK_STACK_DEPTH_BYTES,  (void*) 1, HTTP_POST_TASK_PRIORITY, NULL) != pdPASS) {
        ESP_LOGI(TAG, "xTaskCreate HTTP_POST_request failed");
    }
}
