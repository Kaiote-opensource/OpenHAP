#ifndef _I2C_TCA9534A_H_
#define _I2C_TCA9534A_H_

#include "esp_err.h"
#include "freertos/semphr.h"
#include "driver/i2c.h"

enum
{
    TCA9534_OUTPUT = 0,
    TCA9534_INPUT  = 1
} tca9534a_pin_direction_t;

enum
{
    TCA9534_LOW  = 0,
    TCA9534_HIGH = 1
} tca9534a_level_t;

enum
{
    TCA9534A_INPUT_PORT_REG  = 0x00,
    TCA9534A_OUTPUT_PORT_REG = 0x01,
    TCA9534A_POLARITY_REG    = 0x02,
    TCA9534A_CONFIG_REG      = 0x03
} tca9534a_register_t;

typedef struct
{
    uint8_t address;
    i2c_port_t i2c_port;

    SemaphoreHandle_t i2c_bus_mutex;
}TCA9534;

esp_err_t tca9534_init(TCA9534 *tca9534a_inst, int address, i2c_port_t port, gpio_num_t intr_pin, const SemaphoreHandle_t *i2c_bus_mutex);
esp_err_t tca9534_set_port_direction(const TCA9534 *tca9534a_inst, uint8_t port);
esp_err_t tca9534_get_port_direction(const TCA9534 *tca9534a_inst, uint8_t* port);
esp_err_t tca9534_set_pin_direction(const TCA9534 *tca9534a_inst, uint8_t pin, tca9534a_pintype_t pin_type);
esp_err_t tca9534_get_pin_direction(const TCA9534 *tca9534a_inst, uint8_t pin, tca9534a_pintype_t *value);
esp_err_t tca9534_set_level(const TCA9534 *tca9534a_inst, uint8_t pin, tca9534a_level_t level);
esp_err_t tca9534_get_level(const TCA9534 *tca9534a_inst, uint8_t pin, tca9534a_level_t *level);

#endif
