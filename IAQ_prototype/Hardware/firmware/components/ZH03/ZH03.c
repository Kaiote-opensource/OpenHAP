#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "ZH03.h"

static char* TAG = "ZH03";

void ZH03B_init(void)
{
    uart_config_t uart_config = 
    {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
}

esp_err_t get_particulate_reading(ZH03* ZH03_inst)
{
    uint8_t data[RX_BUF_SIZE]={0};
    uint32_t checksum = 0;
    uint32_t sent_checksum = 0;

    int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1500 / portTICK_RATE_MS);
    if (rxBytes > 0) 
    {
        ESP_LOGI(TAG, "Read %d bytes", rxBytes);
        ESP_LOG_BUFFER_HEXDUMP(TAG, data, rxBytes, ESP_LOG_INFO);
        for(int i=0; i < RX_BUF_SIZE; i++)
        {
            if(data[i] == 0x42)
            {
                ESP_LOGD(TAG, "Found start byte 1");
                if(data[i+(ZH03_START_BYTE_2_POS-1)] == 0x4d)
                {
                    ESP_LOGD(TAG, "Found start byte 2");
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
                        ZH03_inst->PM_1 = ESP_FAIL;
                        ZH03_inst->PM_2_5 = ESP_FAIL;
                        ZH03_inst->PM_10 = ESP_FAIL;
                        return ESP_FAIL;
                    }
                    ZH03_inst->PM_1 = data[i+(ZH03_PM_1_LOW_BYTE_POS-1)] + (256*data[i+(ZH03_PM_1_HIGH_BYTE_POS-1)]);
                    ESP_LOGD(TAG, "PM1 value is %d ug/m3", (int)ZH03_inst->PM_1);
                    ZH03_inst->PM_2_5 = data[i+(ZH03_PM_2_5_LOW_BYTE_POS-1)] + (256*data[i+(ZH03_PM_2_5_HIGH_BYTE_POS-1)]);
                    ESP_LOGD(TAG, "PM2,5 value is %d ug/m3", (int)ZH03_inst->PM_2_5);
                    ZH03_inst->PM_10 = data[i+(ZH03_PM_10_LOW_BYTE_POS-1)] + (256*data[i+(ZH03_PM_10_HIGH_BYTE_POS-1)]);
                    ESP_LOGD(TAG, "PM10 value is %d ug/m3", (int)ZH03_inst->PM_10);
                    return ESP_OK;
                }

            }
        }
    }
    else
    {
        ZH03_inst->PM_1 = ESP_ERR_TIMEOUT;
        ZH03_inst->PM_2_5 = ESP_ERR_TIMEOUT;
        ZH03_inst->PM_10 = ESP_ERR_TIMEOUT;
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}
