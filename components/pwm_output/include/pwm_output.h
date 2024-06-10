#pragma once

#include "esp_system.h"

esp_err_t init_pwm_outputs(void);

esp_err_t update_pwm_outputs(int command_id);

esp_err_t stop_pwm_outputs(void);



