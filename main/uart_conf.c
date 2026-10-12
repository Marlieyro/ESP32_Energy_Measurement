//
// Created by user on 02.09.26.
//

#include "uart_conf.h"
#include "driver/uart.h"
#include "freertos/semphr.h"
#include "hal/uart_ll.h"

/*
 * GPIO_20 - TX
 * GPIO_21 - RX
 */

#define UART_BAUD_RTT           (115200)
#define UART_RX_TX_BUFF_SIZE    (1024 * 2)
#define UART_QUEUE_BUFF_SIZE    (20)
#define UART_GPIO_PIN_TX        (20)
#define UART_GPIO_PIN_RX        (21)

// No events and isr events
// QueueHandle_t uart_queue = NULL;
// Already known at compile time
constexpr uart_port_t uart_numbr = UART_NUM_1; // TODO change PORT
static SemaphoreHandle_t uart_log_semaphore = NULL;

void uart_conf_init(void) {
    uart_config_t uart_config = {0};

    uart_config.baud_rate = UART_BAUD_RTT;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;

    ESP_ERROR_CHECK(uart_param_config(uart_numbr, &uart_config));

    // TODO change pins
    // Set UART pins(TX: IO20, RX: IO21, RTS: UNUSED, CTS: UNUSED, DTR: UNUSED, DSR: UNUSED)
    ESP_ERROR_CHECK(uart_set_pin(uart_numbr, UART_GPIO_PIN_TX, UART_GPIO_PIN_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_ERROR_CHECK(uart_driver_install(uart_numbr, UART_RX_TX_BUFF_SIZE, UART_RX_TX_BUFF_SIZE,
        UART_QUEUE_BUFF_SIZE, NULL, 0));
}

void uart_log_init(void) {
    uart_log_semaphore = xSemaphoreCreateMutex();
}

uint32_t uart_send_data(const char* data) {
    if (data == NULL) return 0;
    return uart_write_bytes(uart_numbr, data, strlen(data));
}

void uart_log(const char* log_data) {
    if (uart_log_semaphore == NULL) return;

    if (xSemaphoreTake(uart_log_semaphore, portMAX_DELAY) == pdTRUE) {
        uart_send_data(log_data);
        xSemaphoreGive(uart_log_semaphore);
    }
}

// TODO xTaskCreate for recive data
