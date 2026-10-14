//
// Created by user on 13.10.26.
//
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"
#include "app_streambuffers.h"

static const char *TAG = "MSG_BUFFER";

#define BUFFER_SIZE 1400 // Bytes, 1400 cause HTTP payload
// #define BUFFER_TRIGGER_LEVEL 1300 // Bytes

static MessageBufferHandle_t xMsgBuffer = NULL;

void app_MsgBuffer_create() {
    xMsgBuffer = xMessageBufferCreate(BUFFER_SIZE);
    if( xMsgBuffer == NULL ) {
        ESP_LOGI(TAG, "xMessageBufferCreate failed");
    } else {
        ESP_LOGI(TAG, "xMessageBufferCreate succeeded");
    }
}

MessageBufferHandle_t getAppMsgBuffer() {
    return xMsgBuffer;
}

void app_streamMsg_destroy() {
    vMessageBufferDelete(xMsgBuffer);
    xMsgBuffer = NULL;
    ESP_LOGI(TAG, "xMsgBufferDelete succeeded");
}

