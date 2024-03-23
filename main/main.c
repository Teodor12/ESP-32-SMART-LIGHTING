#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "main"


int app_main(void)
{

    ESP_LOGI(TAG, "Hello from ESP-32!");
    return 0;
}
