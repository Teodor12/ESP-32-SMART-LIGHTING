#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "bsp.h"
#include "esp32_s3_devkitc_config.h"
#include "driver/i2s_std.h"
#include "driver/i2s_common.h"

static SemaphoreHandle_t i2s_mutex = NULL;
static i2s_chan_handle_t rx_handle;

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
