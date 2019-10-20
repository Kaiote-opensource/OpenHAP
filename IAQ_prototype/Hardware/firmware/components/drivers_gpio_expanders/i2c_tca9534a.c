#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "i2c_tca9534a.h"

#define TCA9534A_I2C_TIMEOUT    1000
#define WRITE_BIT               I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN            0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS           0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                 0x0              /*!< I2C ack value */
#define NACK_VAL                0x1              /*!< I2C nack value */
#define LAST_NACK_VAL           0x0              /*!< I2C final nack value */
                    
static const char* TAG = "GPIO_EXPANDER_TCA9534A";

static esp_err_t tca9534a_i2c_write(const TCA9534 *tca9534a_inst, tca9534a_register_t tca9534a_reg, uint8_t data)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

    if (tca9534a_reg > TCA9534A_CONFIG_REG || tca9534a_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t read_back = 0;

    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, ((tca9534a_inst->address) << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(i2c_cmd, tca9534a_reg, ACK_CHECK_EN);
    i2c_master_write_byte(i2c_cmd, data, ACK_CHECK_EN);
    i2c_master_stop(i2c_cmd);
    xSemaphoreTake(tca9534a_inst->i2c_bus_mutex, portMAX_DELAY);
    esp_err_t ret = i2c_master_cmd_begin(tca9534a_inst->i2c_port, i2c_cmd, TCA9534A_I2C_TIMEOUT/portTICK_RATE_MS);
    xSemaphoreGive(tca9534a_inst->i2c_bus_mutex);
    i2c_cmd_link_delete(i2c_cmd);
    if (ret != ESP_OK)
    {
        return ret;
    }

    /**
     * Readback verification
     */
    if (tca9534_i2c_read(tca9534a_inst, tca9534a_reg, &read_back) != ESP_OK)
    {
        ESP_LOGE(TAG, "Error performing readback verification i2c transactions");
        return ret;
    }

    if (data != read_back)
    {
        ESP_LOGE(TAG, "Readback verification failed! Readback value is 0x%02X, expected 0x%02X", read_back, data);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Readback verification ok!");
    return ESP_OK;
}

static esp_err_t tca9534_i2c_read(const TCA9534 *tca9534a_inst, tca9534a_register_t tca9534a_reg, uint8_t *data)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

    if (tca9534a_reg > TCA9534A_CONFIG_REG || tca9534a_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, ((tca9534a_inst->address) << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(i2c_cmd, tca9534a_reg, ACK_CHECK_EN);
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, ((tca9534a_inst->address) << 1) | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(i2c_cmd, data, I2C_MASTER_LAST_NACK);
    i2c_master_stop(i2c_cmd);
    xSemaphoreTake(tca9534a_inst->i2c_bus_mutex, portMAX_DELAY);
    esp_err_t ret = i2c_master_cmd_begin(tca9534a_inst->i2c_port, i2c_cmd, TCA9534A_I2C_TIMEOUT/portTICK_RATE_MS);
    xSemaphoreGive(tca9534a_inst->i2c_bus_mutex);
    i2c_cmd_link_delete(i2c_cmd);
    return ret;
}

esp_err_t tca9534_init(TCA9534 *tca9534a_inst, int address, i2c_port_t port, gpio_num_t intr_pin, const SemaphoreHandle_t *i2c_bus_mutex)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

    if(i2c_bus_mutex == NULL && tca9534a_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    gpio_config_t io_conf =
    {
        io_conf.intr_type = GPIO_PIN_INTR_NEGEDGE,  /**< interrupt of rising edge*/
        io_conf.pin_bit_mask = (1ULL<<intr_pin),    /**< bit mask of the pins*/
        io_conf.mode = GPIO_MODE_INPUT,             /**< set as input mode*/
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE,   /**< disable pull-up mode*/
        io_conf.pull_down_en = GPIO_PULLUP_DISABLE  /**< disable pull-up mode*/
    };

    tca9534a_inst->address = address;
    tca9534a_inst->i2c_port = port;
    tca9534a_inst->i2c_bus_mutex = *i2c_bus_mutex;

    return gpio_config(&io_conf); /**< Initialise interrupt pin settings from the TCA9534*/
}

/**
 * Set all pin directions on the port - Without any regard as to their current states
 */
esp_err_t tca9534_set_port_direction(const TCA9534 *tca9534a_inst, uint8_t port)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

    if(tca9534a_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = tca9534_i2c_write(tca9534a_inst, TCA9534A_CONFIG_REG);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error writing configuration to [config_reg-0x%02X], returned %s", TCA9534A_CONFIG_REG, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Configuration written to [config_reg-0x%02X]", TCA9534A_CONFIG_REG);
    return ESP_OK;
}

/**
 * Convenience function to set individual pins
 */
esp_err_t tca9534_set_pin_direction(const TCA9534 *tca9534a_inst, uint8_t pin, tca9534a_pintype_t pin_type)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

    if (tca9534a_inst == NULL || pin_type != TCA9534_INPUT || pin_type != TCA9534_OUTPUT || pin > 8)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data = 0;

    esp_err_t ret = tca9534_i2c_read(tca9534a_inst, TCA9534A_CONFIG_REG, &data);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading configuration in [config_reg-0x%02X], returned %s", TCA9534A_CONFIG_REG, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Configuration read from [config_reg-0x%02X] is 0x%02X", TCA9534A_CONFIG_REG, data);
    data = (pin_type == TCA9534_INPUT)?(data|(1 << pin)):(data&~(1 << pin));
    ret = tca9534_i2c_write(tca9534a_inst, TCA9534A_CONFIG_REG, data);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error writing configuration to [config_reg-0x%02X], returned %s", TCA9534A_CONFIG_REG, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Configuration written to [config_reg-0x%02X] is 0x%02X", TCA9534A_CONFIG_REG, data);
    return ESP_OK;
}

esp_err_t tca9534_get_pin_direction(const TCA9534 *tca9534a_inst, uint8_t pin, tca9534a_pintype_t *value)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (tca9534a_inst == NULL || pin > 8)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data = 0;

    esp_err_t ret = tca9534_i2c_read(tca9534a_inst, TCA9534A_CONFIG_REG, &config_data);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading configuration in [config_reg-0x%02X], returned %s", TCA9534A_CONFIG_REG, esp_err_to_name(ret));
        return ret;
    }

    *value = (data&(1 << pin))?TCA9534_INPUT:TCA9534_OUTPUT;
    ESP_LOGI(TAG, "Pin %d direction read from [config_reg-0x%02X] is %s", pin, TCA9534A_CONFIG_REG, (*value == TCA9534_INPUT)?"input":"output");
    return ESP_OK;
}

esp_err_t tca9534_set_level(const TCA9534 *tca9534a_inst, uint8_t pin, tca9534a_level_t level)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (tca9534a_inst == NULL || pin > 8 || level != TCA9534_HIGH || level != TCA9534_LOW)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data;

    esp_err_t ret = tca9534_i2c_read(tca9534a_inst, TCA9534A_OUTPUT_PORT_REG, &data);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading configuration in [Output_reg-0x%02X], returned %s", TCA9534A_OUTPUT_PORT_REG, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Configuration read from [Output_reg-0x%02X] is 0x%02X", TCA9534A_OUTPUT_PORT_REG, data);
    data = (level == TCA9534_HIGH)?(data|(1 << pin)):(data&~(1 << pin));
    ret = tca9534_i2c_write(tca9534a_inst, TCA9534A_OUTPUT_PORT_REG, data);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error writing gpio level to [Output_reg-0x%02X], returned %s", TCA9534A_OUTPUT_PORT_REG, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Configuration written to [Output_reg-0x%02X] is 0x%02X", TCA9534A_OUTPUT_PORT_REG, data);
    return ESP_OK;
}

esp_err_t tca9534_get_level(const TCA9534 *tca9534a_inst, uint8_t pin, tca9534a_level_t *level)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (tca9534a_inst == NULL || pin > 8)
    {
        return ESP_ERR_INVALID_ARG;
    }

    tca9534a_pintype_t pin_direction;
    uint8_t port_level = 0;

    esp_err_t ret = tca9534_get_pin_direction(tca9534a_inst, pin, &pin_direction);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (pin_direction == TCA9534_INPUT)
    {
        ret = tca9534_i2c_read(tca9534a_inst, TCA9534A_INPUT_PORT_REG, &port_level);
        if (ret != ESP_OK)
        {
            ESP_LOGD(TAG, "Error reading configuration in [Input_reg] 0x%02X, returned %s", TCA9534A_INPUT_PORT_REG, esp_err_to_name(ret));
            return ret;
        }

        ESP_LOGD(TAG, "Pin %d is input with level set to %s", pin, (port_level & (1 << pin))?"high":"low");
        *level = port_level & (1 << pin) ? TCA9534_HIGH : TCA9534_LOW;
        return ESP_OK;
    }
    else
    {
        ret = tca9534_i2c_read(tca9534a_inst, TCA9534A_OUTPUT_PORT_REG, &port_level);
        if (ret != ESP_OK)
        {
            ESP_LOGD(TAG, "Error reading configuration in [Output_reg] 0x%02X, returned %s", TCA9534A_OUTPUT_PORT_REG, esp_err_to_name(ret));
            return ret;
        }

        ESP_LOGD(TAG, "Pin %d is output with level set to %s", pin, (port_level & (1 << pin))?"high":"low");
        *level = port_level & (1 << pin) ? 1 : 0;
        return ESP_OK;
    }
}