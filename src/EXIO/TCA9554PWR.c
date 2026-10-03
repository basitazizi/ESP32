#include "TCA9554PWR.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "TCA9554";
static i2c_master_dev_handle_t device;

static esp_err_t read_register(uint8_t reg, uint8_t *value)
{
    return i2c_master_transmit_receive(device, &reg, 1, value, 1, 1000);
}

static esp_err_t write_register(uint8_t reg, uint8_t value)
{
    const uint8_t bytes[] = {reg, value};
    return i2c_master_transmit(device, bytes, sizeof(bytes), 1000);
}

esp_err_t EXIO_Init(i2c_master_bus_handle_t bus)
{
    ESP_RETURN_ON_FALSE(bus && !device, ESP_ERR_INVALID_STATE, TAG, "Invalid bus or already initialized");
    ESP_RETURN_ON_ERROR(i2c_master_probe(bus, TCA9554_ADDRESS, 1000), TAG, "Expander not found at 0x20");
    const i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TCA9554_ADDRESS,
        .scl_speed_hz = 400000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &config, &device), TAG, "Add expander failed");
    // Match Waveshare EXIO_Init(): all eight pins are outputs (configuration 0x00).
    ESP_RETURN_ON_ERROR(write_register(TCA9554_CONFIG_REG, 0x00), TAG, "Configure outputs failed");
    uint8_t value;
    ESP_RETURN_ON_ERROR(read_register(TCA9554_CONFIG_REG, &value), TAG, "Read configuration failed");
    ESP_RETURN_ON_FALSE(value == 0x00, ESP_ERR_INVALID_RESPONSE, TAG, "Output configuration did not stick");
    ESP_LOGI(TAG, "Expander ready at 0x20; EXIO2 controls LCD reset");
    return ESP_OK;
}

esp_err_t Set_EXIO(uint8_t pin, bool state)
{
    ESP_RETURN_ON_FALSE(device, ESP_ERR_INVALID_STATE, TAG, "Expander is not initialized");
    ESP_RETURN_ON_FALSE(pin >= 1 && pin <= 8, ESP_ERR_INVALID_ARG, TAG, "Pin must be 1..8");
    uint8_t value;
    ESP_RETURN_ON_ERROR(read_register(TCA9554_OUTPUT_REG, &value), TAG, "Read output failed");
    const uint8_t mask = 1U << (pin - 1);
    value = state ? (value | mask) : (value & ~mask);
    return write_register(TCA9554_OUTPUT_REG, value);
}
