#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "driver/i2c.h"
#include "i2c-lcd.h"

// ADC Voltage Formula Vout = Dout * Vmax / Dmax
// Vout = Digital Output Result Dout = ADC raw digital reading Vmax = Max input voltage (ADC ATTENUTATION) Dmax = max analog (4095)

//ADC Channels
#if CONFIG_IDF_TARGET_ESP32
#define ADC1_EXAMPLE_CHAN0          ADC1_CHANNEL_5
#define ADC2_EXAMPLE_CHAN0          ADC2_CHANNEL_0
static const char *TAG_CH[2][10] = {{"ADC1_CH5"}, {"ADC2_CH0"}};
#else
#define ADC1_EXAMPLE_CHAN0          ADC1_CHANNEL_2
#define ADC2_EXAMPLE_CHAN0          ADC2_CHANNEL_0
static const char *TAG_CH[2][10] = {{"ADC1_CH2"}, {"ADC2_CH0"}};
#endif

//ADC Attenuation
#define ADC_EXAMPLE_ATTEN           ADC_ATTEN_DB_0

//ADC Calibration
#if CONFIG_IDF_TARGET_ESP32
#define ADC_EXAMPLE_CALI_SCHEME     ESP_ADC_CAL_VAL_EFUSE_VREF
#elif CONFIG_IDF_TARGET_ESP32S2
#define ADC_EXAMPLE_CALI_SCHEME     ESP_ADC_CAL_VAL_EFUSE_TP
#elif CONFIG_IDF_TARGET_ESP32C3
#define ADC_EXAMPLE_CALI_SCHEME     ESP_ADC_CAL_VAL_EFUSE_TP
#elif CONFIG_IDF_TARGET_ESP32S3
#define ADC_EXAMPLE_CALI_SCHEME     ESP_ADC_CAL_VAL_EFUSE_TP_FIT
#endif


static int adc_raw[2][10];
static const char *TAGADC = "ADC SINGLE";

static esp_adc_cal_characteristics_t adc1_chars;
static esp_adc_cal_characteristics_t adc2_chars;

static bool adc_calibration_init(void)
{
    esp_err_t ret;
    bool cali_enable = false;

    ret = esp_adc_cal_check_efuse(ADC_EXAMPLE_CALI_SCHEME);
    if (ret == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW(TAGADC, "Calibration scheme not supported, skip software calibration");
    } else if (ret == ESP_ERR_INVALID_VERSION) {
        ESP_LOGW(TAGADC, "eFuse not burnt, skip software calibration");
    } else if (ret == ESP_OK) {
        cali_enable = true;
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_EXAMPLE_ATTEN, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);
        esp_adc_cal_characterize(ADC_UNIT_2, ADC_EXAMPLE_ATTEN, ADC_WIDTH_BIT_DEFAULT, 0, &adc2_chars);
    } else {
        ESP_LOGE(TAGADC, "Invalid arg");
    }

    return cali_enable;
}

static const char *TAG = "i2c-simple-example";

/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_NUM_0;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_21,
        .scl_io_num = GPIO_NUM_22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

int temp;

void app_main(void)
{
    uint8_t data[2];
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    lcd_init();
    lcd_clear();

    vTaskDelay(pdMS_TO_TICKS(2000));
    lcd_clear();
    vTaskDelay(pdMS_TO_TICKS(2000));
    lcd_put_cur(0,0);
    lcd_send_string("Temp:");
    lcd_put_cur(1,0);
    lcd_send_string("LM35");

    esp_err_t ret = ESP_OK;
    uint32_t voltage = 0;
    bool cali_enable = adc_calibration_init();

    //ADC1 config
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_EXAMPLE_CHAN0, ADC_EXAMPLE_ATTEN));
    //ADC2 config
    ESP_ERROR_CHECK(adc2_config_channel_atten(ADC2_EXAMPLE_CHAN0, ADC_EXAMPLE_ATTEN));

    while (1) {
        adc_raw[0][0] = adc1_get_raw(ADC1_EXAMPLE_CHAN0);
        ESP_LOGI(TAG_CH[0][0], "raw  data: %d", adc_raw[0][0]);
        if (cali_enable) {
            voltage = esp_adc_cal_raw_to_voltage(adc_raw[0][0], &adc1_chars);
            ESP_LOGI(TAG_CH[0][0], "cali data: %" PRIu32 " mV", voltage);
            temp = (int)voltage / 10;

        }
        lcd_clear();
        vTaskDelay(pdMS_TO_TICKS(1000));
	    char buffer[20];
	    sprintf(buffer, "%d DEGREES C", temp);

	    lcd_put_cur(0, 0);
	    lcd_send_string("Temp:");

	    lcd_put_cur(1,0);
	    lcd_send_string(buffer);
        vTaskDelay(pdMS_TO_TICKS(2000));
}}
