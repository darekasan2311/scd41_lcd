#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

// SCD41 I2C config
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_FREQ_HZ 100000

// GPIO config
#define ON_OFF_BUTTON GPIO_NUM_32
#define NEXT_SCREEN_BUTTON GPIO_NUM_33
#define DEBOUNCE_TIME_MS 50

static const char *TAG = "SCD41";
scd41_data_t sensor_data = {0};

static lv_obj_t *label_co2 = NULL;
static lv_obj_t *label_temp = NULL;
static lv_obj_t *label_humid = NULL;

static lv_obj_t *screen_sensor = NULL;

static int current_screen = 0;

