#include "ST77916.h"
#include "TCA9554PWR.h"
#include "board_i2c.h"
#include "UI/greeting.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main()
{
    ESP_LOGI("APP", "Starting display...");

    i2c_master_bus_handle_t bus = nullptr;
    ESP_ERROR_CHECK(board_i2c_init(&bus));
    ESP_ERROR_CHECK(EXIO_Init(bus));
    ESP_ERROR_CHECK(LCD_Init());
    ESP_ERROR_CHECK(UI_ShowGreeting());

    ESP_LOGI("APP", "Display initialized.");

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
