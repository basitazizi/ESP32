#pragma once

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

#define LV_MEM_CUSTOM 0

#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "esp_timer.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (esp_timer_get_time() / 1000LL)

#define LV_USE_LOG 0

#define LV_FONT_MONTSERRAT_14 1