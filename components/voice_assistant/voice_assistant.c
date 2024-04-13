#include "voice_assistant.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_models.h"
#include "esp_afe_config.h"
#include "model_path.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_log.h"
#include "bsp.h"

#define TAG "voice_assistant"

/* Private mutex used for initialization */
static SemaphoreHandle_t sr_init_mutex = NULL;

/* Private AFE-interface to enable functions called by the sr-library*/
static esp_afe_sr_iface_t *_afe_handle = NULL;

/* Private AFE data container */
static esp_afe_sr_data_t *_afe_data = NULL;

/**
 * Initializing the mutex used in the source file
*/
static esp_err_t init_sr_mutex(void)
{
    sr_init_mutex = xSemaphoreCreateMutex();
    return (sr_init_mutex != NULL) ? ESP_OK : ESP_FAIL;
}

/**
 * Initializing the flash partition and returning all loaded models
 */
static esp_err_t sr_flash_models(void)
{
    ESP_ERROR_CHECK(init_sr_mutex());

    xSemaphoreTake(sr_init_mutex, pdMS_TO_TICKS(0));

    /* Load models from the proper SPIFFS partition of the flash memory */
    srmodel_list_t *models = esp_srmodel_init("model");
    if (models == NULL)
    {
        ESP_LOGE(TAG, "Unable to load model(s)");
        return ESP_FAIL;
    }

    for (int i = 0; i < models->num; i++)
    {
        ESP_LOGI(TAG, "%s\n", models->model_name[i]);
    }

    /* Select the model used for speech recognition*/
    char *wn_name = esp_srmodel_filter(models, ESP_WN_PREFIX, "wn9_hiesp");

    /* Initialize wakenet model */
    const esp_wn_iface_t *wakenet = esp_wn_handle_from_name(wn_name);
    model_iface_data_t *wn_model_data = wakenet->create("wn9_hiesp", DET_MODE_2CH_90);

    /* Initialize the afe_handle */
    _afe_handle = (esp_afe_sr_iface_t *)&ESP_AFE_SR_HANDLE;
    afe_config_t afe_cfg = AFE_CUSTOM_CONFIG();
    _afe_data = _afe_handle->create_from_config(&afe_cfg);

    xSemaphoreGive(sr_init_mutex);

    return ESP_OK;
}

static void feed_task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = _afe_handle->get_feed_chunksize(afe_data);
    int nch = _afe_handle->get_channel_num(afe_data);
    int feed_channel = bsp_board_channel_fmt();
    assert(nch <= feed_channel);

    int16_t *i2s_buff = (int16_t *)calloc(audio_chunksize * feed_channel, sizeof(int16_t)); // allocating memory for the mapped audio data
    assert(i2s_buff);

    ESP_LOGI(TAG, "feed_task started");
    while (1) {
        ESP_ERROR_CHECK(bsp_read_i2s_data(i2s_buff, audio_chunksize * feed_channel * sizeof(int16_t)));
        _afe_handle->feed(afe_data, i2s_buff);
    }

}

static void detect_task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = _afe_handle->get_fetch_chunksize(afe_data);
    int16_t *buff = malloc(afe_chunksize * sizeof(int16_t));
    assert(buff);
    ESP_LOGI(TAG, "detect_task started");

    while (1) {
        afe_fetch_result_t *res = _afe_handle->fetch(afe_data);
        if (res->wakeup_state == WAKENET_DETECTED) {
            printf("wakeword detected\n");
        }
    }
}
esp_err_t flash_models()
{
    return sr_flash_models();
}

void start_voice_assistant(void)
{
    ESP_ERROR_CHECK(bsp_board_init(16000, 2, 32));
    ESP_ERROR_CHECK(flash_models());

    xSemaphoreTake(sr_init_mutex, pdMS_TO_TICKS(0));
    xTaskCreatePinnedToCore(&feed_task, "feed", 8 * 1024, (void *)_afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_task, "detect", 4 * 1024, (void *)_afe_data, 5, NULL, 1);
    xSemaphoreGive(sr_init_mutex);
}
