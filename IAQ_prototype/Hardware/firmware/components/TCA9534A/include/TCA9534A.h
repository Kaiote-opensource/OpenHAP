#ifndef _TCA9534A_H_
#define _TCA9534A_H_

#include "freertos/semphr.h"
#include "driver/i2c.h"

#define TCA9534_OUTPUT                     0 
#define TCA9534_INPUT                      1

#define TCA9534_LOW                        0 
#define TCA9534_HIGH                       1

#define TCA9534_ACTIVE_HIGH                0 
#define TCA9534_ACTIVE_LOW                 1
                     
#define TCA9534_INPUT_PORT_REG             0x00U
#define TCA9534_OUTPUT_PORT_REG            0x01U
#define TCA9534_POLARITY_REG               0x02U
#define TCA9534_CONFIG_REG                 0x03U  

#define TCA9534_RTC_INT                    0
#define TCA9534_PIR_INT                    1
#define TCA9534_BUZZER                     2
#define TCA9534_WARN_LED                   3
#define TCA9534_PIR_RST                    4
#define TCA9534_SD_WP                      5
#define TCA9534_SD_CD                      6
#define TCA9534_BATT_SEL                   7

#define GPIO_TCA9534_INT                   35
#define GPIO_INPUT_PIN_SEL                 (1ULL<<GPIO_TCA9534_INT)
#define ESP_INTR_FLAG_DEFAULT              0             

typedef struct
{
    uint8_t pinModeConf;
    uint8_t port;
    uint8_t polarity;

    uint8_t address;
    i2c_port_t i2c_port;

    SemaphoreHandle_t i2c_bus_mutex;
    SemaphoreHandle_t device_data_mutex; 
}TCA9534;

esp_err_t TCA9534_init(TCA9534* TCA9534_inst, int address, i2c_port_t port, int frequency, gpio_num_t sda_gpio, gpio_pullup_t sda_pullup_state, 
                                                                                      gpio_num_t scl_gpio, gpio_pullup_t scl_pullup_state,
                                                                                      SemaphoreHandle_t* device_data_mutex, SemaphoreHandle_t* i2c_bus_mutex);
/*Only to be used if device is the only one on the i2c bus or for testing*/
esp_err_t TCA9534_deinit(i2c_port_t port);

esp_err_t TCA9534_set_port_direction(TCA9534* TCA9534_inst, uint8_t port);
esp_err_t TCA9534_set_pin_direction(TCA9534* TCA9534_inst, uint8_t pin, int pin_type);
esp_err_t TCA9534_get_port_direction(TCA9534* TCA9534_inst, uint8_t pin, uint8_t* value);
esp_err_t TCA9534_set_level(TCA9534* TCA9534_inst, uint8_t pin, int32_t level);
esp_err_t TCA9534_get_level(TCA9534* TCA9534_inst, uint8_t pin, int32_t* level);

#endif
