idf_component_register(SRCS "scd41_lcd.c" "jet_mono_light_32.c"
                    INCLUDE_DIRS "."
                    REQUIRES st7789 driver esp_timer
                    )
