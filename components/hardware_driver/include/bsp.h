#pragma once

#include "esp_err.h"

esp_err_t bsp_board_init(uint32_t sample_rate, int channel_format, int bits_per_chan);

esp_err_t bsp_board_deinit(void);

int bsp_board_channel_fmt(void);
