 //
// Created by user on 12.10.26.
//

#ifndef ENERGYMEASURMENT_APP_HTTP_CLIENT_H
#define ENERGYMEASURMENT_APP_HTTP_CLIENT_H

void app_http_client_init(void);
void app_http_client_post(char *post_data);
void app_http_client_deinit(void);
void vTask_HTTP_POST_request(void *arg);

#endif //ENERGYMEASURMENT_APP_HTTP_CLIENT_H
