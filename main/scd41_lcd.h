#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"

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
static SemaphoreHandle_t data_mutex = NULL;

static lv_obj_t *label_co2 = NULL;
static lv_obj_t *label_temp = NULL;
static lv_obj_t *label_humid = NULL;

static lv_obj_t *label_max_temp_data = NULL;
static lv_obj_t *label_min_temp_data = NULL;

static lv_obj_t *label_max_hum_data = NULL;
static lv_obj_t *label_min_hum_data = NULL;

static lv_obj_t *label_max_co2_data = NULL;
static lv_obj_t *label_min_co2_data = NULL;

static lv_obj_t *screen_sensor = NULL;

static lv_obj_t *screen_temp_graph = NULL;
static lv_obj_t *temp_chart = NULL;
static lv_chart_series_t *temp_series = NULL;

static lv_obj_t *screen_hum_graph = NULL;
static lv_obj_t *hum_chart = NULL;
static lv_chart_series_t *hum_series = NULL;

static lv_obj_t *screen_co2_graph = NULL;
static lv_obj_t *co2_chart = NULL;
static lv_chart_series_t *co2_series = NULL;

static int current_screen = 0;
static scd41_data_t data_buffer[12]; //data buffer
static int data_buffer_index = 0;

static float min_temp = 0.0f;
static float max_temp = 0.0f;
static int min_co2 = 0;
static int max_co2 = 0;
static float min_hum = 0.0f;
static float max_hum = 0.0f;
