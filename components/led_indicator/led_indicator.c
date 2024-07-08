#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "led_indicator.h"
#include "led_strip.h"

#define NUM_OF_LEDS 1
#define LED_INDICATOR_PIN 38

static const char *TAG = "led_indicator";

static SemaphoreHandle_t led_indicator_mutex = NULL;

struct led_indicator
{
    led_strip_handle_t led_strip;
    bool run;
    bool led_state;
};

static esp_err_t init_led_indicator_mutex(void)
{
    led_indicator_mutex = xSemaphoreCreateMutex();
    return (led_indicator_mutex != NULL) ? ESP_OK : ESP_FAIL;
}

static led_strip_handle_t configure_led(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_INDICATOR_PIN,
        .max_leds = NUM_OF_LEDS,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_spi_config_t spi_config = {
        .clk_src = SPI_CLK_SRC_DEFAULT,
        .flags.with_dma = true,
        .spi_bus = SPI2_HOST,
    };

    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with SPI backend");
    return led_strip;
}

static esp_err_t turn_on(struct led_indicator *led_indicator)
{
    if(xSemaphoreTake(led_indicator_mutex, pdMS_TO_TICKS(5)) != pdTRUE){
        return ESP_FAIL;
    };
    led_indicator->led_state = true;
    xSemaphoreGive(led_indicator_mutex);
    return ESP_OK;
}

static esp_err_t turn_off(struct led_indicator *led_indicator)
{
    if (xSemaphoreTake(led_indicator_mutex, pdMS_TO_TICKS(5)) != pdTRUE){
        return ESP_FAIL;
    };
    led_indicator->led_state = false;
    xSemaphoreGive(led_indicator_mutex);
    return ESP_OK;
}

static void led_indicator_task(void *arg)
{
    led_indicator_t led_indicator = (led_indicator_t) arg;
    led_strip_handle_t led_strip = led_indicator->led_strip;
    led_indicator->run = true;
    while (led_indicator->run) {
        if(led_indicator->led_state){
        /*Setting the LED's color to white*/
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, 32, 32, 32));
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        }

        else{
            ESP_ERROR_CHECK(led_strip_clear(led_strip));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}

led_indicator_t led_indicator_init()
{
    led_indicator_t led_indicator = (led_indicator_t)calloc(1, sizeof(struct led_indicator));
    if(led_indicator == NULL){
        ESP_LOGE(TAG, "memory exhausted");
        return NULL;
    }
    led_indicator->led_strip = configure_led();
    ESP_ERROR_CHECK(init_led_indicator_mutex());
    ESP_ERROR_CHECK(turn_off(led_indicator));
    led_indicator->run = false;
    return led_indicator;
}

esp_err_t led_indicator_start(led_indicator_t led_indicator)
{

    if(led_indicator == NULL) {
        ESP_LOGE(TAG, "led_indicator instance is NULL");
        return ESP_FAIL;
    }

    if(xTaskCreate(&led_indicator_task, "led_indicator_task", 2 * 1024, led_indicator, 5, NULL) != pdTRUE){
        ESP_LOGE(TAG, "error starting led_indicator task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t led_indicator_turn_on(led_indicator_t led_indicator){
    return turn_on(led_indicator);
}

esp_err_t led_indicator_turn_off(led_indicator_t led_indicator){
    return turn_off(led_indicator);
}
