#ifndef _HIH6030_H_
#define _HIH6030_H_

#include "freertos/semphr.h"
#include "driver/i2c.h"

typedef enum {
    HIH6030_VALID_DATA = 0,
    HIH6030_STALE_DATA,
    HIH6030_COMMAND_MODE
} hih6030_status_t;

typedef enum {
    HIH6030_HUMIDITY_ONLY = 0,
    HIH6030_TEMP_HUMIDITY
} hih6030_meas_type_t;

tca9534a_register_t

typedef struct
{
    int address;
    i2c_port_t i2c_port;
    SemaphoreHandle_t i2c_bus_mutex;
}HIH6030;

esp_err_t HIH6030_init(HIH6030* hih6030_inst, int address, i2c_port_t port, SemaphoreHandle_t* bus_mutex);
esp_err_t hih6030_get_measurement(HIH6030* hih6030_inst, hih6030_meas_type_t measurement_type, hih6030_status_t* status, float* temperature, float* humidity);

#endif
