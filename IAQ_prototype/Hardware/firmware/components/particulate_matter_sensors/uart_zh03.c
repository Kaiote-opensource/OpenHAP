#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "uart_zh03.h"

/**
 * Particulate measurement frame layout
 */
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

#define RX_BUF_SIZE 1024
#define ZH03_CMD_FRAME_LENGTH 9

static const char* TAG = "ZH03";
/**
 * As the ZH03 sends 1 measurement frame per second asynchronously in initiative upload, we adjust this timeout to 1.5 the interval
 */
static const int ZH03B_UART_TIMEOUT = 1500;
/**
 * Response identifiers
 */
static const uint8_t measurement_query_response_id = 0x86;
static const uint8_t dormancy_state_change_response_id = 0xA7;
/**
 * Dormancy state change command responses
 */
static const uint8_t dormancy_state_change_success = 0x01;
static const uint8_t dormancy_state_change_fail = 0x00;

/**
 * ZH03 command frames
 */
static uint8_t set_QA_mode_cmd_frame[ZH03_CMD_FRAME_LENGTH] = {0xFF, 0x01, 0x78, 0x41, 0x00, 0x00, 0x00, 0x00, 0x46};
static uint8_t set_initiative_upload_mode_cmd_frame[ZH03_CMD_FRAME_LENGTH] = {0xFF, 0x01, 0x78, 0x40, 0x00, 0x00, 0x00, 0x00, 0x47};
static uint8_t set_dormant_mode_cmd_frame[ZH03_CMD_FRAME_LENGTH] = {0xFF, 0x01, 0xA7, 0x01, 0x00, 0x00, 0x00, 0x00, 0x57};
static uint8_t unset_dormant_mode_cmd_frame[ZH03_CMD_FRAME_LENGTH] = {0xFF, 0x01, 0xA7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58};
static uint8_t measurement_query_frame[ZH03_CMD_FRAME_LENGTH] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

esp_err_t zh03_init(ZH03* zh03_inst, uart_port_t uart_num, int txd_pin, int rxd_pin)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(zh03_inst == NULL || uart_num > UART_NUM_MAX)
    {
        return ESP_ERR_INVALID_ARG;
    }
    
    uart_config_t uart_config =
    {
        /**
        * 9600 baud 8N1
        */
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    esp_err_t ret = uart_param_config(uart_num, &uart_config);
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
        zh03_inst->uart_port = uart_num;
    }
    return ret;
}

static esp_err_t zh03_uart_write(const ZH03* zh03_inst, const void* out_data, size_t out_size)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);  
    if(zh03_inst == NULL || out_data == NULL || out_size == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int tx_bytes = uart_write_bytes(zh03_inst->uart_port, (char*)out_data, out_size);
    if(tx_bytes != out_size)
    {
        ESP_LOGE(TAG, "Uart write failed, length is %d bytes, wrote %d bytes", command_name, out_size, tx_bytes);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Wrote all Uart bytes successfully", command_name);       
    return ret;    
}

static esp_err_t zh03_uart_read(const ZH03* zh03_inst, void* in_data, int* in_size)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);  
    if(zh03_inst == NULL || in_data == NULL || in_size == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int rx_bytes = uart_read_bytes(zh03_inst->uart_port, (uint8_t*)in_data, RX_BUF_SIZE, ZH03B_UART_TIMEOUT/portTICK_RATE_MS);
    if(rx_bytes == -1)
    {
        ESP_LOGE(TAG, "Failed to receive Uart bytes");
        return ESP_FAIL;
    }

    *in_size = rx_bytes;
    return ESP_OK;
}

static esp_err_t zh03_command_response_parse(void* in_data, int* in_size)
{
    if(in_data == NULL || in_size == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t calculated_checksum= 0;
    uint8_t sent_checksum = 0;
    int rx_bytes = *in_size;

    if (rx_bytes >= ZH03_CMD_FRAME_LENGTH) 
    {
        ESP_LOGI(TAG, "Buffered Uart data is %d bytes long", rx_bytes);
        ESP_LOG_BUFFER_HEXDUMP(TAG, in_data, rx_bytes, ESP_LOG_DEBUG);
        for(int i=0; i < rx_bytes-1; i++)
        {
            if(data[i] == 0xFF)
            {
                ESP_LOGD(TAG, "Found frame start byte");
                /**
                * Search for QA response or Dormant mode set response
                */
                if(data[i+1] == dormancy_state_change_response_id || data[i+1] == measurement_query_response_id)
                {
                    ESP_LOGD(TAG, "Found [%s] response identifier byte", data[i+1] == measurement_query_response_id ? "Measurement query" : "Dormancy state change");
                    for(int j = 1; j < ZH03_CMD_FRAME_LENGTH-1; j++)
                    {
                        checksum += data[i+j];
                    }
                    /**
                    * Calculate checksum as per datasheet spec
                    */
                    checksum = ~checksum;
                    checksum += 1;
                    ESP_LOGD(TAG, "Calculated checksum is 0x%02x", (int)checksum);
                    sent_checksum = data[i+(ZH03_CMD_FRAME_LENGTH-1)];
                    ESP_LOGD(TAG, "Sent checksum is 0x%02x", (int)sent_checksum);
                    if(checksum != sent_checksum)
                    {
                        ESP_LOGE(TAG, "Checksum failed!");
                        return ESP_ERR_INVALID_CRC;
                    }
                    if(data[i+1] == measurement_query_response_id) 
                    {
                        /*Get queried data*/
                        return ESP_OK;
                    }                 
                    if(data[i+1] == dormancy_state_change_response_id && data[i+2] == dormancy_state_change_success)
                    {
                        ESP_LOGI(TAG, "[Dormancy state change] command returned success from device!");
                        return ESP_OK;
                    }
                    /**
                    * Continue parsing the buffer for additional frames
                    */
                    continue;
                }
            }
        }
    }
    ESP_LOGE(TAG, "Buffered UART bytes are less than the expected %d byte frame length", ZH03_CMD_FRAME_LENGTH);
    return ESP_ERR_INVALID_SIZE;
}

esp_err_t set_QA_mode(ZH03* zh03_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(zh03_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = zh03b_uart_write(zh03_inst, set_QA_mode_cmd_frame, ZH03_CMD_FRAME_LENGTH);
    return ret;
}

esp_err_t set_initiative_upload_mode(ZH03* zh03_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(zh03_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = zh03b_uart_write(zh03_inst, set_initiative_upload_mode_cmd_frame, ZH03_CMD_FRAME_LENGTH);
    return ret;
}

esp_err_t set_dormant_mode(ZH03* zh03_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(zh03_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char* command = "SET_DORMANT_MODE";

    uint8_t data[RX_BUF_SIZE]={0};
    uint8_t checksum = 0;
    uint8_t sent_checksum = 0;
    int rx_bytes = 0;

    esp_err_t ret = zh03b_uart_write(zh03_inst, set_dormant_mode_cmd_frame, ZH03_CMD_FRAME_LENGTH);
    if(ret != ESP_OK)
    {
        //Do something? Maybe pass a function pointer to act as the exception handler from user code?
        ESP_LOGE(TAG, "[%s] command not transmitted", command);
    }

    ESP_LOGI(TAG, "[%s] command transmitted", command_name);
    ret = zh03b_uart_read(zh03_inst, data, &rx_bytes)
    if(ret != ESP_OK)
    {
        //Do something? Maybe pass a function pointer to act as the exception handler from user code?   
        ESP_LOGE(TAG, "[%s]-response not receive", command);     
    }

    ESP_LOGI(TAG, "[%s]-response received", command_name);
    return zh03b_command_response_parse(data, &rx_bytes);
}

esp_err_t unset_dormant_mode(ZH03* zh03_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(zh03_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char* command = "UNSET_DORMANT_MODE";

    uint8_t data[RX_BUF_SIZE]={0};
    uint8_t checksum = 0;
    uint8_t sent_checksum = 0;
    int rx_bytes = 0;

    esp_err_t ret = zh03b_uart_write(zh03_inst, unset_dormant_mode_cmd_frame, ZH03_CMD_FRAME_LENGTH);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "[%s] command not transmitted", command);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "[%s] command transmitted", command_name);
    ret = zh03b_uart_read(zh03_inst, data, &rx_bytes);
    if(ret != ESP_OK)
    {
        //Do something? Maybe pass a function pointer to act as the exception handler from user code?   
        ESP_LOGE(TAG, "[%s]-response not receive", command);     
    }

    ESP_LOGI(TAG, "[%s]-response received", command_name);
    return zh03b_command_response_parse(data, &rx_bytes);
}

esp_err_t get_received_zh03b_measurement(ZH03* zh03_inst, int32_t* PM_1, int32_t* PM_2_5, int32_t* PM_10)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(zh03_inst ==NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if(PM_1 == NULL && PM_2_5 == NULL && PM_10 == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t data[RX_BUF_SIZE]={0};
    uint32_t checksum = 0;
    uint32_t sent_checksum = 0;
    int rx_bytes = 0;

    esp_err_t ret = zh03b_uart_read(zh03_inst, data, &rx_bytes);
    if(ret != ESP_OK)
    {
        //Do something? Maybe pass a function pointer to act as the exception handler from user code?   
        ESP_LOGE(TAG, "Initiative measurement not received", command);     
    }

    ESP_LOGI(TAG, "Initiative measurement received", command_name);
    if (rx_bytes >= ZH03_CHECKSUM_LOW_BYTE_POS) 
    {
        ESP_LOGI(TAG, "Read %d bytes", rx_bytes);
        ESP_LOG_BUFFER_HEXDUMP(TAG, data, rx_bytes, ESP_LOG_INFO);
        for(int i=0; i < rx_bytes-1; i++)
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

esp_err_t force_new_zh03b_measurement(ZH03* zh03_inst, int32_t* PM_1, int32_t* PM_2_5, int32_t* PM_10)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(zh03_inst ==NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if(PM_1 == NULL && PM_2_5 == NULL && PM_10 == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char* command = "MEASUREMENT_QUERY";

    uint8_t data[RX_BUF_SIZE]={0};
    uint8_t checksum = 0;
    uint8_t sent_checksum = 0;
    int rx_bytes = 0;

    esp_err_t ret = zh03b_uart_write(zh03_inst, measurement_query_frame, ZH03_CMD_FRAME_LENGTH);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "[%s] command not transmitted", command);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "[%s] command transmitted", command_name);
    ret = zh03b_uart_read(zh03_inst, data, &rx_bytes);
    if(ret != ESP_OK)
    {
        //Do something? Maybe pass a function pointer to act as the exception handler from user code?   
        ESP_LOGE(TAG, "[%s]-response not receive", command);     
    }

    ESP_LOGI(TAG, "[%s]-response received", command_name);

    if (rx_bytes >= ZH03_CMD_FRAME_LENGTH) 
    {
        ESP_LOGI(TAG, "Read %d bytes", rx_bytes);
        ESP_LOG_BUFFER_HEXDUMP(TAG, data, rx_bytes, ESP_LOG_INFO);
        for(int i=0; i < rx_bytes-1; i++)
        {
            if(data[i] == 0xFF)
            {
                ESP_LOGD(TAG, "Found first start of frame byte");
                if(data[i+1] == 0x86)
                {
                    ESP_LOGD(TAG, "Found 'main command' byte");
                    for(int j = 1; j < ZH03_CMD_FRAME_LENGTH-1; j++)
                    {
                        checksum += data[i+j];
                    }
                    /*Take two's complement*/
                    checksum = ~checksum;
                    checksum += 1;
                    ESP_LOGD(TAG, "Calculated checksum is 0x%02x", (int)checksum);
                    sent_checksum = data[i+(ZH03_CMD_FRAME_LENGTH-1)];
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
