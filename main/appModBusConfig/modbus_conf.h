//
// Created by user on 03.09.26.
//

#ifndef ENERGYMEASURMENT_MODBUS_CONF_H
#define ENERGYMEASURMENT_MODBUS_CONF_H

static void conf_mapping();
void modbus_conf_init();
void modbus_destroy();
void vTask_Modbus_Start_Communication(void* arg);

#endif //ENERGYMEASURMENT_MODBUS_CONF_H
