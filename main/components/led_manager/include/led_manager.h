#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include "led_strip.h"

#define LED_STRIP_USE_DMA  0
#define LED_STRIP_LED_COUNT 30
#define LED_STRIP_MEMORY_BLOCK_WORDS 0
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)
#define LED_STRIP_GPIO_PIN  GPIO_NUM_27

#define CLAMP(value, min, max) ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))

typedef struct{
    int red;
    int green;
    int blue;

    int transitionType;
    int brightness;
} led_data_t;

led_strip_handle_t configure_led(void);

esp_err_t led_init();

void led_manager_task(void *pvParameters);

#endif