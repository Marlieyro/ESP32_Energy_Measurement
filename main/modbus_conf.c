//
// Created by user on 03.09.26.
//

#include "esp_modbus_common.h"
#include "esp_modbus_master.h"
#include "modbus_conf.h"

#include "FreeRTOSConfig.h"
#include "portmacro.h"
#include "uart_conf.h"
#include "../../../../../../home/user/esp/esp-idf/components/freertos/FreeRTOS-Kernel/include/freertos/projdefs.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "hal/emac_hal.h"

// CONFIG_FMB_COMM_MODE_RTU_EN

#define MODBUS_BAUD_RTT (115200)
// Modbus slave device adr
#define MODBUS_SLAVE_1 (0) // TODO

constexpr uart_port_t modbus_uart_port_numbr = UART_NUM_2;
static void *master_handler = NULL;

uint8_t type = 0;

typedef enum {
    CID_TEST_COMUN = 0,
    CID_VOLTAGE,
} modbus_cid;

// Modbus 'Register Start' addresses
enum modbus_register_adr_map {
    modbus_test_register = 0x0,
    modbus_voltage_register = 0x1
};

/* ------------------- Modbus action management Queue ---------------------- */

// The que for action choosing write or read
static QueueHandle_t modbus_communication_command;

// Modbus available actions
typedef enum {
    modbus_action_write = 0,
    modbus_action_read = 1,
    modbus_default_action_read = 2,
} modbus_command_actions;

// Struct for action choosing
typedef struct {
    modbus_command_actions action;
    modbus_cid cid;
    // TODO придумати чи це норм сюди закидувтаи значення
    uint16_t value_to_write;
} Modbus_action_rw;

/* ----------------------------------------------------- */

/* --------------------- Modbus received data management ---------------------- */

/*
 * Already created buffer
 * Getter function available
 */
static QueueHandle_t modbus_received_data_buff;

/*
 * Struct that contains data
 * can be extended
 */
typedef struct {
    uint16_t received_data_val;
} Modbus_data;

/* ----------------------------------------------------- */

/* Memory address resolution is 16 bit or 2 bytes
 * If some value are consume more than 16 bit we need point
 * that in this manner (16 + 16 = 32) (16 + 16 + 16 + 16 + 16 = 80)
 * up to 253 bytes
*/
enum modbus_register_size {
    modbus_16bit_value = 0x1,
    modbus_32bit_value = 0x2,
    modbus_48bit_value = 0x3,
    modbus_64bit_value = 0x4,
    modbus_80bit_value = 0x5,
    modbus_96bit_value = 0x6,
    modbus_112bit_value = 0x7,
    modbus_128bit_value = 0x8,
};

void modbus_conf_init() {
    mb_communication_info_t modbus_config = {0};
    modbus_config.ser_opts.mode = MB_RTU;
    modbus_config.ser_opts.port = modbus_uart_port_numbr;
    modbus_config.ser_opts.baudrate = MODBUS_BAUD_RTT;
    modbus_config.ser_opts.parity = UART_PARITY_DISABLE;
    modbus_config.ser_opts.uid = 0;
    modbus_config.ser_opts.data_bits = UART_DATA_8_BITS;
    modbus_config.ser_opts.stop_bits = UART_STOP_BITS_1;
    modbus_config.ser_opts.response_tout_ms = 1000;

    ESP_ERROR_CHECK(mbc_master_create_serial(&modbus_config, &master_handler));

    // Set UART pins(TX: IO12, RX: IO13, RTS: IO18, CTS: IO19, DTR: UNUSED, DSR: UNUSED)
    ESP_ERROR_CHECK(uart_set_pin(modbus_uart_port_numbr, 12, 13, 18, 19, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Modbus register(table) config
    conf_mapping();

    ESP_ERROR_CHECK(mbc_master_start(master_handler));

    modbus_received_data_buff = xQueueCreate(10, sizeof(Modbus_data));

    modbus_communication_command = xQueueCreate(4, sizeof(Modbus_action_rw));
}

static void conf_mapping() {
    mb_parameter_descriptor_t device_parameters[] = {0};
    // param_offset - optional, set to 0
    // param_opts - optional
    // TODO warning section can be mistakes
    // Test parametr
    device_parameters[0] = {
        CID_TEST_COMUN, STR("Test"), STR("--"), MODBUS_SLAVE_1, MB_PARAM_DISCRETE,
        modbus_test_register, 1, 0, PARAM_TYPE_U8, PARAM_SIZE_U8, 0, PAR_PERMS_READ
    };

    device_parameters[1];

    // Визначаємо к-сть параметрів
    uint16_t num_device_parameters = (sizeof(device_parameters) / sizeof(device_parameters[0]));
    ESP_ERROR_CHECK(mbc_master_set_descriptor(master_handler, &device_parameters[0], num_device_parameters));
}

void vTask_Modbus_Start_Communication(void* arg) {

    // struct that contains the enum field where action is stored
    Modbus_action_rw modbus_action;

    // Temporary field for storing the data from Modbus GET action
    uint16_t received_value = 0;

    // Struct that stores the data from SLAVE device
    Modbus_data data_struct = {0};

    // Errors handling
    uint32_t queue_error_counter = 0;
    uint32_t modbus_communication_error_counter = 0;
    esp_err_t modbus_communication_error = ESP_OK;

    for (;;) {
        /*
         * Wait 50 ms (time interval between reading) and check the command
         * if command arrive earlier just perform the command
         * if no command wait 50 ms and do generic cation
         */
        BaseType_t status = xQueueReceive(modbus_communication_command, &modbus_action , pdMS_TO_TICKS(50));
        if (status == pdTRUE) {
            switch (modbus_action.action) {
                case modbus_action_write:
                    modbus_communication_error = mbc_master_set_parameter(master_handler, modbus_action.cid, (uint8_t*)&modbus_action.value_to_write, &type);
                    if (modbus_communication_error != ESP_OK) {
                        modbus_communication_error_counter++;
                        uart_log("modbus_communication_error");
                    }
                    break;
                case modbus_action_read:
                    modbus_communication_error = mbc_master_get_parameter(master_handler, modbus_action.cid, (uint8_t*)&received_value, &type);
                    if (modbus_communication_error == ESP_OK) {
                        data_struct.received_data_val = received_value;
                        // TODO pdMS_TO_TICKS(10) change interval
                        if (xQueueSend(modbus_received_data_buff, (void*)&data_struct, pdMS_TO_TICKS(10)) != pdTRUE) {
                            queue_error_counter++;
                            uart_log("");
                        }
                    } else {
                        modbus_communication_error_counter++;
                        // TODO
                        uart_log("modbus_communication_error");
                    }
                    break;
                default:
                    break;
            }
        } else {
            modbus_communication_error = mbc_master_get_parameter(master_handler, CID_VOLTAGE, (uint8_t*)&received_value, &type);
            if (modbus_communication_error == ESP_OK) {
                data_struct.received_data_val = received_value;
                // TODO pdMS_TO_TICKS(10) change interval
                if (xQueueSend(modbus_received_data_buff, (void*)&data_struct, pdMS_TO_TICKS(10)) != pdTRUE) {
                    queue_error_counter++;
                    uart_log("");
                }
            } else {
                modbus_communication_error_counter++;
                // TODO
                uart_log("modbus_communication_error");
            }
        }

        // Check the task Stack usage
        UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        uart_log("uxHighWaterMark"); // TODO
    }
    vTaskDelete(NULL);
}

void modbus_destroy() {
    ESP_ERROR_CHECK(mbc_master_delete(master_handler));
}

QueueHandle_t modbusGetReceivedDataBuffer() {
    return modbus_received_data_buff;
}

QueueHandle_t modbusGetCommunicationCommandBuffer() {
    return modbus_communication_command;
}
