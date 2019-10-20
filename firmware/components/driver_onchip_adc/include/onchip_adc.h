#ifndef __ONCHIP_ADC_H__
#define __ONCHIP_ADC_H__

#include "esp_err.h"
#include "driver/adc.h"

typedef struct
{
    esp_adc_cal_characteristics_t adc_characteristics;
    adc1_channel_t channel;
}ADC;

esp_err_t adc_init(ADC* adc_inst, adc1_channel_t adc_channel, adc_bits_width_t adc_width, adc_atten_t adc_attenuation);
esp_err_t get_adc_value(const ADC* adc_inst, uint32_t* value);

#endif