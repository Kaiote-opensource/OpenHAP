#ifndef _UART_ZH03_H_
#define _UART_ZH03_H_

#include <esp_err.h>
#include "driver/uart.h"

typedef struct
{
    uart_port_t uart_port; 
}ZH03;

esp_err_t zh03_init(ZH03* zh03_inst, uart_port_t uart_num, int txd_pin, int rxd_pin);

esp_err_t zh03_sleep(const ZH03* zh03_inst);

esp_err_t zh03_wake(const ZH03* zh03_inst);

esp_err_t set_QA_mode(const ZH03* zh03_inst);

esp_err_t get_QA_measurement(const ZH03* zh03_inst, uint16_t* PM1, uint16_t* PM2_5, uint16_t* PM10);

esp_err_t set_initiative_upload_mode(const ZH03* zh03_inst);

esp_err_t get_buffered_initiative_upload_measurement(const ZH03* zh03_inst, uint16_t* PM1, uint16_t* PM2_5, uint16_t* PM10);

#endif
