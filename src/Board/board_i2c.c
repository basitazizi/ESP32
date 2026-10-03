#include "board_i2c.h"
#include "esp_log.h"

esp_err_t board_i2c_init(i2c_master_bus_handle_t *bus)
{
    if (bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    // Match Waveshare's bsp_i2c_init(), without initializing any audio devices.
    const i2c_master_bus_config_t config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = BOARD_I2C_SDA,
        .scl_io_num = BOARD_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
    };
    esp_err_t result = i2c_new_master_bus(&config, bus);
    if (result == ESP_OK) {
        ESP_LOGI("BOARD", "V2 I2C ready: SDA=11 SCL=10");
    }
    return result;
}
