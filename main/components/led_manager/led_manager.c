#include <stdio.h>
#include "led_manager.h"

#include <cJSON.h>

#include "esp_log.h"
#include "ble_manager.h"
#include "math.h"

static const char *TAG = "LED_MANAGER";

static void fadeOut(led_data_t *rgb_data, int old_brightness, int new_brightness);
static void fadeIn(led_data_t *rgb_data, int old_brightness, int new_brightness);


static led_data_t led_data = {
    .red = 255,
    .green = 0,
    .blue = 0,

    .transitionType = 1,
    .brightness = 50};

static led_strip_handle_t led_handler;

led_strip_handle_t configure_led(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_PIN,                        // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_COUNT,                             // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,                               // LED strip model
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color order of the strip: GRB
        .flags = {
            .invert_out = false, // don't invert the output signal
        }};

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,                    // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ,             // RMT counter clock frequency
        .mem_block_symbols = LED_STRIP_MEMORY_BLOCK_WORDS, // the memory block size used by the RMT channel
        .flags = {
            .with_dma = LED_STRIP_USE_DMA, // Using DMA can improve performance when driving more LEDs
        }};

    // LED Strip object handle
    led_strip_handle_t strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");
    return strip;
}


static void fadeOut(led_data_t *rgb_data, int old_brightness, int new_brightness)
{

    ESP_LOGI(TAG, "FadeOut from %d to %d", old_brightness, new_brightness);

    if(led_handler == NULL) {
        return;
    }

    if(rgb_data == NULL) {
        return;
    }

    for (int brightness = old_brightness; brightness >= new_brightness; brightness-- )
    {
        

        for (int i = 0; i < LED_STRIP_LED_COUNT; i++)
        {
            ESP_ERROR_CHECK(
                led_strip_set_pixel(
                    led_handler, i,
                        CLAMP( ( (rgb_data->red * brightness) / 255 ), 0, 255),
                        CLAMP( ( (rgb_data->green * brightness) / 255 ), 0, 255),
                        CLAMP( ( (rgb_data->blue * brightness) / 255 ), 0, 255)
                    )
                );
        }

        ESP_ERROR_CHECK(
            led_strip_refresh(led_handler));
        vTaskDelay(pdMS_TO_TICKS(8));
        
    }

    return;
}

static void fadeIn(led_data_t *rgb_data, int old_brightness, int new_brightness)
{

    ESP_LOGI(TAG, "FadeIn from %d to %d", old_brightness, new_brightness);

    if(led_handler == NULL) {
        return;
    }

    if(rgb_data == NULL) {
        return;
    }


    for (int brightness = old_brightness; brightness <= new_brightness; brightness++)
    {

        for (int i = 0; i < LED_STRIP_LED_COUNT; i++)
        {
            ESP_ERROR_CHECK(
                led_strip_set_pixel(
                    led_handler, i,
                        CLAMP( ( (rgb_data->red * brightness) / 255 ), 0, 255),
                        CLAMP( ( (rgb_data->green * brightness) / 255 ), 0, 255),
                        CLAMP( ( (rgb_data->blue * brightness) / 255 ), 0, 255)
                    )
                );
        }

        ESP_ERROR_CHECK(
            led_strip_refresh(led_handler));
        vTaskDelay(pdMS_TO_TICKS(8));
    }

    return;
}


static void fadeColor(int red, int green, int blue)
{
    ESP_LOGI(TAG, "Fade color: rgb(%d,%d,%d)", red, green, blue);

    if (led_handler == NULL)
    {
        return;
    }

    led_data_t rgb_data = {
        .red = led_data.red,
        .green = led_data.green,
        .blue = led_data.blue
    };

    fadeOut(&rgb_data, led_data.brightness, 0);

    vTaskDelay(pdMS_TO_TICKS(100));

    rgb_data.red = red;
    rgb_data.green = green;
    rgb_data.blue = blue;

    fadeIn(&rgb_data, 0, led_data.brightness);

    return;
}

static void chaseColor(int red, int green, int blue)
{
    ESP_LOGI(TAG, "Chase color: rgb(%d,%d,%d)", red, green, blue);

    if (led_handler == NULL)
    {
        return;
    }

    int actual_red = (int) CLAMP( ( (red * led_data.brightness) / 255 ), 0, 255);
    int actual_green = (int) CLAMP( ( (green * led_data.brightness) / 255 ), 0, 255);
    int actual_blue = (int) CLAMP( ( (blue * led_data.brightness) / 255 ), 0, 255);

    for (int i = 0; i < LED_STRIP_LED_COUNT; i++)
    {
        ESP_ERROR_CHECK(
            led_strip_set_pixel(
                led_handler, i,
                actual_red,
                actual_green,
                actual_blue));
        ESP_ERROR_CHECK(
            led_strip_refresh(led_handler));
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    return;
}

static void setColor(cJSON *color_json_item)
{

    cJSON *red = cJSON_GetObjectItemCaseSensitive(color_json_item, "red");
    cJSON *green = cJSON_GetObjectItemCaseSensitive(color_json_item, "green");
    cJSON *blue = cJSON_GetObjectItemCaseSensitive(color_json_item, "blue");

    if (
        cJSON_IsNumber(red) &&
        cJSON_IsNumber(green) &&
        cJSON_IsNumber(blue))
    {

        int tmp_r =  CLAMP(red->valueint, 0, 255);
        int tmp_g =  CLAMP(green->valueint, 0, 255);
        int tmp_b =  CLAMP(blue->valueint, 0, 255);
        
        
        switch (led_data.transitionType)
        {
        case 1:
            fadeColor( tmp_r, tmp_g, tmp_b );
            break;

        case 2:
            chaseColor( tmp_r , tmp_g , tmp_b );
            break;

        default:
            break;
        }

        led_data.red = tmp_r;
        led_data.green = tmp_g;
        led_data.blue = tmp_b;
        
    }
    else
    {
        ESP_LOGE(TAG, "Invalid colors");
    }

    return;
}

static void setBrightness(cJSON *brightness_json_item)
{
    if (led_handler == NULL)
    {
        return;
    }

    cJSON *brightness = cJSON_GetObjectItemCaseSensitive(brightness_json_item, "brightness");

    if (
        cJSON_IsNumber(brightness))
    {
        ESP_LOGI(TAG, "Got brightness: %d", brightness->valueint);

        int brightness_value = CLAMP(brightness->valueint, 0, 255);
        
        led_data_t rgb_data = {
            .red = led_data.red,
            .green = led_data.green,
            .blue = led_data.blue
        };

        ESP_LOGI(TAG,"Before Colors R:%d, G:%d, B:%d, Brightness:%d", led_data.red, led_data.green, led_data.blue, led_data.brightness);

        if(brightness_value > led_data.brightness){
            fadeIn( &rgb_data, led_data.brightness, brightness_value );
        }else{
            fadeOut( &rgb_data, led_data.brightness, brightness_value );
        }

        led_data.brightness = brightness_value;

        ESP_LOGI(TAG,"New Colors R:%d, G:%d, B:%d, Brightness:%d", led_data.red, led_data.green, led_data.blue, led_data.brightness);
    }
    else
    {
        ESP_LOGE(TAG, "Invalid brightness");
    }

    return;
}

static void setTransitionType(cJSON *transition_json_item)
{
    cJSON *transition = cJSON_GetObjectItemCaseSensitive(transition_json_item, "transitionType");

    if (
        cJSON_IsNumber(transition))
    {
        ESP_LOGI(TAG, "Got Transition: %d", transition->valueint);

        led_data.transitionType = transition->valueint;
    }
    else
    {
        ESP_LOGE(TAG, "Invalid transition");
    }

    return;
}

static void parseData(const char *json_string)
{
    if (led_handler == NULL)
    {
        ESP_LOGE(TAG, "LED strip not initialized");
        return;
    }

    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL)
    {
        ESP_LOGE(TAG, "LED Manager: Failed to parse JSON data.");
        return;
    }

    cJSON *cmd = cJSON_GetObjectItemCaseSensitive(root, "cmd");
    cJSON *cmd_data = cJSON_GetObjectItemCaseSensitive(root, "data");

    if (cJSON_IsNumber(cmd))
    {

        switch (cmd->valueint)
        {
        case 234: // set led color
            setColor(cmd_data);
            break;

        case 236: // set brightness
            setBrightness(cmd_data);
            break;

        case 237: // set transition
            setTransitionType(cmd_data);
            break;

        default:
            ESP_LOGW(TAG, "LED Manager: INVALID COMMAND: %d", cmd->valueint);
            break;
        }
    }
    else
    {
        ESP_LOGW(TAG, "LED Manager: JSON missing CMD/Data or wrong type.");
    }

    cJSON_Delete(root);
    return;
}

esp_err_t led_init()
{
    led_handler = configure_led();

    if (led_handler == NULL)
    {
        return ESP_FAIL;
    }

    chaseColor(led_data.red, 0, 0);

    return ESP_OK;
}

void led_manager_task(void *pvParameters)
{
    ESP_LOGI(TAG, "LED Manager Task started.");

    ble_data_message_t received_message;

    while (1)
    {
        if (xQueueReceive(ble_data_queue, (void *)&received_message, portMAX_DELAY) == pdPASS)
        {
            ESP_LOGI(TAG, "LED Manager: Received message from queue: %s", received_message.json_data);
            parseData(received_message.json_data);
        }
        else
        {
            ESP_LOGE(TAG, "LED Manager: Failed to receive message from queue (should not happen with portMAX_DELAY).");
        }
    }
    vTaskDelete(NULL);
}