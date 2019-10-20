#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "onchip_adc.h"

static const char* TAG = "ADC";
static const int averaging_samples = 64;

static void check_efuse(void)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    /**
    * Check TP is burned into eFuse
    */
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) 
    {
        ESP_LOGI(TAG,"eFuse Two Point: Supported");
    } 
    else 
    {
        ESP_LOGI(TAG,"eFuse Two Point: NOT supported");
    }

    /**
    * Check Vref is burned into eFuse
    */
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        ESP_LOGI(TAG,"eFuse Vref: Supported");
    }
    else 
    {
        ESP_LOGI(TAG,"eFuse Vref: NOT supported");
    }
}

/**
*Initialise ADC 1 
*Initialisation only supports ADC 1 as ADC2 is used internally by the wifi driver to monitor RF output power when active
*/
esp_err_t adc_init(ADC* adc_inst, adc1_channel_t adc_channel, adc_bits_width_t adc_width, adc_atten_t adc_attenuation)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(adc_inst == NULL adc_channel >= ADC1_CHANNEL_MAX || adc_width >= ADC_WIDTH_MAX ||  adc_attenuation >= ADC_ATTEN_MAX)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /**
    * Check if Two Point or Vref are burned into eFuse
    */
    check_efuse();

    ;
    ;

    if(adc1_config_width(adc_width) !=ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set ADC data width");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Successfully initialised ADC peripheral sample width")
    if(adc1_config_channel_atten(adc_channel, adc_attenuation) !=ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set ADC channel %d attenuation", (int)adc_channel);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Successfully initialised ADC channel attenuation", (int)adc_channel)
    adc_inst->adc_characteristics = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    adc_inst->channel = adc_channel;

    /**
    * Characterize ADC
    */
    esp_adc_cal_value_t characterisation_type = esp_adc_cal_characterize(ADC_UNIT_1, adc_attenuation, adc_width, 1100, adc_inst->adc_characteristics);
    if (characterisation_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        ESP_LOGI(TAG,"ADC characterized using Two Point Value");
    }

    else if (characterisation_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        ESP_LOGI(TAG,"ADC characterized using eFuse Vref");
    }
     
    else 
    {
        ESP_LOGI(TAG,"ADC characterized using Default Vref");
    }

    return ESP_OK;
}

esp_err_t get_adc_value(const ADC* adc_inst, uint32_t* value)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(adc_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t adc_reading = 0;

    for (int i = 0; i < averaging_samples; i++)
    {
        adc_reading += adc1_get_raw(adc_inst->channel);
    }
    
    /**
    * Multisampling...apply boxcar averaging
    */
    adc_reading /= averaging_samples;
    /**
    * Convert raw adc_reading to millivolts
    */
    adc_reading = esp_adc_cal_raw_to_voltage(adc_reading, adc_inst->adc_characteristics);
    ESP_LOGI(TAG, "ADC Voltage: %dmv", adc_reading);
    *value = adc_reading;
    return ESP_OK;
}