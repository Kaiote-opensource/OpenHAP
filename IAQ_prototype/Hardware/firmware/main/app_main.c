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
#include <stdint.h>
#include <string.h>

#include "esp_system.h"
#include "esp_spi_flash.h"

#include "mbedtls/base64.h"

#include "espfs.h"
#include "espfs_image.h"
#include "libesphttpd/httpd-espfs.h"

#include <libesphttpd/esp.h>
#include "libesphttpd/httpd.h"
#include "libesphttpd/cgiwifi.h"
#include "libesphttpd/cgiflash.h"
#include "libesphttpd/auth.h"
#include "libesphttpd/captdns.h"
#include "libesphttpd/cgiwebsocket.h"
#include "libesphttpd/httpd-freertos.h"
#include "libesphttpd/route.h"

#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"

#include "HIH6030.h"
#include "ZH03.h"
#include "DS3231.h"
#include "TCA9534A.h"
#include <MLX90640_API.h>
#include "MLX90640_I2C_Driver.h"
#include "adc_sense.h"
#include "parson.h"

#include "task_sync.h"

#include "esp_system.h"
#include "sdkconfig.h"

#include "driver/i2c.h"
#include "driver/uart.h"

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_defs.h"
#include "esp_gattc_api.h"
#include "esp_gap_ble_api.h"

#include "esp_eddystone_protocol.h"
#include "esp_eddystone_api.h"

#define WEB_PORT 80U
#define MAX_CONNECTIONS 32U /*Maximum tabs open*/

#define GPIO_TCA9534_INT     35

#define I2C_MASTER_SCL_IO    19
#define I2C_MASTER_SDA_IO    21
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_FREQ_HZ  100000

#define HIH6030_ADDR        0x27
#define MLX90640_ADDR       0x33
#define TCA9534_ADDR        0x38
#define DS3231_ADDR         0x68

#define ZH03_BAUDRATE       9600
#define ZH03_UART_NUM       UART_NUM_1
#define ZH03_TXD_PIN        GPIO_NUM_23
#define ZH03_RXD_PIN        GPIO_NUM_22
#define ZH03_RX_BUF_SIZE    1024
#define ZH03_TX_BUF_SIZE    0

static const char *TAG = "APP-MAIN";

static char *infared_handshake_msg = "Hi, Infared websocket!";
static char *status_handshake_msg = "Hi, Status websocket!";
static char *tag_info_handshake_msg = "Hi, Tag websocket!";

static const char infared_cgi_resource_string[] = "/infared.cgi";
static const char status_cgi_resource_string[] = "/status.cgi";
static const char tag_cgi_resource_string[] = "/tag.cgi";

static char *null_test_string = "\0";

struct tm date_time;

typedef struct
{
    HIH6030 IAQ_HIH6030;
    ZH03 IAQ_ZH03;
    TCA9534 IAQ_TCA9534;
    DS3231  IAQ_DS3231;
    MLX90640 IAQ_MLX90640;
    paramsMLX90640 MLX90640params;
} peripherals_struct;

peripherals_struct device_peripherals;

/*Main task handles*/
static TaskHandle_t xWebServerTaskHandle = NULL;
static TaskHandle_t xDataloggerTaskHandle = NULL;
/*Webserver subtask handles - 1 per page*/

static TaskHandle_t xuartSensorTaskHandle = NULL;
static TaskHandle_t xStatusBroadcastHandle = NULL;
static TaskHandle_t xInfaredBroadcastHandle = NULL;
static TaskHandle_t xTCA9534HandlerTaskHandle = NULL;
static TaskHandle_t xTagBroadcastHandle = NULL;

/*Connection memory to be allocated from psram heap via malloc*/
static char* connectionMemory = NULL;
static HttpdFreertosInstance httpdFreertosInstance;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const static int CONNECTED_BIT = BIT0;
static EventGroupHandle_t wifi_ap_event_group;

/*Task synchronisation event group*/
static EventGroupHandle_t task_sync_event_group = NULL;

static xQueueHandle gpio_evt_queue = NULL;
static xQueueHandle particulate_readings_queue = NULL;

/* declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_eddystone_show_inform(const esp_eddystone_result_t *res, const esp_ble_gap_cb_param_t *scan_result);

void uartSensorTask(void *pvParameters);
void statusPageWsBroadcastTask(void *pvParameters);
void infaredPageWsBroadcastTask(void *pvParameters);
void tagPageWsBroadcastTask(void *pvParameters);

static esp_ble_scan_params_t ble_scan_params = 
{
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x50,
    .scan_window = 0x30,
    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE
};

static void IRAM_ATTR TCA9534_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void TCA9534HandlerTask(void *pvParameters)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    int32_t wp_level = 0;
    int32_t cd_level = 0;
    uint32_t io_num;

    EventBits_t xEventGroupValue;
    EventBits_t xBitstoWaitFor = NOTIFY_WEBSERVER_GPIO_INTERRUPT_TASK_CLOSE;

    TCA9534_get_level(&(device_peripherals.IAQ_TCA9534), TCA9534_SD_CD, &cd_level);
    TCA9534_get_level(&(device_peripherals.IAQ_TCA9534), TCA9534_SD_WP, &wp_level);
    TCA9534_set_level(&(device_peripherals.IAQ_TCA9534), TCA9534_WARN_LED, !cd_level);

    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            if (io_num == GPIO_TCA9534_INT)
            {
                /*Delay added to ignore any bouncing on the mechanical parts generating the interrupt*/
                vTaskDelay(100 / portTICK_PERIOD_MS);

                ESP_LOGI(TAG,"TCA9534 interrupt received");
                if (TCA9534_get_level(&(device_peripherals.IAQ_TCA9534), TCA9534_SD_CD, &cd_level) != ESP_OK)
                {
                    ESP_LOGE(TAG,"Error getting card detect level");
                }
                else
                {
                    ESP_LOGI(TAG,"CD level is set to %d", (int)cd_level);
                    TCA9534_set_level(&(device_peripherals.IAQ_TCA9534), TCA9534_WARN_LED, !cd_level);
                }

                if (TCA9534_get_level(&(device_peripherals.IAQ_TCA9534), TCA9534_SD_WP, &wp_level) != ESP_OK)
                {
                    ESP_LOGE(TAG,"Error getting Write protect level");
                }
                else
                {
                    ESP_LOGI(TAG,"Write protect is set to %d", (int)wp_level);
                }
            }
        }
        xEventGroupValue = xEventGroupWaitBits(task_sync_event_group, xBitstoWaitFor, pdTRUE, pdFALSE, 0); 

        if(xEventGroupValue & xBitstoWaitFor)
        {
            //Let task finish up cleanly
            xTCA9534HandlerTaskHandle = NULL;
            vTaskDelete(NULL);
        }
    }
}

static void esp_eddystone_show_inform(const esp_eddystone_result_t *res, const esp_ble_gap_cb_param_t *scan_result)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    int connections;
    char mac_buffer[20];
    char* tag_info_string = NULL;
    switch (res->common.frame_type)
    {
    case EDDYSTONE_FRAME_TYPE_TLM:
    {
        sprintf(mac_buffer, "%02X%02X%02X%02X%02X%02X", scan_result->scan_rst.bda[0],
                                                        scan_result->scan_rst.bda[1],
                                                        scan_result->scan_rst.bda[2],
                                                        scan_result->scan_rst.bda[3],
                                                        scan_result->scan_rst.bda[4],
                                                        scan_result->scan_rst.bda[5]);
        JSON_Value *root_value = json_value_init_object();
        JSON_Object *root_object = json_value_get_object(root_value);
        json_object_set_number(root_object, "RSSI", scan_result->scan_rst.rssi);
        json_object_set_number(root_object, "BATT_VOLTAGE", res->inform.tlm.battery_voltage);
        json_object_set_number(root_object, "TIME", res->inform.tlm.time);
        json_object_set_string(root_object, "MAC", mac_buffer);
        tag_info_string = json_serialize_to_string(root_value);
        ESP_LOGI(TAG, "%s", tag_info_string);
        json_value_free(root_value);
        ESP_LOGI(TAG, "Tag info string length is %d bytes", strlen(tag_info_string));
        if (tag_info_string != NULL)
        {
            connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance, tag_cgi_resource_string, tag_info_string, strlen(tag_info_string), WEBSOCK_FLAG_NONE);
            json_free_serialized_string(tag_info_string);
            ESP_LOGD(TAG, "Broadcast sent to %d connections", connections);
        }
        else
        {
            ESP_LOGD(TAG, " Tag info page JSON assembler returned NULL");
        }
        break;
    }
    default:
        break;
    }
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t err;

    switch (event)
    {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
    {
        uint32_t duration = 0;
        esp_ble_gap_start_scanning(duration);
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
    {
        if ((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(TAG, "Scan start failed: %s", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "Start scanning...");
        }
        break;
    }
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
    {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt)
        {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
        {
            esp_eddystone_result_t eddystone_res;
            memset(&eddystone_res, 0, sizeof(eddystone_res));
            esp_err_t ret = esp_eddystone_decode(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len, &eddystone_res);
            if (ret)
            {
                // error:The received data is not an eddystone frame packet or a correct eddystone frame packet.
                // just return
                return;
            }
            else
            {
                // The received adv data is a correct eddystone frame packet.
                // Here, we get the eddystone infomation in eddystone_res, we can use the data in res to do other things.
                // For example, just print them:
                esp_eddystone_show_inform(&eddystone_res, scan_result);
            }
            break;
        }
        default:
            break;
        }
        break;
    }
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
    {
        if ((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(TAG, "Scan stop failed: %s", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "Stop scan successfully");
        }
        break;
    }
    default:
        break;
    }
}

esp_err_t esp_eddystone_app_register(void)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t status;

    ESP_LOGI(TAG, "Register eddystone tag callback");
    /*<! register the scan callback function to the gap module */
    if ((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK)
    {
        ESP_LOGE(TAG, "gap register error: %s", esp_err_to_name(status));
        return status;
    }
    return ESP_OK;
}

esp_err_t esp_eddystone_init(void)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t ret;
    if((ret = esp_bluedroid_init()) != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not initialise and allocate bluedroid");
    }
    while(esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_INITIALIZED)
    {
        ESP_LOGI(TAG, "Bluedroid initialisation in progress...waiting...");
        vTaskDelay(100/portTICK_RATE_MS);
    }
    ESP_LOGI(TAG, "Bluedroid initialisation complete");
    if((ret = esp_bluedroid_enable()) != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not enable bluedroid");
        return ret;
    }
    while(esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED)
    {
        ESP_LOGI(TAG, "Bluedroid enable is in progress...waiting...");
        vTaskDelay(100/portTICK_RATE_MS);
    }
    ESP_LOGI(TAG, "Bluedroid is now enabled");
    if((ret = esp_eddystone_app_register()) != ESP_OK)
    {
        ESP_LOGE(TAG, "Eddystone callback handler registration unsuccessful");
    }
    ESP_LOGI(TAG, "Eddystone callback handler succesfully registered");
    return ret;
}

esp_err_t esp_eddystone_deinit(void)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t ret;
    if((ret = esp_bluedroid_disable()) != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not disable bluedroid");
        return ret;
    }
    while(esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_INITIALIZED)
    {
        ESP_LOGI(TAG, "Bluedroid disable in progress...waiting...");
        vTaskDelay(100/portTICK_RATE_MS);
    }
    if((ret = esp_bluedroid_deinit()) != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not de-initialise and deallocate bluedroid");
        return ret;
    }
    while(esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_UNINITIALIZED)
    {
        ESP_LOGI(TAG, "Bluedroid de-initialisation in progress...waiting...");
    }
    return ret;
}

static esp_err_t powerOnSelfTest(void)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    /*Test peripherals that can't be auto detected in software such as LEDs and buzzer*/
    /*Warn LED*/
    return ESP_OK;
}

static void receiveCommand(char *cmd)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    /*Work more on this*/
    ESP_LOGI(TAG, "%s", cmd);
}

//Queue all commands from all websockets and have a single decoding handler to avoid race conditions
static void myWebsocketRecv(Websock *ws, char *data, int len, int flags)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    receiveCommand(data);
}

/*Websocket disconnected. Stop task if none connected*/
static void myStatusWebsocketClose(Websock *ws)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    int connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance, status_cgi_resource_string, null_test_string, strlen(null_test_string), WEBSOCK_FLAG_NONE);
    ESP_LOGI(TAG, "Status page connection close received, Previous connections were %d", connections);

    if (connections == 1)
    {
        xEventGroupSetBits( task_sync_event_group,  NOTIFY_WEBSERVER_STATUS_TASK_CLOSE |
                                                    NOTIFY_WEBSERVER_PARTICULATE_MATTER_TASK_CLOSE |
                                                    NOTIFY_WEBSERVER_GPIO_INTERRUPT_TASK_CLOSE);      
    }
}

/*Websocket disconnected. Stop task if none connected*/
static void myInfaredWebsocketClose(Websock *ws)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    int connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance, infared_cgi_resource_string, null_test_string, strlen(null_test_string), WEBSOCK_FLAG_NONE);
    ESP_LOGI(TAG, "Infared page connection close received, Previous connections were %d", connections);

    if (connections == 1)
    {
        xEventGroupSetBits( task_sync_event_group,  NOTIFY_WEBSERVER_THERMAL_VIEWER_TASK_CLOSE);  
    }
}

/*Websocket disconnected. Stop task if none connected*/
static void myTagWebsocketClose(Websock *ws)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    int connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance, tag_cgi_resource_string, null_test_string, strlen(null_test_string), WEBSOCK_FLAG_NONE);
    ESP_LOGI(TAG, "Tag info page connection close received, Previous connections were %d", connections);

    if (connections == 1)
    {
        xEventGroupSetBits(task_sync_event_group,  NOTIFY_WEBSERVER_TAG_INFO_TASK_CLOSE);      
    }
}

/*Websocket connected. Start task if none prior connected, set websocket callback methods and send welcome message.*/
static void myStatusWebsocketConnect(Websock *ws)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    int connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance, status_cgi_resource_string, null_test_string, strlen(null_test_string), WEBSOCK_FLAG_NONE);
    ESP_LOGI(TAG, "Status page connection start received, Previous connections were %d", connections);

    if (connections == 0)
    {
        if (xTaskCreate(statusPageWsBroadcastTask, "statusPageTask", 8192, NULL, 2, &xStatusBroadcastHandle))
        {
            ESP_LOGI(TAG, "Created status broadcast task");
            ws->recvCb = myWebsocketRecv;
            ws->closeCb = myStatusWebsocketClose;
            cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, status_handshake_msg, strlen(status_handshake_msg), WEBSOCK_FLAG_NONE);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to create status broadcast task");
            return;
        }
    }
    ws->recvCb = myWebsocketRecv;
    ws->closeCb = myStatusWebsocketClose;
    cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, status_handshake_msg, strlen(status_handshake_msg), WEBSOCK_FLAG_NONE);
}

/*Websocket connected. Start task if none prior connected, set websocket callback methods and send welcome message.*/
static void myInfaredWebsocketConnect(Websock *ws)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    int connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance, infared_cgi_resource_string, null_test_string, strlen(null_test_string), WEBSOCK_FLAG_NONE);
    ESP_LOGI(TAG, "Infared page connection start received, Previous connections were %d", connections);

    if (connections == 0)
    {
        if (xTaskCreatePinnedToCore(infaredPageWsBroadcastTask, "infaredPageTask", 24000, NULL, 2, &xInfaredBroadcastHandle, 0))
        {
            ESP_LOGI(TAG, "Created infared broadcast task");
            ws->recvCb = myWebsocketRecv;
            ws->closeCb = myInfaredWebsocketClose;
            cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, infared_handshake_msg, strlen(infared_handshake_msg), WEBSOCK_FLAG_NONE);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to create infared broadcast task");
            return;
        }
    }
    ws->recvCb = myWebsocketRecv;
    ws->closeCb = myInfaredWebsocketClose;
    cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, infared_handshake_msg, strlen(infared_handshake_msg), WEBSOCK_FLAG_NONE);
}

/*Websocket connected. Start task if none prior connected, set websocket callback methods and send welcome message.*/
static void myTagWebsocketConnect(Websock *ws)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    int connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance, tag_cgi_resource_string, null_test_string, strlen(null_test_string), WEBSOCK_FLAG_NONE);
    ESP_LOGI(TAG, "Tag info page connection start received, Previous connections were %d", connections);

    if (connections == 0)
    {
        if (xTaskCreatePinnedToCore(tagPageWsBroadcastTask, "tagPageTask", 2048, NULL, 2, &xTagBroadcastHandle, 0))
        {
            ESP_LOGI(TAG, "Created tag info broadcast task");
            ws->recvCb = myWebsocketRecv;
            ws->closeCb = myTagWebsocketClose;
            cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, tag_info_handshake_msg, strlen(tag_info_handshake_msg), WEBSOCK_FLAG_NONE);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to create tag info broadcast task");
            return;
        }
    }
    ws->recvCb = myWebsocketRecv;
    ws->closeCb = myTagWebsocketClose;
    cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, tag_info_handshake_msg, strlen(tag_info_handshake_msg), WEBSOCK_FLAG_NONE);
}

HttpdBuiltInUrl builtInUrls[] = 
{
    ROUTE_CGI_ARG("*", cgiRedirectApClientToHostname, "esp.nonet"),
    ROUTE_REDIRECT("/", "/index.html"),
    ROUTE_WS(status_cgi_resource_string, myStatusWebsocketConnect),
    ROUTE_WS(infared_cgi_resource_string, myInfaredWebsocketConnect),
    ROUTE_WS(tag_cgi_resource_string, myTagWebsocketConnect),
    ROUTE_FILESYSTEM(),
    ROUTE_END()
};
esp_err_t get_thermal_image(peripherals_struct *device_peripherals, float* image_buffer)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    uint16_t mlx90640Frame[2][834]={{0},{0}};
    uint16_t reg_val = 0;
    int cnt = 0;
    float emissivity = 0.95;
    float tr = 0;
    int ta_shift = 8;
    if(1)
    {
        MLX90640_I2CRead(&(device_peripherals->IAQ_MLX90640), 0x800D, 1, &reg_val);
        if(reg_val != 0x1901)
        {
            while(reg_val != 0x1901 && cnt < 5)
            {
                MLX90640_I2CWrite(&(device_peripherals->IAQ_MLX90640), 0x800D, 0x1901);
                vTaskDelay(50 / portTICK_RATE_MS);
                MLX90640_I2CRead(&(device_peripherals->IAQ_MLX90640), 0x800D, 1, &reg_val);
                cnt++;
            }
            if(reg_val != 0x1901)
            {
                ESP_LOGE(TAG, "Failed to set default value to config reg 0x800D");
                return ESP_FAIL;
            }
        }
        if(MLX90640_GetFrameData(&(device_peripherals->IAQ_MLX90640), mlx90640Frame) != 0)
        {
            ESP_LOGE(TAG, "Failed to get subpage frames for full image composition");
            return ESP_FAIL;
        }
        tr = MLX90640_GetTa(mlx90640Frame[0], &(device_peripherals->MLX90640params)) - ta_shift;
        MLX90640_CalculateTo(mlx90640Frame[0], &(device_peripherals->MLX90640params), emissivity, tr, image_buffer);
        tr = MLX90640_GetTa(mlx90640Frame[1], &(device_peripherals->MLX90640params)) - ta_shift;
        MLX90640_CalculateTo(mlx90640Frame[1], &(device_peripherals->MLX90640params), emissivity, tr, image_buffer);

        return ESP_OK; 
    }
    else
    {
        return ESP_ERR_INVALID_ARG;
    }
}


char *simulateStatusValues(char *status_string, peripherals_struct *device_peripherals)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

    int32_t cd_level;
    uint32_t battery_level;

    hih6030_status_t status;
    float temperature;
    float humidity;
    
    int32_t PM_2_5;

    /*Query time from DS3231 RTC*/
    if (ds3231_get_time(&(device_peripherals->IAQ_DS3231), &date_time) != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not get time from DS3231");
    }

    /*Poll SD card connection state*/
    TCA9534_get_level(&(device_peripherals->IAQ_TCA9534), TCA9534_SD_CD, &cd_level);

    /*Get temperature and humidity readings*/
    get_temp_humidity(&(device_peripherals->IAQ_HIH6030), &status, &temperature, &humidity);

    /*Get battery voltage*/
    TCA9534_set_level(&(device_peripherals->IAQ_TCA9534), TCA9534_BATT_SEL, TCA9534_HIGH);
    vTaskDelay(100/portTICK_RATE_MS);
    get_adc_value(&battery_level);

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_number(root_object, "HUM", (int)(humidity));
    json_object_set_number(root_object, "TEMP",/*temp*/(int)(temperature));

    /*Get particulate matter reading from queue*/
    if(particulate_readings_queue != 0 )
    {
        // Receive a message from the queue. Block for 1000ms if there are no particulate readings in queue, such as at startup.
        if( xQueueReceive(particulate_readings_queue, &PM_2_5, 1000/portTICK_RATE_MS) )
        {
            json_object_set_number(root_object, "PM25", PM_2_5);
        }
    }
    else
    {
        /* code */
        ESP_LOGE(TAG, "Particulate matter queue not created! Cant receive readings, PM 2.5 json not created");
    }
    //json_object_set_number(root_object, "TIME", rand()%100);
    json_object_set_number(root_object, "RTC_BATT", (int)((battery_level/3300)*100));
    //json_object_set_number(root_object, "MAIN_BATT", rand()%100);
    json_object_set_number(root_object, "DEVICE_TIME", mktime(&date_time)/*unix_time*/);
    json_object_set_string(root_object, "SD_CARD", cd_level == TCA9534_HIGH ? "DISCONNECTED" : "CONNECTED");
    status_string = json_serialize_to_string(root_value);
    ESP_LOGI(TAG, "%s", status_string);
    json_value_free(root_value);

    ESP_LOGI(TAG, "status string length is %d bytes", strlen(status_string));

    return status_string;
}

char *b64_encode_thermal_img(char *base64_dst, float *data)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    int ret;
    size_t base64_length = 0;

    ret = mbedtls_base64_encode(NULL, 0, &base64_length, (unsigned char *)data, 768 * sizeof(float));
    if (ret == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL)
    {
        ESP_LOGI(TAG, "Buffer for allocation of infared is too small, length needed is %d bytes", base64_length);
        base64_dst = (char *)malloc((base64_length * sizeof(size_t)) + 1);
    }
    else if (ret == MBEDTLS_ERR_BASE64_INVALID_CHARACTER)
            {
        ESP_LOGI(TAG, "Buffer for allocation of infared is too small, length needed is %d bytes", base64_length);
        ESP_LOGI(TAG, "Invalid character found");
    }
    else
    {
        ESP_LOGI(TAG, "Function returned succesful");
    }
    base64_dst = (char *)malloc((base64_length * sizeof(size_t)) + 1);
    ret = mbedtls_base64_encode((unsigned char *)base64_dst, base64_length, &base64_length, (unsigned char *)data, 768 * sizeof(float));
    if (ret == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL)
    {
        ESP_LOGI(TAG, "Buffer for allocation of infared is too small, length needed is %d bytes", base64_length);
        return NULL;
    }
    else if (ret == MBEDTLS_ERR_BASE64_INVALID_CHARACTER)
    {
        ESP_LOGI(TAG, "Buffer for allocation of infared is too small, length needed is %d bytes", base64_length);
        ESP_LOGI(TAG, "Invalid character found");
        return NULL;
    }
    ESP_LOGI(TAG, "Function returned successful");

    return base64_dst;
}

void uartSensorTask(void *pvParameters)
{
    esp_err_t ret;
    int32_t PM_2_5;

    EventBits_t xEventGroupValue;
    EventBits_t xBitstoWaitFor = NOTIFY_WEBSERVER_PARTICULATE_MATTER_TASK_CLOSE;

    /*Allocate queue that can hold upto 5 particulate matter readings*/
    particulate_readings_queue = xQueueCreate(5, sizeof(int32_t));

    if(particulate_readings_queue == 0)
    {
        ESP_LOGE(TAG, "Could not create particulate readings queue");
    }
    uart_flush(device_peripherals.IAQ_ZH03.uart_port);
    while (1)
    {
        /*We only care about the PM 2.5 value*/
        /*This can fail if checksum fails or bytes are less than frame length*/
        if((ret = get_particulate_reading(&(device_peripherals.IAQ_ZH03), NULL, &PM_2_5, NULL)) == ESP_OK)
        {
            if(particulate_readings_queue != 0)
            {
                ESP_LOGI(TAG, "Found particulate matter queue allocated, now writing PM 2.5 value %d", (int)PM_2_5);
                /*Don't block for the receiving task to dequeue a single item before we post to it*/
                xQueueSend(particulate_readings_queue, &PM_2_5, 100/portTICK_RATE_MS);
            }
            else
            {
                ESP_LOGE(TAG, "Particulate matter queue not allocated, can't write PM 2.5 value %d", (int)PM_2_5);
            }  
        }
        else if(ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Particulate matter acquisition function returned FAIL");
            vTaskDelay(500/portTICK_RATE_MS);
        }
        else if(ret == ESP_ERR_INVALID_SIZE)
        {

            ESP_LOGI(TAG, "Particulate matter acquisition function returned invalid buffer size");
            vTaskDelay(500/portTICK_RATE_MS);
        }
        else
        {
            ESP_LOGI(TAG, "Particulate matter acquisition function returned undocumented error");
            vTaskDelay(500/portTICK_RATE_MS);
        }
        
        xEventGroupValue = xEventGroupWaitBits(task_sync_event_group, xBitstoWaitFor, pdTRUE, pdFALSE, 0); 

        if(xEventGroupValue & xBitstoWaitFor)
        {
            //Let task finish up cleanly
            xuartSensorTaskHandle = NULL;
            vTaskDelete(NULL);
        } 
    }
    vTaskDelete(NULL);
}

void statusPageWsBroadcastTask(void *pvParameters)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    
    int connections;
    char *status_string = NULL;

    EventBits_t xEventGroupValue;
    EventBits_t xBitstoWaitFor = NOTIFY_WEBSERVER_STATUS_TASK_CLOSE;

    /*Initialise ADC to sample battery cell voltage*/
    init_adc();

    if (xTaskCreate(TCA9534HandlerTask, "TCA9534Interrupt", 4096, NULL, 1, &xTCA9534HandlerTaskHandle))
    {
        ESP_LOGI(TAG, "Successfully created TCA9534 handler task");
    }
    else
    {
        ESP_LOGE(TAG, "Could not create TCA9534 handler task");
    }
        
    if(xTaskCreate(uartSensorTask, "uartSensorTask", 8192, NULL, 1, &xuartSensorTaskHandle))
    {
        ESP_LOGI(TAG, "Successfully created particulate matter sensor handler task");
    }
    else
    {
        ESP_LOGE(TAG, "Could not create particulate matter sensor handler task");
    }
    while (1)
    {

        status_string = simulateStatusValues(status_string, &device_peripherals);
        if (status_string != NULL)
        {
            connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance, status_cgi_resource_string, status_string, strlen(status_string), WEBSOCK_FLAG_NONE);
            json_free_serialized_string(status_string);
            ESP_LOGD(TAG, "Broadcast sent to %d connections", connections);
        }
        else
        {
            ESP_LOGE(TAG, "Status page JSON assembler returned NULL");
        }

        xEventGroupValue = xEventGroupWaitBits(task_sync_event_group, xBitstoWaitFor, pdTRUE, pdFALSE, 0); 

        if(xEventGroupValue & xBitstoWaitFor)
        {
            //Let task finish up cleanly
            xStatusBroadcastHandle = NULL;
            vTaskDelete(NULL);
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

void wsStringBurstBroadcast(const char* base64_string, const char* data_continue_identifier, const char* data_end_identifier)
{
    int connections = 0;
    char *json_string = NULL;
    char *data_txt_chunk = NULL;
    int size_to_do;
    int chunk_size;
    int size;
    int offset;
    /* Keep the max burst size at half the maximum allowable HTTPD buffer, incase the string is filled with only escaped 
       characters after jsonification, which would double the size of characters to the HTTPD buffer length*/
    int burst_size = ((HTTPD_MAX_SENDBUFF_LEN%2) == 0) ? (HTTPD_MAX_SENDBUFF_LEN/2) : ((HTTPD_MAX_SENDBUFF_LEN/2)-1);
    size = strlen(base64_string);
    size_to_do = size;
    for (int i = 0; size_to_do > 0; i++)
    {
        JSON_Value *root_value = json_value_init_object();
        JSON_Object *root_object = json_value_get_object(root_value);
        chunk_size = (size_to_do < HTTPD_MAX_SENDBUFF_LEN) ? size_to_do : burst_size;
        offset = i * burst_size;
        data_txt_chunk = (char *)malloc(HTTPD_MAX_SENDBUFF_LEN);
        memset(data_txt_chunk, 0, chunk_size * sizeof(char));
        strlcpy(data_txt_chunk, base64_string + offset, chunk_size + 1);
        ESP_LOGI(TAG, "Allocated string chunk length is %d", strlen(data_txt_chunk));
        //ESP_LOG_BUFFER_HEXDUMP("Raw data chunk:", data_txt_chunk, strlen(data_txt_chunk)+1, ESP_LOG_DEBUG);
        if (json_object_set_string(root_object, (size_to_do <= burst_size) ? data_end_identifier : data_continue_identifier, data_txt_chunk) != JSONSuccess)
        {
            ESP_LOGE(TAG, "Setting json string failed");
            free(data_txt_chunk);
            json_value_free(root_value);
            json_free_serialized_string(json_string);
            break;
        }
        json_string = json_serialize_to_string(root_value);
        if (json_string == NULL)
        {
            ESP_LOGI(TAG, "Allocation of json string failed");
            free(data_txt_chunk);
            json_value_free(root_value);
            json_free_serialized_string(json_string);
            break;
        }
        //ESP_LOG_BUFFER_HEXDUMP("JSON string:", json_string, strlen(json_string)+1, ESP_LOG_DEBUG);
        connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance, infared_cgi_resource_string, json_string, strlen(json_string), WEBSOCK_FLAG_NONE);
        ESP_LOGD(TAG, "Sent chunk %d of raw data size %d of a maximum %d byte raw payload to %d connections", i, chunk_size, strlen(base64_string), connections);
        ESP_LOGD(TAG, "json data size for this chunk is %d", strlen(json_string));
        free(data_txt_chunk);
        json_value_free(root_value);
        json_free_serialized_string(json_string);
        size_to_do -= chunk_size; //Subtract the quantity of data already transferred.
        if (size_to_do < 1024)
        {
            if ((offset + chunk_size) != size)
            {
                ESP_LOGE(TAG, "Error: Burst size mismatch");
            }
        }
    }
}

void infaredPageWsBroadcastTask(void *pvParameters)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    char *base64_camera_string = NULL;
    float image_buffer[768] = {0.0};
    esp_err_t ret;

    EventBits_t xEventGroupValue;
    EventBits_t xBitstoWaitFor = NOTIFY_WEBSERVER_THERMAL_VIEWER_TASK_CLOSE;
    while (1)
    {
        ret = get_thermal_image(&device_peripherals, image_buffer);
        if(ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to get thermal image, returned error code %d", ret);
            //Display Zeroed out image buffer to signify that there is an error.
            memset(image_buffer, 0, 768);
            //Do something else
        }
        base64_camera_string = b64_encode_thermal_img(base64_camera_string, image_buffer/*camera_data_float*/);

        if (base64_camera_string != NULL)
        {
            //ESP_LOG_BUFFER_HEXDUMP("Base64 data:", base64_string, strlen(base64_string)+1, ESP_LOG_DEBUG);
            wsStringBurstBroadcast(base64_camera_string, "CAMERA_CONT", "CAMERA_END");
            free(base64_camera_string);
        }
        else
        {
            ESP_LOGD(TAG, "Infared page JSON assembler returned NULL");
        }

        xEventGroupValue = xEventGroupWaitBits(task_sync_event_group, xBitstoWaitFor, pdTRUE, pdFALSE, 0); 

        if(xEventGroupValue & xBitstoWaitFor)
        {
            //Let task finish up cleanly
            xInfaredBroadcastHandle = NULL;
            vTaskDelete(NULL);
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

void tagPageWsBroadcastTask(void *pvParameters)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t ret;
    EventBits_t xEventGroupValue;
    EventBits_t xBitstoWaitFor = NOTIFY_WEBSERVER_TAG_INFO_TASK_CLOSE;

    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    /*Set up bluetooth peripheral hardware*/
    esp_eddystone_init();
    /*<! set scan parameters*/
    esp_ble_gap_set_scan_params(&ble_scan_params);
    while(1)
    {
        xEventGroupValue = xEventGroupWaitBits(task_sync_event_group, xBitstoWaitFor, pdTRUE, pdFALSE, 0); 

        if(xEventGroupValue & xBitstoWaitFor)
        {
            //Let task finish up cleanly
            if((ret = esp_eddystone_deinit()) != ESP_OK)
            {
                ESP_LOGE(TAG, "Eddystone deinitialise failed, returned %d", ret);
            }
            ESP_LOGI(TAG, "Eddystone deinitialised succesfully");

            if((ret = esp_bt_controller_disable()) != ESP_OK)
            {
                ESP_LOGE(TAG, "Bluetooth controller disable failed, returned %d", ret);
            }
            ESP_LOGI(TAG, "Bluetooth controller disabled succesfully");

            xTagBroadcastHandle = NULL; 
            vTaskDelete(NULL);
        }
        /*Yield control of the CPU briefly for 10ms*/
        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    switch (event->event_id)
    {
    case SYSTEM_EVENT_AP_START:
    {
        tcpip_adapter_ip_info_t ap_ip_info;
        if (tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ap_ip_info) == 0)
        {
            ESP_LOGI(TAG, "~~~~~~~~~~~");
            ESP_LOGI(TAG, "IP:" IPSTR, IP2STR(&ap_ip_info.ip));
            ESP_LOGI(TAG, "MASK:" IPSTR, IP2STR(&ap_ip_info.netmask));
            ESP_LOGI(TAG, "GW:" IPSTR, IP2STR(&ap_ip_info.gw));
            ESP_LOGI(TAG, "~~~~~~~~~~~");
        }
    }
    break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:" MACSTR " join association ID(AID)=%d\n",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        xEventGroupSetBits(wifi_ap_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station " MACSTR "left the Wifi chat, association ID(AID)=%d\n",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        xEventGroupClearBits(wifi_ap_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_SCAN_DONE:
        break;
    default:
        break;
    }
    return ESP_OK;
}

void ICACHE_FLASH_ATTR init_wifi(void)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    uint8_t mac[6];
    char *ssid;
    char *password;
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    asprintf(&ssid, "%s-%02X%02X%02X", "Kaiote-AQ", mac[3], mac[4], mac[5]);
    asprintf(&password, "%02X%02X%02X%02X%02X%02X", mac[3], mac[4], mac[5], mac[3], mac[4], mac[5]);

    wifi_ap_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    wifi_config_t ap_config;
    strcpy((char *)(&ap_config.ap.ssid), ssid);
    ap_config.ap.ssid_len = strlen(ssid);
    ap_config.ap.channel = 1;
    strcpy((char *)(&ap_config.ap.password), password);
    ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    ap_config.ap.ssid_hidden = 0;
    ap_config.ap.max_connection = 2;
    ap_config.ap.beacon_interval = 100;

    esp_wifi_set_config(WIFI_IF_AP, &ap_config);

    ESP_ERROR_CHECK(esp_wifi_start());
}

esp_err_t setupTempHumiditySensor(HIH6030 *HIH6030_inst, SemaphoreHandle_t *bus_mutex)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    *bus_mutex = xSemaphoreCreateMutex();

    esp_err_t ret = HIH6030_init(HIH6030_inst, HIH6030_ADDR, I2C_MASTER_NUM, bus_mutex);
    return ret;
}
esp_err_t setupRTC(DS3231 *DS3231_inst, SemaphoreHandle_t *bus_mutex)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    *bus_mutex = xSemaphoreCreateMutex();

    esp_err_t ret = ds3231_init(DS3231_inst, DS3231_ADDR, I2C_MASTER_NUM, bus_mutex);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not set up ds3231 RTC!");
        return ret;
    }
    /*Set unix timestamp and convert to date-time format*/
    setenv("TZ", "EAT-3", 1);
    tzset();

    /*time_t now = 1555427390;
    // setup datetime: 2016-10-09 13:50:10
    localtime_r(&now, &date_time);

    while (ds3231_set_time(DS3231_inst, &date_time) != ESP_OK)
    {
        ESP_LOGI(TAG, "Could not set time\n");
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
    */
    return ret;
}
esp_err_t setupGpioExpander(TCA9534 *TCA9534_inst, SemaphoreHandle_t *bus_mutex)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    *bus_mutex = xSemaphoreCreateMutex();

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    //install gpio isr service
    //Pass value 0, it will default to allocating a non-shared interrupt of level 1, 2 or 3
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_TCA9534_INT, TCA9534_isr_handler, (void *)GPIO_TCA9534_INT);

    esp_err_t ret = TCA9534_init(TCA9534_inst, TCA9534_ADDR, I2C_MASTER_NUM, GPIO_TCA9534_INT, bus_mutex);
    if (ret != ESP_OK)
    {
        /*Do something*/
    }
    ret = TCA9534_set_pin_direction(TCA9534_inst, TCA9534_SD_WP, TCA9534_INPUT);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error setting SD WP pin direction, returned %d", (int)ret);
    }
    ret = TCA9534_set_pin_direction(TCA9534_inst, TCA9534_SD_CD, TCA9534_INPUT);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error setting SD CD pin direction, returned %d", (int)ret);
    }
    ret = TCA9534_set_pin_direction(TCA9534_inst, TCA9534_WARN_LED, TCA9534_OUTPUT);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error setting LED pin direction, returned %d", (int)ret);
    }
    ret = TCA9534_set_pin_direction(TCA9534_inst, TCA9534_BATT_SEL, TCA9534_OUTPUT);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error setting Battery selector pin direction, returned %d", (int)ret);
    }
    return ret;
}

esp_err_t setupMLX90640(peripherals_struct *device_peripherals, SemaphoreHandle_t *bus_mutex)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t ret;
    int cnt = 0;
    uint16_t reg_val;
    uint16_t eedata[832]={0};

    *bus_mutex = xSemaphoreCreateMutex();

    ret = MLX90640_init(&(device_peripherals->IAQ_MLX90640), MLX90640_ADDR, I2C_MASTER_NUM, bus_mutex);
    if (ret != ESP_OK)
    {
        return ret;
    }
    memset(&(device_peripherals->MLX90640params), 0, sizeof(paramsMLX90640));
    /*Due to some devices having hardware issues leading to wrong values in EEPROM we chack that the status register in RAM has default value 0x1901,
    if not we manually set RAM value to 0x1901*/
    MLX90640_I2CRead(&(device_peripherals->IAQ_MLX90640), 0x800D, 1, &reg_val);
    if(reg_val != 0x1901)
    {
        while(reg_val != 0x1901 && cnt < 5)
        {
            MLX90640_I2CWrite(&(device_peripherals->IAQ_MLX90640), 0x800D, 0x1901);
            vTaskDelay(50 / portTICK_RATE_MS);
            MLX90640_I2CRead(&(device_peripherals->IAQ_MLX90640), 0x800D, 1, &reg_val);
            cnt++;
        }
        if(reg_val != 0x1901)
        {
            return ESP_FAIL;
        }
    }
    if(MLX90640_DumpEE(&(device_peripherals->IAQ_MLX90640), eedata) != 0)
    {
        return ESP_FAIL;
    }
    /*Removed check until I understand what this does to bad pixels..check eeData[10] manually for now*/
    MLX90640_ExtractParameters(eedata, &(device_peripherals->MLX90640params));

    ESP_LOGI(TAG,"EEPROM kVdd: %d", device_peripherals->MLX90640params.kVdd);
    ESP_LOGI(TAG,"EEPROM vdd25: %d", device_peripherals->MLX90640params.vdd25);
    ESP_LOGI(TAG,"EEPROM KvPTAT: %f", device_peripherals->MLX90640params.KvPTAT);
    ESP_LOGI(TAG,"EEPROM KtPTAT: %f", device_peripherals->MLX90640params.KtPTAT);
    ESP_LOGI(TAG,"EEPROM vPTAT25: %d", device_peripherals->MLX90640params.vPTAT25);
    ESP_LOGI(TAG,"EEPROM alphaPTAT: %f", device_peripherals->MLX90640params.alphaPTAT);
    ESP_LOGI(TAG,"EEPROM gainEE: %d", device_peripherals->MLX90640params.gainEE);
    ESP_LOGI(TAG,"EEPROM tgc: %f", device_peripherals->MLX90640params.tgc);
    ESP_LOGI(TAG,"EEPROM cpKv: %f", device_peripherals->MLX90640params.cpKv);
    ESP_LOGI(TAG,"EEPROM cpKta: %f", device_peripherals->MLX90640params.cpKta);    
    ESP_LOGI(TAG,"EEPROM resolutionEE: %d", device_peripherals->MLX90640params.resolutionEE);       
    ESP_LOGI(TAG,"EEPROM calibrationModeEE: %d", device_peripherals->MLX90640params.calibrationModeEE); 
    ESP_LOGI(TAG,"EEPROM KsTa: %f", device_peripherals->MLX90640params.KsTa);
    return ESP_OK;
}

esp_err_t setup_i2c(i2c_port_t port, int frequency, gpio_num_t sda_gpio, gpio_pullup_t sda_pullup_state, gpio_num_t scl_gpio, gpio_pullup_t scl_pullup_state)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
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

void webServerTask(void *pvParameters)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

    /*Allocate this connection memory to psram*/
    connectionMemory = (char*)malloc(sizeof(RtosConnType) * MAX_CONNECTIONS);

    espFsInit((void *)(image_espfs_start));

    tcpip_adapter_init();

    httpdFreertosInit(&httpdFreertosInstance, builtInUrls, WEB_PORT, connectionMemory, MAX_CONNECTIONS, HTTPD_FLAG_NONE);

    httpdFreertosStart(&httpdFreertosInstance);

    init_wifi();

    while (1)
    {
        /*Jusr a delay to yield control of the CPU, to feed the watchdog via the idle task, though this task priority is very low*/
        vTaskDelay(100 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}
/*

void acquisitionTask(void *pvParameters)
{

}
*/

void app_main()
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t ret;
    SemaphoreHandle_t i2c_bus_mutex = NULL;

    ret = setup_i2c(I2C_MASTER_NUM, I2C_MASTER_FREQ_HZ, I2C_MASTER_SDA_IO, GPIO_PULLUP_DISABLE, I2C_MASTER_SCL_IO, GPIO_PULLUP_DISABLE);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Successfully setup I2C peripheral");
        /*Do something*/
        ret = setupGpioExpander(&(device_peripherals.IAQ_TCA9534), &i2c_bus_mutex);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Could not setup GPIO expander");
            /*Do something*/
        }
        ret = setupTempHumiditySensor(&(device_peripherals.IAQ_HIH6030), &i2c_bus_mutex);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Could not setup Temp-humidity sensor");
            /*Do something*/
        }
        ret = setupMLX90640(&device_peripherals, &i2c_bus_mutex);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Could not setup thermal camera");
            /*Do something*/
        }
        ret = setupRTC(&(device_peripherals.IAQ_DS3231), &i2c_bus_mutex);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Could not setup RTC");
            /*Do something*/
        }
    }
    else
    {
        ESP_LOGE(TAG, "Could not setup I2C peripheral");
        /* Do something */
    }
    
    ret = setup_particulate_sensor(&(device_peripherals.IAQ_ZH03), ZH03_UART_NUM, ZH03_BAUDRATE, ZH03_TXD_PIN, ZH03_RXD_PIN);
    if (ret != ESP_OK)
    {
        /*Do something*/
    }
    /*Erasing NVS for use by bluetooth and Wifi*/
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGI(TAG, "Erasing NVS...");
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    /*Reboot if unsuccessful*/
    ESP_ERROR_CHECK(ret);

    /*Release memory set aside for clasic BT as we shall not use it*/
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    /*Initiallise BT controller*/
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);

    /*Event group for task synchronisation*/
    task_sync_event_group = xEventGroupCreate();

    /*This should Same priority as the bluetooth task, but we will set the antenna access prioritisation in ESP-IDF, of the two*/
    xTaskCreatePinnedToCore(webServerTask, "webServerTask", 4096, NULL, 10, &xWebServerTaskHandle, 0);

    while (1)
    {
        /*Just a delay to yield control of the CPU, to feed the watchdog via the idle task, though this task priority is very low*/
        vTaskDelay(100 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}
