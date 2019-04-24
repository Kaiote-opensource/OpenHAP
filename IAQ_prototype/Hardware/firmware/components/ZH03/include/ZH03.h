#ifndef _ZH03_H_
#define _ZH03_H_

#include <esp_err.h>
#include "freertos/semphr.h"
#include "driver/uart.h"

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
    uart_port_t uart_port; 
}ZH03;

/*Assumes 8N1 message format from datasheet*/
esp_err_t setup_particulate_sensor(ZH03* ZH03_inst, uart_port_t uart_num, int baudrate, int txd_pin, int rxd_pin);

esp_err_t set_QA_mode(ZH03* ZH03_inst);

esp_err_t set_initiative_upload_mode(ZH03* ZH03_inst);

esp_err_t set_dormant_mode(ZH03* ZH03_inst);

esp_err_t unset_dormant_mode(ZH03* ZH03_inst);

esp_err_t get_particulate_reading_initiative_upload_mode(ZH03* ZH03_inst, int32_t* PM_1, int32_t* PM_2_5, int32_t* PM_10);

esp_err_t get_particulate_reading_QA_mode(ZH03* ZH03_inst, int32_t* PM_1, int32_t* PM_2_5, int32_t* PM_10);

#endif
