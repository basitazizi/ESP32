#pragma once

#include <stdint.h>
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

// Pinout and QSPI settings from the working Waveshare ST77916.h.
#define EXAMPLE_LCD_WIDTH 360
#define EXAMPLE_LCD_HEIGHT 360
#define EXAMPLE_LCD_COLOR_BITS 16
#define ESP_PANEL_HOST_SPI_ID_DEFAULT SPI2_HOST
#define ESP_PANEL_LCD_SPI_MODE 0
#define ESP_PANEL_LCD_SPI_CLK_HZ (80 * 1000 * 1000)
#define ESP_PANEL_LCD_SPI_TRANS_QUEUE_SZ 10
#define ESP_PANEL_LCD_SPI_CMD_BITS 32
#define ESP_PANEL_LCD_SPI_PARAM_BITS 8
#define ESP_PANEL_LCD_SPI_IO_TE 18
#define ESP_PANEL_LCD_SPI_IO_SCK 40
#define ESP_PANEL_LCD_SPI_IO_DATA0 46
#define ESP_PANEL_LCD_SPI_IO_DATA1 45
#define ESP_PANEL_LCD_SPI_IO_DATA2 42
#define ESP_PANEL_LCD_SPI_IO_DATA3 41
#define ESP_PANEL_LCD_SPI_IO_CS 21
#define EXAMPLE_LCD_PIN_NUM_RST (-1)  // Reset via TCA9554 EXIO2.
#define EXAMPLE_LCD_PIN_NUM_BK_LIGHT 5
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL 1
#define LCD_TRANSFER_LINES 16
#define ESP_PANEL_HOST_SPI_MAX_TRANSFER_SIZE (EXAMPLE_LCD_WIDTH * LCD_TRANSFER_LINES * 2)

#ifdef __cplusplus
extern "C" {
#endif

extern esp_lcd_panel_handle_t panel_handle;
extern esp_lcd_panel_io_handle_t panel_io_handle;

// Initialize the I2C bus and expander before calling LCD_Init(). Single-task API.
esp_err_t LCD_Init(void);
esp_err_t Set_Backlight(uint8_t brightness);
esp_err_t LCD_FillColor(uint16_t rgb565);
// Draw a tightly packed RGB565 rectangle. Coordinates and sizes are in pixels.
// Single-task API; returns only after DMA has finished reading the pixels.
esp_err_t LCD_DrawBitmap(int x, int y, int width, int height, const uint16_t *pixels);
esp_err_t LCD_ShowTestPattern(void);

#ifdef __cplusplus
}
#endif
