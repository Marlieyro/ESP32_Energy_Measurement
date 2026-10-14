//
// Created by user on 13.10.26.
//

#ifndef ENERGYMEASURMENT_APP_STREAMBUFFERS_H
#define ENERGYMEASURMENT_APP_STREAMBUFFERS_H
#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"

typedef struct {
    int id;
    int time; // do batter time
    int data;
} telemetry_data_t;

void app_MsgBuffer_create();
MessageBufferHandle_t getAppMsgBuffer();
void app_streamMsg_destroy();

#endif //ENERGYMEASURMENT_APP_STREAMBUFFERS_H
