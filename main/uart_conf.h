//
// Created by user on 02.09.26.
//

#ifndef ENERGYMEASURMENT_UART_CONF_H
#define ENERGYMEASURMENT_UART_CONF_H
#include <stdint.h>

void uart_conf_init(void);
uint32_t uart_send_data(const char* data);
void uart_log_init(void);
void uart_log(const char* log_data);

#endif //ENERGYMEASURMENT_UART_CONF_H
