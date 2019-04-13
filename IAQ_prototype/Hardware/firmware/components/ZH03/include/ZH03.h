#ifndef _ZH03_H_
#define _ZH03_H_

#include <esp_err.h>
#include "freertos/semphr.h"

#define TXD_PIN  (GPIO_NUM_23)
#define RXD_PIN  (GPIO_NUM_22)

#define RX_BUF_SIZE (128)

/*Taken as is from datasheet*/
#define ZH03_START_BYTE_2_POS           2
#define ZH03_FRAME_LENGTH_POS           4
#define ZH03_PM_1_HIGH_BYTE_POS         11
#define ZH03_PM_1_LOW_BYTE_POS          12
#define ZH03_PM_2_5_HIGH_BYTE_POS       13
#define ZH03_PM_2_5_LOW_BYTE_POS        14
#define ZH03_PM_10_HIGH_BYTE_POS        15
#define ZH03_PM_10_LOW_BYTE_POS         16
#define ZH03_CHECKSUM_HIGH_BYTE_POS     23
#define ZH03_CHECKSUM_LOW_BYTE_POS      24

typedef struct
{
    esp_err_t PM_1; 
    esp_err_t PM_2_5; 
    esp_err_t PM_10; 
    SemaphoreHandle_t device_data_mutex; 
}ZH03;

void ZH03B_init();

esp_err_t get_particulate_reading(ZH03* ZH03_inst);

#endif
