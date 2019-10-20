#ifndef _TCA9534A_H_
#define _TCA9534A_H_

#include "esp_err.h"
#include "freertos/semphr.h"
#include "driver/i2c.h"


//This should be defined at user code level
#define TCA9534_RTC_INT                    0
#define TCA9534_PIR_INT                    1
#define TCA9534_BUZZER                     2
#define TCA9534_WARN_LED                   3
#define TCA9534_PIR_RST                    4
#define TCA9534_SD_WP                      5
#define TCA9534_SD_CD                      6
#define TCA9534_BATT_SEL                   7

enum
{
    TCA9534_OUTPUT = 0,
    TCA9534_INPUT  = 1
} tca9534a_pintype_t;

enum
{
    TCA9534_LOW  = 0,
    TCA9534_HIGH = 1
} tca9534a_level_t;

enum
{
    TCA9534_ACTIVE_HIGH = 0,
    TCA9534_ACTIVE_LOW  = 1
} tca9534a_active_level_t;

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
esp_err_t tca9534_set_pin_direction(const TCA9534 *tca9534a_inst, uint8_t pin, tca9534a_pintype_t pin_type);
esp_err_t tca9534_get_pin_direction(const TCA9534 *tca9534a_inst, uint8_t pin, tca9534a_pintype_t *value);
esp_err_t tca9534_set_level(const TCA9534 *tca9534a_inst, uint8_t pin, tca9534a_level_t level);
esp_err_t tca9534_get_level(const TCA9534 *tca9534a_inst, uint8_t pin, tca9534a_level_t *level);

#endif
