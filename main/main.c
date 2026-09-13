#include <stdio.h>

#include "esp_log.h"
#include "uart_conf.h"
#include "modbus_conf.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MODBUS_TASK_STACK_DEPTH_BYTES (3072)
// Range 0-25 where 0 is lowest
#define MODBUS_TASK_PRIORITY (12)

// Це звичайна таска не idle
void app_main(void){
    // Setup UART
    uart_conf_init();

    // Custom UART log
    uart_log_init();

    // Setup Modbus RTU
    modbus_conf_init();

    // Create the modbus task
    if ( xTaskCreate(vTask_Modbus_Start_Communication,
                    "modbus_communication_task",
                     MODBUS_TASK_STACK_DEPTH_BYTES,
                     (void*)1,
                     MODBUS_TASK_PRIORITY,
                     NULL) != pdPASS ) {
        uart_log("err");
    }
    
}
