idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
       # INCLUDE_DIRS "." "Inc"
        #SRC_DIRS "." "Src"
        REQUIRES esp_driver_i2c esp_driver_gpio esp_driver_gptimer
        REQUIRES esp_driver_uart esp_wifi
)
