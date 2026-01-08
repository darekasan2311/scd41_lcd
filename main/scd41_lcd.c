#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
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
static lv_obj_t *screen_sensor = NULL;
static lv_obj_t *screen_temp_graph = NULL;
static int current_screen = 0;  // 0 = sensor screen, 1 = temp graph screen

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

// Create temp graph screen
void create_temp_graph_screen()
{
    extern lv_font_t jet_mono_light_32;
    
    // Create temp screen
    screen_temp_graph = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_temp_graph, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_temp_graph, LV_OPA_COVER, LV_PART_MAIN);
    
    // Add text
    lv_obj_t *label_temp_graph = lv_label_create(screen_temp_graph);
    lv_label_set_text(label_temp_graph, "Temp Graph");
    lv_obj_set_style_text_font(label_temp_graph, &jet_mono_light_32, 0);
    lv_obj_set_style_text_color(label_temp_graph, lv_color_make(31, 31, 63), 0);
    lv_obj_align(label_temp_graph, LV_ALIGN_TOP_MID, 0, 0);  // Center the text
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
                ESP_LOGI(TAG, "Button PRESSED");
                
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
                current_screen = (current_screen + 1) % 2;  // Toggle between 0 and 1
                
                // Switch screen (must be done in LVGL lock)
                if (lvgl_port_lock(0)) {
                    if (current_screen == 0) {
                        lv_screen_load(screen_sensor);
                        ESP_LOGI(TAG, "Switched to Sensor screen");
                    } else {
                        lv_screen_load(screen_temp_graph);
                        ESP_LOGI(TAG, "Switched to Temp screen");
                    }
                    lvgl_port_unlock();
                }
            }
        }

        last_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(10));
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
            // ESP_LOGI(TAG, "CO2: %d ppm, Temperature: %.1f°C, Humidity: %.1f%%",
            //          data.co2_ppm, data.temperature, data.humidity);
            
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
    
    // Lock LVGL before creating screens
    if (lvgl_port_lock(0)) {
        // Create both screens
        create_sensor_screen();
        create_temp_graph_screen();
        
        // Load the sensor screen initially
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
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
