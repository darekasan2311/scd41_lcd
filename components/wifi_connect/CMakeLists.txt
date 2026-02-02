idf_component_register(SRCS "wifi_connect.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_timer esp_wifi nvs_flash)
