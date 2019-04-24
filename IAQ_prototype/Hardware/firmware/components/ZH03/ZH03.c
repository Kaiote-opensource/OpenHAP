#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ZH03.h"

#define RX_BUF_SIZE 1024
#define ZH03B_PACKET_LENGTH 9

static char* TAG = "ZH03";
static int timeout_ms = 1500;

esp_err_t setup_particulate_sensor(ZH03* ZH03_inst, uart_port_t uart_num, int baudrate, int txd_pin, int rxd_pin)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

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

esp_err_t set_QA_mode(ZH03* ZH03_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

    const uint8_t set_QA_command[ZH03B_PACKET_LENGTH] = {0xFF, 0x01, 0x78, 0x41, 0x00, 0x00, 0x00, 0x00, 0x46};

    int txBytes = uart_write_bytes(ZH03_inst->uart_port, (char*)set_QA_command, ZH03B_PACKET_LENGTH);

    if(txBytes != ZH03B_PACKET_LENGTH)
    {
        ESP_LOGE(TAG, "'set QA' command failed, command length was %d bytes, wrote %d bytes", ZH03B_PACKET_LENGTH, txBytes);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Wrote all 'set QA' command bytes successfully");
    return ESP_OK;
}

esp_err_t set_initiative_upload_mode(ZH03* ZH03_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

    const uint8_t set_initiative_upload_command[ZH03B_PACKET_LENGTH] = {0xFF, 0x01, 0x78, 0x40, 0x00, 0x00, 0x00, 0x00, 0x47};

    int txBytes = uart_write_bytes(ZH03_inst->uart_port, (char*)set_initiative_upload_command, ZH03B_PACKET_LENGTH);

    if(txBytes != ZH03B_PACKET_LENGTH)
    {
        ESP_LOGE(TAG, "'set initiative upload' command failed, command length was %d bytes, wrote %d bytes", ZH03B_PACKET_LENGTH, txBytes);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Wrote all 'set initiative upload' command bytes successfully");
    return ESP_OK;
}

esp_err_t set_dormant_mode(ZH03* ZH03_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

    uint8_t data[RX_BUF_SIZE]={0};
    uint8_t checksum = 0;
    uint8_t sent_checksum = 0;

    const uint8_t set_dormant_command[ZH03B_PACKET_LENGTH] = {0xFF, 0x01, 0xA7, 0x01, 0x00, 0x00, 0x00, 0x00, 0x57};

    int txBytes = uart_write_bytes(ZH03_inst->uart_port, (char*)set_dormant_command, ZH03B_PACKET_LENGTH);

    if(txBytes != ZH03B_PACKET_LENGTH)
    {
        ESP_LOGE(TAG, "'set dormant' command failed, command length was %d bytes, wrote %d bytes", ZH03B_PACKET_LENGTH, txBytes);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Wrote all 'set dormant' command bytes successfully");

    int rxBytes = uart_read_bytes(ZH03_inst->uart_port, data, RX_BUF_SIZE, 1000/portTICK_RATE_MS);

    if(rxBytes == -1)
    {
        ESP_LOGI(TAG, "Could not get 'set dormant' command response bytes from port");
        return ESP_FAIL;
    }
    if (rxBytes >= ZH03B_PACKET_LENGTH) 
    {
        ESP_LOGI(TAG, "Read %d bytes", rxBytes);
        ESP_LOG_BUFFER_HEXDUMP(TAG, data, rxBytes, ESP_LOG_INFO);
        for(int i=0; i < rxBytes-1; i++)
        {
            if(data[i] == 0xFF)
            {
                ESP_LOGD(TAG, "Found first start of frame byte");
                if(data[i+1] == 0xA7)
                {
                    ESP_LOGD(TAG, "Found 'main command' byte");
                    for(int j = 1; j < ZH03B_PACKET_LENGTH-1; j++)
                    {
                        checksum += data[i+j];
                    }
                    /*Take two's complement*/
                    checksum = ~checksum;
                    checksum += 1;
                    ESP_LOGD(TAG, "Calculated checksum is 0x%02x", (int)checksum);
                    sent_checksum = data[i+(ZH03B_PACKET_LENGTH-1)];
                    ESP_LOGD(TAG, "Sent checksum is 0x%02x", (int)sent_checksum);
                    if(checksum != sent_checksum)
                    {
                        ESP_LOGE(TAG, "Checksum failed!");
                        return ESP_ERR_INVALID_CRC;
                    }
                    else
                    {                    
                        if(data[i+2] != 0x01)
                        {
                            ESP_LOGE(TAG, "'set dormant' command set returned failure from device! Retry again");
                            return ESP_FAIL;
                        }
                        return ESP_OK;
                    }
                }

            }
        }
    }
    return ESP_OK;
}

esp_err_t unset_dormant_mode(ZH03* ZH03_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

    uint8_t data[RX_BUF_SIZE]={0};
    uint8_t checksum = 0;
    uint8_t sent_checksum = 0;

    const uint8_t unset_dormant_command[ZH03B_PACKET_LENGTH] = {0xFF, 0x01, 0xA7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58};

    int txBytes = uart_write_bytes(ZH03_inst->uart_port, (char*)unset_dormant_command, ZH03B_PACKET_LENGTH);

    if(txBytes != ZH03B_PACKET_LENGTH)
    {
        ESP_LOGE(TAG, "'unset dormant' command failed, command length was %d bytes, wrote %d bytes", ZH03B_PACKET_LENGTH, txBytes);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Wrote all 'unset dormant' command bytes successfully");

    int rxBytes = uart_read_bytes(ZH03_inst->uart_port, data, RX_BUF_SIZE, 1000/portTICK_RATE_MS);

    if(rxBytes == -1)
    {
        ESP_LOGI(TAG, "Could not get 'unset dormant' command response bytes from port");
        return ESP_FAIL;
    }
    if (rxBytes >= ZH03B_PACKET_LENGTH) 
    {
        ESP_LOGI(TAG, "Read %d bytes", rxBytes);
        ESP_LOG_BUFFER_HEXDUMP(TAG, data, rxBytes, ESP_LOG_INFO);
        for(int i=0; i < rxBytes-1; i++)
        {
            if(data[i] == 0xFF)
            {
                ESP_LOGD(TAG, "Found first start of frame byte");
                if(data[i+1] == 0xA7)
                {
                    ESP_LOGD(TAG, "Found 'main command' byte");
                    for(int j = 1; j < ZH03B_PACKET_LENGTH-1; j++)
                    {
                        checksum += data[i+j];
                    }
                    /*Take two's complement*/
                    checksum = ~checksum;
                    checksum += 1;
                    ESP_LOGD(TAG, "Calculated checksum is 0x%02x", (int)checksum);
                    sent_checksum = data[i+(ZH03B_PACKET_LENGTH-1)];
                    ESP_LOGD(TAG, "Sent checksum is 0x%02x", (int)sent_checksum);
                    if(checksum != sent_checksum)
                    {
                        ESP_LOGE(TAG, "Checksum failed!");
                        return ESP_ERR_INVALID_CRC;
                    }
                    else
                    {                    
                        if(data[i+2] != 0x01)
                        {
                            ESP_LOGE(TAG, "'unset dormant' command set returned failure from device! Retry again");
                            return ESP_FAIL;
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

esp_err_t get_particulate_reading_initiative_upload_mode(ZH03* ZH03_inst, int32_t* PM_1, int32_t* PM_2_5, int32_t* PM_10)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

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
                    ESP_LOGD(TAG, "Sent checksum is 0x%04x", (int)sent_checksum);
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

esp_err_t get_particulate_reading_QA_mode(ZH03* ZH03_inst, int32_t* PM_1, int32_t* PM_2_5, int32_t* PM_10)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    uint8_t data[RX_BUF_SIZE]={0};
    uint8_t checksum = 0;
    uint8_t sent_checksum = 0;

    const uint8_t QA_command[ZH03B_PACKET_LENGTH] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

    int txBytes = uart_write_bytes(ZH03_inst->uart_port, (char*)QA_command, ZH03B_PACKET_LENGTH);

    if(txBytes != ZH03B_PACKET_LENGTH)
    {
        ESP_LOGE(TAG, "'QA' command failed, command length was %d bytes, wrote %d bytes", ZH03B_PACKET_LENGTH, txBytes);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Wrote all 'QA' command bytes successfully");

    int rxBytes = uart_read_bytes(ZH03_inst->uart_port, data, RX_BUF_SIZE, 1000/portTICK_RATE_MS);

    if(rxBytes == -1)
    {
        ESP_LOGI(TAG, "Could not get 'QA' command response bytes from port");
        return ESP_FAIL;
    }
    if (rxBytes >= ZH03B_PACKET_LENGTH) 
    {
        ESP_LOGI(TAG, "Read %d bytes", rxBytes);
        ESP_LOG_BUFFER_HEXDUMP(TAG, data, rxBytes, ESP_LOG_INFO);
        for(int i=0; i < rxBytes-1; i++)
        {
            if(data[i] == 0xFF)
            {
                ESP_LOGD(TAG, "Found first start of frame byte");
                if(data[i+1] == 0x86)
                {
                    ESP_LOGD(TAG, "Found 'main command' byte");
                    for(int j = 1; j < ZH03B_PACKET_LENGTH-1; j++)
                    {
                        checksum += data[i+j];
                    }
                    /*Take two's complement*/
                    checksum = ~checksum;
                    checksum += 1;
                    ESP_LOGD(TAG, "Calculated checksum is 0x%02x", (int)checksum);
                    sent_checksum = data[i+(ZH03B_PACKET_LENGTH-1)];
                    ESP_LOGD(TAG, "Sent checksum is 0x%02x", (int)sent_checksum);
                    if(checksum != sent_checksum)
                    {
                        ESP_LOGE(TAG, "Checksum failed!");
                        return ESP_ERR_INVALID_CRC;
                    }
                    else
                    {                    
                        if(PM_1 != NULL)
                        {
                            *PM_1 = data[i+3] + (256*data[i+2]);
                            ESP_LOGD(TAG, "PM 1 value is %d ug/m3", (int)*PM_1);
                        }
                        if(PM_2_5 != NULL)
                        {
                            *PM_2_5 = data[i+7] + (256*data[i+6]);
                            ESP_LOGD(TAG, "PM 2.5 value is %d ug/m3", (int)*PM_2_5);
                        }
                        if(PM_10 != NULL)
                        {
                            *PM_10 = data[i+5] + (256*data[i+4]);
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
