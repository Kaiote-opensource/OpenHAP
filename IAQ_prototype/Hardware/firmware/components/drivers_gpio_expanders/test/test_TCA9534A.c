#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "TCA9534A.h"

#define I2C_MASTER_SCL_IO          19               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO          21               /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM             I2C_NUM_0        /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ         100000           /*!< I2C master clock frequency */

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR TCA9534_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}


void TCA9534_handler_task(void* arg)
{
    int32_t wp_level = 0;
    int32_t cd_level = 0;
    uint32_t io_num;

    TCA9534 my_TCA9534;

    if(arg !=NULL)
    {
        my_TCA9534 = *((TCA9534*) arg);
    }
    else
    {
        printf("NULL pointer passed to task! I shall not dereference!...Deleting myself!");
        vTaskDelete(NULL);
    }

    TCA9534_get_level(&my_TCA9534, TCA9534_SD_CD, &cd_level);
    TCA9534_get_level(&my_TCA9534, TCA9534_SD_WP, &wp_level);

    TCA9534_set_level(&my_TCA9534, TCA9534_WARN_LED, !cd_level);

    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) 
        {
            if(io_num == GPIO_TCA9534_INT)
            {
                /*Delay added to ignore any bouncing on the mechanical parts generating the interrupt*/
                vTaskDelay(50 / portTICK_PERIOD_MS);

                printf("\nTCA9534 interrupt received\n");
                if (TCA9534_get_level(&my_TCA9534, TCA9534_SD_CD, &cd_level) != ESP_OK)
                {
                    printf("\nError getting card detect level");
                }
                else
                {
                    printf("\nCD level is set to %d\n", (int)cd_level);
                    TCA9534_set_level(&my_TCA9534, TCA9534_WARN_LED, !cd_level);
                }

                if (TCA9534_get_level(&my_TCA9534, TCA9534_SD_WP, &wp_level) != ESP_OK)
                {
                    printf("\nError getting Write protect level\n");
                }
                else
                {
                    printf("\nWrite protect is set to %d\n", (int)wp_level);
                }

            }
        }
    }
}

TEST_CASE("TCA9534A", "[TCA9534A]")
{
    esp_err_t ret;

    SemaphoreHandle_t i2c_bus_mutex = NULL;
    SemaphoreHandle_t data_mutex = NULL;

    i2c_bus_mutex = xSemaphoreCreateMutex();
    data_mutex = xSemaphoreCreateMutex();

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_TCA9534_INT, TCA9534_isr_handler, (void*)GPIO_TCA9534_INT);

    TCA9534_init(&my_TCA9534, 0x38, I2C_MASTER_NUM, I2C_MASTER_FREQ_HZ, I2C_MASTER_SDA_IO, GPIO_PULLUP_DISABLE, I2C_MASTER_SCL_IO, GPIO_PULLUP_DISABLE,
                                                                                                                            &data_mutex, &i2c_bus_mutex);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ret = TCA9534_set_pin_direction(&my_TCA9534, TCA9534_SD_WP, TCA9534_INPUT);
    if( ret != ESP_OK)
    {
        printf("\nError setting WP pin direction, returned %d", (int)ret);
    }
    ret = TCA9534_set_pin_direction(&my_TCA9534, TCA9534_SD_CD, TCA9534_INPUT);
    if(ret != ESP_OK)
    {
        printf("\nError setting CD pin direction, returned %d", (int)ret);
    }
    ret = TCA9534_set_pin_direction(&my_TCA9534, TCA9534_WARN_LED, TCA9534_OUTPUT);
    if(ret != ESP_OK)
    {
        printf("\nError setting LED pin direction, returned %d", (int)ret);
    }
    //start gpio task
    xTaskCreate(TCA9534_handler_task, "TCA9534_interrupt", 2048, (void*)&my_TCA9534, 10, NULL);

    while(1)
    {

    }
}
