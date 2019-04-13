#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "HIH6030.h"

char* TAG = "HIH6030";

#define WRITE_BIT                          I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                           I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN                       0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                      0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                            0x0              /*!< I2C ack value */
#define NACK_VAL                           0x1              /*!< I2C nack value */
#define LAST_NACK_VAL                      0x0              /*!< I2C final nack value */ 

static esp_err_t HIH6030_i2c_start_measurement(HIH6030* HIH6030_inst);
static esp_err_t HIH6030_i2c_read_measurement(HIH6030* HIH6030_inst, uint8_t* data_rd);
static uint16_t extract_bit_field_value(void* sensor_data, int bit_width, int start_byte, int end_byte, int start_pos);

static uint16_t extract_bit_field_value(void* sensor_data, int bit_width, int start_byte, int end_byte, int start_pos)
{
/*No input validation, you are on your own*/
uint16_t ret;

uint16_t temp;

uint8_t* data_ptr = (uint8_t*)sensor_data;

/*We know from the datasheet, none of the data fields will never span more than a 2 byte boundary*/

if(start_byte == end_byte)
{   
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    ESP_LOGD(TAG, "Original data is 0x%02X", data_ptr[start_byte]);
    temp = data_ptr[start_byte] >> start_pos;
    ESP_LOGD(TAG, "Shifted data by %d positions is 0x%02X", start_pos, temp);
    ret = ((1 << bit_width) - 1) & (data_ptr[start_byte] >> start_pos);
    ESP_LOGD(TAG, "Eventual data is 0x%02X", ret);
}
else
{

    ESP_LOGD(TAG, "First data field byte is 0x%02X", (((1 << (8-start_pos))-1)&(data_ptr[start_byte]>>start_pos)));
    ESP_LOGD(TAG, "Second data field byte is 0x%02X", (((1 >> ((bit_width-start_pos)-8)) - 1) & data_ptr[end_byte]));
    ret = (((1 << (8-start_pos))-1)&(data_ptr[start_byte]>>start_pos))+(256*(((1 >> ((bit_width-start_pos)-8)) - 1) & (data_ptr[end_byte]>>start_pos)));
    ESP_LOGD(TAG, "Combined data field is 0x%02X", ret);
}
return ret;
}


static esp_err_t HIH6030_i2c_start_measurement(HIH6030* HIH6030_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, ((HIH6030_inst->address)<<1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_stop(i2c_cmd);
    esp_err_t ret = i2c_master_cmd_begin(HIH6030_inst->port, i2c_cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(i2c_cmd);
    return ret;
}

static esp_err_t HIH6030_i2c_read_measurement(HIH6030* HIH6030_inst, uint8_t* data_rd)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    i2c_master_start(i2c_cmd);

    i2c_master_write_byte(i2c_cmd, ((HIH6030_inst->address) << 1 ) | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(i2c_cmd, data_rd, (HIH6030_PAYLOAD_BYTES-1), I2C_MASTER_ACK);
    i2c_master_read_byte(i2c_cmd, data_rd+(HIH6030_PAYLOAD_BYTES-1), I2C_MASTER_LAST_NACK);

    i2c_master_stop(i2c_cmd);
    esp_err_t ret = i2c_master_cmd_begin(HIH6030_inst->port, i2c_cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(i2c_cmd);
    if(ret != ESP_OK)
    {
        return ret;
    }

    ESP_LOG_BUFFER_HEX_LEVEL("I2C UINT8 BUFFER", data_rd, HIH6030_PAYLOAD_BYTES, ESP_LOG_DEBUG);

    return ret;    
}


esp_err_t HIH6030_init(HIH6030* HIH6030_inst, int address, i2c_port_t port, int frequency, gpio_num_t sda_gpio, gpio_pullup_t sda_pullup_state, 
                                                                                      gpio_num_t scl_gpio, gpio_pullup_t scl_pullup_state,
                                                                                      SemaphoreHandle_t* data_mutex, SemaphoreHandle_t* bus_mutex)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t ret = ESP_OK;

    int i2c_master_port = port;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda_gpio;
    conf.sda_pullup_en = sda_pullup_state;
    conf.scl_io_num = scl_gpio;
    conf.scl_pullup_en = scl_pullup_state;
    conf.master.clk_speed = frequency;
    //i2c_param_config(i2c_master_port, &conf);
    //ret=i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
    //if(ret != ESP_OK)
    //{
        /*return ret*/;
    //} 
    HIH6030_inst->address = address;
    HIH6030_inst->port = port;
    HIH6030_inst->i2c_bus_mutex=*bus_mutex;
    HIH6030_inst->device_data_mutex=*data_mutex;

    return ret;
}

/*Only to be used if device is the only one on the i2c bus or for testing*/
esp_err_t HIH6030_deinit(i2c_port_t port)
{
    return(i2c_driver_delete(port));
}

esp_err_t get_temp_humidity(HIH6030* HIH6030_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

    uint8_t data_rd[HIH6030_PAYLOAD_BYTES] = {0};

    uint16_t humidity_raw = 0;
    uint16_t temperature_raw = 0;

    xSemaphoreTake(HIH6030_inst->i2c_bus_mutex, portMAX_DELAY);
    xSemaphoreTake(HIH6030_inst->device_data_mutex, portMAX_DELAY);
    esp_err_t ret = HIH6030_i2c_start_measurement(HIH6030_inst);
    xSemaphoreGive(HIH6030_inst->i2c_bus_mutex);
    if(ret != ESP_OK)
    {
        return ret;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
    xSemaphoreTake(HIH6030_inst->i2c_bus_mutex, portMAX_DELAY);
    ret = HIH6030_i2c_read_measurement(HIH6030_inst, data_rd);
    xSemaphoreGive(HIH6030_inst->i2c_bus_mutex);
    xSemaphoreGive(HIH6030_inst->device_data_mutex);
    if(ret != ESP_OK)
    {
        return ret;
    }

    HIH6030_inst->status = extract_bit_field_value(data_rd, 
                                                   HIH6030_STATUS_BIT_WIDTH, 
                                                   HIH6030_STATUS_START_BYTE, 
                                                   HIH6030_STATUS_END_BYTE, 
                                                   HIH6030_STATUS_START_POS);

    humidity_raw = extract_bit_field_value(data_rd, 
                                           HIH6030_HUMIDITY_BIT_WIDTH, 
                                           HIH6030_HUMIDITY_START_BYTE, 
                                           HIH6030_HUMIDITY_END_BYTE, 
                                           HIH6030_HUMIDITY_START_POS);

    temperature_raw = extract_bit_field_value(data_rd, 
                                              HIH6030_TEMPERATURE_BIT_WIDTH, 
                                              HIH6030_TEMPERATURE_START_BYTE, 
                                              HIH6030_TEMPERATURE_END_BYTE, 
                                              HIH6030_TEMPERATURE_START_POS);
    
    HIH6030_inst->humidity = (humidity_raw/(pow(2, 14) - 2))*100;
    HIH6030_inst->temperature = ((temperature_raw/(pow(2, 14) - 2))*165)-40;

    ESP_LOGD(TAG, "Status from is %d", (int)HIH6030_inst->status);
    ESP_LOGD(TAG, "Humidity is %f%% RH", HIH6030_inst->humidity);
    ESP_LOGD(TAG, "Temperature is %f degree celsius", HIH6030_inst->temperature);
    return ret;
}
