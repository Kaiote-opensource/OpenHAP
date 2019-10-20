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
#include <math.h>
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

#include "nvs.h"
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
#include "user_commands.h"

#include "task_sync.h"
#include "dev_settings.h"

#include "esp_sleep.h"
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

#include "statistics.h"

#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"


#define WEB_PORT 80U
#define MAX_CONNECTIONS 12U /*Maximum tabs open...spread over all clients*/

#define GPIO_TCA9534_INT     35

#define I2C_MASTER_SCL_IO    19
#define I2C_MASTER_SDA_IO    21
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_FREQ_HZ  400000

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

static nvs_handle my_handle;

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
static TaskHandle_t xacquisitionTaskHandle = NULL;
/*Webserver subtask handles - 1 per page*/

static TaskHandle_t xStatusBroadcastHandle = NULL;
static TaskHandle_t xInfaredBroadcastHandle = NULL;
static TaskHandle_t xTagBroadcastHandle = NULL;

size_t FCBDB62F1727_count = 0;
int64_t FCBDB62F1727_cumsum_RSSI = 0;
float FCBDB62F1727_running_RSSI_mean = 0;

size_t F2416138F240_count = 0;
int64_t F2416138F240_cumsum_RSSI = 0;
float F2416138F240_running_RSSI_mean = 0;

typedef struct
{
    Websock* ws;
    char* command;
    union
    {
        int32_t* signed_value;
        uint32_t* unsigned_value;
        float* float_value;
        double* double_value;
    }value;
    char* uid;
    char* return_code_string;
} user_command;

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

/* declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_eddystone_show_inform(const esp_eddystone_result_t *res, const esp_ble_gap_cb_param_t *scan_result);

void statusPageWsBroadcastTask(void *pvParameters);
void infaredPageWsBroadcastTask(void *pvParameters);
void tagPageWsBroadcastTask(void *pvParameters);

static esp_ble_scan_params_t ble_scan_params = 
{
    .scan_type = BLE_SCAN_TYPE_PASSIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    /*Wifi takes over antenna time for (scan_window-scan_interval) in webserver mode, if hardware arbitration is preffered over software in menuconfig*/
    /*scan interval*0.625ms = scan interval time*/
    .scan_interval = 0x50,
    /*scan_window*0.625ms = scan window time*/
    .scan_window = 0x30,
    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE
};

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
            ESP_LOGI(TAG, " Webserver setup mode bluetooth handler entry");
            if ((xEventGroupGetBits(task_sync_event_group) & CURRENT_MODE_ACQUISITION) != CURRENT_MODE_ACQUISITION)
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
            else if((xEventGroupGetBits(task_sync_event_group) & CURRENT_MODE_ACQUISITION) == CURRENT_MODE_ACQUISITION)
            {
                ESP_LOGI(TAG, " Acquisition mode bluetooth handler entry");
                sprintf(mac_buffer, "%02X%02X%02X%02X%02X%02X", scan_result->scan_rst.bda[0],
                                                                scan_result->scan_rst.bda[1],
                                                                scan_result->scan_rst.bda[2],
                                                                scan_result->scan_rst.bda[3],
                                                                scan_result->scan_rst.bda[4],
                                                                scan_result->scan_rst.bda[5]);
                if(strcmp(mac_buffer, "FCBDB62F1727") == 0)
                {
                    ESP_LOGI(TAG, " Gotten 0xFCBDB62F1727");
                    FCBDB62F1727_count++;
                    ESP_LOGI(TAG, "FCBDB62F1727 TLM observation received, total: %d", FCBDB62F1727_count);
                    FCBDB62F1727_cumsum_RSSI += (scan_result->scan_rst.rssi);
                    ESP_LOGI(TAG, "FCBDB62F1727 TLM observation RSSI is %d dBm", scan_result->scan_rst.rssi); 
                    /*Check if count is zero to prevent a division by zero exception*/            
                    FCBDB62F1727_count ==0?(FCBDB62F1727_running_RSSI_mean = 0):(FCBDB62F1727_running_RSSI_mean = (float)FCBDB62F1727_cumsum_RSSI/FCBDB62F1727_count);
                    ESP_LOGI(TAG, "FCBDB62F1727 TLM running RSSI mean is %f dBm", FCBDB62F1727_running_RSSI_mean);  
                }
                else if(strcmp(mac_buffer, "F2416138F240") == 0)
                {
                    ESP_LOGI(TAG, " Gotten 0xF2416138F240");
                    F2416138F240_count++;
                    ESP_LOGI(TAG, "F2416138F240 TLM observation received, total: %d", F2416138F240_count);
                    F2416138F240_cumsum_RSSI += (scan_result->scan_rst.rssi);
                    ESP_LOGI(TAG, "F2416138F240 TLM observation RSSI is %d dBm", scan_result->scan_rst.rssi);
                    /*Check if count is zero to prevent a division by zero exception*/       
                    F2416138F240_count == 0? (F2416138F240_running_RSSI_mean = 0):(F2416138F240_running_RSSI_mean = (float)F2416138F240_cumsum_RSSI/F2416138F240_count);
                    ESP_LOGI(TAG, "F2416138F240 TLM running RSSI mean is %f dBm", F2416138F240_running_RSSI_mean);  
                }
                else
                {
                    ESP_LOGI(TAG, " Gotten unregistered tag %s", mac_buffer);
                }
                break;
            }
        }
        default:
        {
            ESP_LOGW(TAG, "Other frame type received");
            break;
        }
    }
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t ret;

    EventBits_t xEventGroupValue;
    EventBits_t xBitstoWaitFor = NOTIFY_WEBSERVER_TAG_INFO_TASK_CLOSE | NOTIFY_ACQUISITION_WINDOW_DONE;

    switch (event)
    {
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
        {
            ret = esp_ble_gap_start_scanning(BLE_SCAN_DURATION);
            if(ret != ESP_OK)
            {
                ESP_LOGE(TAG, "Scanning returned error!");
            }
            break;
        }
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        {
            if ((ret = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS)
            {
                ESP_LOGE(TAG, "Scan start failed: returned code %s", esp_err_to_name(ret));
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
                    ret = esp_eddystone_decode(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len, &eddystone_res);
                    if (ret)
                    {
                        //ESP_LOGI(TAG, "The received data is not an eddystone frame packet or a correct eddystone frame packet.");
                        // error:The received data is not an eddystone frame packet or a correct eddystone frame packet.
                        // just return
                        return;
                    }
                    else
                    {
                        esp_eddystone_show_inform(&eddystone_res, scan_result);
                    }
                    break;
                }
                case ESP_GAP_SEARCH_INQ_CMPL_EVT:
                {
                    ESP_LOGI(TAG, "Scan is complete!");
                    xEventGroupValue = xEventGroupWaitBits(task_sync_event_group, xBitstoWaitFor, pdTRUE, pdFALSE, 0); 
                    if((xEventGroupValue & xBitstoWaitFor) == NOTIFY_WEBSERVER_TAG_INFO_TASK_CLOSE ||
                    (xEventGroupValue & xBitstoWaitFor) == NOTIFY_ACQUISITION_WINDOW_DONE)
                    {
                        ESP_LOGI(TAG, "Stopping scan due to signal being raised!");                
                        //Let task finish up cleanly...signal back to the page that these results are hosted!
                        if((ret = esp_bt_controller_disable()) != ESP_OK)
                        {
                            ESP_LOGE(TAG, "Could not disable bluetooth controller");
                        }
                        ESP_LOGI(TAG, "Disabled bluetooth controller");
                        if((ret = esp_bt_controller_deinit()) != ESP_OK)
                        {
                            ESP_LOGE(TAG, "Could not deinit bluetooth controller");
                        }
                        ESP_LOGI(TAG, "Deinited bluetooth controller");
                        xEventGroupSetBits( task_sync_event_group,  NOTIFY_BT_CALLBACK_DONE);
                    }
                    else
                    {
                        ESP_LOGI(TAG, "Restarting scan!");
                        esp_ble_gap_set_scan_params(&ble_scan_params);
                    }
                }
                default:
                {
                    break;
                }
            }
        }
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        {
            if ((ret = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS)
            {
                ESP_LOGE(TAG, "Scan stop failed: Returned code %s", esp_err_to_name(ret));
            }
            else
            {
                ESP_LOGI(TAG, "Scan stopped");
            }
            break;
        }
        default:
        {
            ESP_LOGE(TAG, "Other callback event received");
            break;
        }
    }
}

esp_err_t esp_eddystone_app_register(void)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t status;

    /*<! register the scan callback function to the gap module */
    if ((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK)
    {
        ESP_LOGE(TAG, "gap register error: %s", esp_err_to_name(status));
        return status;
    }
    return ESP_OK;
}

static void myWebsocketRecv(Websock *ws, char *data, int len, int flags)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);

    char* value_search_string;
    char* uid_search_string;
    char* sanitised_json_string = NULL;
    JSON_Value *root_value = NULL;
    JSON_Object * root_object = NULL;
    root_value = json_parse_string(data);
    if(root_value != NULL)
    {
        sanitised_json_string = json_serialize_to_string(root_value);
        ESP_LOGI(TAG, "Actual JSON received %s", sanitised_json_string);
        json_free_serialized_string(sanitised_json_string);
        if (json_value_get_type(root_value) != JSONObject)
        {
            ESP_LOGE(TAG, "Error! Json root type is type %d, expected %d", json_value_get_type(root_value), JSONObject);
            /*Add error handling here*/
            return;
        }
        root_object = json_value_get_object(root_value);
        ESP_LOGI(TAG, "Root object has %d element(s)", json_object_get_count(root_object));
        for(int i=0; i < COMMAND_NUM; i++)
        {   
            /*Search for the first valid command string...discard any else*/
            if(json_object_has_value(root_object, user_commands[i]))
            {
                ESP_LOGI(TAG, "Found command %s", user_commands[i]);
                if(strcmp(user_commands[i], "SET_TIME") == 0)
                {
                    asprintf(&value_search_string, "%s.VALUE", user_commands[i]);
                    ESP_LOGD(TAG, "Searching for %s...", value_search_string);
                    time_t unix_time = (uint32_t)json_object_dotget_number(root_object, value_search_string);
                    ESP_LOGD(TAG, "Found %s as %ul", value_search_string, (unsigned int)unix_time);
                    //command.value.unsigned_value = (uint32_t)json_object_dotget_number(root_object, value_search_string);
                    asprintf(&uid_search_string, "%s.UID", user_commands[i]);
                    ESP_LOGD(TAG, "Searching for %s...", uid_search_string);
                    ESP_LOGD(TAG, "Found %s as %s", value_search_string, json_object_dotget_string(root_object, uid_search_string));
                    localtime_r(&unix_time, &date_time);
                    if(ds3231_set_time(&(device_peripherals.IAQ_DS3231), &date_time) == ESP_OK)
                    {
                        ESP_LOGI(TAG, "Successfully set device time");
                        if(json_object_dotremove(root_object, value_search_string) == JSONSuccess)
                        {
                            ESP_LOGI(TAG, "Successfully created command feedback message");
                            sanitised_json_string = json_serialize_to_string(root_value);
                            ESP_LOGI(TAG, "Json response to client %s", sanitised_json_string);
                            cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, sanitised_json_string, strlen(sanitised_json_string), WEBSOCK_FLAG_NONE);
                            json_free_serialized_string(sanitised_json_string);
                        }
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Could not set device time at this time, try again!");
                    }       
                    free(value_search_string);
                    free(uid_search_string);
                }
                else if(strcmp(user_commands[i], "START_MEASUREMENT") == 0)
                {
                    asprintf(&value_search_string, "%s.VALUE", user_commands[i]);
                    ESP_LOGD(TAG, "Searching for %s...", value_search_string);
                    uint8_t shutdown_value = (uint32_t)json_object_dotget_number(root_object, value_search_string);
                    ESP_LOGD(TAG, "Found %s as %u", value_search_string, (unsigned int)shutdown_value);
                    asprintf(&uid_search_string, "%s.UID", user_commands[i]);
                    ESP_LOGD(TAG, "Searching for %s...", uid_search_string);
                    ESP_LOGD(TAG, "Found %s as %s", value_search_string, json_object_dotget_string(root_object, uid_search_string));
                    if(shutdown_value == 1)
                    {
                        esp_err_t ret = nvs_open("nvs", NVS_READWRITE, &my_handle);
                        if (ret != ESP_OK)
                        {
                            ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(ret));
                        }
                        else 
                        {
                            ESP_LOGI(TAG, "Opened NVS handle for R/W operation");
                            ret = nvs_set_i32(my_handle, "mode", acquisition_mode);
                            if(ret == ESP_OK)
                            {
                                ESP_LOGI(TAG, "Successfully set aquisition mode in NVS");
                                ESP_LOGI(TAG, "Committing updates in NVS ... ");
                                ret = nvs_commit(my_handle);
                                if(ret == ESP_OK)
                                {
                                    ESP_LOGI(TAG, "Successfully committed updates to flash");
                                    if(json_object_dotremove(root_object, value_search_string) == JSONSuccess)
                                    {
                                        ESP_LOGD(TAG, "Sending message back to client...");
                                        sanitised_json_string = json_serialize_to_string(root_value);
                                        ESP_LOGI(TAG, "Json response to client %s", sanitised_json_string);
                                        cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, sanitised_json_string, strlen(sanitised_json_string), WEBSOCK_FLAG_NONE);
                                        json_free_serialized_string(sanitised_json_string);
                                    }
                                    else
                                    {
                                        ESP_LOGW(TAG, "Failed to send message to client!");
                                    }
                                    ESP_LOGI(TAG, "Closing webserver...");                                
                                    vTaskDelay(3000/portTICK_RATE_MS);
                                    xEventGroupSetBits( task_sync_event_group,  NOTIFY_WEBSERVER_CLOSE);
                                    /*Block until the webserver finishes cleanly...for the maximum available time*/
                                    EventBits_t xEventGroupValue = xEventGroupWaitBits(task_sync_event_group, NOTIFY_WEBSERVER_DONE, pdTRUE, pdFALSE, 5000/portTICK_RATE_MS);
                                    if(xEventGroupValue & NOTIFY_WEBSERVER_DONE)
                                    {
                                        ESP_LOGI(TAG, "Webserver closed cleanly, restarting...");                                         
                                        esp_restart();
                                    }
                                    else
                                    {
                                        //TODO sort this part to restart 
                                        ESP_LOGI(TAG, "Failed to close webserver cleanly in time, restarting back to webserver!");
                                        ret = nvs_set_i32(my_handle, "mode", acquisition_mode);
                                        /*As the webserver may be in an unstable state due to incomplete closing, just restart in case of a failure here*/
                                        ESP_ERROR_CHECK(ret);
                                        ret = nvs_commit(my_handle);
                                        ESP_ERROR_CHECK(ret);
                                        esp_restart();
                                    }
                                }
                                else
                                {
                                    ESP_LOGE(TAG, "Error (%s) Failed to commit updates to flash...not restarting", esp_err_to_name(ret));
                                }
                            }
                            else
                            {
                                ESP_LOGD(TAG, "Failed to set next mode in NVS!...not restarting");
                            }
                            nvs_close(my_handle);
                        }
                    }
                    free(value_search_string);
                    free(uid_search_string);                    
                }
            }
        }
    }
    else
    {
        ESP_LOGE(TAG, "Parse string returned error!");
        /*Add error handling here*/
    }
    return;
}

/*Websocket disconnected. Stop task if none connected*/
static void myStatusWebsocketClose(Websock *ws)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    int connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance, status_cgi_resource_string, null_test_string, strlen(null_test_string), WEBSOCK_FLAG_NONE);
    ESP_LOGI(TAG, "Status page connection close received, Previous connections were %d", connections);

    if (connections == 1)
    {
        xEventGroupSetBits( task_sync_event_group,  NOTIFY_WEBSERVER_STATUS_TASK_CLOSE);      
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
        if (xTaskCreatePinnedToCore(statusPageWsBroadcastTask, "statusPageTask", 8192, NULL, 2, &xStatusBroadcastHandle,1))
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
        if (xTaskCreatePinnedToCore(infaredPageWsBroadcastTask, "infaredPageTask", 24000, NULL, 2, &xInfaredBroadcastHandle, 1))
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
        if (xTaskCreatePinnedToCore(tagPageWsBroadcastTask, "tagPageTask", 4096, NULL, 2, &xTagBroadcastHandle, 1))
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
            ESP_LOGE(TAG, "Config reg 0x800D is not 0x1901, value found 0x%04X...attempting I2C Config reg 0x800D write to reconfigure", reg_val);
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
            ESP_LOGI(TAG, "Successfully set config reg to 0x800D");
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


char *pollSensorStatus(char *status_string, peripherals_struct *device_peripherals)
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
    if(get_particulate_reading_QA_mode(&(device_peripherals->IAQ_ZH03), NULL, &PM_2_5, NULL)==ESP_OK)
    {
        json_object_set_number(root_object, "PM25", PM_2_5);
    }
    json_object_set_number(root_object, "RTC_BATT", (int)(((battery_level*2)/30)));
    json_object_set_number(root_object, "DEVICE_TIME", mktime(&date_time));
    json_object_set_string(root_object, "SD_CARD", cd_level == TCA9534_HIGH ? "DISCONNECTED" : "CONNECTED");
    status_string = json_serialize_to_string(root_value);
    ESP_LOGD(TAG, "%s", status_string);
    json_value_free(root_value);
    ESP_LOGI(TAG, "status string length is %d bytes", strlen(status_string));

    return status_string;
}

char* b64_encode_thermal_img(char *base64_dst, float *data)
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
        return NULL;
    }
    ESP_LOGI(TAG, "Function returned succesful");
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

void statusPageWsBroadcastTask(void *pvParameters)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    
    int connections;
    char *status_string = NULL;

    EventBits_t xEventGroupValue;
    EventBits_t xBitsDontNotifyCaller = NOTIFY_WEBSERVER_STATUS_TASK_CLOSE;
    EventBits_t xBitsNotifyCaller = (NOTIFY_WEBSERVER_STATUS_TASK_CLOSE | NOTIFY_CALLER_WEBSERVER_STATUS_TASK_DONE);

    /*Initialise ADC to sample battery cell voltage*/
    init_adc();

    while (1)
    {

        status_string = pollSensorStatus(status_string, &device_peripherals);
        /*Take care of race conditions here when waiting on more than one bit*/
        xEventGroupValue = xEventGroupWaitBits(task_sync_event_group, xBitsNotifyCaller, pdTRUE, pdFALSE, 0); 

        if (status_string != NULL)
        {
            if((xEventGroupValue & xBitsNotifyCaller) == xBitsNotifyCaller)
            {
                //Let task finish up cleanly
                json_free_serialized_string(status_string);
                xStatusBroadcastHandle = NULL;
                xEventGroupSetBits(task_sync_event_group, NOTIFY_WEBSERVER_STATUS_TASK_DONE);
                vTaskDelete(NULL);
            }
            else if ((xEventGroupValue & xBitsNotifyCaller) == xBitsDontNotifyCaller)
            {
                //Let task finish up cleanly
                json_free_serialized_string(status_string);
                xStatusBroadcastHandle = NULL;
                vTaskDelete(NULL);                
            }
            else
            {
                connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance, status_cgi_resource_string, status_string, strlen(status_string), WEBSOCK_FLAG_NONE);
                json_free_serialized_string(status_string);
                ESP_LOGD(TAG, "Broadcast sent to %d connections", connections);
            }
        }
        else
        {
            if((xEventGroupValue & xBitsNotifyCaller) == xBitsNotifyCaller)
            {
                //Let task finish up cleanly
                xStatusBroadcastHandle = NULL;
                xEventGroupSetBits(task_sync_event_group, NOTIFY_WEBSERVER_STATUS_TASK_DONE);
                vTaskDelete(NULL);
            }
            else if ((xEventGroupValue & xBitsNotifyCaller) == xBitsDontNotifyCaller)
            {
                //Let task finish up cleanly
                xStatusBroadcastHandle = NULL;
                vTaskDelete(NULL);                
            }
            else
            {
                ESP_LOGE(TAG, "Status page JSON assembler returned NULL");
            }
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
        /*Subtract the quantity of data already transferred.*/
        size_to_do -= chunk_size; 
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
    EventBits_t xBitsDontNotifyCaller = NOTIFY_WEBSERVER_THERMAL_VIEWER_TASK_CLOSE;
    EventBits_t xBitsNotifyCaller = (NOTIFY_WEBSERVER_THERMAL_VIEWER_TASK_CLOSE | NOTIFY_CALLER_WEBSERVER_THERMAL_VIEWER_TASK_DONE);

    while (1)
    {
        ret = get_thermal_image(&device_peripherals, image_buffer);
        if(ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to get thermal image, returned error code %d", ret);
            /*Display Zeroed out image buffer to signify that there is an error.*/
            memset(image_buffer, 0, 768*sizeof(float));
        }
        base64_camera_string = b64_encode_thermal_img(base64_camera_string, image_buffer);
        /*Take care of race conditions on bit setting by the caller task*/
        xEventGroupValue = xEventGroupWaitBits(task_sync_event_group, xBitsNotifyCaller, pdTRUE, pdFALSE, 0);
    
        if (base64_camera_string != NULL)
        { 
            if((xEventGroupValue & xBitsNotifyCaller) == xBitsNotifyCaller)
            {
                /*Let task finish up cleanly*/
                free(base64_camera_string);
                xInfaredBroadcastHandle = NULL;
                xEventGroupSetBits(task_sync_event_group, NOTIFY_WEBSERVER_THERMAL_VIEWER_TASK_DONE);
                vTaskDelete(NULL);
            }
            else if ((xEventGroupValue & xBitsNotifyCaller) == xBitsDontNotifyCaller)
            {
                /*Let task finish up cleanly*/
                 xInfaredBroadcastHandle = NULL;
                vTaskDelete(NULL);                
            }
            else
            {
                /*ESP_LOG_BUFFER_HEXDUMP("Base64 data:", base64_string, strlen(base64_string)+1, ESP_LOG_DEBUG);*/
                wsStringBurstBroadcast(base64_camera_string, "CAMERA_CONT", "CAMERA_END");
                free(base64_camera_string);
            }
        }
        else
        {
            if((xEventGroupValue & xBitsNotifyCaller) == xBitsNotifyCaller)
            {
                /*Let task finish up cleanly*/
                xInfaredBroadcastHandle = NULL;
                xEventGroupSetBits(task_sync_event_group, NOTIFY_WEBSERVER_THERMAL_VIEWER_TASK_DONE);
                vTaskDelete(NULL);
            }
            else if ((xEventGroupValue & xBitsNotifyCaller) == xBitsDontNotifyCaller)
            {
                /*Let task finish up cleanly*/
                xInfaredBroadcastHandle = NULL;
                vTaskDelete(NULL);                
            }
            else
            {
                ESP_LOGE(TAG, "Infared page JSON assembler returned NULL");
            }
            
        }
        /*1000ms if the delay tested that would be longer than (data transfer+browser render) time both on mobile and desktop.
          This means that the next image will be sent 1 second later after all browser image operations have completed */
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

void tagPageWsBroadcastTask(void *pvParameters)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t ret;

    EventBits_t xEventGroupValue;
    EventBits_t xBitsDontNotifyCaller = NOTIFY_WEBSERVER_TAG_INFO_TASK_CLOSE;
    EventBits_t xBitsNotifyCaller = (NOTIFY_WEBSERVER_TAG_INFO_TASK_CLOSE | NOTIFY_CALLER_WEBSERVER_TAG_INFO_TASK_DONE);

    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED && esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_UNINITIALIZED)
    {
        ESP_LOGI(TAG, "Enabling bluetooth controller and bluedroid");
        esp_bt_controller_enable(ESP_BT_MODE_BLE);
        /*Set up bluetooth peripheral hardware*/
        if((ret = esp_bluedroid_init()) != ESP_OK)
        {
            ESP_LOGE(TAG, "Could not initialise and allocate bluedroid");
        }
        ESP_LOGI(TAG, "Bluedroid initialisation complete");
        if((ret = esp_bluedroid_enable()) != ESP_OK)
        {
            ESP_LOGE(TAG, "Could not enable bluedroid");
        }
        ESP_LOGI(TAG, "Bluedroid is now enabled");
        if((ret = esp_eddystone_app_register()) != ESP_OK)
        {
            ESP_LOGE(TAG, "Eddystone callback handler registration unsuccessful");
        }
        ESP_LOGI(TAG, "Eddystone callback handler succesfully registered");
        /*set parameters for the next scan*/
        esp_ble_gap_set_scan_params(&ble_scan_params);
    }
    else if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED && esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED)
    {
        ESP_LOGI(TAG, "Bluetooth controller and bluedroid already enabled...starting scan");
        esp_ble_gap_set_scan_params(&ble_scan_params);
    }
    else
    {
        ESP_LOGE(TAG, "Other controller- host state received! Error!");
    }
    
    while(1)
    {
        /*Take care of race conditions on bit setting by the caller task*/
        xEventGroupValue = xEventGroupWaitBits(task_sync_event_group, xBitsNotifyCaller, pdTRUE, pdFALSE, 10/portTICK_RATE_MS);
        if(xEventGroupValue & xBitsNotifyCaller)
        {
            if((xEventGroupValue & xBitsNotifyCaller) == xBitsNotifyCaller)
            {
                xEventGroupValue = xEventGroupWaitBits(task_sync_event_group, NOTIFY_BT_CALLBACK_DONE, pdTRUE, pdFALSE, 12000/portTICK_RATE_MS);
                if(xEventGroupValue & NOTIFY_BT_CALLBACK_DONE)
                {
                    /*Let task finish up cleanly*/
                    xTagBroadcastHandle = NULL;
                    xEventGroupSetBits(task_sync_event_group, NOTIFY_WEBSERVER_TAG_INFO_TASK_DONE);
                    vTaskDelete(NULL);
                }
            }
            else if ((xEventGroupValue & xBitsNotifyCaller) == xBitsDontNotifyCaller)
            {
                xEventGroupValue = xEventGroupWaitBits(task_sync_event_group, NOTIFY_BT_CALLBACK_DONE, pdTRUE, pdFALSE, 12000/portTICK_RATE_MS);
                if(xEventGroupValue & NOTIFY_BT_CALLBACK_DONE)
                {
                    /*Let task finish up cleanly*/
                    xTagBroadcastHandle = NULL;
                    vTaskDelete(NULL);
                }               
            }
        }
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
    free(ssid);
    free(password);
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

    return ret;
}
esp_err_t setupGpioExpander(TCA9534 *TCA9534_inst, SemaphoreHandle_t *bus_mutex)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    *bus_mutex = xSemaphoreCreateMutex();

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

    esp_err_t ret=unset_dormant_mode(&(device_peripherals.IAQ_ZH03));
    ESP_ERROR_CHECK(ret);

    ret = set_QA_mode(&(device_peripherals.IAQ_ZH03));
    ESP_ERROR_CHECK(ret);

    /*Allocate this connection memory to psram*/
    connectionMemory = (char*)malloc(sizeof(RtosConnType) * MAX_CONNECTIONS);

    espFsInit((void *)(image_espfs_start));

    tcpip_adapter_init();

    httpdFreertosInit(&httpdFreertosInstance, builtInUrls, WEB_PORT, connectionMemory, MAX_CONNECTIONS, HTTPD_FLAG_NONE);

    httpdFreertosStart(&httpdFreertosInstance);

    init_wifi();

    while (1)
    {
        /*Block for infinitely for 100ms waiting for the webserver close signal to be set*/
        EventBits_t xEventGroupValue = xEventGroupWaitBits(task_sync_event_group, NOTIFY_WEBSERVER_CLOSE, pdTRUE, pdFALSE,  1000/portTICK_RATE_MS);
        if(xEventGroupValue & NOTIFY_WEBSERVER_CLOSE)
        {
            ESP_LOGI(TAG, "Webserver task received close event");
            EventBits_t xBitstoWaitFor = 0;
            if(xStatusBroadcastHandle != NULL)
            {
                ESP_LOGI(TAG, "Sending close event to status broadcast task");
                xBitstoWaitFor = xBitstoWaitFor | NOTIFY_WEBSERVER_STATUS_TASK_DONE;
                xEventGroupSetBits(task_sync_event_group, NOTIFY_WEBSERVER_STATUS_TASK_CLOSE | 
                                                          NOTIFY_CALLER_WEBSERVER_STATUS_TASK_DONE);
            }
            if(xInfaredBroadcastHandle != NULL)
            {
                ESP_LOGI(TAG, "Sending close event to infared viewer broadcast task");
                xBitstoWaitFor = xBitstoWaitFor | NOTIFY_WEBSERVER_THERMAL_VIEWER_TASK_DONE;
                xEventGroupSetBits(task_sync_event_group, NOTIFY_WEBSERVER_THERMAL_VIEWER_TASK_CLOSE | 
                                                          NOTIFY_CALLER_WEBSERVER_THERMAL_VIEWER_TASK_DONE);
            }
            if(xTagBroadcastHandle != NULL)
            {
                ESP_LOGI(TAG, "Sending close event to Tag info broadcast task");
                xBitstoWaitFor = xBitstoWaitFor | NOTIFY_WEBSERVER_TAG_INFO_TASK_DONE;
                xEventGroupSetBits(task_sync_event_group, NOTIFY_WEBSERVER_TAG_INFO_TASK_CLOSE | 
                                                          NOTIFY_CALLER_WEBSERVER_TAG_INFO_TASK_DONE);               
            }
            /*Block for 10 seconds for all tasks to close*/
            xEventGroupValue = xEventGroupWaitBits(task_sync_event_group, xBitstoWaitFor, pdTRUE, pdTRUE, 10000/portTICK_RATE_MS);
            if(xEventGroupValue & xBitstoWaitFor)
            {
                xEventGroupSetBits(task_sync_event_group, NOTIFY_WEBSERVER_DONE);
            }
        }
    }
    vTaskDelete(NULL);
}


void acquisitionTask(void *pvParameters)
{
    float temperature = 0.0;;
    float humidity = 0.0;;
    hih6030_status_t status;

    int32_t PM_1 = 0;
    int32_t PM_2_5 = 0;
    int32_t PM_10 = 0;
    
    float image_buffer[768] = {0.0};

    ESP_LOGI(TAG, "Enabling bluetooth controller and bluedroid");
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());
    ESP_ERROR_CHECK(esp_eddystone_app_register());
    /*Both webserver setup and acquisition share the same GAP scan parameters*/
    ESP_ERROR_CHECK(esp_ble_gap_set_scan_params(&ble_scan_params));

    /*Switch on fan and block for ACQUISITION_FAN_SETUP_DURATION_MS milliseconds as bluetooth task is getting tag ID's*/
    ESP_ERROR_CHECK(unset_dormant_mode(&(device_peripherals.IAQ_ZH03)));
    vTaskDelay((ACQUISITION_FAN_SETUP_DURATION*1000)/portTICK_RATE_MS);

    ESP_LOGI(TAG, "Initializing SD card");
    ESP_LOGI(TAG, "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    gpio_set_pull_mode(15, GPIO_FLOATING);
    gpio_set_pull_mode(2, GPIO_FLOATING);
    gpio_set_pull_mode(4, GPIO_FLOATING);
    gpio_set_pull_mode(12, GPIO_FLOATING);
    gpio_set_pull_mode(13, GPIO_FLOATING);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = 
    {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;
    ESP_ERROR_CHECK(esp_vfs_fat_sdmmc_mount(SD_CARD_MOUNT_POINT, &host, &slot_config, &mount_config, &card));
    /*Card has been initialized, print its properties*/
    sdmmc_card_print_info(stdout, card);
    const char* logfile_name = "/sdcard/final.csv";

    int exists= 0;
    struct stat st;
    esp_err_t ret = ESP_FAIL;
    while(1)
    {
        /*Open file by append mode, if file does not exist, create it*/
        if (stat(logfile_name, &st) == 0)
        {
            /*File exists, set integer to be checked*/
            ESP_LOGI(TAG, "File exists, appending to file");
            exists = 1; 
        }
        else
        {
            /*File exists, set integer to be checked*/
            ESP_LOGW(TAG, "File does not exist, creating it...");
            exists = 0;           
        }
        
        int i;
        FILE* logfile = NULL;
        if(exists)
        {
            logfile = fopen(logfile_name, "a");
            if (logfile == NULL)
            {
                
                ESP_LOGE(TAG, "Failed to open file for writing");
                esp_vfs_fat_sdmmc_unmount();
                ESP_LOGI(TAG, "Card unmounted");
                esp_restart();
            }
            //Get time
            if (ds3231_get_time(&(device_peripherals.IAQ_DS3231), &date_time) != ESP_OK)
            {
                ESP_LOGE(TAG, "Could not get time from DS3231, returned %s", esp_err_to_name(ret));
                i = fprintf(logfile, "\"%d\",", -1);
            }
            else
            {
                i = fprintf(logfile, "\"%u\",", (unsigned int)mktime(&date_time));
            }
            if (i < 0)
            {
                ESP_LOGE(TAG, "ERROR: impossible to write to log file");
                esp_vfs_fat_sdmmc_unmount();
                ESP_LOGI(TAG, "Card unmounted");
                esp_restart();
            }
        }
        else
        {
            logfile = fopen(logfile_name, "w");
            if (logfile == NULL)
            {
                
                ESP_LOGE(TAG, "Failed to open file for writing");
                esp_vfs_fat_sdmmc_unmount();
                ESP_LOGI(TAG, "Card unmounted");
                esp_restart();
            }
            i = fprintf(logfile, "\"Time\",\"Mean_temp\",\"Max_temp\",\"Min_temp\",\"Temp_variance\",\"Temp_stddev\",\"Humidity\",\"Temperature\",\"PM 1\",\"PM 2.5\",\"PM 10\",\"FCBDB62F1727_count\",\"FCBDB62F1727_mean_dBm\",\"F2416138F240_count\",\"F2416138F240_mean_dBm\"\n");
            if (i < 0)
            {
                ESP_LOGE(TAG, "ERROR: impossible to write to log file");
                esp_vfs_fat_sdmmc_unmount();
                ESP_LOGI(TAG, "Card unmounted");
                esp_restart();
            }
            if (ds3231_get_time(&(device_peripherals.IAQ_DS3231), &date_time) != ESP_OK)
            {
                ESP_LOGE(TAG, "Could not get time from DS3231, returned %s", esp_err_to_name(ret));
                fprintf(logfile, "\"%d\",", -1);
            }
            else
            {
                fprintf(logfile, "\"%u\",", (unsigned int)mktime(&date_time));
            }
        
        }
        //Get two images prior as the first image is usually garbled
        get_thermal_image(&device_peripherals, image_buffer);
        get_thermal_image(&device_peripherals, image_buffer);
        ret = get_thermal_image(&device_peripherals, image_buffer);
        if(ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to get thermal image, returned %s", esp_err_to_name(ret));
            //avg
            fprintf(logfile, "\"%d\",", -1);
            //Max_temp
            fprintf(logfile, "\"%d\",", -1);
            //Min_temp
            fprintf(logfile, "\"%d\",", -1);
            //var_var
            fprintf(logfile, "\"%d\",", -1);
            //Temp_stddev
            fprintf(logfile, "\"%d\",", -1);
        }
//\"Mean_temp\",\"Temp_max\",\"Temp_min\",\"Temp_variance\",,\"Temp_stddev\"
        float avg;
        size_t validElements;
        if(meanValue(image_buffer, 768, &avg, false, &validElements) == ESP_OK)
        {
            fprintf(logfile, "\"%.2f\",", avg);
            ESP_LOGI(TAG, "Image mean value is %.2f degree celsius", avg);
        }
        else
        {
            fprintf(logfile, "\"%d\",", -1);
            ESP_LOGI(TAG, "Could not obtain image mean");            
        }
        float max;
        /*Dont exit on Nan and inf values, ignore*/
        if(floatMaxValue(image_buffer, 768, &max, false, &validElements) == ESP_OK)
        {
            fprintf(logfile, "\"%.2f\",", max);
            ESP_LOGI(TAG, "Image max value is %.2f degree celsius", max);
        }
        else
        {
            fprintf(logfile, "\"%d\",", -1);
            ESP_LOGI(TAG, "Could not obtain image max value");            
        }
        float min;
        /*Dont exit on Nan and inf values, ignore*/
        if(floatMinValue(image_buffer, 768, &min, false, &validElements) == ESP_OK)
        {
            fprintf(logfile, "\"%.2f\",", min);
            ESP_LOGI(TAG, "Image min value is %.2f degree celsius", min);
        }
        else
        {
            fprintf(logfile, "\"%d\",", -1);
            ESP_LOGI(TAG, "Could not obtain image min value");            
        }
        float var;
        /*Dont exit on Nan and inf values, ignore*/
        if(variance(image_buffer, 768, &var, false, &validElements) == ESP_OK)
        {
            fprintf(logfile, "\"%.2f\",", var);
            ESP_LOGI(TAG, "Image variance is +/-%.2f degree celsius", var);
        }
        else
        {
            fprintf(logfile, "\"%d\",", -1);
            ESP_LOGI(TAG, "Could not obtain image variance");            
        }
        float std_deviation;
        /*/*Dont exit on Nan and inf values, ignore*/
        if(stddev(image_buffer, 768, &std_deviation, false, &validElements) == ESP_OK)
        {
            fprintf(logfile, "\"%.2f\",", std_deviation);
            ESP_LOGI(TAG, "Image standard deviation is +/-%.2f degree celsius", std_deviation);
        }
        else
        {
            fprintf(logfile, "\"%d\",", -1);
            ESP_LOGI(TAG, "Could not obtain image standard deviation");            
        }
        ret = get_temp_humidity(&(device_peripherals.IAQ_HIH6030), &status, &temperature, &humidity);
        if(ret != ESP_OK)
        {             
            //Humidity
            fprintf(logfile, "\"%d\",", -1);
            //Temperature
            fprintf(logfile, "\"%d\",", -1);
        }
        //Humidity
        fprintf(logfile, "\"%.2f\",", humidity);
        //Temperature
        fprintf(logfile, "\"%.2f\",", temperature);
        if(get_particulate_reading_QA_mode(&(device_peripherals.IAQ_ZH03), &PM_1, &PM_2_5, &PM_10) != ESP_OK)
        {
            //PM 1
            fprintf(logfile, "\"%d\",", -1);
            //PM 2.5
            fprintf(logfile, "\"%d\",", -1);
            //PM 10
            fprintf(logfile, "\"%d\"\n", -1);
        }
        //PM 1
        fprintf(logfile, "\"%d\",", (int)PM_1);
        //PM 2.5
        fprintf(logfile, "\"%d\",", (int)PM_2_5);
        //PM 10
        fprintf(logfile, "\"%d\",", (int)PM_10);
        /* end of log file line */
        xEventGroupSetBits( task_sync_event_group,  NOTIFY_ACQUISITION_WINDOW_DONE);
        while((xEventGroupWaitBits(task_sync_event_group, NOTIFY_BT_CALLBACK_DONE, pdTRUE, pdFALSE, 100/portTICK_RATE_MS) & NOTIFY_BT_CALLBACK_DONE) != NOTIFY_BT_CALLBACK_DONE)
        {
            ESP_LOGI(TAG, "BT task done not set");
        }

        //Tag_1_count
        fprintf(logfile, "\"%d\",", FCBDB62F1727_count);
        //mean_dBm
        fprintf(logfile, "\"%.1f\",", FCBDB62F1727_running_RSSI_mean);
        //Tag_2_count
        fprintf(logfile, "\"%d\",", F2416138F240_count);
        //mean_dBm
        fprintf(logfile, "\"%.1f\"\n", F2416138F240_running_RSSI_mean);

        fflush(logfile);
        fclose(logfile);
        esp_vfs_fat_sdmmc_unmount();
        ESP_LOGI(TAG, "Unmounted SD card");
        set_dormant_mode(&(device_peripherals.IAQ_ZH03));
        esp_bt_controller_deinit();
        ESP_LOGI(TAG, "Entering deep sleep for %d seconds", DEEP_SLEEP_SEC);
        esp_deep_sleep(1000000LL * DEEP_SLEEP_SEC); 
    }
}


void app_main()
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    esp_err_t ret;
    SemaphoreHandle_t i2c_bus_mutex = NULL;

    ret = setup_i2c(I2C_MASTER_NUM, I2C_MASTER_FREQ_HZ, I2C_MASTER_SDA_IO, GPIO_PULLUP_DISABLE, I2C_MASTER_SCL_IO, GPIO_PULLUP_DISABLE);
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "Successfully setup I2C peripheral");
    ret = setupGpioExpander(&(device_peripherals.IAQ_TCA9534), &i2c_bus_mutex);
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "Successfully setup I2C based GPIO expander");
    ret = setupTempHumiditySensor(&(device_peripherals.IAQ_HIH6030), &i2c_bus_mutex);
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "Successfully setup Temperature-humidity sensor");
    ret = setupMLX90640(&device_peripherals, &i2c_bus_mutex);
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "Successfully setup infared thermal camera");
    ret = setupRTC(&(device_peripherals.IAQ_DS3231), &i2c_bus_mutex);
    ESP_LOGI(TAG, "Successfully setup Real time clock");
    ESP_ERROR_CHECK(ret);
    ret = setup_particulate_sensor(&(device_peripherals.IAQ_ZH03), ZH03_UART_NUM, ZH03_BAUDRATE, ZH03_TXD_PIN, ZH03_RXD_PIN);
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "Successfully setup aerosol sensor");

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

    /*Initialise BT controller*/
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);

    task_sync_event_group = xEventGroupCreate();

    int32_t cd_level;
    TCA9534_get_level(&(device_peripherals.IAQ_TCA9534), TCA9534_SD_CD, &cd_level);

    ret = nvs_open("nvs", NVS_READWRITE, &my_handle);
    ESP_ERROR_CHECK(ret);

    int32_t startup_mode;
    ret = nvs_get_i32(my_handle, "mode", &startup_mode);
    ESP_ERROR_CHECK(ret);
    if(startup_mode == acquisition_mode)
    {  
        ESP_LOGI(TAG, "Startup mode found set to 'acquisition mode' in NVS");
        if(cd_level == TCA9534_HIGH)
        {
            ESP_LOGI(TAG, "SD card disconnected, now setting startup mode to 'webserver setup mode'");
            ret = nvs_set_i32(my_handle, "mode", setup_mode);
            ESP_ERROR_CHECK(ret);
            ret = nvs_commit(my_handle);
            ESP_ERROR_CHECK(ret);
            nvs_close(my_handle);
            esp_restart();
        }
        else
        {
            ESP_LOGI(TAG, "SD card connected");
            xEventGroupSetBits(task_sync_event_group, CURRENT_MODE_ACQUISITION);
            for(int i=0;i<2;i++)
            {
                TCA9534_set_level(&(device_peripherals.IAQ_TCA9534), TCA9534_WARN_LED, TCA9534_HIGH);
                vTaskDelay(100 / portTICK_RATE_MS);
                TCA9534_set_level(&(device_peripherals.IAQ_TCA9534), TCA9534_WARN_LED, TCA9534_LOW);
                vTaskDelay(100 / portTICK_RATE_MS);
            }
            xTaskCreate(acquisitionTask, "acquisitionTask", 16384, NULL, 2, &xacquisitionTaskHandle);
        }
    }
    else
    {
        ESP_LOGI(TAG, "Startup mode found set to 'webserver setup mode' in NVS");
        xEventGroupClearBits(task_sync_event_group, CURRENT_MODE_ACQUISITION);
        /*Event group for task synchronisation*/
        /*This should Same priority as the bluetooth task, but we will set the antenna access prioritisation in ESP-IDF, of the two*/
        xTaskCreate(webServerTask, "webServerTask", 4096, NULL, 10, &xWebServerTaskHandle);
    }
    while (1)
    {
        /*Just a delay to yield control of the CPU, to feed the watchdog via the idle task, though this task priority is very low*/
        vTaskDelay(100 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}
