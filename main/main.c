#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "voice_assistant.h"

#define TAG "main"

void app_main(void)
{
    start_voice_assistant();
}
