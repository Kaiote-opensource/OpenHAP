#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ZH03.h"

TEST_CASE("ZH03", "[ZH03]")
{

    int iterations=3;
    int delay = 500;

    int32_t PM_1;
    int32_t PM_2_5;
    int32_t PM_10;

    esp_err_t ret = ESP_FAIL;

    SemaphoreHandle_t ZH03_data_mutex = NULL;

    ZH03_data_mutex = xSemaphoreCreateMutex();

    ZH03 my_ZH03 =
    {
        .device_data_mutex = ZH03_data_mutex
    };

    ZH03B_init();

    for(int i=0; i<iterations;i++)
    {
        while(ret != ESP_OK)
        {
            ret = get_particulate_reading(&my_ZH03, &PM_1, &PM_2_5, &PM_10);
        }
        vTaskDelay(delay / portTICK_PERIOD_MS);
        ret=ESP_FAIL;
    }
}
