#pragma once

#include "driver/i2c_master.h"

// Official V2 board configuration: ESP32_S3_AUDIO_Board.h.
#define BOARD_I2C_SDA GPIO_NUM_11
#define BOARD_I2C_SCL GPIO_NUM_10

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t board_i2c_init(i2c_master_bus_handle_t *bus);

#ifdef __cplusplus
}
#endif
