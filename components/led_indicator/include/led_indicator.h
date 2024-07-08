#pragma once

typedef struct led_indicator *led_indicator_t;

led_indicator_t led_indicator_init();

esp_err_t led_indicator_start(led_indicator_t led_indicator);

esp_err_t led_indicator_turn_on(led_indicator_t led_indicator);

esp_err_t led_indicator_turn_off(led_indicator_t led_indicator);