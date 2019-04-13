#ifndef _HIH6030_H_
#define _HIH6030_H_

#include "freertos/semphr.h"
#include "driver/i2c.h"

#define HIH6030_PAYLOAD_BYTES              4

#define HIH6030_STATUS_BIT_WIDTH           2
#define HIH6030_STATUS_START_BYTE          0
#define HIH6030_STATUS_END_BYTE            0
#define HIH6030_STATUS_START_POS           7

#define HIH6030_HUMIDITY_BIT_WIDTH         14
#define HIH6030_HUMIDITY_START_BYTE        1
#define HIH6030_HUMIDITY_END_BYTE          0
#define HIH6030_HUMIDITY_START_POS         0

#define HIH6030_TEMPERATURE_BIT_WIDTH      14
#define HIH6030_TEMPERATURE_START_BYTE     3
#define HIH6030_TEMPERATURE_END_BYTE       2
#define HIH6030_TEMPERATURE_START_POS      2

typedef struct
{
    float temperature;
    float humidity; 
    uint16_t status;

    int address;
    i2c_port_t port;

    SemaphoreHandle_t i2c_bus_mutex;
    SemaphoreHandle_t device_data_mutex;
}HIH6030;

esp_err_t get_temp_humidity(HIH6030* HIH6030_data);

esp_err_t HIH6030_init(HIH6030* HIH6030_inst, int address, i2c_port_t port, int frequency, gpio_num_t sda_gpio, gpio_pullup_t sda_pullup_state, 
                                                                                           gpio_num_t scl_gpio, gpio_pullup_t scl_pullup_state,
                                                                                           SemaphoreHandle_t* data_mutex, SemaphoreHandle_t* bus_mutex);
/*Only to be used if device is the only one on the i2c bus or for testing*/
esp_err_t HIH6030_deinit(i2c_port_t port);

#endif
