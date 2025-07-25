#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include "sdkconfig.h"

#include "ble_manager.h"
#include "led_manager.h"

static const char *TAG = "LIGHTBOX";

QueueHandle_t ble_data_queue;

void app_main(void)
{

    ESP_LOGI(TAG, "Application Starting. Creating tasks...");

    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    if (ret != ESP_OK)
    {
        return;
    }

    ble_data_queue = xQueueCreate(5, sizeof(ble_data_message_t));

    if (ble_data_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create queue. Assert will follow in LED task if it tries to use it.");
        return;
    }

    ESP_LOGI(TAG, "Queue created successfully. Handle: %p", ble_data_queue);

     if (led_init() == 0) {
        ESP_LOGE(TAG, "Failed to initialize leds.");
        return;
    }

    ble_init();

    xTaskCreate(&led_manager_task, "led_manager", 4096, NULL, 4, NULL);

    ESP_LOGI(TAG, "Tasks created. Application running.");

    return;
    
}
