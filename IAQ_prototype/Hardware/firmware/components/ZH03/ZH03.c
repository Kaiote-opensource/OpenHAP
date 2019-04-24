#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ZH03.h"

#define RX_BUF_SIZE 1024

static char* TAG = "ZH03";
static int timeout_ms = 1500;

esp_err_t setup_particulate_sensor(ZH03* ZH03_inst, uart_port_t uart_num, int baudrate, int txd_pin, int rxd_pin)
{
    if(ZH03_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if(uart_num > UART_NUM_MAX || uart_num <= UART_NUM_0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;   
    
    uart_config_t uart_config =
    {
        .baud_rate = baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ret = uart_param_config(uart_num, &uart_config);
    if (ret != ESP_OK)
    {
        return ret;
    }
    ret = uart_set_pin(uart_num, txd_pin, rxd_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK)
    {
        return ret;
    }
    ret = uart_driver_install(uart_num, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    if(ret == ESP_OK)
    {
        ZH03_inst->uart_port = uart_num;
    }
    return ret;
}

esp_err_t get_particulate_reading(ZH03* ZH03_inst, int32_t* PM_1, int32_t* PM_2_5, int32_t* PM_10)
{
    uint8_t data[RX_BUF_SIZE]={0};
    uint32_t checksum = 0;
    uint32_t sent_checksum = 0;

    if(ZH03_inst ==NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if(PM_1 == NULL && PM_2_5 == NULL && PM_10 == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int rxBytes = uart_read_bytes(ZH03_inst->uart_port, data, RX_BUF_SIZE, timeout_ms/portTICK_RATE_MS);
    if(rxBytes == -1)
    {
        return ESP_FAIL;
    }
    if (rxBytes >= ZH03_CHECKSUM_LOW_BYTE_POS) 
    {
        ESP_LOGI(TAG, "Read %d bytes", rxBytes);
        ESP_LOG_BUFFER_HEXDUMP(TAG, data, rxBytes, ESP_LOG_INFO);
        for(int i=0; i < rxBytes-1; i++)
        {
            if(data[i] == 0x42)
            {
                ESP_LOGD(TAG, "Found first start of frame byte");
                if(data[i+(ZH03_START_BYTE_2_POS-1)] == 0x4d)
                {
                    ESP_LOGD(TAG, "Found second start of frame byte");
                    for(int j = 0; j < (data[i+(ZH03_FRAME_LENGTH_POS-1)]) + 2; j++)
                    {
                        checksum += data[i+j];
                    }
                    checksum = (checksum)%0xffff;
                    ESP_LOGD(TAG, "Calculated checksum is 0x%04x", (int)checksum);
                    sent_checksum = data[i+(ZH03_CHECKSUM_LOW_BYTE_POS-1)] + (256*data[i+(ZH03_CHECKSUM_HIGH_BYTE_POS-1)]);
                    ESP_LOGD(TAG, "Sent checksum is 0x%04x", (int)checksum);
                    if(checksum != sent_checksum)
                    {
                        ESP_LOGE(TAG, "Checksum failed!");
                        return ESP_ERR_INVALID_CRC;
                    }
                    else
                    {                    
                        if(PM_1 != NULL)
                        {
                            *PM_1 = data[i+(ZH03_PM_1_LOW_BYTE_POS-1)] + (256*data[i+(ZH03_PM_1_HIGH_BYTE_POS-1)]);
                            ESP_LOGD(TAG, "PM 1 value is %d ug/m3", (int)*PM_1);
                        }
                        if(PM_2_5 != NULL)
                        {
                            *PM_2_5 = data[i+(ZH03_PM_2_5_LOW_BYTE_POS-1)] + (256*data[i+(ZH03_PM_2_5_HIGH_BYTE_POS-1)]);
                            ESP_LOGD(TAG, "PM 2.5 value is %d ug/m3", (int)*PM_2_5);
                        }
                        if(PM_10 != NULL)
                        {
                            *PM_10 = data[i+(ZH03_PM_10_LOW_BYTE_POS-1)] + (256*data[i+(ZH03_PM_10_HIGH_BYTE_POS-1)]);
                            ESP_LOGD(TAG, "PM 10 value is %d ug/m3", (int)*PM_10);
                        }
                        return ESP_OK;
                    }
                }

            }
        }
    }
    ESP_LOGE(TAG, "Bytes read are less than frame length");
    return ESP_ERR_INVALID_SIZE;
}
