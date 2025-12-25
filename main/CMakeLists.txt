idf_component_register(SRCS "scd41_lcd.c" "noto_sans_jap.c" "jet_mono_light_32.c"
                    INCLUDE_DIRS "."
                    REQUIRES st7789 driver esp_timer
                    )
