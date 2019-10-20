/**
 * @copyright (C) 2017 Melexis N.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "MLX90640_I2C_Driver.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <stdlib.h>

#define WRITE_BIT                          I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                           I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN                       0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                      0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                            0x0              /*!< I2C ack value */
#define NACK_VAL                           0x1              /*!< I2C nack value */
#define LAST_NACK_VAL                      0x0              /*!< I2C final nack value */

#define READ_BUFFER_SIZE                   1664             /*!< Derived from 832x2*/

static const char* TAG = "MLX90640_I2C_DRIVER";

static const int MLX90640_I2C_TIMEOUT = 1000;

esp_err_t MLX90640_init(MLX90640 *MLX90640_inst, int address, i2c_port_t port, SemaphoreHandle_t *i2c_bus_mutex)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

    if(i2c_bus_mutex == NULL && MLX90640_inst == NULL && port >= I2C_NUM_MAX)
    {
        return ESP_ERR_INVALID_ARG;
    }

    MLX90640_inst->address = address;
    MLX90640_inst->i2c_port = port;
    MLX90640_inst->i2c_bus_mutex = *i2c_bus_mutex;
    return ESP_OK;
}

int MLX90640_I2CRead(MLX90640 *MLX90640_inst,  uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data)
{             
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);  

    int cnt = 0;
    int i = 0;
    uint8_t mlx_cmd[2] = {0,0};
    uint8_t data_rd[READ_BUFFER_SIZE] = {0};
    uint16_t *p = data;
    
    /*Change data orientation on the wire*/
    mlx_cmd[0] = (uint8_t)(startAddress >> 8);
    mlx_cmd[1] = (uint8_t)(startAddress & 0x00FF);
    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, ( (MLX90640_inst->address) << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(i2c_cmd, mlx_cmd, 2, ACK_CHECK_EN);
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, ( (MLX90640_inst->address) << 1 ) | READ_BIT, ACK_CHECK_EN);
    if ((nMemAddressRead*2) > 1)
    {
        i2c_master_read(i2c_cmd, data_rd, (2*nMemAddressRead)-1, I2C_MASTER_ACK);
    }

    i2c_master_read_byte(i2c_cmd, data_rd+((2*nMemAddressRead)-1), I2C_MASTER_LAST_NACK);
    i2c_master_stop(i2c_cmd);
    xSemaphoreTake(MLX90640_inst->i2c_bus_mutex, portMAX_DELAY);
    esp_err_t ret = i2c_master_cmd_begin(MLX90640_inst->i2c_port, i2c_cmd, MLX90640_I2C_TIMEOUT / portTICK_RATE_MS);
    xSemaphoreGive(MLX90640_inst->i2c_bus_mutex);
    i2c_cmd_link_delete(i2c_cmd);
    if (ret != ESP_OK)
    {
        return -1;
    }

    ESP_LOG_BUFFER_HEX_LEVEL("RAW I2C BUFFER", data_rd, 2*nMemAddressRead, ESP_LOG_DEBUG);
    for(cnt=0; cnt < nMemAddressRead; cnt++)
    {
        i = cnt << 1;
        *p++ =  (uint16_t)data_rd[i+1] + (uint16_t)data_rd[i]*256;
    }

    ESP_LOG_BUFFER_HEXDUMP("RECONSTRUCTED UINT16 DATA:", data, 2*nMemAddressRead, ESP_LOG_DEBUG);
    return 0;   
} 

int MLX90640_I2CWrite(MLX90640 *MLX90640_inst, uint16_t writeAddress, uint16_t data)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

    uint8_t mlx_cmd[4] = {0};
    uint16_t dataCheck;

    mlx_cmd[0] = writeAddress >> 8;
    mlx_cmd[1] = writeAddress & 0x00FF;
    mlx_cmd[2] = data >> 8;
    mlx_cmd[3] = data & 0x00FF;

    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, ( (MLX90640_inst->address) << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(i2c_cmd, mlx_cmd, 4, ACK_CHECK_EN);
    i2c_master_stop(i2c_cmd);
    xSemaphoreTake(MLX90640_inst->i2c_bus_mutex, portMAX_DELAY);
    esp_err_t ret = i2c_master_cmd_begin(MLX90640_inst->i2c_port, i2c_cmd, MLX90640_I2C_TIMEOUT / portTICK_RATE_MS);
    xSemaphoreGive(MLX90640_inst->i2c_bus_mutex);
    i2c_cmd_link_delete(i2c_cmd);
    if (ret != ESP_OK)
    {
        return -1;
    }   
    
    /*Read-back verification*/
    MLX90640_I2CRead(MLX90640_inst, writeAddress, 1, &dataCheck);
    if ( dataCheck != data)
    {
        return -2;
    }    
    return 0;
}