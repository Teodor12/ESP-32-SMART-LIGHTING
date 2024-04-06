#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <bsp.h>

#define TAG "main"

int app_main(void)
{
    ESP_ERROR_CHECK(bsp_board_init(16000, 2, 32));
    ESP_LOGI(TAG, "I2S initialized successfully!");
    return 0;
}
