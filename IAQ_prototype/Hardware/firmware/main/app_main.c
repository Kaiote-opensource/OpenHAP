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
#include "parson.h"
#include <libesphttpd/esp.h>
#include "libesphttpd/httpd.h"
#include "libesphttpd/httpdespfs.h"
#include "libesphttpd/cgiwifi.h"
#include "libesphttpd/cgiflash.h"
#include "libesphttpd/auth.h"
#include "libesphttpd/espfs.h"
#include "libesphttpd/captdns.h"
#include "libesphttpd/webpages-espfs.h"
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
// #include "ZH03.h"
#include "TCA9534A.h"
#include <MLX90640_API.h>
#include "MLX90640_I2C_Driver.h"

#include "esp_system.h"
#include "sdkconfig.h"
//#include "MLX90640_API.h"
//#include "MLX90640_I2C_Driver.h"

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

#define ZH03_BAUDRATE       9600
#define ZH03_UART_NUM       UART_NUM_1
#define ZH03_TXD_PIN        GPIO_NUM_23
#define ZH03_RXD_PIN        GPIO_NUM_22
#define ZH03_RX_BUF_SIZE    1024
#define ZH03_TX_BUF_SIZE    0

static const char *TAG = "APP-MAIN";

static char *infared_ws_message = "Hi, Infared websocket!";
static char *status_ws_message = "Hi, Status websocket!";

static const char infared_cgi_resource_string[] = "/infared.cgi";
static const char status_cgi_resource_string[] = "/status.cgi";

static char *null_test_string = "\0";

uint32_t unix_time = 1554909924;

typedef struct
{
    HIH6030 IAQ_HIH6030;
    // ZH03 IAQ_ZH03;
    TCA9534 IAQ_TCA9534;
    MLX90640 IAQ_MLX90640;
    paramsMLX90640 MLX90640params;
} peripherals_struct;

peripherals_struct device_peripherals;

/*Main task handleS*/
static TaskHandle_t xWebServerTaskHandle = NULL;
static TaskHandle_t xDataloggerTaskHandle = NULL;
/*Webserver subtask handles - 1 per page*/

static TaskHandle_t xuartSensorTaskHandle = NULL;
static TaskHandle_t xStatusBroadcastHandle = NULL;
static TaskHandle_t xInfaredBroadcastHandle = NULL;
static TaskHandle_t xTCA9534HandlerTaskHandle = NULL;

static char* connectionMemory = NULL;
static HttpdFreertosInstance httpdFreertosInstance;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const static int CONNECTED_BIT = BIT0;

static EventGroupHandle_t wifi_ap_event_group;
static xQueueHandle gpio_evt_queue = NULL;

/* declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_eddystone_show_inform(const esp_eddystone_result_t *res, const esp_ble_gap_cb_param_t *scan_result);

void uartSensorTask(void *pvParameters);
void statusPageWsBroadcastTask(void *pvParameters);
void infaredPageWsBroadcastTask(void *pvParameters);

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x50,
    .scan_window = 0x30,
    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE};

static void IRAM_ATTR TCA9534_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void TCA9534HandlerTask(void *pvParameters)
{
    int32_t wp_level = 0;
    int32_t cd_level = 0;
    uint32_t io_num;

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
    }
}

static void esp_eddystone_show_inform(const esp_eddystone_result_t *res, const esp_ble_gap_cb_param_t *scan_result)
{
    switch (res->common.frame_type)
    {
    case EDDYSTONE_FRAME_TYPE_TLM:
    {
        ESP_LOGI(TAG, "--------Eddystone TLM Found----------");
        esp_log_buffer_hex("EDDYSTONE_DEMO: Device address:", scan_result->scan_rst.bda, ESP_BD_ADDR_LEN);
        ESP_LOGI(TAG, "RSSI of packet:%d dbm", scan_result->scan_rst.rssi);
        ESP_LOGI(TAG, "Eddystone TLM inform:");
        ESP_LOGI(TAG, "version: %d", res->inform.tlm.version);
        ESP_LOGI(TAG, "battery voltage: %d mv", res->inform.tlm.battery_voltage);
        ESP_LOGI(TAG, "beacon temperature in degrees Celsius: %6.1f", res->inform.tlm.temperature);
        ESP_LOGI(TAG, "adv pdu count since power-up: %d", res->inform.tlm.adv_count);
        ESP_LOGI(TAG, "time since power-up: %d s", res->inform.tlm.time);
        break;
    }
    default:
        break;
    }
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
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
    esp_err_t status;

    ESP_LOGI(TAG, "Register callback");
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
    esp_err_t ret;
    ret = esp_bluedroid_init();
    ret = esp_bluedroid_enable();
    ret = esp_eddystone_app_register();
    return ret;
}

static esp_err_t powerOnSelfTest(void)
{
    /*Test peripherals that can't be auto detected in software such as LEDs and buzzer*/
    /*Warn LED*/
    return ESP_OK;
}

static void receiveCommand(char *cmd)
{
    /*Work more on this*/
    ESP_LOGI(TAG, "%s", cmd);
}

//Queue all commands from all websockets and have a single decoding handler to avoid race conditions
static void myWebsocketRecv(Websock *ws, char *data, int len, int flags)
{
    receiveCommand(data);
}

/*Websocket disconnected. Stop task if none connected*/
static void myStatusWebsocketClose(Websock *ws)
{
    int connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance, status_cgi_resource_string, null_test_string, strlen(null_test_string), WEBSOCK_FLAG_NONE);
    ESP_LOGI(TAG, "Status page connection close received, Previous connections were %d", connections);

    if (connections == 1)
    {
        vTaskDelete(xStatusBroadcastHandle);
//        vTaskDelete(xuartSensorTaskHandle);
        vTaskDelete(xTCA9534HandlerTaskHandle);
        //Remember to flush hardware buffer via a deinit method for the peripheral(Device insensitive)
    }
}

/*Websocket disconnected. Stop task if none connected*/
static void myInfaredWebsocketClose(Websock *ws)
{
    int connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance, infared_cgi_resource_string, null_test_string, strlen(null_test_string), WEBSOCK_FLAG_NONE);
    ESP_LOGI(TAG, "Infared page connection close received, Previous connections were %d", connections);

    if (connections == 1)
    {
        vTaskDelete(xInfaredBroadcastHandle);
        //Remember to flush hardware buffer via a deinit method for the peripheral(Device insensitive)
    }
}

/*Websocket connected. Start task if none prior connected, set websocket callback methods and send welcome message.*/
static void myStatusWebsocketConnect(Websock *ws)
{
    int connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance, status_cgi_resource_string, null_test_string, strlen(null_test_string), WEBSOCK_FLAG_NONE);
    ESP_LOGI(TAG, "Status page connection start received, Previous connections were %d", connections);

    if (connections == 0)
    {
        // device_peripherals.IAQ_ZH03.PM_2_5 = 0;
        device_peripherals.IAQ_TCA9534.pinModeConf = 0;
        device_peripherals.IAQ_TCA9534.port = 0;
        device_peripherals.IAQ_TCA9534.polarity = 0;
        device_peripherals.IAQ_TCA9534.polarity = 0;
        device_peripherals.IAQ_HIH6030.temperature = 0;
        device_peripherals.IAQ_HIH6030.humidity = 0;
        device_peripherals.IAQ_HIH6030.status = 0;

        if (xTaskCreate(statusPageWsBroadcastTask, "statusPageTask", 4096, NULL, 3, &xStatusBroadcastHandle) == pdPASS)
        {
            ESP_LOGI(TAG, "Created status broadcast task");
            ws->recvCb = myWebsocketRecv;
            ws->closeCb = myStatusWebsocketClose;
            cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, status_ws_message, strlen(status_ws_message), WEBSOCK_FLAG_NONE);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to create status broadcast task");
            return;
        }
    }
    ws->recvCb = myWebsocketRecv;
    ws->closeCb = myStatusWebsocketClose;
    cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, status_ws_message, strlen(status_ws_message), WEBSOCK_FLAG_NONE);
}

/*Websocket connected. Start task if none prior connected, set websocket callback methods and send welcome message.*/
static void myInfaredWebsocketConnect(Websock *ws)
{
    int connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance, infared_cgi_resource_string, null_test_string, strlen(null_test_string), WEBSOCK_FLAG_NONE);
    ESP_LOGI(TAG, "Infared page connection start received, Previous connections were %d", connections);

    if (connections == 0)
    {
        if (xTaskCreatePinnedToCore(infaredPageWsBroadcastTask, "infaredPageTask", 24000, NULL, 2, &xInfaredBroadcastHandle, 0) == pdPASS)
        {
            ESP_LOGI(TAG, "Created infared broadcast task");
            ws->recvCb = myWebsocketRecv;
            ws->closeCb = myInfaredWebsocketClose;
            cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, infared_ws_message, strlen(infared_ws_message), WEBSOCK_FLAG_NONE);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to create infared broadcast task");
            return;
        }
    }
    ws->recvCb = myWebsocketRecv;
    ws->closeCb = myInfaredWebsocketClose;
    cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, infared_ws_message, strlen(infared_ws_message), WEBSOCK_FLAG_NONE);
}

HttpdBuiltInUrl builtInUrls[] = 
{
    ROUTE_CGI_ARG("*", cgiRedirectApClientToHostname, "esp.nonet"),
    ROUTE_REDIRECT("/", "/index.html"),
    ROUTE_WS(status_cgi_resource_string, myStatusWebsocketConnect),
    ROUTE_WS(infared_cgi_resource_string, myInfaredWebsocketConnect),
    //ROUTE_WS("/bluetooth.cgi", myBluetoothWebsocketConnect),

    ROUTE_FILESYSTEM(),

    ROUTE_END()
};
esp_err_t get_thermal_image(peripherals_struct *device_peripherals, float* image_buffer)
{
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
    int32_t cd_level;
    TCA9534_get_level(&(device_peripherals->IAQ_TCA9534), TCA9534_SD_CD, &cd_level);
    get_temp_humidity(&(device_peripherals->IAQ_HIH6030));
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_number(root_object, "HUM", (int)(device_peripherals->IAQ_HIH6030.humidity));
    json_object_set_number(root_object, "TEMP", (int)(device_peripherals->IAQ_HIH6030.temperature));
    // if ((device_peripherals->IAQ_ZH03.PM_2_5) != ESP_FAIL && (device_peripherals->IAQ_ZH03.PM_2_5) != ESP_ERR_TIMEOUT && (device_peripherals->IAQ_ZH03.PM_2_5) != 0)
    // {
    //     json_object_set_number(root_object, "PM25", device_peripherals->IAQ_ZH03.PM_2_5);
    // }
    //json_object_set_number(root_object, "TIME", rand()%100);
    json_object_set_number(root_object, "RTC_BATT", rand() % 100);
    //json_object_set_number(root_object, "MAIN_BATT", rand()%100);
    json_object_set_number(root_object, "DEVICE_TIME", unix_time);
    json_object_set_string(root_object, "SD_CARD", cd_level == TCA9534_HIGH ? "DISCONNECTED" : "CONNECTED");
    status_string = json_serialize_to_string(root_value);
    ESP_LOGI(TAG, "%s", status_string);
    json_value_free(root_value);

    ESP_LOGI(TAG, "status string length is %d bytes", strlen(status_string));

    return status_string;
}

char *b64_encode_thermal_img(char *base64_dst, float *data)
{
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
// void uartSensorTask(void *pvParameters)
// {
//     uart_flush(UART_NUM_1);
//     while (1)
//     {
//         get_particulate_reading(&(device_peripherals.IAQ_ZH03));
//     }
//     vTaskDelete(NULL);
// }

void statusPageWsBroadcastTask(void *pvParameters)
{
    int connections;
    char *status_string = NULL;
    if (xTaskCreate(TCA9534HandlerTask, "TCA9534Interrupt", 2048, NULL, 1, &xTCA9534HandlerTaskHandle) == pdPASS //&&
        /*xTaskCreatePinnedToCore(uartSensorTask, "uartSensorTask", 4096, NULL, 2, &xuartSensorTaskHandle, 0) == pdPASS*/)
    {

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
                ESP_LOGD(TAG, "Status page JSON assembler returned NULL");
            }
            unix_time++;
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
        vTaskDelete(NULL);
    }
}

void infaredPageWsBroadcastTask(void *pvParameters)
{
    /*Allocate this image buffer to the psram instead of the stack in internal RAM*/
    float image_buffer[768] = {0};
    esp_err_t ret;
    int connections = 0;
    char *base64_string = NULL;
    char *json_string = NULL;
    char *data_txt_chunk = NULL;
    char *json_identifier = NULL;
    int size_to_do;
    int chunk_size;
    int size;
    int offset;
    while (1)
    {
        ret = get_thermal_image(&device_peripherals, image_buffer);
        if(ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to get thermal image, returned error code %d", ret);
            //Do something
        }
        base64_string = b64_encode_thermal_img(base64_string, image_buffer/*camera_data_float*/);

        if (base64_string != NULL)
        {
            //ESP_LOG_BUFFER_HEXDUMP("Base64 data:", base64_string, strlen(base64_string)+1, ESP_LOG_DEBUG);
            size = strlen(base64_string);
            size_to_do = size;
            for (int i = 0; size_to_do > 0; i++)
            {
                JSON_Value *root_value = json_value_init_object();
                JSON_Object *root_object = json_value_get_object(root_value);
                chunk_size = (size_to_do < HTTPD_MAX_SENDBUFF_LEN) ? size_to_do : 1024;
                offset = i * 1024;
                json_identifier = (size_to_do <= 1024) ? "CAMERA_END" : "CAMERA_CONT";
                data_txt_chunk = (char *)malloc(HTTPD_MAX_SENDBUFF_LEN);
                memset(data_txt_chunk, 0, chunk_size * sizeof(char));
                strlcpy(data_txt_chunk, base64_string + offset, chunk_size + 1);
                ESP_LOGI(TAG, "Allocated string chunk length is %d", strlen(data_txt_chunk));
                //ESP_LOG_BUFFER_HEXDUMP("Raw data chunk:", data_txt_chunk, strlen(data_txt_chunk)+1, ESP_LOG_DEBUG);
                if (json_object_set_string(root_object, json_identifier, data_txt_chunk) != JSONSuccess)
                {
                    ESP_LOGI(TAG, "Setting json string failed");
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
            free(base64_string);
        }
        else
        {
            ESP_LOGD(TAG, "Infared page JSON assembler returned NULL");
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
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
        ESP_LOGI(TAG, "station:" MACSTR " join,AID=%d\n",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        xEventGroupSetBits(wifi_ap_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station " MACSTR "left the Wifi chat,AID=%d\n",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        xEventGroupClearBits(wifi_ap_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_SCAN_DONE:
        break;
    default:
        break;
    }

    /* Forward event to to the WiFi CGI module */
    cgiWifiEventCb(event);

    return ESP_OK;
}

void ICACHE_FLASH_ATTR init_wifi(void)
{
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

void webServerTask(void *pvParameters)
{
    /*Allocate this connection memory to psram*/
    connectionMemory = (char*)malloc(sizeof(RtosConnType) * MAX_CONNECTIONS);

    espFsInit((void *)(webpages_espfs_start));

    tcpip_adapter_init();

    httpdFreertosInit(&httpdFreertosInstance, builtInUrls, WEB_PORT, connectionMemory, MAX_CONNECTIONS, HTTPD_FLAG_NONE);

    httpdFreertosStart(&httpdFreertosInstance);

    /*Allow a 2 second delay to allow stabilisation, avoiding brown out*/
    vTaskDelay(2000 / portTICK_RATE_MS);

    init_wifi();

    while (1)
    {
        /*Jusr a delay to yield control of the CPU, to feed the watchdog via the idle task, though this task priority is very low*/
        vTaskDelay(100 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

esp_err_t setupTempHumiditySensor(HIH6030 *HIH6030_inst, SemaphoreHandle_t *bus_mutex)
{
    *bus_mutex = xSemaphoreCreateMutex();

    esp_err_t ret = HIH6030_init(HIH6030_inst, HIH6030_ADDR, I2C_MASTER_NUM, bus_mutex);
    return ret;
}

esp_err_t setupGpioExpander(TCA9534 *TCA9534_inst, SemaphoreHandle_t *bus_mutex)
{
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
        ESP_LOGE(TAG, "Error setting WP pin direction, returned %d", (int)ret);
    }
    ret = TCA9534_set_pin_direction(TCA9534_inst, TCA9534_SD_CD, TCA9534_INPUT);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error setting WP pin direction, returned %d", (int)ret);
    }
    ret = TCA9534_set_pin_direction(TCA9534_inst, TCA9534_WARN_LED, TCA9534_OUTPUT);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error setting WP pin direction, returned %d", (int)ret);
    }
    return ret;
}

esp_err_t setupMLX90640(peripherals_struct *device_peripherals, SemaphoreHandle_t *bus_mutex)
{
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
    if not we set RAM value to 0x1901*/
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

// esp_err_t setup_uart(uart_port_t uart_num, int baudrate, tx_io_num txd_pin, rx_io_num rxd_pin, int rx_buffer_size, int tx_buffer_size)
// {
//     esp_err_t ret;
//     uart_config_t uart_config =
//     {
//         .baud_rate = baudrate,
//         .data_bits = UART_DATA_8_BITS,
//         .parity = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
//     };
//     ret = uart_param_config(uart_num, &uart_config);
//     if (ret != ESP_OK)
//     {
//         return ret;
//     }
//     ret = uart_set_pin(uart_num, txd_pin, rxd_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
//     if (ret != ESP_OK)
//     {
//         return ret;
//     }
//     ret = uart_driver_install(uart_num, rx_buffer_size * 2, 0, 0, NULL, 0);
//     return ret;
// }
/*

void acquisitionTask(void *pvParameters)
{

}
*/

void app_main()
{
    esp_err_t ret;
    SemaphoreHandle_t i2c_bus_mutex = NULL;

    ret = setup_i2c(I2C_MASTER_NUM, I2C_MASTER_FREQ_HZ, I2C_MASTER_SDA_IO, GPIO_PULLUP_DISABLE, I2C_MASTER_SCL_IO, GPIO_PULLUP_DISABLE);
    if (ret == ESP_OK)
    {
        /*Do something*/
    }
    ret = setupGpioExpander(&(device_peripherals.IAQ_TCA9534), &i2c_bus_mutex);
    if (ret != ESP_OK)
    {
        /*Do something*/
    }
    ret = setupTempHumiditySensor(&(device_peripherals.IAQ_HIH6030), &i2c_bus_mutex);
    if (ret != ESP_OK)
    {
        /*Do something*/
    }
    ret = setupMLX90640(&device_peripherals, &i2c_bus_mutex);
    if (ret != ESP_OK)
    {
        /*Do something*/
    }
    // ret = setup_uart(ZH03_UART_NUM, ZH03_BAUDRATE, ZH03_TXD_PIN, ZH03_RXD_PIN, ZH03_RX_BUF_SIZE, ZH03_TX_BUF_SIZE);
    // if (ret != ESP_OK)
    // {
    //     /*Do something*/
    // }
    // ZH03B_init();

    /*Test communication by i2c scanning*/
    ret = powerOnSelfTest();
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

    //ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    //esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    //esp_bt_controller_init(&bt_cfg);
    //esp_bt_controller_enable(ESP_BT_MODE_BLE);
    //esp_bt_controller_init(&bt_cfg);

    //esp_eddystone_init();

    /*<! set scan parameters */
    //esp_ble_gap_set_scan_params(&ble_scan_params);
    /*Set up supervisor task to perform task setup and synchronisation*/

    /*This should Same priority as the bluetooth task, but we will set the antenna access prioritisation in ESP-IDF, of the two*/
    xTaskCreatePinnedToCore(webServerTask, "webServerTask", 4096, NULL, 10, &xWebServerTaskHandle, 0);

    while (1)
    {
        /*Jusr a delay to yield control of the CPU, to feed the watchdog via the idle task, though this task priority is very low*/
        vTaskDelay(100 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}
