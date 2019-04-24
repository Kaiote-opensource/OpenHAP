#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"


static char* TAG = "adc_sense";

static const adc1_channel_t channel = ADC1_CHANNEL_6;     //GPIO34 with ADC1
static esp_adc_cal_characteristics_t *adc_chars;
static const adc_unit_t unit = ADC_UNIT_1;
static const int averaging_samples = 64;
static const adc_atten_t atten = ADC_ATTEN_DB_6;
static const uint32_t default_vref = 1100;

static void check_efuse()
{
    //Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) 
    {
        ESP_LOGI(TAG,"eFuse Two Point: Supported");
    } 
    else 
    {
        ESP_LOGI(TAG,"eFuse Two Point: NOT supported");
    }

    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        ESP_LOGI(TAG,"eFuse Vref: Supported");
    }
    else 
    {
        ESP_LOGI(TAG,"eFuse Vref: NOT supported");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        ESP_LOGI(TAG,"Characterized using Two Point Value");
    } 
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        ESP_LOGI(TAG,"Characterized using eFuse Vref");
    } 
    else 
    {
        ESP_LOGI(TAG,"Characterized using Default Vref");
    }
}

void init_adc(void)
{
    //Check if Two Point or Vref are burned into eFuse
    check_efuse();

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(channel, atten);

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, atten, ADC_WIDTH_BIT_12, default_vref, adc_chars);
    print_char_val_type(val_type);
}

void get_adc_value(uint32_t* value)
{
        uint32_t adc_reading = 0;
        //Multisampling...apply boxcar averaging
        for (int i = 0; i < averaging_samples; i++)
        {
            adc_reading += adc1_get_raw(channel);
        }
        adc_reading /= averaging_samples;
        //Convert adc_reading to voltage in mV
        *value = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
        ESP_LOGI(TAG, "Raw: %d\tVoltage: %dmv", adc_reading, *value);
}