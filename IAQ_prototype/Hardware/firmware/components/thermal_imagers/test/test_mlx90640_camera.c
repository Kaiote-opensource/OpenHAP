#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "HIH6030.h"
#include "esp_log.h"


#define I2C_MASTER_SCL_IO          19               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO          21               /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM             I2C_NUM_0        /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ         100000           /*!< I2C master clock frequency */


TEST_CASE("MLX90640_CAMERA", "[HIH6030]")
{

    int iterations=3;
    int delay = 500;

    SemaphoreHandle_t HIH6030_data_mutex = NULL;
    SemaphoreHandle_t HIH6030_bus_mutex = NULL;

    HIH6030_bus_mutex = xSemaphoreCreateMutex();
    HIH6030_data_mutex = xSemaphoreCreateMutex();

    HIH6030 my_HIH6030 =
    {
        .i2c_bus_mutex = HIH6030_bus_mutex,
        .device_data_mutex = HIH6030_data_mutex
    };

    HIH6030_init(&my_HIH6030, 0x27, I2C_MASTER_NUM, I2C_MASTER_FREQ_HZ, I2C_MASTER_SDA_IO, GPIO_PULLUP_DISABLE, I2C_MASTER_SCL_IO, GPIO_PULLUP_DISABLE);

    while(iterations!= 0)
    {
        TEST_ASSERT_EQUAL(ESP_OK, get_temp_humidity(&my_HIH6030));
        iterations--;
        vTaskDelay(delay / portTICK_PERIOD_MS);
    }

    HIH6030_deinit(I2C_MASTER_NUM);
}
