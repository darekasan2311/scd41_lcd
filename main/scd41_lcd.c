#include "scd41.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdio.h>

#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_FREQ_HZ 100000

static const char *TAG = "SCD41";

static void scd_task(void *arg)
{
    // Configure I2C master (you need to do this before using the component)
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,                // GPIO pin for SDA
        .scl_io_num = I2C_MASTER_SCL_IO,                // GPIO pin for SCL
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,      // 100kHz
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
        } else {
            ESP_LOGW(TAG, "Failed to read sensor data");
        }

        // SCD41 provides new data every 5 seconds
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

}

void app_main(void)
{
    xTaskCreate(scd_task, "scd_task", 8192, NULL, 5, NULL);
}

