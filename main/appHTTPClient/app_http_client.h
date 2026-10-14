 //
// Created by user on 12.10.26.
//

#ifndef ENERGYMEASURMENT_APP_HTTP_CLIENT_H
#define ENERGYMEASURMENT_APP_HTTP_CLIENT_H
#include "app_streambuffers.h"

 void app_http_client_init(void);
void app_http_client_post(const char *post_data);
void app_http_client_deinit(void);
void vTask_HTTP_POST_request(void *arg);
int telemetry_to_json_iso_time(const telemetry_data_t *data, char *buffer, size_t buffer_size);
#endif //ENERGYMEASURMENT_APP_HTTP_CLIENT_H
