#include "lvgl.h"
#include "esp_lvgl_port.h"
// Display configuration - adjust these to match your setup
#define LCD_HOST           SPI2_HOST
#define LCD_PIXEL_CLK_HZ   (40 * 1000 * 1000)
#define LCD_BK_LIGHT_ON    1
#define LCD_BK_LIGHT_OFF   0

// Pin definitions - adjust these to match your wiring
#define PIN_NUM_MOSI       23
#define PIN_NUM_CLK        18
#define PIN_NUM_CS         5
#define PIN_NUM_DC         16
#define PIN_NUM_RST        17
#define PIN_NUM_BK_LIGHT   4

// Display resolution
#define LCD_H_RES          240
#define LCD_V_RES          320

// Display rotation
#define LV_DISP_ROT_NONE   0
#define LV_DISP_ROT_90     1
#define LV_DISP_ROT_180    2
#define LV_DISP_ROT_270    3

// LVGL settings
#define LVGL_TICK_PERIOD_MS 2
#define LVGL_BUFFER_HEIGHT  50

void init_lcd(int rotation);
void create_label(const lv_font_t *font, int x, int y, char *text);
void create_background(void);
