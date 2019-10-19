#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "TCA9534A.h"

#define WRITE_BIT I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ   /*!< I2C master read */
#define ACK_CHECK_EN 0x1           /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0          /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                /*!< I2C ack value */
#define NACK_VAL 0x1               /*!< I2C nack value */
#define LAST_NACK_VAL 0x0          /*!< I2C final nack value */

static char *TAG = "TCA9534A";

static esp_err_t TCA9534_i2c_read(TCA9534 *TCA9534_inst, uint8_t reg, uint8_t *data);
static esp_err_t TCA9534_i2c_write(TCA9534 *TCA9534_inst, uint8_t reg);

static esp_err_t TCA9534_i2c_write(TCA9534 *TCA9534_inst, uint8_t reg)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    uint8_t read_back = 0;

    if (reg <= 3 && reg > 0)
    {
        i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
        i2c_master_start(i2c_cmd);
        i2c_master_write_byte(i2c_cmd, ((TCA9534_inst->address) << 1) | WRITE_BIT, ACK_CHECK_EN);
        i2c_master_write_byte(i2c_cmd, reg, ACK_CHECK_EN);
        if (reg == TCA9534_OUTPUT_PORT_REG)
        {
            ESP_LOGD(TAG, "Now performing I2C write to [Output port reg] with value 0x%02X", TCA9534_inst->port);
            i2c_master_write_byte(i2c_cmd, TCA9534_inst->port, ACK_CHECK_EN);
        }
        else if (reg == TCA9534_POLARITY_REG)
        {
            ESP_LOGD(TAG, "Now performing I2C write to [Polarity reg] with value 0x%02X", TCA9534_inst->polarity);
            i2c_master_write_byte(i2c_cmd, TCA9534_inst->polarity, ACK_CHECK_EN);
        }
        else
        {
            ESP_LOGD(TAG, "Now performing I2C write to [config reg] with value 0x%02X", TCA9534_inst->pinModeConf);
            i2c_master_write_byte(i2c_cmd, TCA9534_inst->pinModeConf, ACK_CHECK_EN);
        }
        i2c_master_stop(i2c_cmd);
        esp_err_t ret = i2c_master_cmd_begin(TCA9534_inst->i2c_port, i2c_cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(i2c_cmd);

        if (ret != ESP_OK)
        {
            return ret;
        }
        ret = TCA9534_i2c_read(TCA9534_inst, reg, &read_back);

        if (ret != ESP_OK)
        {
            return ret;
        }
        if (reg == TCA9534_OUTPUT_PORT_REG && read_back != TCA9534_inst->port)
        {
            return ESP_FAIL;
        }
        else if (reg == TCA9534_POLARITY_REG && read_back != TCA9534_inst->polarity)
        {
            return ESP_FAIL;
        }
        else if (reg == TCA9534_CONFIG_REG && read_back != TCA9534_inst->pinModeConf)
        {
            return ESP_FAIL;
        }
        return ESP_OK;
    }
    else
    {
        return ESP_ERR_INVALID_ARG;
    }
}

static esp_err_t TCA9534_i2c_read(TCA9534 *TCA9534_inst, uint8_t reg, uint8_t *data)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (reg <= 3)
    {
        i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
        i2c_master_start(i2c_cmd);
        i2c_master_write_byte(i2c_cmd, ((TCA9534_inst->address) << 1) | WRITE_BIT, ACK_CHECK_EN);
        i2c_master_write_byte(i2c_cmd, reg, ACK_CHECK_EN);
        i2c_master_start(i2c_cmd);
        i2c_master_write_byte(i2c_cmd, ((TCA9534_inst->address) << 1) | READ_BIT, ACK_CHECK_EN);
        i2c_master_read_byte(i2c_cmd, data, I2C_MASTER_LAST_NACK);
        i2c_master_stop(i2c_cmd);
        esp_err_t ret = i2c_master_cmd_begin(TCA9534_inst->i2c_port, i2c_cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(i2c_cmd);
        return ret;
    }
    else
    {
        return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t TCA9534_init(TCA9534 *TCA9534_inst, int address, i2c_port_t port, gpio_num_t intr_pin, SemaphoreHandle_t *i2c_bus_mutex)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t ret;

    if(i2c_bus_mutex != NULL && TCA9534_inst != NULL)
    {
        TCA9534_inst->address = address;
        TCA9534_inst->i2c_port = port;
        TCA9534_inst->i2c_bus_mutex = *i2c_bus_mutex;
    }
    else
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*Initialise interrupt pin settings from the TCA9534*/
    gpio_config_t io_conf =
    {
        //interrupt of rising edge
        io_conf.intr_type = GPIO_PIN_INTR_NEGEDGE,
        //bit mask of the pins
        io_conf.pin_bit_mask = (1ULL<<intr_pin),
        //set as input mode
        io_conf.mode = GPIO_MODE_INPUT,
        //disable pull-up mode
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE,
        //disable pull-up mode
        io_conf.pull_down_en = GPIO_PULLUP_DISABLE
    };

    ret = gpio_config(&io_conf);

    return ret;
}

// /*Only to be used if device is the only one on the i2c bus or for testing*/
// esp_err_t TCA9534_deinit(i2c_port_t port)
// {
//     return (i2c_driver_delete(port));
// }

esp_err_t TCA9534_set_port_direction(TCA9534 *TCA9534_inst, uint8_t port)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    xSemaphoreTake(TCA9534_inst->i2c_bus_mutex, portMAX_DELAY);
    esp_err_t ret = TCA9534_i2c_read(TCA9534_inst, TCA9534_CONFIG_REG, &(TCA9534_inst->pinModeConf));
    if (ret != ESP_OK)
    {
        ESP_LOGD(TAG, "Error reading configuration in [config reg] 0x%02X on TCA9534 device, returned %d", TCA9534_CONFIG_REG, ret);
        return ret;
    }
    ESP_LOGD(TAG, "Configuration read from [config reg] 0x%02X on TCA9534 device is 0x%02X", TCA9534_CONFIG_REG, TCA9534_inst->pinModeConf);
    TCA9534_inst->pinModeConf = (uint8_t)port;
    ret = TCA9534_i2c_write(TCA9534_inst, TCA9534_CONFIG_REG);
    xSemaphoreGive(TCA9534_inst->i2c_bus_mutex);
    if (ret != ESP_OK)
    {
        ESP_LOGD(TAG, "Error writing configuration to [config reg] 0x%02X on TCA9534 device, returned %d", TCA9534_CONFIG_REG, ret);
        return ret;
    }
    ESP_LOGD(TAG, "Configuration written to [config reg] 0x%02X on TCA9534 device is 0x%02X", TCA9534_CONFIG_REG, TCA9534_inst->pinModeConf);
    return ESP_OK;
}

/*Convenience function to set pins individually*/
esp_err_t TCA9534_set_pin_direction(TCA9534 *TCA9534_inst, uint8_t pin, int pin_type)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t ret;

    if (pin > 8)
    {
        ESP_LOGD(TAG, "Error, number of pin passed is invalid");
        return ESP_ERR_INVALID_ARG;
    }
    if (pin_type == TCA9534_INPUT)
    {
        xSemaphoreTake(TCA9534_inst->i2c_bus_mutex, portMAX_DELAY);
        ret = TCA9534_i2c_read(TCA9534_inst, TCA9534_CONFIG_REG, &(TCA9534_inst->pinModeConf));
        if (ret != ESP_OK)
        {
            ESP_LOGD(TAG, "Error reading configuration in [config reg] 0x%02X on TCA9534 device, returned %d", TCA9534_CONFIG_REG, ret);
            return ret;
        }
        ESP_LOGD(TAG, "Configuration read from [config reg] 0x%02X on TCA9534 device is 0x%02X", TCA9534_CONFIG_REG, TCA9534_inst->pinModeConf);
        TCA9534_inst->pinModeConf = TCA9534_inst->pinModeConf | (1 << pin);
        ret = TCA9534_i2c_write(TCA9534_inst, TCA9534_CONFIG_REG);
        xSemaphoreGive(TCA9534_inst->i2c_bus_mutex);
        if (ret != ESP_OK)
        {
            ESP_LOGD(TAG, "Error writing configuration to [config reg] 0x%02X on TCA9534 device, returned %d", TCA9534_CONFIG_REG, ret);
            return ret;
        }
        ESP_LOGD(TAG, "Configuration written to [config reg] 0x%02X on TCA9534 device is 0x%02X", TCA9534_CONFIG_REG, TCA9534_inst->pinModeConf);
        return ESP_OK;
    }
    else if (pin_type == TCA9534_OUTPUT)
    {
        xSemaphoreTake(TCA9534_inst->i2c_bus_mutex, portMAX_DELAY);
        ret = TCA9534_i2c_read(TCA9534_inst, TCA9534_CONFIG_REG, &(TCA9534_inst->pinModeConf));
        if (ret != ESP_OK)
        {
            ESP_LOGD(TAG, "Error reading configuration in [config reg] 0x%02X on TCA9534 device, returned %d", TCA9534_CONFIG_REG, ret);
            return ret;
        }
        ESP_LOGD(TAG, "Configuration read from [config reg] 0x%02X on TCA9534 device is 0x%02X", TCA9534_CONFIG_REG, TCA9534_inst->pinModeConf);
        TCA9534_inst->pinModeConf = TCA9534_inst->pinModeConf & ~(1 << pin);
        ret = TCA9534_i2c_write(TCA9534_inst, TCA9534_CONFIG_REG);
        xSemaphoreGive(TCA9534_inst->i2c_bus_mutex);
        if (ret != ESP_OK)
        {
            ESP_LOGD(TAG, "Error writing configuration to [config reg] 0x%02X on TCA9534 device, returned %d", TCA9534_CONFIG_REG, ret);
            return ret;
        }
        ESP_LOGD(TAG, "Configuration written to [config reg] 0x%02X on TCA9534 device is 0x%02X", TCA9534_CONFIG_REG, TCA9534_inst->pinModeConf);
        return ESP_OK;
    }
    else
    {
        return ESP_FAIL;
    }
}

esp_err_t TCA9534_get_port_direction(TCA9534 *TCA9534_inst, uint8_t pin, uint8_t *value)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    uint8_t config_data = 0;

    if (pin > 8)
    {
        ESP_LOGD(TAG, "Error, number of pin passed is invalid");
        return ESP_ERR_INVALID_ARG;
    }
    xSemaphoreTake(TCA9534_inst->i2c_bus_mutex, portMAX_DELAY);
    esp_err_t ret = TCA9534_i2c_read(TCA9534_inst, TCA9534_CONFIG_REG, &config_data);
    xSemaphoreGive(TCA9534_inst->i2c_bus_mutex);
    if (ret != ESP_OK)
    {
        ESP_LOGD(TAG, "Error reading configuration in [config reg] 0x%02X on TCA9534 device, returned %d", TCA9534_CONFIG_REG, ret);
        return ret;
    }
    *value = config_data & (1 << pin);
    return ESP_OK;
}

esp_err_t TCA9534_set_level(TCA9534 *TCA9534_inst, uint8_t pin, int32_t level)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t ret;

    if (pin > 8)
    {
        ESP_LOGD(TAG, "Error, number of pin passed is invalid");
        return ESP_ERR_INVALID_ARG;
    }

    if (level == 1)
    {
        xSemaphoreTake(TCA9534_inst->i2c_bus_mutex, portMAX_DELAY);
        ret = TCA9534_i2c_read(TCA9534_inst, TCA9534_OUTPUT_PORT_REG, &(TCA9534_inst->port));
        if (ret != ESP_OK)
        {
            ESP_LOGD(TAG, "Error reading configuration in [Output port reg] 0x%02X on TCA9534 device, returned %d", TCA9534_OUTPUT_PORT_REG, ret);
            return ret;
        }
        ESP_LOGD(TAG, "Configuration read from [Output port reg] 0x%02X on TCA9534 device is 0x%02X", TCA9534_OUTPUT_PORT_REG, TCA9534_inst->port);
        TCA9534_inst->port = TCA9534_inst->port | (1 << pin);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        ret = TCA9534_i2c_write(TCA9534_inst, TCA9534_OUTPUT_PORT_REG);
        xSemaphoreGive(TCA9534_inst->i2c_bus_mutex);
        if (ret != ESP_OK)
        {
            ESP_LOGD(TAG, "Error writing configuration to [Output port reg] 0x%02X on TCA9534 device, returned error code %d, wrote data 0x%02X", TCA9534_OUTPUT_PORT_REG,
                     ret, TCA9534_inst->port);
            return ret;
        }
        ESP_LOGD(TAG, "Configuration written to [Output port reg] 0x%02X is 0x%02X, pin %d set to HIGH", TCA9534_OUTPUT_PORT_REG, TCA9534_inst->port, pin);
        return ESP_OK;
    }
    else if (level == 0)
    {
        xSemaphoreTake(TCA9534_inst->i2c_bus_mutex, portMAX_DELAY);
        ret = TCA9534_i2c_read(TCA9534_inst, TCA9534_OUTPUT_PORT_REG, &(TCA9534_inst->port));
        if (ret != ESP_OK)
        {
            ESP_LOGD(TAG, "Error reading configuration in [Output port reg] 0x%02X on TCA9534 device, returned error code %d, wrote data 0x%02X", TCA9534_OUTPUT_PORT_REG,
                     ret, TCA9534_inst->port);
            return ret;
        }
        ESP_LOGD(TAG, "Configuration read from [Output port reg] 0x%02X on TCA9534 device is 0x%02X", TCA9534_OUTPUT_PORT_REG, TCA9534_inst->port);
        TCA9534_inst->port = TCA9534_inst->port & ~(1 << pin);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        ret = TCA9534_i2c_write(TCA9534_inst, TCA9534_OUTPUT_PORT_REG);
        xSemaphoreGive(TCA9534_inst->i2c_bus_mutex);
        if (ret != ESP_OK)
        {
            ESP_LOGD(TAG, "Error writing configuration to [Output port reg] 0x%02X on TCA9534 device, returned %d", TCA9534_OUTPUT_PORT_REG, ret);
            return ret;
        }
        ESP_LOGD(TAG, "Configuration written to [Output port reg] 0x%02X is 0x%02X, pin %d set to LOW", TCA9534_OUTPUT_PORT_REG, TCA9534_inst->port, pin);
        return ESP_OK;
    }
    else
    {
        return ESP_FAIL;
    }
}

esp_err_t TCA9534_get_level(TCA9534 *TCA9534_inst, uint8_t pin, int32_t *level)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    uint8_t port_config = 0;
    uint8_t port_level = 0;
    uint8_t port_polarity = 0;
    uint8_t pin_value = 0;

    if (pin > 8)
    {
        ESP_LOGD(TAG, "Error, number of pin passed is invalid");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = TCA9534_get_port_direction(TCA9534_inst, pin, &port_config);

    if (ret != ESP_OK)
    {
        return ret;
    }
    else
    {
        pin_value = port_config & (1 << pin) ? 1 : 0;
        if (pin_value == TCA9534_INPUT)
        {
            xSemaphoreTake(TCA9534_inst->i2c_bus_mutex, portMAX_DELAY);
            ret = TCA9534_i2c_read(TCA9534_inst, TCA9534_INPUT_PORT_REG, &port_level);
            if (ret != ESP_OK)
            {
                return ret;
            }
            ret = TCA9534_i2c_read(TCA9534_inst, TCA9534_POLARITY_REG, &port_polarity);
            xSemaphoreGive(TCA9534_inst->i2c_bus_mutex);
            if (ret != ESP_OK)
            {
                ESP_LOGD(TAG, "Error reading configuration in [Polarity reg] 0x%02X on TCA9534 device, returned %d", TCA9534_POLARITY_REG, ret);
                return ret;
            }
            ESP_LOGD(TAG, "Pin %d is input with level %s and its polarity level set to %s",
                     pin,
                     port_level & (1 << pin) ? "HIGH" : "LOW",
                     port_polarity & (1 << pin) ? "ACTIVE_LOW" : "ACTIVE_HIGH");

            *level = port_level & (1 << pin) ? 1 : 0;
            return ESP_OK;
        }
        else
        {
            xSemaphoreTake(TCA9534_inst->i2c_bus_mutex, portMAX_DELAY);
            ret = TCA9534_i2c_read(TCA9534_inst, TCA9534_OUTPUT_PORT_REG, &port_level);
            xSemaphoreGive(TCA9534_inst->i2c_bus_mutex);
            if (ret != ESP_OK)
            {
                ESP_LOGD(TAG, "Error reading configuration in [Output port reg] 0x%02X on TCA9534 device, returned %d", TCA9534_OUTPUT_PORT_REG, ret);
                return ret;
            }
            ESP_LOGD(TAG, "Pin %d is output with level %s", pin, port_level & (1 << pin) ? "HIGH" : "LOW");
            *level = port_level & (1 << pin) ? 1 : 0;
            return ESP_OK;
        }
    }
}

static esp_err_t TCA9534_set_pin_polarity(TCA9534 *TCA9534_inst, uint8_t pin, int state)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t ret;
    uint8_t polarity_value = 0;

    if (pin > 8U)
    {
        ESP_LOGD(TAG, "Error, number of pin passed is invalid");
        return ESP_ERR_INVALID_ARG;
    }

    ret = TCA9534_get_port_direction(TCA9534_inst, pin, &(TCA9534_inst->pinModeConf));
    if (ret != ESP_OK)
    {
        ESP_LOGD(TAG, "Error reading configuration in [config reg] 0x%02X on TCA9534 device", TCA9534_CONFIG_REG);
        return ret;
    }
    polarity_value = TCA9534_inst->pinModeConf & (1 << pin) ? TCA9534_INPUT : TCA9534_OUTPUT;

    if (polarity_value == TCA9534_INPUT)
    {
        if (state == TCA9534_ACTIVE_HIGH)
        {
            ESP_LOGD(TAG, "Configuration read from [Polarity reg] 0x%02X on TCA9534 device is  0x%02X", TCA9534_CONFIG_REG, TCA9534_inst->polarity);
            xSemaphoreTake(TCA9534_inst->i2c_bus_mutex, portMAX_DELAY);
            ret = TCA9534_i2c_read(TCA9534_inst, TCA9534_POLARITY_REG, &(TCA9534_inst->polarity));
            if (ret != ESP_OK)
            {
                ESP_LOGD(TAG, "Error reading configuration in [Polarity reg] 0x%02X on TCA9534 device, returned %d", TCA9534_POLARITY_REG, ret);
                return ret;
            }
            ret = TCA9534_i2c_write(TCA9534_inst, TCA9534_POLARITY_REG);
            xSemaphoreGive(TCA9534_inst->i2c_bus_mutex);
            if (ret != ESP_OK)
            {
                ESP_LOGD(TAG, "Error writing configuration to [Polarity reg] 0x%02X on TCA9534 device, returned %d", TCA9534_POLARITY_REG, ret);
                return ret;
            }
            ESP_LOGD(TAG, "Configuration written to [Polarity reg] 0x%02X is 0x%02X, pin %d set to ACTIVE_HIGH", TCA9534_POLARITY_REG, TCA9534_inst->polarity, pin);
            return ESP_OK;
        }
        else if (state == TCA9534_ACTIVE_LOW)
        {
            ESP_LOGD(TAG, "Configuration read from [Polarity reg] 0x%02X on TCA9534 device is  0x%02X", TCA9534_CONFIG_REG, TCA9534_inst->polarity);
            xSemaphoreTake(TCA9534_inst->i2c_bus_mutex, portMAX_DELAY);
            ret = TCA9534_i2c_read(TCA9534_inst, TCA9534_POLARITY_REG, &(TCA9534_inst->polarity));
            if (ret != ESP_OK)
            {
                ESP_LOGD(TAG, "Error reading configuration in [Polarity reg] 0x%02X on TCA9534 device, returned %d", TCA9534_POLARITY_REG, ret);
                return ret;
            }
            TCA9534_inst->polarity = TCA9534_inst->polarity | (1 << pin);
            ret = TCA9534_i2c_write(TCA9534_inst, TCA9534_POLARITY_REG);
            xSemaphoreGive(TCA9534_inst->i2c_bus_mutex);
            if (ret != ESP_OK)
            {
                ESP_LOGD(TAG, "Error writing configuration to [Polarity reg] 0x%02X on TCA9534 device, returned %d", TCA9534_POLARITY_REG, ret);
                return ret;
            }
            ESP_LOGD(TAG, "Configuration written to [Polarity reg] 0x%02X is 0x%02X, pin %d set to ACTIVE_HIGH", TCA9534_POLARITY_REG, TCA9534_inst->polarity, pin);
            return ESP_OK;
        }
        else
        {
            return ESP_ERR_INVALID_ARG;
        }
    }
    else
    {
        return ESP_FAIL;
    }
}
