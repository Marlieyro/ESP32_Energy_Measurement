idf_component_register(
            SRCS
        "main.c"
        "appWiFiConfig/wifi_conf.c"
        "appHTTPClient/app_http_client.c"
        "appModBusConfig/modbus_conf.c"
        "uart_conf.c"
        "appStreamBuffer/app_streambuffers.c"
            INCLUDE_DIRS
        "."
        "appStreamBuffer"
        "appWiFiConfig"
        "appModBusConfig"
        "appHTTPClient"
        #SRC_DIRS "." "Src"
            REQUIRES
        esp_driver_i2c
        esp_driver_gpio
        esp_driver_gptimer
        esp_driver_uart
        esp_wifi
        esp_http_client
        esp-tls
        nvs_flash
        esp_event esp_netif
)

