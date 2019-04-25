/**
 * @file ds3231.c
 *
 * ESP-IDF driver for DS3231 high precision RTC module
 *
 * Ported from esp-open-rtos
 * Copyright (C) 2015 Richard A Burton <richardaburton@gmail.com>
 * Copyright (C) 2016 Bhuvanchandra DV <bhuvanchandra.dv@gmail.com>
 * MIT Licensed as described in the file LICENSE
 */

#include "DS3231.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"


#define DS3231_STAT_OSCILLATOR 0x80
#define DS3231_STAT_32KHZ      0x08
#define DS3231_STAT_BUSY       0x04
#define DS3231_STAT_ALARM_2    0x02
#define DS3231_STAT_ALARM_1    0x01

#define DS3231_CTRL_OSCILLATOR    0x80
#define DS3231_CTRL_SQUAREWAVE_BB 0x40
#define DS3231_CTRL_TEMPCONV      0x20
#define DS3231_CTRL_ALARM_INTS    0x04
#define DS3231_CTRL_ALARM2_INT    0x02
#define DS3231_CTRL_ALARM1_INT    0x01

#define DS3231_ALARM_WDAY   0x40
#define DS3231_ALARM_NOTSET 0x80

#define DS3231_ADDR_TIME    0x00
#define DS3231_ADDR_ALARM1  0x07
#define DS3231_ADDR_ALARM2  0x0b
#define DS3231_ADDR_CONTROL 0x0e
#define DS3231_ADDR_STATUS  0x0f
#define DS3231_ADDR_AGING   0x10
#define DS3231_ADDR_TEMP    0x11

#define DS3231_12HOUR_FLAG  0x40
#define DS3231_12HOUR_MASK  0x1f
#define DS3231_PM_FLAG      0x20
#define DS3231_MONTH_MASK   0x1f

enum {
    DS3231_SET = 0,
    DS3231_CLEAR,
    DS3231_REPLACE
};

static char* TAG = "DS3231";

static uint8_t bcd2dec(uint8_t val)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    return (val >> 4) * 10 + (val & 0x0f);
}

static uint8_t dec2bcd(uint8_t val)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    return ((val / 10) << 4) + (val % 10);
}

esp_err_t ds3231_init(DS3231* DS3231_inst, int address, i2c_port_t port, SemaphoreHandle_t* bus_mutex)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(bus_mutex != NULL && DS3231_inst != NULL)
    {
        DS3231_inst->address = address;
        DS3231_inst->i2c_port = port;
        DS3231_inst->i2c_bus_mutex=*bus_mutex;
        return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
}

static esp_err_t ds3231_i2c_read(const DS3231* DS3231_inst, const void *out_data, size_t out_size, void *in_data, size_t in_size)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t ret;
    
    if (DS3231_inst == NULL || in_data == NULL || in_size == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (out_data && out_size)
    {
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (DS3231_inst->address) << 1, true);
        i2c_master_write(cmd, (void *)out_data, out_size, true);
    }
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_inst->address) << 1 | 1, true);
    i2c_master_read(cmd, in_data, in_size, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin((DS3231_inst->i2c_port), cmd, 1000/portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not read from device [0x%02x at %d]: returned %d", (DS3231_inst->address), (DS3231_inst->i2c_port), ret);
    }
    return ret;    
}
static inline esp_err_t ds3231_i2c_read_reg(const DS3231* DS3231_inst, uint8_t reg, void *in_data, size_t in_size)
{
    return ds3231_i2c_read(DS3231_inst, &reg, 1, in_data, in_size);
}

esp_err_t ds3231_i2c_write(const DS3231* DS3231_inst, const void *out_reg, size_t out_reg_size, const void *out_data, size_t out_size)
{
    if (DS3231_inst == NULL || out_data == NULL || out_size == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_inst->address) << 1, true);
    if (out_reg && out_reg_size)
        i2c_master_write(cmd, (void *)out_reg, out_reg_size, true);
    i2c_master_write(cmd, (void *)out_data, out_size, true);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin((DS3231_inst->i2c_port), cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not read from device [0x%02x at %d]: returned %d", (DS3231_inst->address), (DS3231_inst->i2c_port), ret);
    }
    return ret; 

}

static inline esp_err_t ds3231_i2c_write_reg(const DS3231* DS3231_inst, uint8_t reg, const void *out_data, size_t out_size)
{
    return  ds3231_i2c_write(DS3231_inst, &reg, 1, out_data, out_size);
}

esp_err_t ds3231_set_time(DS3231* DS3231_inst, struct tm *time)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (DS3231_inst == NULL || time == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;

    uint8_t data[7];

    /* time/date data */
    data[0] = dec2bcd(time->tm_sec);
    data[1] = dec2bcd(time->tm_min);
    data[2] = dec2bcd(time->tm_hour);
    /* The week data must be in the range 1 to 7, and to keep the start on the
     * same day as for tm_wday have it start at 1 on Sunday. */
    data[3] = dec2bcd(time->tm_wday + 1);
    data[4] = dec2bcd(time->tm_mday);
    data[5] = dec2bcd(time->tm_mon + 1);
    data[6] = dec2bcd((time->tm_year+1900)-2000);

    xSemaphoreTake(DS3231_inst->i2c_bus_mutex, portMAX_DELAY);
    ret = ds3231_i2c_write_reg(DS3231_inst, DS3231_ADDR_TIME, data, 7);
    xSemaphoreGive(DS3231_inst->i2c_bus_mutex);

    return ret;
}

esp_err_t ds3231_set_alarm(DS3231* DS3231_inst, ds3231_alarm_t alarms, struct tm *time1,
        ds3231_alarm1_rate_t option1, struct tm *time2, ds3231_alarm2_rate_t option2)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (DS3231_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    int i = 0;
    uint8_t data[7];

    /* alarm 1 data */
    if (alarms != DS3231_ALARM_2)
    {
        if (time1 == NULL)
        {
            return ESP_ERR_INVALID_ARG;
        }
        data[i++] = (option1 >= DS3231_ALARM1_MATCH_SEC ? dec2bcd(time1->tm_sec) : DS3231_ALARM_NOTSET);
        data[i++] = (option1 >= DS3231_ALARM1_MATCH_SECMIN ? dec2bcd(time1->tm_min) : DS3231_ALARM_NOTSET);
        data[i++] = (option1 >= DS3231_ALARM1_MATCH_SECMINHOUR ? dec2bcd(time1->tm_hour) : DS3231_ALARM_NOTSET);
        data[i++] = (option1 == DS3231_ALARM1_MATCH_SECMINHOURDAY ? (dec2bcd(time1->tm_wday + 1) & DS3231_ALARM_WDAY):
                    (option1 == DS3231_ALARM1_MATCH_SECMINHOURDATE ? dec2bcd(time1->tm_mday) : DS3231_ALARM_NOTSET));
    }

    /* alarm 2 data */
    if (alarms != DS3231_ALARM_1)
    {
        if (time2 == NULL)
        {
            return ESP_ERR_INVALID_ARG;
        }
        data[i++] = (option2 >= DS3231_ALARM2_MATCH_MIN ? dec2bcd(time2->tm_min) : DS3231_ALARM_NOTSET);
        data[i++] = (option2 >= DS3231_ALARM2_MATCH_MINHOUR ? dec2bcd(time2->tm_hour) : DS3231_ALARM_NOTSET);
        data[i++] = (option2 == DS3231_ALARM2_MATCH_MINHOURDAY ? (dec2bcd(time2->tm_wday + 1) & DS3231_ALARM_WDAY) :
                    (option2 == DS3231_ALARM2_MATCH_MINHOURDATE ? dec2bcd(time2->tm_mday) : DS3231_ALARM_NOTSET));
    }

    xSemaphoreTake(DS3231_inst->i2c_bus_mutex, portMAX_DELAY);
    ret = ds3231_i2c_write_reg(DS3231_inst, (alarms == DS3231_ALARM_2 ? DS3231_ADDR_ALARM2 : DS3231_ADDR_ALARM1), data, i);
    xSemaphoreGive(DS3231_inst->i2c_bus_mutex);

    return ret;
}

/* Get a byte containing just the requested bits
 * pass the register address to read, a mask to apply to the register and
 * an uint* for the output
 * you can test this value directly as true/false for specific bit mask
 * of use a mask of 0xff to just return the whole register byte
 * returns true to indicate success
 */
static esp_err_t ds3231_get_flag(DS3231* DS3231_inst, uint8_t addr, uint8_t mask, uint8_t *flag)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    uint8_t data;

    /* get register */
    esp_err_t ret = ds3231_i2c_read_reg(DS3231_inst, addr, &data, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }
    /* return only requested flag */
    *flag = (data & mask);

    return ESP_OK;
}

/* Set/clear bits in a byte register, or replace the byte altogether
 * pass the register address to modify, a byte to replace the existing
 * value with or containing the bits to set/clear and one of
 * DS3231_SET/DS3231_CLEAR/DS3231_REPLACE
 * returns true to indicate success
 */
static esp_err_t ds3231_set_flag(DS3231* DS3231_inst, uint8_t addr, uint8_t bits, uint8_t mode)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    uint8_t data;

    /* get status register */
    esp_err_t ret = ds3231_i2c_read_reg(DS3231_inst, addr, &data, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }
    /* clear the flag */
    if (mode == DS3231_REPLACE)
        data = bits;
    else if (mode == DS3231_SET)
        data |= bits;
    else
        data &= ~bits;

    return ds3231_i2c_write_reg(DS3231_inst, addr, &data, 1);
}

esp_err_t ds3231_get_oscillator_stop_flag(DS3231* DS3231_inst, bool *flag)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (DS3231_inst == NULL || flag == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    uint8_t f;

    xSemaphoreTake(DS3231_inst->i2c_bus_mutex, portMAX_DELAY);
    ret = ds3231_get_flag(DS3231_inst, DS3231_ADDR_STATUS, DS3231_STAT_OSCILLATOR, &f);
    xSemaphoreGive(DS3231_inst->i2c_bus_mutex);

    if(ret != ESP_OK)
    {
        return ret;
    }
    *flag = (f ? true : false);

    return ESP_OK;

}

esp_err_t ds3231_clear_oscillator_stop_flag(DS3231* DS3231_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (DS3231_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;

    xSemaphoreTake(DS3231_inst->i2c_bus_mutex, portMAX_DELAY);
    ret = ds3231_set_flag(DS3231_inst, DS3231_ADDR_STATUS, DS3231_STAT_OSCILLATOR, DS3231_CLEAR);
    xSemaphoreGive(DS3231_inst->i2c_bus_mutex);

    return ret;
}

esp_err_t ds3231_get_alarm_flags(DS3231* DS3231_inst, ds3231_alarm_t *alarms)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (DS3231_inst == NULL || alarms == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;

    xSemaphoreTake(DS3231_inst->i2c_bus_mutex, portMAX_DELAY);
    ret = ds3231_get_flag(DS3231_inst, DS3231_ADDR_STATUS, DS3231_ALARM_BOTH, (uint8_t *)alarms);
    xSemaphoreGive(DS3231_inst->i2c_bus_mutex);

    return ret;
}

esp_err_t ds3231_clear_alarm_flags(DS3231* DS3231_inst, ds3231_alarm_t alarms)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (DS3231_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;

    xSemaphoreTake(DS3231_inst->i2c_bus_mutex, portMAX_DELAY);
    ret = ds3231_set_flag(DS3231_inst, DS3231_ADDR_STATUS, alarms, DS3231_CLEAR);
    xSemaphoreGive(DS3231_inst->i2c_bus_mutex);

    return ret;
}

esp_err_t ds3231_enable_alarm_ints(DS3231* DS3231_inst, ds3231_alarm_t alarms)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (DS3231_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;

    xSemaphoreTake(DS3231_inst->i2c_bus_mutex, portMAX_DELAY);
    ret = ds3231_set_flag(DS3231_inst, DS3231_ADDR_CONTROL, DS3231_CTRL_ALARM_INTS | alarms, DS3231_SET);
    xSemaphoreGive(DS3231_inst->i2c_bus_mutex);

    return ret;
}

esp_err_t ds3231_disable_alarm_ints(DS3231* DS3231_inst, ds3231_alarm_t alarms)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (DS3231_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Just disable specific alarm(s) requested
     * does not disable alarm interrupts generally (which would enable the squarewave)
     */
    esp_err_t ret;

    xSemaphoreTake(DS3231_inst->i2c_bus_mutex, portMAX_DELAY);
    ret = ds3231_set_flag(DS3231_inst, DS3231_ADDR_CONTROL, alarms, DS3231_CLEAR);
    xSemaphoreGive(DS3231_inst->i2c_bus_mutex);

    return ret;
}

esp_err_t ds3231_enable_32khz(DS3231* DS3231_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (DS3231_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;

    xSemaphoreTake(DS3231_inst->i2c_bus_mutex, portMAX_DELAY);
    ret = ds3231_set_flag(DS3231_inst, DS3231_ADDR_STATUS, DS3231_STAT_32KHZ, DS3231_SET);
    xSemaphoreGive(DS3231_inst->i2c_bus_mutex);

    return ret;
}

esp_err_t ds3231_disable_32khz(DS3231* DS3231_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (DS3231_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;

    xSemaphoreTake(DS3231_inst->i2c_bus_mutex, portMAX_DELAY);
    ret = ds3231_set_flag(DS3231_inst, DS3231_ADDR_STATUS, DS3231_STAT_32KHZ, DS3231_CLEAR);
    xSemaphoreGive(DS3231_inst->i2c_bus_mutex);

    return ret;
}

esp_err_t ds3231_enable_squarewave(DS3231* DS3231_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (DS3231_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;

    xSemaphoreTake(DS3231_inst->i2c_bus_mutex, portMAX_DELAY);
    ret = ds3231_set_flag(DS3231_inst, DS3231_ADDR_CONTROL, DS3231_CTRL_ALARM_INTS, DS3231_CLEAR);
    xSemaphoreGive(DS3231_inst->i2c_bus_mutex);

    return ret;
}

esp_err_t ds3231_disable_squarewave(DS3231* DS3231_inst)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (DS3231_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;

    xSemaphoreTake(DS3231_inst->i2c_bus_mutex, portMAX_DELAY);
    ret = ds3231_set_flag(DS3231_inst, DS3231_ADDR_CONTROL, DS3231_CTRL_ALARM_INTS, DS3231_SET);
    xSemaphoreGive(DS3231_inst->i2c_bus_mutex);

    return ret;
}

esp_err_t ds3231_set_squarewave_freq(DS3231* DS3231_inst, ds3231_sqwave_freq_t freq)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (DS3231_inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;
    uint8_t flag = 0;

    xSemaphoreTake(DS3231_inst->i2c_bus_mutex, portMAX_DELAY);
    ret = ds3231_get_flag(DS3231_inst, DS3231_ADDR_CONTROL, 0xff, &flag);
    if(ret != ESP_OK)
    {
        xSemaphoreGive(DS3231_inst->i2c_bus_mutex);
        return ret;
    }
    flag &= ~DS3231_SQWAVE_8192HZ;
    flag |= freq;
    ret = ds3231_set_flag(DS3231_inst, DS3231_ADDR_CONTROL, flag, DS3231_REPLACE);
    xSemaphoreGive(DS3231_inst->i2c_bus_mutex);

    return ret;
}

esp_err_t ds3231_get_raw_temp(DS3231* DS3231_inst, int16_t *temp)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (DS3231_inst == NULL || temp== NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    uint8_t data[2];

    xSemaphoreTake(DS3231_inst->i2c_bus_mutex, portMAX_DELAY);
    ret = ds3231_i2c_read_reg(DS3231_inst, DS3231_ADDR_TEMP, data, sizeof(data));
    xSemaphoreGive(DS3231_inst->i2c_bus_mutex);

    if(ret != ESP_OK)
    {
        xSemaphoreGive(DS3231_inst->i2c_bus_mutex);
        return ret;
    }
    *temp = (int16_t)(int8_t)data[0] << 2 | data[1] >> 6;

    return ESP_OK;
}

esp_err_t ds3231_get_temp_integer(DS3231* DS3231_inst, int8_t *temp)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (temp == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int16_t t_int;

    esp_err_t ret = ds3231_get_raw_temp(DS3231_inst, &t_int);
    if (ret != ESP_OK)
    {
        return ret;
    }
    *temp = t_int >> 2;

    return ESP_OK;
}

esp_err_t ds3231_get_temp_float(DS3231* DS3231_inst, float *temp)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (DS3231_inst ==NULL || temp == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int16_t t_int;

    esp_err_t ret = ds3231_get_raw_temp(DS3231_inst, &t_int);
    if (ret == ESP_OK)
    {
        return ret;
    }
    *temp = t_int * 0.25;

    return ESP_OK;
}

esp_err_t ds3231_get_time(DS3231* DS3231_inst, struct tm *time)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if (DS3231_inst ==NULL || time == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    uint8_t data[7];

    /* read time */
    xSemaphoreTake(DS3231_inst->i2c_bus_mutex, portMAX_DELAY);
    ret = ds3231_i2c_read_reg(DS3231_inst, DS3231_ADDR_TIME, data, 7);
    xSemaphoreGive(DS3231_inst->i2c_bus_mutex);
    if(ret != ESP_OK)
    {
        return ret;
    }

    /* convert to unix time structure */
    time->tm_sec = bcd2dec(data[0]);
    time->tm_min = bcd2dec(data[1]);
    if (data[2] & DS3231_12HOUR_FLAG)
    {
        /* 12H */
        time->tm_hour = bcd2dec(data[2] & DS3231_12HOUR_MASK) - 1;
        /* AM/PM? */
        if (data[2] & DS3231_PM_FLAG) time->tm_hour += 12;
    }
    else
    {
        time->tm_hour = bcd2dec(data[2]); /* 24H */
    }
    time->tm_wday = bcd2dec(data[3]) - 1;
    time->tm_mday = bcd2dec(data[4]);
    time->tm_mon  = bcd2dec(data[5] & DS3231_MONTH_MASK) - 1;
    time->tm_year = (bcd2dec(data[6])+2000)-1900;
    time->tm_isdst = 0;

    // apply a time zone (if you are not using localtime on the rtc or you want to check/apply DST)
    //applyTZ(time);

    return ESP_OK;
}
