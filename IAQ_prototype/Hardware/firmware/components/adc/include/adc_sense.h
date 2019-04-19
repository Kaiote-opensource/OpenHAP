#ifndef __ADC_SENSE_H__
#define __ADC_SENSE_H__

void init_adc(void);
esp_err_t get_adc_value(uint32_t* value);

#endif