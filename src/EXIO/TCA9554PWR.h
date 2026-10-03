#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c_master.h"

// Waveshare numbers the expander pins from 1 to 8; EXIO2 is LCD reset.
#define TCA9554_EXIO1 1
#define TCA9554_EXIO2 2
#define TCA9554_ADDRESS 0x20
#define TCA9554_INPUT_REG 0x00
#define TCA9554_OUTPUT_REG 0x01
#define TCA9554_CONFIG_REG 0x03

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t EXIO_Init(i2c_master_bus_handle_t bus);
esp_err_t Set_EXIO(uint8_t pin, bool state);

#ifdef __cplusplus
}
#endif
