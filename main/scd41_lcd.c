#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "st7789.h"
#include "scd41.h"
#include "driver/i2c.h"

// SCD41 I2C config
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_FREQ_HZ 100000

static const char *TAG = "SCD41";

scd41_data_t sensor_data = {0};
static SemaphoreHandle_t data_mutex = NULL;

static lv_obj_t *label_co2 = NULL;
static lv_obj_t *label_temp = NULL;
static lv_obj_t *label_humid = NULL;

extern void create_sensor_labels(const lv_font_t *font);

// LVGL timer callback - runs in LVGL context
static void lvgl_update_timer_cb(lv_timer_t *timer)
{
    char text_buffer[64];
    
    // Take mutex to safely read sensor data
    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (sensor_data.data_ready) {
            if (label_co2) {
                snprintf(text_buffer, sizeof(text_buffer), "CO2: %d ppm", sensor_data.co2_ppm);
                lv_label_set_text(label_co2, text_buffer);
            }
            
            if (label_temp) {
                snprintf(text_buffer, sizeof(text_buffer), "Temp: %.1f C", sensor_data.temperature);
                lv_label_set_text(label_temp, text_buffer);
            }
            
            if (label_humid) {
                snprintf(text_buffer, sizeof(text_buffer), "Humidity: %.1f %%", sensor_data.humidity);
                lv_label_set_text(label_humid, text_buffer);
            }
        }
        xSemaphoreGive(data_mutex);
    }
}

void create_sensor_labels(const lv_font_t *font)
{
    // CO2 Label
    label_co2 = lv_label_create(lv_screen_active());
    lv_label_set_text(label_co2, "CO2: -- ppm");
    lv_obj_set_style_text_font(label_co2, font, 0);
    lv_obj_set_style_text_color(label_co2, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(label_co2, 10, 60);
    
    // Temperature Label
    label_temp = lv_label_create(lv_screen_active());
    lv_label_set_text(label_temp, "Temp: -- C");
    lv_obj_set_style_text_font(label_temp, font, 0);
    lv_obj_set_style_text_color(label_temp, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(label_temp, 10, 110);
    
    // Humidity Label
    label_humid = lv_label_create(lv_screen_active());
    lv_label_set_text(label_humid, "Humidity: -- %");
    lv_obj_set_style_text_font(label_humid, font, 0);
    lv_obj_set_style_text_color(label_humid, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(label_humid, 10, 160);
    
    // Create LVGL timer to update UI every 500ms
    lv_timer_create(lvgl_update_timer_cb, 500, NULL);
}

void scd_task(void *arg)
{
    // Configure I2C master
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0));
    
    // Initialize SCD41
    scd41_config_t config = SCD41_CONFIG_DEFAULT();
    config.i2c_port = I2C_NUM_0;
    config.timeout_ms = 1000;
    
    ESP_ERROR_CHECK(scd41_init(&config));
    ESP_ERROR_CHECK(scd41_start_measurement());
    
    // Wait for first measurement (5 seconds)
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    while (1) {
        scd41_data_t data;
        esp_err_t ret = scd41_read_measurement(&data);
        
        if (ret == ESP_OK && data.data_ready) {
            ESP_LOGI(TAG, "CO2: %d ppm, Temperature: %.1f°C, Humidity: %.1f%%",
                     data.co2_ppm, data.temperature, data.humidity);
            
            if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                sensor_data = data;
                xSemaphoreGive(data_mutex);
            }
        } else {
            ESP_LOGW(TAG, "Failed to read sensor data");
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    data_mutex = xSemaphoreCreateMutex();
    
    init_lcd(LV_DISP_ROT_270);
    
    char *message = "Sensor Monitor";
    extern lv_font_t jet_mono_light_32;
    
    create_background();
    create_label(&jet_mono_light_32, 10, 10, message);
    create_sensor_labels(&jet_mono_light_32);
    
    // Start sensor task
    xTaskCreate(scd_task, "scd_task", 4096, NULL, 5, NULL);
    
    // LVGL task handler loop
    while (1) {
        lv_timer_handler();  // Process LVGL tasks
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms is typical for LVGL
    }
}
