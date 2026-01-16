#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "st7789.h"
#include "scd41.h"
#include "driver/i2c.h"

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

extern void create_sensor_labels();
extern void create_sensor_co2(const lv_font_t *font_label, const lv_font_t *font_value);
extern void create_sensor_temp(const lv_font_t *font_label, const lv_font_t *font_value);
extern void create_sensor_hum(const lv_font_t *font_label, const lv_font_t *font_value);

// LVGL timer callback - runs in LVGL context
static void lvgl_update_timer_cb(lv_timer_t *timer)
{
    char text_buffer[64];
    
    // Take mutex to safely read sensor data
    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (sensor_data.data_ready) {
            if (label_co2) {
                snprintf(text_buffer, sizeof(text_buffer), ": %d ppm", sensor_data.co2_ppm);
                lv_label_set_text(label_co2, text_buffer);
            }
            
            if (label_temp) {
                snprintf(text_buffer, sizeof(text_buffer), ": %.1f C", sensor_data.temperature);
                lv_label_set_text(label_temp, text_buffer);
            }
            
            if (label_humid) {
                snprintf(text_buffer, sizeof(text_buffer), ": %.1f %%", sensor_data.humidity);
                lv_label_set_text(label_humid, text_buffer);
            }

            if (label_max_temp_data) {
                snprintf(text_buffer, sizeof(text_buffer), " %.1f C", max_temp);
                lv_label_set_text(label_max_temp_data, text_buffer);
            }

            if (label_min_temp_data) {
                snprintf(text_buffer, sizeof(text_buffer), " %.1f C", min_temp);
                lv_label_set_text(label_min_temp_data, text_buffer);
            }

            if (label_max_hum_data) {
                snprintf(text_buffer, sizeof(text_buffer), " %.1f %%", max_hum);
                lv_label_set_text(label_max_hum_data, text_buffer);
            }

            if (label_min_hum_data) {
                snprintf(text_buffer, sizeof(text_buffer), " %.1f %%", min_hum);
                lv_label_set_text(label_min_hum_data, text_buffer);
            }

            if (label_max_co2_data) {
                snprintf(text_buffer, sizeof(text_buffer), " %d ppm", max_co2);
                lv_label_set_text(label_max_co2_data, text_buffer);
            }

            if (label_min_co2_data) {
                snprintf(text_buffer, sizeof(text_buffer), " %d ppm", min_co2);
                lv_label_set_text(label_min_co2_data, text_buffer);
            }
        }
        xSemaphoreGive(data_mutex);
    }
}
// in lv_color_make order is BRG with RGB565 notation
// max values are: 31, 31, 63
void create_sensor_co2(const lv_font_t *font_label, const lv_font_t *font_value)
{
    // CO2 Label and Value
    lv_obj_t *label_co2_text = lv_label_create(screen_sensor);  // Changed from lv_screen_active()
    lv_label_set_text(label_co2_text, "CO2");
    lv_obj_set_style_text_font(label_co2_text, font_label, 0);
    lv_obj_set_style_text_color(label_co2_text, lv_color_make(31, 31, 63), 0);
    lv_obj_set_pos(label_co2_text, 10, 60);
    
    label_co2 = lv_label_create(screen_sensor);  // Changed from lv_screen_active()
    lv_label_set_text(label_co2, ": -- ppm");
    lv_obj_set_style_text_font(label_co2, font_value, 0);
    lv_obj_set_style_text_color(label_co2, lv_color_make(31, 31, 63), 0);
    lv_obj_set_pos(label_co2, 80, 60);
}

void create_sensor_temp(const lv_font_t *font_label, const lv_font_t *font_value)
{
    // Temperature Label and Value
    lv_obj_t *label_temp_text = lv_label_create(screen_sensor);  // Changed
    lv_label_set_text(label_temp_text, "湿度");
    lv_obj_set_style_text_font(label_temp_text, font_label, 0);
    lv_obj_set_style_text_color(label_temp_text, lv_color_make(31, 31, 63), 0);
    lv_obj_set_pos(label_temp_text, 10, 110);
    
    label_temp = lv_label_create(screen_sensor);  // Changed
    lv_label_set_text(label_temp, ": -- C");
    lv_obj_set_style_text_font(label_temp, font_value, 0);
    lv_obj_set_style_text_color(label_temp, lv_color_make(31, 31, 63), 0);
    lv_obj_set_pos(label_temp, 80, 110);
}

void create_sensor_hum(const lv_font_t *font_label, const lv_font_t *font_value)
{
    // Humidity Label and Value
    lv_obj_t *label_humid_text = lv_label_create(screen_sensor);  // Changed
    lv_label_set_text(label_humid_text, "温度");
    lv_obj_set_style_text_font(label_humid_text, font_label, 0);
    lv_obj_set_style_text_color(label_humid_text, lv_color_make(31, 31, 63), 0);
    lv_obj_set_pos(label_humid_text, 10, 160);
    
    label_humid = lv_label_create(screen_sensor);  // Changed
    lv_label_set_text(label_humid, ": -- %");
    lv_obj_set_style_text_font(label_humid, font_value, 0);
    lv_obj_set_style_text_color(label_humid, lv_color_make(31, 31, 63), 0);
    lv_obj_set_pos(label_humid, 80, 160);
}
// Create the sensor screen
void create_sensor_screen()
{
    extern lv_font_t jet_mono_light_32;
    extern lv_font_t noto_sans_jap;
    
    // Create sensor screen
    screen_sensor = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_sensor, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_sensor, LV_OPA_COVER, LV_PART_MAIN);
    
    // Add title
    lv_obj_t *label_title = lv_label_create(screen_sensor);
    lv_label_set_text(label_title, "Sensor Monitor");
    lv_obj_set_style_text_font(label_title, &jet_mono_light_32, 0);
    lv_obj_set_style_text_color(label_title, lv_color_make(31, 31, 63), 0);
    lv_obj_set_pos(label_title, 10, 10);
    
    // Create sensor labels
    create_sensor_co2(&jet_mono_light_32, &jet_mono_light_32);
    create_sensor_temp(&noto_sans_jap, &jet_mono_light_32);
    create_sensor_hum(&noto_sans_jap, &jet_mono_light_32);
}

void create_graph_screen(lv_obj_t **graph, lv_obj_t **lv_max_data, lv_obj_t **lv_min_data,
                         lv_obj_t **chart, lv_chart_series_t **series, int y_high_lim)
{
    *graph = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(*graph, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(*graph, LV_OPA_COVER, LV_PART_MAIN);
    
    // Max label
    lv_obj_t *label_max = lv_label_create(*graph);
    lv_label_set_text(label_max, "Max:");
    lv_obj_set_style_text_font(label_max, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_max, lv_color_make(0, 31, 0), 0);
    lv_obj_set_pos(label_max, 70, 220);

    // Max data
    *lv_max_data = lv_label_create(*graph);
    lv_label_set_text(*lv_max_data, " --");
    lv_obj_set_style_text_font(*lv_max_data, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(*lv_max_data, lv_color_make(31, 31, 63), 0);
    lv_obj_set_pos(*lv_max_data, 100, 220);

    // Min label
    lv_obj_t *label_min = lv_label_create(*graph);
    lv_label_set_text(label_min, "Min:");
    lv_obj_set_style_text_font(label_min, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_min, lv_color_make(31, 0, 0), 0);
    lv_obj_set_pos(label_min, 170, 220);

    // Min data
    *lv_min_data = lv_label_create(*graph);
    lv_label_set_text(*lv_min_data, " --");
    lv_obj_set_style_text_font(*lv_min_data, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(*lv_min_data, lv_color_make(31, 31, 63), 0);
    lv_obj_set_pos(*lv_min_data, 200, 220);

    // Create scale for Y-axis
    lv_obj_t *scale_y = lv_scale_create(*graph);
    lv_obj_set_size(scale_y, 25, 167);
    lv_obj_set_pos(scale_y, 5, 17);
    lv_scale_set_mode(scale_y, LV_SCALE_MODE_VERTICAL_LEFT);
    lv_scale_set_range(scale_y, 0, y_high_lim);
    lv_scale_set_total_tick_count(scale_y, 9);
    lv_scale_set_major_tick_every(scale_y, 2);
    lv_obj_set_style_text_font(scale_y, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(scale_y, lv_color_make(20, 20, 40), 0);
    lv_obj_set_style_line_color(scale_y, lv_color_make(15, 15, 30), LV_PART_INDICATOR);
    lv_obj_set_style_length(scale_y, 5, LV_PART_INDICATOR);
    
    // Create scale for X-axis (time)
    lv_obj_t *scale_x = lv_scale_create(*graph);
    lv_obj_set_size(scale_x, 250, 25);
    lv_obj_set_pos(scale_x, 35, 190);
    lv_scale_set_mode(scale_x, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
    lv_scale_set_range(scale_x, -55, 0);
    lv_scale_set_total_tick_count(scale_x, 12);
    lv_scale_set_major_tick_every(scale_x, 2);
    lv_obj_set_style_text_font(scale_x, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(scale_x, lv_color_make(20, 20, 40), 0);
    lv_obj_set_style_line_color(scale_x, lv_color_make(15, 15, 30), LV_PART_INDICATOR);
    lv_obj_set_style_length(scale_x, 5, LV_PART_INDICATOR);
 
    // Create line
    static lv_point_precise_t line_points[] = {
        {0, 0},      // Start point (x, y)
        {320, 0}     // End point (x, y)
    };

    // Create line object
    lv_obj_t *line = lv_line_create(*graph);
    lv_line_set_points(line, line_points, 2);

    // Position the line
    lv_obj_set_pos(line, 0, 212);

    // Style the line
    lv_obj_set_style_line_width(line, 2, 0);
    lv_obj_set_style_line_color(line, lv_color_make(31, 31, 63), 0);
    
    // Create chart
    *chart = lv_chart_create(*graph);
    lv_obj_set_size(*chart, 260, 180);
    lv_obj_set_pos(*chart, 30, 10);
    
    // Chart styling
    lv_obj_set_style_bg_color(*chart, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_style_border_width(*chart, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(*chart, lv_color_make(15, 15, 31), LV_PART_MAIN);
    lv_obj_set_style_pad_all(*chart, 5, LV_PART_MAIN);
    
    // Configure chart
    lv_chart_set_type(*chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(*chart, 12);
    lv_chart_set_range(*chart, LV_CHART_AXIS_PRIMARY_Y, 0, y_high_lim);
    lv_chart_set_update_mode(*chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_div_line_count(*chart, 5, 0);
    
    // Grid styling
    lv_obj_set_style_line_color(*chart, lv_color_make(10, 10, 20), LV_PART_MAIN);
    lv_obj_set_style_line_width(*chart, 1, LV_PART_MAIN);
    
    // Add series
    *series = lv_chart_add_series(*chart, lv_color_make(31, 20, 50), LV_CHART_AXIS_PRIMARY_Y);
    
    // Style the line
    lv_obj_set_style_line_width(*chart, 3, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(*chart, LV_OPA_50, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(*chart, lv_color_make(31, 20, 50), LV_PART_ITEMS);
    
    // Initialize
    for (int i = 0; i < 12; i++) {
        lv_chart_set_next_value(*chart, *series, 0);
    }
    
    lv_chart_refresh(*chart);
}

void update_chart(uint16_t temp_value, uint16_t hum_value, uint16_t co2_value)
{

    if (temp_chart && temp_series) {
        lvgl_port_lock(0);
        lv_chart_set_next_value(temp_chart, temp_series, temp_value);
        lv_chart_refresh(temp_chart);
        lvgl_port_unlock();
    }

    if (hum_chart && hum_series) {
        lvgl_port_lock(0);
        lv_chart_set_next_value(hum_chart, hum_series, hum_value);
        lv_chart_refresh(hum_chart);
        lvgl_port_unlock();
    }

    if (co2_chart && co2_series) {
        lvgl_port_lock(0);
        lv_chart_set_next_value(co2_chart, co2_series, co2_value);
        lv_chart_refresh(co2_chart);
        lvgl_port_unlock();
    }
}

void create_sensor_labels()
{
    // Create LVGL timer to update UI every 500ms
    lv_timer_create(lvgl_update_timer_cb, 500, NULL);
}

void on_off_button_task(void *arg)
{
   // Configure GPIO as input
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ON_OFF_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,  // Enable internal pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    bool last_state = true;
    bool current_state;
    bool screen_state = true;
    
    ESP_LOGI(TAG, "Button monitoring started on GPIO %d", ON_OFF_BUTTON);

    while (1) {
        current_state = gpio_get_level(ON_OFF_BUTTON);
        
        if (last_state && !current_state) {
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS));
            current_state = gpio_get_level(ON_OFF_BUTTON);
            
            if (!current_state) {
                ESP_LOGI(TAG, "Power button PRESSED");
                
                screen_state = !screen_state;
                
                // Just toggle the backlight GPIO
                gpio_set_level(PIN_NUM_BK_LIGHT, screen_state ? 1 : 0);
                ESP_LOGI(TAG, "Backlight %s", screen_state ? "ON" : "OFF");
            }
        }
        
        last_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void next_screen_button_task(void *arg)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << NEXT_SCREEN_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    bool last_state = true;
    bool current_state;
    
    ESP_LOGI(TAG, "Button monitoring started on GPIO %d", NEXT_SCREEN_BUTTON);

    while (1) {
        current_state = gpio_get_level(NEXT_SCREEN_BUTTON);
        
        if (last_state && !current_state) {
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS));
            current_state = gpio_get_level(NEXT_SCREEN_BUTTON);
            
            if (!current_state) {
                ESP_LOGI(TAG, "Next Button PRESSED");
                
                // Toggle between screens
                current_screen = (current_screen + 1) % 4;
                if (lvgl_port_lock(0)) {
                    switch (current_screen) {
                        case 0:
                            lv_screen_load(screen_sensor);
                            ESP_LOGI(TAG, "Switched to Sensor screen");
                            break;
                        case 1:
                            lv_screen_load(screen_temp_graph);
                            ESP_LOGI(TAG, "Switched to Temperature graph screen");
                            break;
                        case 2:
                            lv_screen_load(screen_hum_graph);
                            ESP_LOGI(TAG, "Switched to Humidity graph screen");
                            break;
                        case 3:
                            lv_screen_load(screen_co2_graph);
                            ESP_LOGI(TAG, "Switched to CO2 graph screen");
                            break;
                        default:
                            lv_screen_load(screen_sensor);
                            ESP_LOGI(TAG, "Default: Switched to Sensor screen");
                            break;
                    }
                    lvgl_port_unlock();
                }
            }
        }

        last_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
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
            if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                sensor_data = data;
                xSemaphoreGive(data_mutex);
            }
            
            update_chart(data.temperature, data.humidity, data.co2_ppm);
            
            data_buffer[data_buffer_index] = data;

            min_temp = data_buffer[0].temperature;
            max_temp = data_buffer[0].temperature;
            min_co2 = data_buffer[0].co2_ppm;
            max_co2 = data_buffer[0].co2_ppm;
            min_hum = data_buffer[0].humidity;
            max_hum = data_buffer[0].humidity;
            
            data_buffer_index++;
            if (data_buffer_index >= 12) {
                data_buffer_index = 0;
            }

            for (int i = 1; i < 12; i++) {
                if (data_buffer[i].temperature < min_temp) {
                    min_temp = data_buffer[i].temperature;
                }
                if (data_buffer[i].temperature > max_temp) {
                    max_temp = data_buffer[i].temperature;
                }
            }

            for (int i = 1; i < 12; i++) {
                if (data_buffer[i].co2_ppm < min_co2) {
                    min_co2 = data_buffer[i].co2_ppm;
                }
                if (data_buffer[i].co2_ppm > max_co2) {
                    max_co2 = data_buffer[i].co2_ppm;
                }
            }

            for (int i = 1; i < 12; i++) {
                if (data_buffer[i].humidity < min_hum) {
                    min_hum = data_buffer[i].humidity;
                }
                if (data_buffer[i].humidity > max_hum) {
                    max_hum = data_buffer[i].humidity;
                }
            }

        } else {
            ESP_LOGW(TAG, "Failed to read sensor data");
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    // esp_task_wdt_init(10, true);
    data_mutex = xSemaphoreCreateMutex();
    
    init_lcd(LV_DISP_ROT_270);
    
    if (lvgl_port_lock(0)) {
        create_sensor_screen();

        esp_task_wdt_reset();
        // Create temperature graph screen (0-40°C)
        create_graph_screen(&screen_temp_graph, &label_max_temp_data, &label_min_temp_data,
                          &temp_chart, &temp_series, 40);
        
        esp_task_wdt_reset();
        // Create humidity graph screen (0-100%)
        create_graph_screen(&screen_hum_graph, &label_max_hum_data, &label_min_hum_data,
                          &hum_chart, &hum_series, 100);
        
        esp_task_wdt_reset();
        // Create CO2 graph screen (0-2000 ppm)
        create_graph_screen(&screen_co2_graph, &label_max_co2_data, &label_min_co2_data,
                          &co2_chart, &co2_series, 2000);
        esp_task_wdt_reset();
        
        lv_screen_load(screen_sensor);
        
        lvgl_port_unlock();
    }
    
    // Create LVGL timer to update UI every 500ms
    create_sensor_labels();

    
    // Start tasks
    xTaskCreate(scd_task, "scd_task", 4096, NULL, 5, NULL);
    xTaskCreate(on_off_button_task, "on_off_button_task", 4096, NULL, 5, NULL);
    xTaskCreate(next_screen_button_task, "next_screen_button_task", 4096, NULL, 5, NULL);
    
    // LVGL task handler loop
    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
