//
// Created by user on 12.10.26.
//

// --------------- SYSTEM INCLUDES -----------------
#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_http_client.h"
#include "freertos/message_buffer.h"

// -------------- USER INCLUDES ------------------
#include "app_http_client.h"
#include "../appStreamBuffer/app_streambuffers.h"

// -------------- DEFINES -----------------
#define HTTP_HOST_IP_ADDR "192.168.0.222"
#define HTTP_HOST_PORT (45000)
#define HOST_ENDPOINT_PATH "/energy"
#define HTTP_TIMEOUT_MS (5000) // 5000ms = 5sec
#define HTTP_MAX_AUTHORIZATION_RETRIES (5)
#define HTTP_RX_BUFFER_SIZE (1024) // Bytes
#define HTTP_TX_BUFFER_SIZE (1024) // Bytes
#define KEEP_ALIVE_ENABLE (true)
#define HTTP_MAX_OUTPUT_BUFFER (1024)

// ------------- ESPIDF LOG TAG -------------
static const char *TAG = "HTTP_CLIENT";

static char http_response_from_server_buffer[HTTP_MAX_OUTPUT_BUFFER + 1] = {0};
static esp_http_client_handle_t http_client_handle;

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_HEADERS_COMPLETE:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADERS_COMPLETE");
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // Clean the buffer in case of a new request
            if (output_len == 0 && evt->user_data) {
                // we are just starting to copy the output data into the use
                memset(evt->user_data, 0, HTTP_MAX_OUTPUT_BUFFER);
            }
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                    copy_len = MIN(evt->data_len, (HTTP_MAX_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    int content_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                        output_buffer = (char *) calloc(content_len + 1, sizeof(char));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (content_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
#if CONFIG_EXAMPLE_ENABLE_RESPONSE_BUFFER_DUMP
                ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
#endif
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
        default:
            break;
    }
    return ESP_OK;
}

void app_http_client_init() {
    esp_http_client_config_t http_client_config = {0};
    http_client_config.host = HTTP_HOST_IP_ADDR;
    http_client_config.port = HTTP_HOST_PORT;
    http_client_config.path = HOST_ENDPOINT_PATH;
   // http_client_config.query = ; // Це типу таке: "type=temp&limit=10"
    http_client_config.method = HTTP_METHOD_POST;
    http_client_config.timeout_ms = HTTP_TIMEOUT_MS;
    http_client_config.max_authorization_retries = HTTP_MAX_AUTHORIZATION_RETRIES; // 5
    http_client_config.event_handler = _http_event_handler;
    http_client_config.transport_type = HTTP_TRANSPORT_OVER_TCP;
    http_client_config.buffer_size = HTTP_RX_BUFFER_SIZE;
    http_client_config.buffer_size_tx = HTTP_TX_BUFFER_SIZE;
    http_client_config.keep_alive_enable = KEEP_ALIVE_ENABLE;
    http_client_config.addr_type = HTTP_ADDR_TYPE_INET; // IPv4
    http_client_config.user_data = http_response_from_server_buffer; // HTTP Response Data

    http_client_handle = esp_http_client_init(&http_client_config);
}

void app_http_client_post(const char* post_data) {

    // Change HTTP header type to JSON
    esp_http_client_set_header(http_client_handle, "Content-Type", "application/json");

    // Set payload to HTTP frame
    esp_http_client_set_post_field(http_client_handle, post_data, strlen(post_data));

    // Send POST into network
    esp_err_t err = esp_http_client_perform(http_client_handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(http_client_handle),
                esp_http_client_get_content_length(http_client_handle));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
}

void vTask_HTTP_POST_request(void *arg) {

    // Data buffer to send into the Network
    char post_data[1500] = {0};
    int postData_Offset = 0;

    // Data struct from Modbus
    telemetry_data_t temp_telemetry_data = {0};

    // Delay management
    const TickType_t xBlockTime = portMAX_DELAY;

    for (;;) {
        // Receiving a data from the Modbus Task
        size_t xReceivedBytes = xMessageBufferReceive(getAppMsgBuffer(), &temp_telemetry_data, sizeof(telemetry_data_t),xBlockTime);

        // Check
        if (xReceivedBytes == 0) {
            ESP_LOGI(TAG, "Received %d bytes of data. ERROR", xReceivedBytes);
        }

        // Convert received struct to the JSON format
        int written_len = telemetry_to_json_iso_time(&temp_telemetry_data, post_data + postData_Offset, sizeof(post_data) - postData_Offset);

        // Add offset to next buffer cell
        postData_Offset += written_len;

        // Send the data to Network
        if (postData_Offset > 1200) {
            app_http_client_post(post_data);
            postData_Offset = 0;
            written_len = 0;
        } else continue;
    }
    vTaskDelete(NULL);
}

int telemetry_to_json_iso_time(const telemetry_data_t *data, char *buffer, size_t buffer_size) {
    if (data == NULL || buffer == NULL || buffer_size == 0) {
        return -1;
    }

    char time_str[32];
    time_t raw_time = (time_t)data->time;
    struct tm *tm_info = gmtime(&raw_time); // Конвертація у UTC

    // Якщо конвертація пройшла успішно, формуємо рядок, інакше ставимо null
    if (tm_info != NULL) {
        strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    } else {
        snprintf(time_str, sizeof(time_str), "null");
    }

    // Записуємо готовий JSON (зверніть увагу на лапки навколо %s для часу)
    return snprintf(buffer, buffer_size,
                    "{\"id\": %d, \"time\": \"%s\", \"data\": %d}",
                    data->id, time_str, data->data);
}

void app_http_client_deinit(void){
    esp_http_client_cleanup(http_client_handle);
}
