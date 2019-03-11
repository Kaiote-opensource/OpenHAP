/*
 Author: Alois Mbutura<ambutura@eedadvisory.com>
 Copyright 2019 Kaiote.io
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "peripheral_settings.h"

#define I2C_SPEED 100000U
#define CELL_DATARATE=115200U
#define PM_DATARATE=115200U


static const char* APP_MAIN_TAG = "APP-MAIN";
static const char* SUPERVISOR_TASK_TAG = "SUPERVISOR-TASK";
static const char* SETUP_TASK_TAG = "SETUP-TASK";
static const char* ACQUISITION_TASK_TAG = "ACQUISITION-TASK";


/*Task handleS*/
TaskHandle_t xSupervisorTaskHandle = NULL;
TaskHandle_t xSetupTaskHandle = NULL;
TaskHandle_t xAcquisitionTaskHandle = NULL;


/*Task synchronisation event group*/
EventGroupHandle_t xEventGroup = NULL;


static void PowerOnSelfTest(void)
{
    /*Test peripherals that can't be auto detected in software such as LEDs and buzzer*/
    /*Warn LED*/
}

void SupervisorTask(void *pvParameters)
{
    /*Use event groups to synchronise tasks*/

}

void SetupTask(void *pvParameters)
{
    
}

void AcquisitionTask(void *pvParameters)
{
    
}

void app_main()
{
    xEventGroup = xEventGroupCreate();

    PowerOnSelfTest();

    /*Set up supervisor task to perform task setup and synchronisation*/
    xTaskCreate(SupervisorTask, "SupervisorTask", 8192, NULL, 1, &xSupervisorTaskHandle);

    if(xSupervisorTaskHandle != NULL)
    {
        ESP_LOGI(APP_MAIN_TAG, "Supervisor task created");
    }
    else
    {
        ESP_LOGE(APP_MAIN_TAG, "function:\"%s\" Supervisor task creation failed, check heap memory availability", __func__);
        /*Restart so that a repreated Power on self test can be a visual cue of a defective unit*/
        esp_restart();
    }
}