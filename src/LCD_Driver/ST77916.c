#include "ST77916.h"
#include "TCA9554PWR.h"
#include "esp_lcd_st77916.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define LCD_OPCODE_READ_CMD 0x0B

static const char *TAG_LCD = "ST77916";
esp_lcd_panel_handle_t panel_handle;
esp_lcd_panel_io_handle_t panel_io_handle;
static SemaphoreHandle_t transfer_done;
static uint16_t *draw_buffer;

// Preserve the official alternate-panel initialization commands verbatim.
static const st77916_lcd_init_cmd_t vendor_specific_init_new[] = {
  {0xF0, (uint8_t []){0x28}, 1, 0},
  {0xF2, (uint8_t []){0x28}, 1, 0},
  {0x73, (uint8_t []){0xF0}, 1, 0},
  {0x7C, (uint8_t []){0xD1}, 1, 0},
  {0x83, (uint8_t []){0xE0}, 1, 0},
  {0x84, (uint8_t []){0x61}, 1, 0},
  {0xF2, (uint8_t []){0x82}, 1, 0},
  {0xF0, (uint8_t []){0x00}, 1, 0},
  {0xF0, (uint8_t []){0x01}, 1, 0},
  {0xF1, (uint8_t []){0x01}, 1, 0},
  {0xB0, (uint8_t []){0x56}, 1, 0},
  {0xB1, (uint8_t []){0x4D}, 1, 0},
  {0xB2, (uint8_t []){0x24}, 1, 0},
  {0xB4, (uint8_t []){0x87}, 1, 0},
  {0xB5, (uint8_t []){0x44}, 1, 0},
  {0xB6, (uint8_t []){0x8B}, 1, 0},
  {0xB7, (uint8_t []){0x40}, 1, 0},
  {0xB8, (uint8_t []){0x86}, 1, 0},
  {0xBA, (uint8_t []){0x00}, 1, 0},
  {0xBB, (uint8_t []){0x08}, 1, 0},
  {0xBC, (uint8_t []){0x08}, 1, 0},
  {0xBD, (uint8_t []){0x00}, 1, 0},
  {0xC0, (uint8_t []){0x80}, 1, 0},
  {0xC1, (uint8_t []){0x10}, 1, 0},
  {0xC2, (uint8_t []){0x37}, 1, 0},
  {0xC3, (uint8_t []){0x80}, 1, 0},
  {0xC4, (uint8_t []){0x10}, 1, 0},
  {0xC5, (uint8_t []){0x37}, 1, 0},
  {0xC6, (uint8_t []){0xA9}, 1, 0},
  {0xC7, (uint8_t []){0x41}, 1, 0},
  {0xC8, (uint8_t []){0x01}, 1, 0},
  {0xC9, (uint8_t []){0xA9}, 1, 0},
  {0xCA, (uint8_t []){0x41}, 1, 0},
  {0xCB, (uint8_t []){0x01}, 1, 0},
  {0xD0, (uint8_t []){0x91}, 1, 0},
  {0xD1, (uint8_t []){0x68}, 1, 0},
  {0xD2, (uint8_t []){0x68}, 1, 0},
  {0xF5, (uint8_t []){0x00, 0xA5}, 2, 0},
  {0xDD, (uint8_t []){0x4F}, 1, 0},
  {0xDE, (uint8_t []){0x4F}, 1, 0},
  {0xF1, (uint8_t []){0x10}, 1, 0},
  {0xF0, (uint8_t []){0x00}, 1, 0},
  {0xF0, (uint8_t []){0x02}, 1, 0},
  {0xE0, (uint8_t []){0xF0, 0x0A, 0x10, 0x09, 0x09, 0x36, 0x35, 0x33, 0x4A, 0x29, 0x15, 0x15, 0x2E, 0x34}, 14, 0},
  {0xE1, (uint8_t []){0xF0, 0x0A, 0x0F, 0x08, 0x08, 0x05, 0x34, 0x33, 0x4A, 0x39, 0x15, 0x15, 0x2D, 0x33}, 14, 0},
  {0xF0, (uint8_t []){0x10}, 1, 0},
  {0xF3, (uint8_t []){0x10}, 1, 0},
  {0xE0, (uint8_t []){0x07}, 1, 0},
  {0xE1, (uint8_t []){0x00}, 1, 0},
  {0xE2, (uint8_t []){0x00}, 1, 0},
  {0xE3, (uint8_t []){0x00}, 1, 0},
  {0xE4, (uint8_t []){0xE0}, 1, 0},
  {0xE5, (uint8_t []){0x06}, 1, 0},
  {0xE6, (uint8_t []){0x21}, 1, 0},
  {0xE7, (uint8_t []){0x01}, 1, 0},
  {0xE8, (uint8_t []){0x05}, 1, 0},
  {0xE9, (uint8_t []){0x02}, 1, 0},
  {0xEA, (uint8_t []){0xDA}, 1, 0},
  {0xEB, (uint8_t []){0x00}, 1, 0},
  {0xEC, (uint8_t []){0x00}, 1, 0},
  {0xED, (uint8_t []){0x0F}, 1, 0},
  {0xEE, (uint8_t []){0x00}, 1, 0},
  {0xEF, (uint8_t []){0x00}, 1, 0},
  {0xF8, (uint8_t []){0x00}, 1, 0},
  {0xF9, (uint8_t []){0x00}, 1, 0},
  {0xFA, (uint8_t []){0x00}, 1, 0},
  {0xFB, (uint8_t []){0x00}, 1, 0},
  {0xFC, (uint8_t []){0x00}, 1, 0},
  {0xFD, (uint8_t []){0x00}, 1, 0},
  {0xFE, (uint8_t []){0x00}, 1, 0},
  {0xFF, (uint8_t []){0x00}, 1, 0},
  {0x60, (uint8_t []){0x40}, 1, 0},
  {0x61, (uint8_t []){0x04}, 1, 0},
  {0x62, (uint8_t []){0x00}, 1, 0},
  {0x63, (uint8_t []){0x42}, 1, 0},
  {0x64, (uint8_t []){0xD9}, 1, 0},
  {0x65, (uint8_t []){0x00}, 1, 0},
  {0x66, (uint8_t []){0x00}, 1, 0},
  {0x67, (uint8_t []){0x00}, 1, 0},
  {0x68, (uint8_t []){0x00}, 1, 0},
  {0x69, (uint8_t []){0x00}, 1, 0},
  {0x6A, (uint8_t []){0x00}, 1, 0},
  {0x6B, (uint8_t []){0x00}, 1, 0},
  {0x70, (uint8_t []){0x40}, 1, 0},
  {0x71, (uint8_t []){0x03}, 1, 0},
  {0x72, (uint8_t []){0x00}, 1, 0},
  {0x73, (uint8_t []){0x42}, 1, 0},
  {0x74, (uint8_t []){0xD8}, 1, 0},
  {0x75, (uint8_t []){0x00}, 1, 0},
  {0x76, (uint8_t []){0x00}, 1, 0},
  {0x77, (uint8_t []){0x00}, 1, 0},
  {0x78, (uint8_t []){0x00}, 1, 0},
  {0x79, (uint8_t []){0x00}, 1, 0},
  {0x7A, (uint8_t []){0x00}, 1, 0},
  {0x7B, (uint8_t []){0x00}, 1, 0},
  {0x80, (uint8_t []){0x48}, 1, 0},
  {0x81, (uint8_t []){0x00}, 1, 0},
  {0x82, (uint8_t []){0x06}, 1, 0},
  {0x83, (uint8_t []){0x02}, 1, 0},
  {0x84, (uint8_t []){0xD6}, 1, 0},
  {0x85, (uint8_t []){0x04}, 1, 0},
  {0x86, (uint8_t []){0x00}, 1, 0},
  {0x87, (uint8_t []){0x00}, 1, 0},
  {0x88, (uint8_t []){0x48}, 1, 0},
  {0x89, (uint8_t []){0x00}, 1, 0},
  {0x8A, (uint8_t []){0x08}, 1, 0},
  {0x8B, (uint8_t []){0x02}, 1, 0},
  {0x8C, (uint8_t []){0xD8}, 1, 0},
  {0x8D, (uint8_t []){0x04}, 1, 0},
  {0x8E, (uint8_t []){0x00}, 1, 0},
  {0x8F, (uint8_t []){0x00}, 1, 0},
  {0x90, (uint8_t []){0x48}, 1, 0},
  {0x91, (uint8_t []){0x00}, 1, 0},
  {0x92, (uint8_t []){0x0A}, 1, 0},
  {0x93, (uint8_t []){0x02}, 1, 0},
  {0x94, (uint8_t []){0xDA}, 1, 0},
  {0x95, (uint8_t []){0x04}, 1, 0},
  {0x96, (uint8_t []){0x00}, 1, 0},
  {0x97, (uint8_t []){0x00}, 1, 0},
  {0x98, (uint8_t []){0x48}, 1, 0},
  {0x99, (uint8_t []){0x00}, 1, 0},
  {0x9A, (uint8_t []){0x0C}, 1, 0},
  {0x9B, (uint8_t []){0x02}, 1, 0},
  {0x9C, (uint8_t []){0xDC}, 1, 0},
  {0x9D, (uint8_t []){0x04}, 1, 0},
  {0x9E, (uint8_t []){0x00}, 1, 0},
  {0x9F, (uint8_t []){0x00}, 1, 0},
  {0xA0, (uint8_t []){0x48}, 1, 0},
  {0xA1, (uint8_t []){0x00}, 1, 0},
  {0xA2, (uint8_t []){0x05}, 1, 0},
  {0xA3, (uint8_t []){0x02}, 1, 0},
  {0xA4, (uint8_t []){0xD5}, 1, 0},
  {0xA5, (uint8_t []){0x04}, 1, 0},
  {0xA6, (uint8_t []){0x00}, 1, 0},
  {0xA7, (uint8_t []){0x00}, 1, 0},
  {0xA8, (uint8_t []){0x48}, 1, 0},
  {0xA9, (uint8_t []){0x00}, 1, 0},
  {0xAA, (uint8_t []){0x07}, 1, 0},
  {0xAB, (uint8_t []){0x02}, 1, 0},
  {0xAC, (uint8_t []){0xD7}, 1, 0},
  {0xAD, (uint8_t []){0x04}, 1, 0},
  {0xAE, (uint8_t []){0x00}, 1, 0},
  {0xAF, (uint8_t []){0x00}, 1, 0},
  {0xB0, (uint8_t []){0x48}, 1, 0},
  {0xB1, (uint8_t []){0x00}, 1, 0},
  {0xB2, (uint8_t []){0x09}, 1, 0},
  {0xB3, (uint8_t []){0x02}, 1, 0},
  {0xB4, (uint8_t []){0xD9}, 1, 0},
  {0xB5, (uint8_t []){0x04}, 1, 0},
  {0xB6, (uint8_t []){0x00}, 1, 0},
  {0xB7, (uint8_t []){0x00}, 1, 0},
  
  {0xB8, (uint8_t []){0x48}, 1, 0},
  {0xB9, (uint8_t []){0x00}, 1, 0},
  {0xBA, (uint8_t []){0x0B}, 1, 0},
  {0xBB, (uint8_t []){0x02}, 1, 0},
  {0xBC, (uint8_t []){0xDB}, 1, 0},
  {0xBD, (uint8_t []){0x04}, 1, 0},
  {0xBE, (uint8_t []){0x00}, 1, 0},
  {0xBF, (uint8_t []){0x00}, 1, 0},
  {0xC0, (uint8_t []){0x10}, 1, 0},
  {0xC1, (uint8_t []){0x47}, 1, 0},
  {0xC2, (uint8_t []){0x56}, 1, 0},
  {0xC3, (uint8_t []){0x65}, 1, 0},
  {0xC4, (uint8_t []){0x74}, 1, 0},
  {0xC5, (uint8_t []){0x88}, 1, 0},
  {0xC6, (uint8_t []){0x99}, 1, 0},
  {0xC7, (uint8_t []){0x01}, 1, 0},
  {0xC8, (uint8_t []){0xBB}, 1, 0},
  {0xC9, (uint8_t []){0xAA}, 1, 0},
  {0xD0, (uint8_t []){0x10}, 1, 0},
  {0xD1, (uint8_t []){0x47}, 1, 0},
  {0xD2, (uint8_t []){0x56}, 1, 0},
  {0xD3, (uint8_t []){0x65}, 1, 0},
  {0xD4, (uint8_t []){0x74}, 1, 0},
  {0xD5, (uint8_t []){0x88}, 1, 0},
  {0xD6, (uint8_t []){0x99}, 1, 0},
  {0xD7, (uint8_t []){0x01}, 1, 0},
  {0xD8, (uint8_t []){0xBB}, 1, 0},
  {0xD9, (uint8_t []){0xAA}, 1, 0},
  {0xF3, (uint8_t []){0x01}, 1, 0},
  {0xF0, (uint8_t []){0x00}, 1, 0},
  {0x21, (uint8_t []){0x00}, 1, 0},
  {0x11, (uint8_t []){0x00}, 1, 120},
  {0x29, (uint8_t []){0x00}, 1, 0},  
};
static bool color_transfer_done(esp_lcd_panel_io_handle_t io,
                                esp_lcd_panel_io_event_data_t *event, void *context)
{
    (void)io;
    (void)event;
    BaseType_t task_woken = pdFALSE;
    xSemaphoreGiveFromISR((SemaphoreHandle_t)context, &task_woken);
    return task_woken == pdTRUE;
}

static esp_err_t panel_init(void)
{
    // Waveshare's hardware reset through the GPIO expander.
    ESP_RETURN_ON_ERROR(Set_EXIO(TCA9554_EXIO2, false), TAG_LCD, "Assert reset failed");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(Set_EXIO(TCA9554_EXIO2, true), TAG_LCD, "Release reset failed");
    vTaskDelay(pdMS_TO_TICKS(50));

    const spi_bus_config_t bus_config = {
        .data0_io_num = ESP_PANEL_LCD_SPI_IO_DATA0,
        .data1_io_num = ESP_PANEL_LCD_SPI_IO_DATA1,
        .sclk_io_num = ESP_PANEL_LCD_SPI_IO_SCK,
        .data2_io_num = ESP_PANEL_LCD_SPI_IO_DATA2,
        .data3_io_num = ESP_PANEL_LCD_SPI_IO_DATA3,
        .data4_io_num = -1, .data5_io_num = -1,
        .data6_io_num = -1, .data7_io_num = -1,
        .max_transfer_sz = ESP_PANEL_HOST_SPI_MAX_TRANSFER_SIZE,
        .flags = SPICOMMON_BUSFLAG_MASTER,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(ESP_PANEL_HOST_SPI_ID_DEFAULT, &bus_config, SPI_DMA_CH_AUTO),
                        TAG_LCD, "Initialize QSPI failed");

    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = ESP_PANEL_LCD_SPI_IO_CS,
        .dc_gpio_num = -1,
        .spi_mode = ESP_PANEL_LCD_SPI_MODE,
        .pclk_hz = 3 * 1000 * 1000,
        .trans_queue_depth = ESP_PANEL_LCD_SPI_TRANS_QUEUE_SZ,
        .lcd_cmd_bits = ESP_PANEL_LCD_SPI_CMD_BITS,
        .lcd_param_bits = ESP_PANEL_LCD_SPI_PARAM_BITS,
        .flags = {.quad_mode = 1},
    };

    // Keep the official 3 MHz ID read and panel-specific sequence selection.
    esp_lcd_panel_io_handle_t read_io;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)ESP_PANEL_HOST_SPI_ID_DEFAULT,
                        &io_config, &read_io), TAG_LCD, "Create ID-read IO failed");
    uint8_t id[4] = {0};
    const int read_id_command = (LCD_OPCODE_READ_CMD << 24) | (0x04 << 8);
    esp_err_t read_result = esp_lcd_panel_io_rx_param(read_io, read_id_command, id, sizeof(id));
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_del(read_io), TAG_LCD, "Release ID-read IO failed");
    ESP_RETURN_ON_ERROR(read_result, TAG_LCD, "Read panel ID failed");
    ESP_LOGI(TAG_LCD, "Panel ID: %02x %02x %02x %02x", id[0], id[1], id[2], id[3]);

    st77916_vendor_config_t vendor_config = {.flags = {.use_qspi_interface = 1}};
    if (id[0] == 0x00 && id[1] == 0x02 && id[2] == 0x7F && id[3] == 0x7F) {
        vendor_config.init_cmds = vendor_specific_init_new;
        vendor_config.init_cmds_size = sizeof(vendor_specific_init_new) / sizeof(vendor_specific_init_new[0]);
        ESP_LOGI(TAG_LCD, "Using Waveshare alternate LCD sequence (case 2)");
    } else {
        if (!(id[0] == 0x00 && id[1] == 0x7F && id[2] == 0x7F && id[3] == 0x7F)) {
            ESP_LOGW(TAG_LCD, "Unrecognized panel ID; using Waveshare's default sequence");
        }
        ESP_LOGI(TAG_LCD, "Using Waveshare default LCD sequence");
    }

    io_config.pclk_hz = ESP_PANEL_LCD_SPI_CLK_HZ;
    io_config.on_color_trans_done = color_transfer_done;
    io_config.user_ctx = transfer_done;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)ESP_PANEL_HOST_SPI_ID_DEFAULT,
                        &io_config, &panel_io_handle), TAG_LCD, "Create display IO failed");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_LCD_PIN_NUM_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = EXAMPLE_LCD_COLOR_BITS,
        .vendor_config = &vendor_config,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st77916(panel_io_handle, &panel_config, &panel_handle),
                        TAG_LCD, "Create panel failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel_handle), TAG_LCD, "Panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel_handle), TAG_LCD, "Panel initialization failed");
    return esp_lcd_panel_disp_on_off(panel_handle, true);
}

static esp_err_t backlight_init(void)
{
    const gpio_config_t gpio_config_backlight = {
        .pin_bit_mask = 1ULL << EXAMPLE_LCD_PIN_NUM_BK_LIGHT,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&gpio_config_backlight), TAG_LCD, "Configure backlight failed");
    ESP_RETURN_ON_ERROR(gpio_set_level(EXAMPLE_LCD_PIN_NUM_BK_LIGHT, 0), TAG_LCD, "Backlight off failed");
    const ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer), TAG_LCD, "Configure backlight timer failed");
    const ledc_channel_config_t channel = {
        .gpio_num = EXAMPLE_LCD_PIN_NUM_BK_LIGHT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
    };
    return ledc_channel_config(&channel);
}

esp_err_t Set_Backlight(uint8_t brightness)
{
    if (brightness > 100) brightness = 100;
    const uint32_t duty = 8191U * brightness / 100U;
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty),
                        TAG_LCD, "Set backlight duty failed");
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static esp_err_t draw_screen(bool pattern, uint16_t solid_color)
{
    ESP_RETURN_ON_FALSE(panel_handle && draw_buffer && transfer_done, ESP_ERR_INVALID_STATE,
                        TAG_LCD, "LCD is not initialized");
    for (int y = 0; y < EXAMPLE_LCD_HEIGHT; y += LCD_TRANSFER_LINES) {
        int lines = EXAMPLE_LCD_HEIGHT - y;
        if (lines > LCD_TRANSFER_LINES) lines = LCD_TRANSFER_LINES;
        for (int row = 0; row < lines; ++row) {
            for (int x = 0; x < EXAMPLE_LCD_WIDTH; ++x) {
                uint16_t color = solid_color;
                if (pattern) {
                    color = x < 120 ? 0xF800 : (x < 240 ? 0x07E0 : 0x001F);
                    const int pixel_y = y + row;
                    if ((x >= 174 && x < 186 && pixel_y >= 135 && pixel_y < 225) ||
                        (pixel_y >= 174 && pixel_y < 186 && x >= 135 && x < 225)) {
                        color = 0xFFFF;
                    }
                }
                // ST77916 expects RGB565 MSB first; ESP32 memory is little endian.
                draw_buffer[row * EXAMPLE_LCD_WIDTH + x] = (color >> 8) | (color << 8);
            }
        }
        ESP_RETURN_ON_ERROR(esp_lcd_panel_draw_bitmap(panel_handle, 0, y, EXAMPLE_LCD_WIDTH,
                            y + lines, draw_buffer), TAG_LCD, "Queue pixel transfer failed");
        // A single internal DMA buffer: never modify/release it while SPI is reading it.
        ESP_RETURN_ON_FALSE(xSemaphoreTake(transfer_done, pdMS_TO_TICKS(2000)) == pdTRUE,
                            ESP_ERR_TIMEOUT, TAG_LCD, "LCD DMA completion timed out at row %d", y);
    }
    return ESP_OK;
}

esp_err_t LCD_FillColor(uint16_t rgb565)
{
    return draw_screen(false, rgb565);
}

esp_err_t LCD_DrawBitmap(int x, int y, int width, int height, const uint16_t *pixels)
{
    ESP_RETURN_ON_FALSE(panel_handle && draw_buffer && transfer_done, ESP_ERR_INVALID_STATE,
                        TAG_LCD, "LCD is not initialized");
    ESP_RETURN_ON_FALSE(pixels && x >= 0 && y >= 0 && width > 0 && height > 0 &&
                        width <= EXAMPLE_LCD_WIDTH && height <= EXAMPLE_LCD_HEIGHT &&
                        x <= EXAMPLE_LCD_WIDTH - width && y <= EXAMPLE_LCD_HEIGHT - height,
                        ESP_ERR_INVALID_ARG, TAG_LCD, "Invalid bitmap rectangle");
    for (int row = 0; row < height; row += LCD_TRANSFER_LINES) {
        int lines = height - row;
        if (lines > LCD_TRANSFER_LINES) lines = LCD_TRANSFER_LINES;
        for (int i = 0; i < width * lines; ++i) {
            const uint16_t color = pixels[row * width + i];
            draw_buffer[i] = (color >> 8) | (color << 8);
        }
        ESP_RETURN_ON_ERROR(esp_lcd_panel_draw_bitmap(panel_handle, x, y + row, x + width,
                            y + row + lines, draw_buffer), TAG_LCD, "Queue bitmap transfer failed");
        ESP_RETURN_ON_FALSE(xSemaphoreTake(transfer_done, pdMS_TO_TICKS(2000)) == pdTRUE,
                            ESP_ERR_TIMEOUT, TAG_LCD, "Bitmap DMA completion timed out");
    }
    return ESP_OK;
}

esp_err_t LCD_ShowTestPattern(void)
{
    return draw_screen(true, 0);
}

esp_err_t LCD_Init(void)
{
    ESP_RETURN_ON_FALSE(!panel_handle && !transfer_done, ESP_ERR_INVALID_STATE,
                        TAG_LCD, "LCD already initialized");
    ESP_RETURN_ON_ERROR(backlight_init(), TAG_LCD, "Backlight initialization failed");
    transfer_done = xSemaphoreCreateBinary();
    ESP_RETURN_ON_FALSE(transfer_done, ESP_ERR_NO_MEM, TAG_LCD, "Allocate completion semaphore failed");
    draw_buffer = heap_caps_malloc(ESP_PANEL_HOST_SPI_MAX_TRANSFER_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    ESP_RETURN_ON_FALSE(draw_buffer, ESP_ERR_NO_MEM, TAG_LCD, "Allocate internal DMA buffer failed");
    ESP_RETURN_ON_ERROR(panel_init(), TAG_LCD, "LCD initialization failed");
    ESP_RETURN_ON_ERROR(LCD_FillColor(0x0000), TAG_LCD, "Clear LCD failed");
    return Set_Backlight(80);
}
