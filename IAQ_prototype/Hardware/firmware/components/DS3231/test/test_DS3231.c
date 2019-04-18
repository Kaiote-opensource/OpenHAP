#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "DS3231.h"
#include "esp_err.h"
#include "esp_log.h"

#define TAG "DS3231-TEST"

#define DS3231_ADDR         0x68
#define I2C_MASTER_SCL_IO    19
#define I2C_MASTER_SDA_IO    21
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_FREQ_HZ  100000

// setup datetime: 2016-10-09 13:50:10
struct tm time_info;

DS3231 DS3231_inst;

esp_err_t setup_i2c(i2c_port_t port, int frequency, gpio_num_t sda_gpio, gpio_pullup_t sda_pullup_state, gpio_num_t scl_gpio, gpio_pullup_t scl_pullup_state)
{
    int i2c_master_port = port;

    i2c_config_t conf =
    {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_gpio,
        .sda_pullup_en = sda_pullup_state,
        .scl_io_num = scl_gpio,
        .scl_pullup_en = scl_pullup_state,
        .master.clk_speed = frequency
    };

    esp_err_t ret = i2c_param_config(i2c_master_port, &conf);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
    return ret;
}

esp_err_t setupRTC(DS3231 *DS3231_inst, SemaphoreHandle_t *bus_mutex)
{
    *bus_mutex = xSemaphoreCreateMutex();

    esp_err_t ret = ds3231_init(DS3231_inst, DS3231_ADDR, I2C_MASTER_NUM, bus_mutex);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not set up ds3231 RTC!");
        return ret;
    }

    //Fill in your unix time here
    setenv("TZ", "EAT-3", 1);
    time_t now = 1555425481;

    localtime_r(&now, &time_info);

    while (ds3231_set_time(DS3231_inst, &time_info) != ESP_OK)
    {
        ESP_LOGI(TAG, "Could not set time\n");
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
    return ret;
}

TEST_CASE("DS3231", "DS3231")
{

    char strftime_buf[64];
    esp_err_t ret;
    SemaphoreHandle_t i2c_bus_mutex = NULL;

    ret = setup_i2c(I2C_MASTER_NUM, I2C_MASTER_FREQ_HZ, I2C_MASTER_SDA_IO, GPIO_PULLUP_DISABLE, I2C_MASTER_SCL_IO, GPIO_PULLUP_DISABLE);
    if (ret == ESP_OK)
    {
        ESP_LOGE(TAG, "Could not setup I2C");
        /*Do something*/
    }

    ret = setupRTC(&DS3231_inst, &i2c_bus_mutex);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not setup RTC");
        /*Do something*/
    }

    while (1)
    {
        float temp;

        vTaskDelay(250 / portTICK_PERIOD_MS);

        if (ds3231_get_temp_float(&DS3231_inst, &temp) != ESP_OK)
        {
            ESP_LOGE(TAG, "Could not get temperature");
            continue;
        }

        if (ds3231_get_time(&DS3231_inst, &time_info) != ESP_OK)
        {
            ESP_LOGE(TAG, "Could not get time");
            continue;
        }

        ESP_LOGI(TAG, "%04d-%02d-%02d %02d:%02d:%02d, %.2f deg Cel\n", time_info.tm_year, time_info.tm_mon + 1,
            time_info.tm_mday, time_info.tm_hour, time_info.tm_min, time_info.tm_sec, temp);

        strftime(strftime_buf, sizeof(strftime_buf), "%c", &time_info);
        ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);
    }
}
