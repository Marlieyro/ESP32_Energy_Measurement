//
// Created by user on 13.10.26.
//
#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"
#include "app_streambuffers.h"

#define BUFFER_SIZE 1400 // Bytes, 1400 cause HTTP payload
#define BUFFER_TRIGGER_LEVEL 1300 // Bytes

static StreamBufferHandle_t xStreamBuffer = NULL;

void app_streamBuffer_create() {
    xStreamBuffer = xStreamBufferCreate(BUFFER_SIZE, BUFFER_TRIGGER_LEVEL);
    if( xStreamBuffer == NULL ) {
        // LOG Error //TODO
    } else {
        // LOG Success TODO
    }
}

StreamBufferHandle_t getAppStreamBuffer() {
    return xStreamBuffer;
}

void app_streamBuffer_destroy() {
    xStreamBuffer = NULL;
}