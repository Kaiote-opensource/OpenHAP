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
#ifndef _MLX90640_I2C_Driver_H_
#define _MLX90640_I2C_Driver_H_

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c.h"
#include "freertos/semphr.h"

typedef struct
{
    uint8_t address;
    i2c_port_t i2c_port;

    SemaphoreHandle_t i2c_bus_mutex;
}MLX90640;

    esp_err_t MLX90640_init(MLX90640 *MLX90640_inst, int address, i2c_port_t port, SemaphoreHandle_t *i2c_bus_mutex);
    int MLX90640_I2CRead(MLX90640 *MLX90640_inst,  uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data);
    int MLX90640_I2CWrite(MLX90640 *MLX90640_inst, uint16_t writeAddress, uint16_t data);

#endif
