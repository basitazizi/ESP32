#pragma once

#include "esp_err.h"

// Call from the application task after LCD_Init().
esp_err_t UI_ShowGreeting();
