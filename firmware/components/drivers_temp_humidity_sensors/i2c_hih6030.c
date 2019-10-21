#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "i2c_hih6030.h"

#define HIH6030_I2C_TIMEOUT                   1000
#define WRITE_BIT                             I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                              I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN                          0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                         0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                               0x0              /*!< I2C ack value */
#define NACK_VAL                              0x1              /*!< I2C nack value */
#define LAST_NACK_VAL                         0x0              /*!< I2C final nack value */ 

static const char* TAG = "TEMP_HUMIDITY_HIH6030";

static const uint32_t long_measurement_status_bitmask        = 0xC0000000;
static const uint32_t long_measurement_humidity_bitmask      = 0x3fff0000;
static const uint32_t long_measurement_temperature_bitmask   = 0x0000ffff;
static const uint16_t short_measurement_status_bitmask       = 0xC000;
static const uint16_t short_measurement_humidity_bitmask     = 0x3fff;

static const int short_measurement_length = 2;
static const int long_measurement_length  = 4;

static esp_err_t hih6030_measurement_request(const HIH6030 *hih6030_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (hih6030_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, ((hih6030_inst->address)<<1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_stop(i2c_cmd);
    xSemaphoreTake(hih6030_inst->i2c_bus_mutex, portMAX_DELAY);
    esp_err_t ret = i2c_master_cmd_begin(hih6030_inst->i2c_port, i2c_cmd, HIH6030_I2C_TIMEOUT/portTICK_RATE_MS);
    xSemaphoreGive(hih6030_inst->i2c_bus_mutex);
    i2c_cmd_link_delete(i2c_cmd);
    return ret;
}

static esp_err_t hih6030_data_fetch(HIH6030* hih6030_inst, hih6030_meas_type_t measurement_type, uint8_t* data)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (hih6030_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, ((hih6030_inst->address) << 1 ) | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(i2c_cmd, data, (measurement_type==HIH6030_HUMIDITY_ONLY)?(short_measurement_length-1):(long_measurement_length-1), I2C_MASTER_ACK);
    i2c_master_read_byte(i2c_cmd, (measurement_type==HIH6030_HUMIDITY_ONLY)?(data+short_measurement_length):(data+long_measurement_length), I2C_MASTER_LAST_NACK);
    i2c_master_stop(i2c_cmd);
    xSemaphoreTake(hih6030_inst->i2c_bus_mutex, portMAX_DELAY);
    esp_err_t ret = i2c_master_cmd_begin(hih6030_inst->i2c_port, i2c_cmd, HIH6030_I2C_TIMEOUT/ portTICK_RATE_MS);
    xSemaphoreGive(hih6030_inst->i2c_bus_mutex);
    i2c_cmd_link_delete(i2c_cmd);
    ESP_LOG_BUFFER_HEX_LEVEL("I2C UINT8 BUFFER", data, (measurement_type==HIH6030_HUMIDITY_ONLY)?short_measurement_length:long_measurement_length, ESP_LOG_DEBUG);
    return ret;    
}

static esp_err_t extract_measurements(uint8_t* data, hih6030_meas_type_t measurement_type, hih6030_status_t* status, float* temperature, float* humidity)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (data == NULL || status == NULL || humidity == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t temperature_raw;
    uint16_t humidity_raw;

    humidity_raw = (measurement_type==HIH6030_HUMIDITY_ONLY)?(*data & short_measurement_humidity_bitmask):((*data & long_measurement_status_bitmask)>>16);
    *humidity = (humidity_raw/(pow(2, 14) - 2))*100;
    ESP_LOGI(TAG, "Humidity is %f%% RH", *humidity);
    if(status != NULL)
    {    
        *status = (measurement_type==HIH6030_HUMIDITY_ONLY)?(*data & short_measurement_status_bitmask):((*data & long_measurement_status_bitmask)>>16);
    }

    if(measurement_type == HIH6030_TEMP_HUMIDITY && temperature != NULL)
    {
        temperature_raw = (*data & long_measurement_status_bitmask);
        *temperature = ((temperature_raw/(pow(2, 14) - 2))*165)-40;
        ESP_LOGI(TAG, "Temperature is %f degrees celsius", *temperature);
    }

    return ESP_OK;
}


esp_err_t HIH6030_init(HIH6030* hih6030_inst, int address, i2c_port_t port, SemaphoreHandle_t* bus_mutex)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(hih6030_inst == NULL || bus_mutex == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    HIH6030_inst->address = address;
    HIH6030_inst->i2c_port = port;
    HIH6030_inst->i2c_bus_mutex=*bus_mutex;
    return ESP_OK;
}

esp_err_t hih6030_get_measurement(HIH6030* hih6030_inst, hih6030_meas_type_t measurement_type, hih6030_status_t* status, float* temperature, float* humidity)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (HIH6030_inst == NULL || status == NULL || humidity == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[4] = {0};

    esp_err_t ret = hih6030_measurement_request(hih6030_inst);
    if(ret != ESP_OK)
    {
        return ret;
    }
    
    ret = hih6030_data_fetch(hih6030_inst, measurement_type, data);
    if(ret != ESP_OK)
    {
        return ret;
    }

    ret = extract_measurements(data, measurement_type, status, temperature, humidity);
    if(ret != ESP_OK)
    {
        return ret;
    }
    return ESP_OK;
}
