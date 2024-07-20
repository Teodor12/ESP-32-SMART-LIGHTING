#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"
#include "pwm_output.h"

#define TAG "pwm_output"

#define DUTY_CYCLE_100  (uint32_t) 256U
#define DUTY_CYCLE_75   (uint32_t) 192U
#define DUTY_CYCLE_50   (uint32_t) 128U
#define DUTY_CYCLE_25   (uint32_t)  64U
#define DUTY_CYCLE_MOOD (uint32_t)  25U
#define DUTY_CYCLE_0    (uint32_t)   0U
#define DUTY_CYCLE_NUM               6U

static SemaphoreHandle_t pwm_output_mtuex = NULL;
typedef enum {
    DUTYC_100_IDX = 0,
    DUTYC_0_IDX,
    DUTYC_25_IDX,
    DUTYC_50_IDX,
    DUTYC_75_IDX,
    DUTYC_MOOD_IDX,
} duty_cycle_idx_t;

static const uint32_t duty_cycle_values[DUTY_CYCLE_NUM] = {DUTY_CYCLE_100, DUTY_CYCLE_0, DUTY_CYCLE_25, DUTY_CYCLE_50, DUTY_CYCLE_75, DUTY_CYCLE_MOOD};

static uint32_t select_duty_cycle_value(duty_cycle_idx_t dutyc_idx) {
    return duty_cycle_values[dutyc_idx % DUTY_CYCLE_NUM];
}

static esp_err_t init_pwm_output_mtuex(void) {
    pwm_output_mtuex = xSemaphoreCreateMutex();
    return (pwm_output_mtuex != NULL) ? ESP_OK : ESP_FAIL;
}

static esp_err_t pwm_output_init(void)
{
    esp_err_t ret = ESP_OK;
    ESP_ERROR_CHECK(init_pwm_output_init_mutex());

    xSemaphoreTake(pwm_output_mtuex, pdMS_TO_TICKS(1));

    ledc_timer_config_t ledc_timer_0 = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ret |= ledc_timer_config(&ledc_timer_0);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "unable to configure ledc timer 0");
        goto end;
    }

    ledc_channel_config_t ledc_channel_0 = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = 4,
        .duty = 0,
        .hpoint = 0
    };

    ret |= ledc_channel_config(&ledc_channel_0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "unable to configure ledc channel 0");
        goto end;
    }

    ledc_timer_config_t ledc_timer_1 = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK};

    ret |= ledc_timer_config(&ledc_timer_1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "unable to configure ledc timer 1");
        goto end;
    }

    ledc_channel_config_t ledc_channel_1 = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_1,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = 5,
        .duty = 0,
        .hpoint = 0
    };

    ret |= ledc_channel_config(&ledc_channel_1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "unable to configure ledc channel 1");
        goto end;
    }

    ESP_LOGI(TAG, "pwm outputs initialized successfully.");

    end:
        xSemaphoreGive(pwm_output_mtuex);
        return ret;
}

static esp_err_t update_duty_cycle(ledc_channel_t led_channel, uint32_t *new_duty_cycle_value)
{
    esp_err_t ret = ESP_OK;
    if(xSemaphoreTake(pwm_output_mtuex, pdMS_TO_TICKS(5))) {
        return ESP_FAIL;
    }
    ret |= ledc_set_duty(LEDC_LOW_SPEED_MODE, led_channel, *new_duty_cycle_value);
    ret |= ledc_update_duty(LEDC_LOW_SPEED_MODE, led_channel);
    xSemaphoreGive(pwm_output_mtuex);
    return ret;
}


static esp_err_t update_outputs(int command_id)
{
    esp_err_t ret = ESP_OK;

    uint32_t duty_cycle_value = select_duty_cycle_value((duty_cycle_idx_t)command_id);

    if(command_id >= 0 && command_id <= 5) {
        ret|= update_duty_cycle(LEDC_CHANNEL_0, &duty_cycle_value);
    }
    else if(command_id >= 6 && command_id <= 11) {
        ret |= update_duty_cycle(LEDC_CHANNEL_1, &duty_cycle_value);
    }
    else{
        ret |= update_duty_cycle(LEDC_CHANNEL_0, &duty_cycle_value);
        ret |= update_duty_cycle(LEDC_CHANNEL_1, &duty_cycle_value);
    }

    return ret;
}

static esp_err_t stop_outputs(void) {
    esp_err_t ret = ESP_OK;
    ret |= ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ret |= ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
    return ret;
}

esp_err_t init_pwm_outputs(void) {
    return pwm_output_init();
}

esp_err_t update_pwm_outputs(int command_id) {
    return update_outputs(command_id);
}

esp_err_t stop_pwm_outputs(void) {
    return stop_outputs();
}
