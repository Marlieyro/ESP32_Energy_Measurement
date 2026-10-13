idf_component_register(
            SRCS
        "main.c"
        "wifi_conf.c"
        "app_http_client.c"
        "modbus_conf.c"
        "uart_conf.c"
        "appStreamBuffer/app_streambuffers.c"
            INCLUDE_DIRS
        "."
        "appStreamBuffer"
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

