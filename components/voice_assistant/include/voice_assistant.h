#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <bsp.h>

void start_voice_assistant(void);

#define AFE_CUSTOM_CONFIG()                                                                        \
    {                                                                                              \
        .aec_init = false,                                                                         \
        .se_init = true,                                                                           \
        .vad_init = true,                                                                          \
        .wakenet_init = true,                                                                      \
        .voice_communication_init = false,                                                         \
        .voice_communication_agc_init = false,                                                     \
        .voice_communication_agc_gain = 15,                                                        \
        .vad_mode = VAD_MODE_3,                                                                    \
        .wakenet_model_name = "wn9_sophia_tts",                                                    \
        .wakenet_mode = DET_MODE_2CH_90,                                                           \
        .afe_mode = SR_MODE_HIGH_PERF,                                                             \
        .afe_perferred_core = 0,                                                                   \
        .afe_perferred_priority = 5,                                                               \
        .afe_linear_gain = 1.0,                                                                    \
        .afe_ringbuf_size = 50,                                                                    \
        .memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM,                                          \
        .agc_mode = AFE_MN_PEAK_AGC_MODE_2,                                                        \
        .pcm_config.total_ch_num = 2,                                                              \
        .pcm_config.mic_num = 2,                                                                   \
        .pcm_config.ref_num = 0,                                                                   \
        .pcm_config.sample_rate = 16000,                                                           \
        .debug_init = false,                                                                       \
        .debug_hook = {{AFE_DEBUG_HOOK_MASE_TASK_IN, NULL}, {AFE_DEBUG_HOOK_FETCH_TASK_IN, NULL}}, \
    }
