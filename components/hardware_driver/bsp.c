#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "bsp.h"
#include "esp_log.h"
#include "esp32_s3_devkitc_config.h"
#include "driver/i2s_std.h"
#include "driver/i2s_common.h"

#define TAG "bsp"

static SemaphoreHandle_t i2s_mutex = NULL;
static i2s_chan_handle_t rx_handle = NULL;

static esp_err_t init_i2s_mutex(void)
{
    i2s_mutex = xSemaphoreCreateMutex();
    return (i2s_mutex != NULL) ? ESP_OK : ESP_FAIL;
}

static esp_err_t i2s_init(i2s_port_t i2s_port_num, uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    esp_err_t ret = ESP_OK;

    ESP_ERROR_CHECK(init_i2s_mutex());

    xSemaphoreTake(i2s_mutex, pdMS_TO_TICKS(0));

    /* Specifiy I2S role and i2s port*/
    i2s_chan_config_t channel_cfg = I2S_CHANNEL_DEFAULT_CONFIG(i2s_port_num, I2S_ROLE_MASTER);
    ret |= i2s_new_channel(&channel_cfg, NULL, &rx_handle);

    /* Specifying gpio-pins, slot config, etc. */
    i2s_std_config_t std_cfg = I2S_CONFIG_DEFAULT(sample_rate, channel_format, bits_per_chan);

    ret |= i2s_channel_init_std_mode(rx_handle, &std_cfg);

    ret |= i2s_channel_enable(rx_handle);

    xSemaphoreGive(i2s_mutex);

    return ret;
}

static esp_err_t i2s_read_data(int16_t *buffer, int buffer_size)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_read;

    /* buffer_size in measured in bytes, so the number of samples = buffer_size / sizeof(int16_t), number of 16-bit integers */
    int sample_num = (buffer_size / sizeof(int16_t));

    /**
     * The microphones output is 24-bit 2's complement data, the upper 8-bits are undefined due to the high-impedance state.
     * The allocated memory for the microphone's output will store sample_num 32-bit integers (same amount of sample as param. buffer)
    */
    int32_t *temp_buffer = (int32_t *)calloc(sample_num, sizeof(int32_t));
    if (temp_buffer == NULL) {
        ret = ESP_ERR_NO_MEM;
        return ret;
    }

    ret = i2s_channel_read(rx_handle, temp_buffer, sample_num * sizeof(int32_t), &bytes_read, portMAX_DELAY);
    if (ret != ESP_OK) {
        return ret;
    }

    /**
     * Mapping the the upper 24-bit microphone data into the 16-bit int buffer.
     * Discarding the lower 8 high-impedance bit, and shifting the upper 24-bit values into 16-bit integers.
    */
    for (int i = 0; i < sample_num; i++) {
        buffer[i] = (temp_buffer[i] >> 16);
    }
    free(temp_buffer);
    return ret;
}

static esp_err_t i2s_deinit(i2s_port_t i2s_num)
{
    esp_err_t ret = ESP_OK;

    xSemaphoreTake(i2s_mutex, pdMS_TO_TICKS(0));
    ret |= i2s_channel_disable(rx_handle);
    ret |= i2s_del_channel(rx_handle);
    rx_handle = NULL;
    xSemaphoreGive(i2s_mutex);

    return ret;
}

static int i2s_get_channel_fmt(void)
{
    return STEREO_FMT;
}

esp_err_t bsp_board_init(uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    esp_err_t ret = ESP_OK;
    ret |= i2s_init(I2S_NUM_1, sample_rate, channel_format, bits_per_chan);
    if(ret == ESP_OK){
        ESP_LOGI(TAG, "I2S peripherial intialized successfully.");
    }
    else{
        ESP_LOGI(TAG, "Error while initializing I2S peripherial: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t bsp_read_i2s_data(int16_t *buffer, int buffer_size)
{
    esp_err_t ret = ESP_OK;
    ret |= i2s_read_data(buffer, buffer_size);
    return ret;
}

esp_err_t bsp_board_deinit(void)
{
    esp_err_t ret = ESP_OK;
    ret |= i2s_deinit(I2S_NUM_1);
    return ret;
}

int bsp_board_channel_fmt(void)
{
    return i2s_get_channel_fmt();
}
