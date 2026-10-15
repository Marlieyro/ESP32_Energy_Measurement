//
// Created by user on 03.09.26.
//

// --------------- SYSTEM INCLUDES -----------------
#include "FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "portmacro.h"
#include "../uart_conf.h"
#include "../../../../../../home/user/esp/esp-idf/components/freertos/FreeRTOS-Kernel/include/freertos/projdefs.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "hal/emac_hal.h"
#include "../appStreamBuffer/app_streambuffers.h"
#include "freertos/message_buffer.h"

// -------------- USER INCLUDES ------------------
#include "esp_modbus_common.h"
#include "esp_modbus_master.h"
#include "modbus_conf.h"

// -------------- DEFINES -----------------
#define MODBUS_BAUD_RTT (115200)
// Modbus slave device adr
#define MODBUS_SLAVE_1 (1)
#define OPTS(min_val, max_val, step_val) { .opt1 = min_val, .opt2 = max_val, .opt3 = step_val }

// ------------- ESPIDF LOG TAG -------------
static const char *TAG = "MODBUS";

constexpr uart_port_t modbus_uart_port_numbr = UART_NUM_2;
static void *master_handler = NULL;

uint8_t type = 0;

typedef enum {
    CID_TEST_COMUN = 0,
    CID_VOLTAGE = 1,
} modbus_cid;

// Modbus 'Register Start' addresses
enum modbus_register_adr_map {
    modbus_test_register = 0x0,
    modbus_voltage_register = 0x1
};

/* ------------------- Modbus action management Queue ---------------------- */
// Modbus available actions
typedef enum {
    modbus_action_write = 0,
    modbus_action_read = 1,
    modbus_default_action_read = 2,
} modbus_command_actions;

// The que for action choosing write or read
static QueueHandle_t modbus_communication_command;

// Struct for action choosing
typedef struct {
    modbus_command_actions action;
    modbus_cid cid;
    // TODO придумати чи це норм сюди закидувтаи значення
    uint16_t value_to_write;
} Modbus_action_rw;


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

    // Set UART pins(TX: IO17, RX: IO16, RTS: IO(UNUSED)18, CTS: IO(UNUSED)19, DTR: UNUSED, DSR: UNUSED)
    ESP_ERROR_CHECK(uart_set_pin(modbus_uart_port_numbr, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Modbus register(table) config
    conf_mapping();

    ESP_ERROR_CHECK(mbc_master_start(master_handler));
    modbus_communication_command = xQueueCreate(4, sizeof(Modbus_action_rw));
    ESP_LOGI(TAG, "Modbus Config success");
}

static void conf_mapping() {
    static mb_parameter_descriptor_t device_parameters[] = {
        {.cid = 0, // Має бути строго 0 для першого  елмента КУРВА ТИХ РОЗРОБНИКІВ ESPIDF!
            .param_key = STR("VoltageUsage"),
            .param_units = STR("V"),
            .mb_slave_addr = MODBUS_SLAVE_1,
            .mb_param_type = MB_PARAM_INPUT,
        .mb_reg_start = modbus_voltage_register,
            .mb_size = 1,
            .param_offset = 0,
            .param_type = PARAM_TYPE_U16,
            .param_size = 2,
            .param_opts = OPTS(0, 0, 0),
            .access = PAR_PERMS_READ}
    };
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
    telemetry_data_t data_struct = {0};

    // Errors handling
    uint32_t queue_error_counter = 0;
    uint32_t modbus_communication_error_counter = 0;
    esp_err_t modbus_communication_error = ESP_OK;

    const TickType_t xBlockTime = pdTICKS_TO_MS(20);

    for (;;) {
        /*
         * Wait 2000 ms (time interval between reading) and check the command
         * if command arrive earlier just perform the command
         * if no command wait 2000 ms and do generic cation
         */
        BaseType_t status = xQueueReceive(modbus_communication_command, &modbus_action , pdMS_TO_TICKS(2000));
        if (status == pdTRUE) {
            switch (modbus_action.action) {
                case modbus_action_write:
                    modbus_communication_error = mbc_master_set_parameter(master_handler, modbus_action.cid, (uint8_t*)&modbus_action.value_to_write, &type);
                    if (modbus_communication_error != ESP_OK) {
                        modbus_communication_error_counter++;
                        ESP_LOGI(TAG, "modbus_action_write - Communication Error");
                    }
                    break;
                case modbus_action_read:
                    modbus_communication_error = mbc_master_get_parameter(master_handler, modbus_action.cid, (uint8_t*)&received_value, &type);
                    if (modbus_communication_error == ESP_OK) {
                        data_struct.data = received_value;
                        data_struct.id = 1; // TODO
                        data_struct.time = 16561743; // TODO
                        if (xMessageBufferSend(getAppMsgBuffer(), &data_struct, sizeof(telemetry_data_t), xBlockTime) != 0) {
                            queue_error_counter++;
                            ESP_LOGI(TAG, "modbus_action_read - Queue Error");
                        }
                    } else {
                        modbus_communication_error_counter++;
                    }
                    break;
                default:
                    break;
            }
        } else {
            modbus_communication_error = mbc_master_get_parameter(master_handler, 0, (uint8_t*)&received_value, &type);
            if (modbus_communication_error == ESP_OK) {
                data_struct.data = received_value;
                data_struct.id = 1; // TODO
                data_struct.time = 16561743; // TODO
                if (xMessageBufferSend(getAppMsgBuffer(), &data_struct, sizeof(telemetry_data_t), xBlockTime) != 0) {
                    queue_error_counter++;
                    ESP_LOGI(TAG, "modbus_default_action - Queue Error");
                }
            } else {
                modbus_communication_error_counter++;
            }
        }

        // Check the task Stack usage
        UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGI(TAG, "uxTaskGetStackHighWaterMark: %d", uxHighWaterMark);
    }
    vTaskDelete(NULL);
}

void modbus_destroy() {
    ESP_ERROR_CHECK(mbc_master_delete(master_handler));
}

QueueHandle_t modbusGetCommunicationCommandBuffer() {
    return modbus_communication_command;
}
