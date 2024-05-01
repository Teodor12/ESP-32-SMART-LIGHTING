#include "voice_assistant.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_models.h"
#include "esp_afe_config.h"
#include "model_path.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "model_path.h"
#include "esp_process_sdkconfig.h"
#include "esp_log.h"
#include "bsp.h"
#include "led_indicator.h"

#include <stdbool.h>

#define TAG "voice_assistant"

/* Private mutex used for initialization */
static SemaphoreHandle_t sr_init_mutex = NULL;

/* Private AFE-interface to enable functions called by the sr-library */
static esp_afe_sr_iface_t *_afe_handle = NULL;

/* Private AFE data container */
static esp_afe_sr_data_t *_afe_data = NULL;

/* Private wakenet interface */
const esp_wn_iface_t *wakenet = NULL;

/* Private multinet interface */
const esp_mn_iface_t *multinet = NULL;

/* Private multinet interface data */
model_iface_data_t *mn_model_data = NULL;


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
    if (models == NULL) {
        ESP_LOGE(TAG, "Unable to load model(s)");
        return ESP_FAIL;
    }

    for (int i = 0; i < models->num; i++) {
        ESP_LOGI(TAG, "%s", models->model_name[i]);
    }

    /* Select the wakenet model used for speech recognition*/
    char *wn_name = esp_srmodel_filter(models, ESP_WN_PREFIX, "wn9_sophia_tts");
    wakenet = esp_wn_handle_from_name(wn_name);
    model_iface_data_t *wn_model_data = wakenet->create("wn9_sophia_tts", DET_MODE_2CH_90);

    /* Select the multit used for speech recognition*/
    char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_ENGLISH);
    multinet = esp_mn_handle_from_name(mn_name);
    mn_model_data = multinet->create(mn_name, 5000);

    /* Add speech commands from sdkconfig, and print them */
    esp_mn_commands_update_from_sdkconfig(multinet, mn_model_data);

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

    /** Allocating memory for the mapped audio data */
    int16_t *i2s_buff = (int16_t *)calloc(audio_chunksize * feed_channel, sizeof(int16_t)); 
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
    /**
     * The frame length of MultiNet must be equal to the AFE fetch frame length!
     * afe_chunksize = AFE fetch frame length (number of samples(=signed 16-bit values))
     * mu_chunksize = frame length (number of samples(=signed 16-bit values) of chunk that must be passed to Multinet
     */
    int afe_chunksize = _afe_handle->get_fetch_chunksize(afe_data);
    int mu_chunksize = multinet->get_samp_chunksize(mn_model_data);
    assert(mu_chunksize == afe_chunksize);
    bool detect_flag = false;

    led_indicator_t led_indicator = led_indicator_init();
    ESP_ERROR_CHECK(led_indicator_start(led_indicator));

    ESP_LOGI(TAG, "detect_task started");
    while (1) {
        afe_fetch_result_t *res = _afe_handle->fetch(afe_data);
        if (res->wakeup_state == WAKENET_DETECTED) {
            printf("wakeword detected\n");
            multinet->clean(mn_model_data);
            _afe_handle->disable_wakenet(afe_data);
            led_indicator_turn_on(led_indicator);
            detect_flag = true;
        }

        if(detect_flag == true)
        {
            /* Feed the audio sample to multinet model in order to recognize speech commands */
            esp_mn_state_t mn_state = multinet->detect(mn_model_data, res->data);
            if (mn_state == ESP_MN_STATE_DETECTING) {
                continue;
            }

            if (mn_state == ESP_MN_STATE_DETECTED) {
                /* Check the results calculated by Multinet */
                esp_mn_results_t *mn_result = multinet->get_results(mn_model_data);
                for (int i = 0; i < mn_result->num; i++)
                {
                    printf("TOP %d, command_id: %d, phrase_id: %d, string: %s, prob: %f\n",
                           i + 1, mn_result->command_id[i], mn_result->phrase_id[i], mn_result->string, mn_result->prob[i]);
                }
                ESP_LOGI(TAG, "-----------listening-----------\n");
            }

            if (mn_state == ESP_MN_STATE_TIMEOUT) {
                esp_mn_results_t *mn_result = multinet->get_results(mn_model_data);
                /* Re-enable wakenet if the detecting-time of multinet has expired. */
                _afe_handle->enable_wakenet(afe_data);
                detect_flag = false;
                led_indicator_turn_off(led_indicator);
                ESP_LOGI(TAG, "-----------awaits to be waken up-----------\n");
            }
        }


    }
}

static esp_err_t init_sr_tasks(void)
{
    ESP_ERROR_CHECK(sr_flash_models());

    esp_err_t ret = ESP_OK;

    xSemaphoreTake(sr_init_mutex, pdMS_TO_TICKS(10));
    ret = (xTaskCreatePinnedToCore(&feed_task, "feed", 8 * 1024, (void *)_afe_data, 10, NULL, 0) == pdTRUE) ? ESP_OK : ESP_FAIL;
    ret = (xTaskCreatePinnedToCore(&detect_task, "detect", 4 * 1024, (void *)_afe_data, 10, NULL, 1) == pdTRUE) ? ESP_OK : ESP_FAIL;
    xSemaphoreGive(sr_init_mutex);
    return ret;
}

void start_voice_assistant(void)
{
    ESP_ERROR_CHECK(bsp_board_init(16000, 2, 32));
    ESP_ERROR_CHECK(init_sr_tasks());
}
