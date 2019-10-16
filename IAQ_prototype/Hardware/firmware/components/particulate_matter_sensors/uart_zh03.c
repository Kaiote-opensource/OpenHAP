#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "uart_zh03.h"

/**********************************************************************************************************************************/
/**
 * Uart transceiver operational parameters
 */
/**********************************************************************************************************************************/
#define RX_BUF_SIZE 256
#define ZH03B_UART_TIMEOUT 1500;

/**********************************************************************************************************************************/
/**
 * Frame lengths of generalised frame types (Command&response frames and initiative upload frames)
 */
/**********************************************************************************************************************************/
#define ZH03_CMD_FRAME_LENGTH 9
#define ZH03_INITIATIVE_MEASUREMENT_FRAME_LENGTH 23

/**********************************************************************************************************************************/
/**
 * Generalised measurement query & command response frame layout byte[0-8]
 */
/**********************************************************************************************************************************/
#define ZH03_CMD_START_BYTE_POS        0
#define ZH03_CMD_ID_POS                1
#define ZH03_CMD_RETURN                2
#define ZH03_QA_PM2_5_HIGH_BYTE        2
#define ZH03_QA_PM2_5_LOW_BYTE         3
#define ZH03_QA_PM10_HIGH_BYTE         4
#define ZH03_QA_PM10_LOW_BYTE          5
#define ZH03_QA_PM1_HIGH_BYTE          6
#define ZH03_QA_PM1_LOW_BYTE           7
#define ZH03_QA_CMD_CHECKSUM           8
/**********************************************************************************************************************************/
/**
 * Initiative upload measurement frame layout byte[0-23]
 */
/**********************************************************************************************************************************/
#define ZH03_UPLOAD_START_BYTE_POS             0
#define ZH03_UPLOAD_START_BYTE_2_POS           1
#define ZH03_UPLOAD_FRAME_LENGTH_POS           3
#define ZH03_UPLOAD_PM_1_HIGH_BYTE_POS         10
#define ZH03_UPLOAD_PM_1_LOW_BYTE_POS          11
#define ZH03_UPLOAD_PM_2_5_HIGH_BYTE_POS       12
#define ZH03_UPLOAD_PM_2_5_LOW_BYTE_POS        13
#define ZH03_UPLOAD_PM_10_HIGH_BYTE_POS        14
#define ZH03_UPLOAD_PM_10_LOW_BYTE_POS         15
#define ZH03_UPLOAD_CHECKSUM_HIGH_BYTE_POS     22
#define ZH03_UPLOAD_CHECKSUM_LOW_BYTE_POS      23


static const char* TAG = "PM_SENSOR_ZH03";

/**********************************************************************************************************************************/
/**
 * ZH03 command frames types and structure
 */
/**********************************************************************************************************************************/
static const uint8_t set_QA_mode_cmd_frame[ZH03_CMD_FRAME_LENGTH] = {0xff, 0x01, 0x78, 0x41, 0x00, 0x00, 0x00, 0x00, 0x46};
static const uint8_t set_initiative_upload_mode_cmd_frame[ZH03_CMD_FRAME_LENGTH] = {0xff, 0x01, 0x78, 0x40, 0x00, 0x00, 0x00, 0x00, 0x47};
static const uint8_t set_dormant_mode_cmd_frame[ZH03_CMD_FRAME_LENGTH] = {0xff, 0x01, 0xa7, 0x01, 0x00, 0x00, 0x00, 0x00, 0x57};
static const uint8_t unset_dormant_mode_cmd_frame[ZH03_CMD_FRAME_LENGTH] = {0xff, 0x01, 0xa7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58};
static const uint8_t measurement_query_frame[ZH03_CMD_FRAME_LENGTH] = {0xff, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

/**********************************************************************************************************************************/
/**
 * ZH03 response frames
 */
/**********************************************************************************************************************************/
/**********************************************************************************************************************************/
/**
 * Response frame identifier
 */
/**********************************************************************************************************************************/
static const uint8_t start_of_response_frame = 0xff;
/**********************************************************************************************************************************/
/**
 * Response type identifiers
 */
/**********************************************************************************************************************************/
static const uint8_t measurement_query_response_id = 0x86;
static const uint8_t dormancy_state_change_response_id = 0xA7;
/**********************************************************************************************************************************/
/**
 * Power state change request, success identifiers
 */
/**********************************************************************************************************************************/
static const uint8_t dormancy_state_change_success = 0x01;
static const uint8_t dormancy_state_change_fail = 0x00;


/**********************************************************************************************************************************/
/**
 * Initiative upload frames
 */
/**********************************************************************************************************************************/
/**********************************************************************************************************************************/
/**
 * Start of initiative upload frame identifier
 */
/**********************************************************************************************************************************/
static const uint8_t start_of_response_frame_byte_1 = 0x42;
static const uint8_t start_of_response_frame_byte_2 = 0x4d;

enum
{
    ZH03_CMD_DORMANCY_STATE_CHANGE = 0,
    ZH03_CMD_QA_MODE,
    ZH03_MAX =255
} zh03_command_t;


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
        ESP_LOGE(TAG, "Writing to device failed, length is %d bytes, wrote %d bytes", out_size, tx_bytes);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Wrote all bytes to device successfully");       
    return ret;    
}

static esp_err_t zh03_uart_read(const ZH03* zh03_inst, uint8_t* in_data, int* in_size)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);  
    if(zh03_inst == NULL || in_data == NULL || in_size == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int rx_bytes = uart_read_bytes(zh03_inst->uart_port, in_data, RX_BUF_SIZE, ZH03B_UART_TIMEOUT/portTICK_RATE_MS);
    if(rx_bytes == -1)
    {
        ESP_LOGE(TAG, "Failed to read data from device");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Read %d bytes from device", rx_bytes);
    ESP_LOG_BUFFER_HEXDUMP(TAG, in_data, rx_bytes, ESP_LOG_DEBUG);
    *in_size = rx_bytes;
    return ESP_OK;
}

static esp_err_t zh03_verify_checksum_8(uint8_t* data, int length)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(data == NULL || length <= 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t calculated_checksum= 0;
    uint8_t frame_checksum = 0;

    for(int j = 0; j < length; j++)
    {
        calculated_checksum += data[j];
    }

    calculated_checksum = ~calculated_checksum;
    calculated_checksum += 1;
    ESP_LOGD(TAG, "Calculated checksum is 0x%02x", (int)calculated_checksum);
    sent_checksum = data[i+(ZH03_CMD_FRAME_LENGTH-1)];
    ESP_LOGD(TAG, "Frame checksum is 0x%02x", (int)frame_checksum);
    if(checksum != frame_checksum)
    {
        ESP_LOGE(TAG, "Checksum failure!");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Checksum success!"); 
    return ESP_OK;   
}

static esp_err_t zh03_verify_checksum_16(uint8_t* data, int length)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(data == NULL || length <= 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t calculated_checksum= 0;
    uint8_t frame_checksum = 0;

    for(int j = 0; j < length; j++)
    {
        calculated_checksum += data[j];
    }

    calculated_checksum = ~calculated_checksum;
    calculated_checksum += 1;
    ESP_LOGD(TAG, "Calculated checksum is 0x%02x", (int)calculated_checksum);
    sent_checksum = data[i+(ZH03_CMD_FRAME_LENGTH-1)];
    ESP_LOGD(TAG, "Frame checksum is 0x%02x", (int)frame_checksum);
    if(checksum != frame_checksum)
    {
        ESP_LOGE(TAG, "Checksum failure!");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Checksum success!"); 
    return ESP_OK;   
}

static esp_err_t zh03_get_cmd_response(const uint8_t* in_data, int rx_bytes, zh03_command_t command_type_sent, int32_t* PM1, int32_t* PM2_5, int32_t* PM10)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);  
    if(in_data == NULL || in_size == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t* frame_data = NULL;

    if (rx_bytes >= ZH03_CMD_FRAME_LENGTH)
    {
        for(int i=0; i < rx_bytes-1; i++)
        {
            if(in_data[i] == start_of_response_frame)
            {
                frame_data = &(in_data[i]);
                if(command_type_sent == ZH03_CMD_DORMANCY_STATE_CHANGE && frame_data[ZH03_CMD_ID_POS] == dormancy_state_change_response_id)
                {
                    ESP_LOGD(TAG, "Successfully found response to dormancy state change command");
                    esp_err_t ret = zh03_verify_checksum(frame_data[ZH03_CMD_ID_POS], ZH03_CMD_FRAME_LENGTH-1);
                    if(ret != ESP_OK)
                    {
                        continue;
                    }

                    if(frame_data[ZH03_CMD_RETURN] == dormancy_state_change_fail)
                    {
                        ESP_LOGI(TAG, "Device executed power state change command unsuccessfully");
                        return ESP_FAIL;                        
                    }

                    ESP_LOGI(TAG, "Device executed power state change command successfully");
                    return ESP_OK;
                }
                else if(command_type_sent == ZH03_CMD_QA_MODE && frame_data[ZH03_CMD_ID_POS] == measurement_query_response_id)
                {
                    if(PM1 == NULL && PM2_5 == NULL && PM10 == NULL)
                    {
                        return ESP_ERR_INVALID_ARG;
                    }

                    ESP_LOGD(TAG, "Successfully found response to measurement query");
                    esp_err_t ret = zh03_verify_checksum(frame_data[ZH03_CMD_ID_POS], ZH03_CMD_FRAME_LENGTH-1);
                    if(ret != ESP_OK)
                    {
                        continue;
                    }

                    /**
                    * Get queried measurement data
                    */
                    if(PM_1 != NULL)
                    {
                        *PM1 = (256*(frame_data[ZH03_QA_PM1_HIGH_BYTE])) + frame_data[ZH03_QA_PM1_LOW_BYTE];
                        ESP_LOGD(TAG, "PM 1 value is %d ug/m3", (int)*PM_1);
                    }
                    if(PM_2_5 != NULL)
                    {
                        *PM2_5 = (256*(frame_data[ZH03_QA_PM2_5_HIGH_BYTE])) + frame_data[ZH03_QA_PM2_5_LOW_BYTE];
                        ESP_LOGD(TAG, "PM 2.5 value is %d ug/m3", (int)*PM_2_5);
                    }
                    if(PM_10 != NULL)
                    {
                        *PM10 = (256*(frame_data[ZH03_QA_PM10_HIGH_BYTE])) + frame_data[ZH03_QA_PM10_LOW_BYTE];
                        ESP_LOGD(TAG, "PM 10 value is %d ug/m3", (int)*PM_10);
                    }
                    return ESP_OK;
                }
                else
                {
                    ESP_LOGD(TAG, "Found other response frame type...continuing buffer scan!");
                    continue;
                }
            }
        }
        ESP_LOGW(TAG, "Valid response frame not found in buffer!");
    }
    ESP_LOGE(TAG, "Bytes in UART buffer are less than the expected %d byte frame length, try increasing the wait time", ZH03_CMD_FRAME_LENGTH);
    return ESP_ERR_INVALID_SIZE;
}

static inline esp_err_t get_queried_measurement(const void* in_data, int rx_bytes, int32_t* PM1, int32_t* PM2_5, int32_t* PM10)
{
    return zh03_get_cmd_response(in_data, rx_bytes, ZH03_CMD_QA_MODE, PM1, PM2_5, PM10);
}

static inline esp_err_t get_power_state_change_response(const void* in_data, int rx_bytes)
{
    return zh03_get_cmd_response(in_data, rx_bytes, ZH03_CMD_DORMANCY_STATE_CHANGE, NULL, NULL, NULL);
}

/**
 * ZH03 active state commands - Sleep/Wake mode
 */
esp_err_t zh03_sleep(const ZH03* zh03_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(zh03_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[RX_BUF_SIZE]={0};
    int rx_bytes = 0;

    esp_err_t ret = zh03_uart_write(zh03_inst, set_dormant_mode_cmd_frame, ZH03_CMD_FRAME_LENGTH);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Sending command unsuccessful");
        return ret;
    }

    ESP_LOGI(TAG, "Successfully sent command");
    ret = zh03_uart_read(zh03_inst, data, &rx_bytes)
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Did not receive data from device");
        return ret;
    }

    ESP_LOGI(TAG, "Received data from device");
    return get_power_state_change_response(data, &rx_bytes);
}

esp_err_t zh03_wake(const ZH03* zh03_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(zh03_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[RX_BUF_SIZE]={0};
    int rx_bytes = 0;

    esp_err_t ret = zh03_uart_write(zh03_inst, unset_dormant_mode_cmd_frame, ZH03_CMD_FRAME_LENGTH);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Sending command unsuccessful");
        return ret;
    }

    ESP_LOGI(TAG, "Successfully sent command");
    ret = zh03_uart_read(zh03_inst, data, &rx_bytes);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Did not receive data from device");
        return ret;
    }

    ESP_LOGI(TAG, "Received data from device");
    return get_power_state_change_response(data, &rx_bytes);
}

/**
 * ZH03 QA measurement mode command and measurement retrieval
 */
esp_err_t set_QA_mode(const ZH03* zh03_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(zh03_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return zh03_uart_write(zh03_inst, set_QA_mode_cmd_frame, ZH03_CMD_FRAME_LENGTH);
}

esp_err_t get_QA_measurement(const ZH03* zh03_inst, int32_t* PM1, int32_t* PM2_5, int32_t* PM10)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(zh03_inst ==NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if(PM1 == NULL && PM2_5 == NULL && PM10 == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[RX_BUF_SIZE]={0};
    int rx_bytes = 0;

    esp_err_t ret = zh03_uart_write(zh03_inst, measurement_query_frame, ZH03_CMD_FRAME_LENGTH);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Sending command unsuccessful");
        return ret;
    }

    ESP_LOGI(TAG, "Sending command unsuccessful");
    ret = zh03_uart_read(zh03_inst, data, &rx_bytes);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Did not receive data from device");   
        return ret;  
    }

    ESP_LOGI(TAG, "Received data from device");
    return get_queried_measurement(data, rx_bytes, PM1, PM2_5, PM10);
}

/**
 * ZH03 initiative upload measurement mode command and measurement retrieval
 */
esp_err_t set_initiative_upload_mode(const ZH03* zh03_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(zh03_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return zh03_uart_write(zh03_inst, set_initiative_upload_mode_cmd_frame, ZH03_CMD_FRAME_LENGTH);
}

esp_err_t get_buffered_initiative_upload_measurement(const ZH03* zh03_inst, int32_t* PM1, int32_t* PM2_5, int32_t* PM10)
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
    int rx_bytes = 0;

    esp_err_t ret = zh03_uart_read(zh03_inst, data, &rx_bytes);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Initiative measurement not received", command);
        return ret;   
    }

    ESP_LOGI(TAG, "Initiative measurement received", command_name);
    if (rx_bytes >= ZH03_INITIATIVE_MEASUREMENT_FRAME_LENGTH) 
    {
        for(int i=0; i < rx_bytes-1; i++)
        {
            if(data[i] == start_of_response_frame_byte_1)
            {
                ESP_LOGD(TAG, "Found first start of frame byte");
                if(data[i+ZH03_START_BYTE_2_POS] == start_of_response_frame_byte_2)
                {
                    ESP_LOGD(TAG, "Found second start of frame byte");
                    esp_err_t ret = zh03_verify_checksum_16(data[i+ZH03_FRAME_LENGTH_POS], ZH03_CMD_FRAME_LENGTH-2);
                    for(int j = 0; j < () + 2; j++)
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
