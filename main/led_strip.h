#include <stdint.h>
#include "driver/rmt_encoder.h"


#define RMT_LED_STRIP_RESOLUTION_HZ     10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM          4
#define LED_NUMBERS                     8

void led_strip_fill(uint32_t color);
void led_strip_init(void);



