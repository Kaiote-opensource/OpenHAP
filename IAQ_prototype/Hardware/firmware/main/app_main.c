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
#include "ZH03.h"
#include "TCA9534A.h"

#include "esp_system.h"
#include "sdkconfig.h"
//#include "MLX90640_API.h"
//#include "MLX90640_I2C_Driver.h"

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_defs.h"
#include "esp_gattc_api.h"
#include "esp_gap_ble_api.h"

#include "esp_eddystone_protocol.h"
#include "esp_eddystone_api.h"

#define WEB_PORT                   80U
#define MAX_CONNECTIONS            3U               /*Maximum tabs open*/

#define I2C_MASTER_SCL_IO          19               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO          21               /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM             I2C_NUM_0        /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ         100000           /*!< I2C master clock frequency */

static const char* TAG = "APP-MAIN";

uint32_t unix_time = 1554909924;

typedef struct
{
    HIH6030 IAQ_HIH6030;
    ZH03 IAQ_ZH03;
    TCA9534 IAQ_TCA9534;
}peripherals_struct;

/*Main task handleS*/
static TaskHandle_t xWebServerTaskHandle = NULL;
static TaskHandle_t xDataloggerTaskHandle = NULL;
/*Webserver subtask handles - 1 per page*/

static TaskHandle_t xuartSensorTaskHandle = NULL;
static TaskHandle_t xStatusBroadcastHandle = NULL;
static TaskHandle_t xInfaredBroadcastHandle = NULL;

static char connectionMemory[sizeof(RtosConnType) * MAX_CONNECTIONS];
static HttpdFreertosInstance httpdFreertosInstance;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const static int CONNECTED_BIT = BIT0;

static EventGroupHandle_t wifi_ap_event_group;
static xQueueHandle gpio_evt_queue = NULL;

/*Task synchronisation event group*/
//EventGroupHandle_t xEventGroup = NULL;

float camera_data_float[768]= {0.825253,0.455252,0.720179,30.544859,30.427420,30.370266,29.946110,29.928658,29.878696,30.046879,30.317205,31.169159,33.990673,35.194588,35.997345,36.098236,36.031712,35.897179,36.039021,35.784195,35.328114,35.644867,35.863697,35.174343,31.032515,31.371862,30.977913,30.588465,30.133257,
30.008118,30.236732,
30.231815,30.976789,30.689404,30.787285,30.362507,30.149279,30.146961,30.035376,30.241760,30.489824,30.103189,30.620626,31.206104,35.185364,35.667862,36.217941,35.894447,35.881859,35.292969,35.147324,35.744308,35.361080,35.174133,35.637871,35.915424,32.157627,31.172319,30.133978,30.620142,30.077423,
29.957914,30.049553,30.963652,30.955786,30.545006,32.262329,30.053732,30.133377,30.106586,31.323080,29.745535,30.029339,30.132275,31.237171,33.113396,35.322514,35.405560,35.708084,35.426910,33.949474,34.209705,34.342945,34.208508,34.998226,35.118893,36.396896,35.762486,33.242210,32.095470,30.882133,
30.798094,30.593410,30.201653,30.372244,30.420557,30.871033,30.348743,30.257111,30.570904,30.083139,30.148439,29.986992,29.818211,29.564634,29.975609,31.276020,33.663185,35.581955,35.840919,35.192551,34.320091,32.080173,32.254898,32.203354,32.421917,33.327965,33.865158,34.779900,34.088661,33.353344,
32.041279,31.380322,30.768456,30.482023,31.081444,29.956898,30.223719,30.339663,30.205742,30.265926,30.145273,30.305040,30.179245,30.155495,29.484919,29.640444,30.064463,32.542854,34.320049,35.053146,35.165974,33.747772,33.833889,31.328650,31.578316,31.994202,32.109375,33.064148,32.973824,33.972363,
33.738358,32.720371,32.628704,31.945860,30.206486,30.670797,30.118706,29.981321,28.589109,30.237324,30.031094,30.333559,30.369200,29.809341,29.831825,29.624437,29.970108,29.929329,30.226299,32.220379,34.503220,34.974258,33.980598,34.773674,34.643135,32.091923,31.376667,32.039097,32.077644,32.826157,
34.001637,34.346165,34.727680,33.437244,32.627235,32.021400,31.101357,29.732843,29.929398,30.093491,29.902218,30.573723,29.981726,30.058355,29.946224,29.900709,29.617571,29.825027,29.898470,29.462370,30.350552,32.004219,34.320049,34.948685,34.968773,35.805622,35.928032,32.918358,32.612209,32.546959,
32.588249,34.324471,34.577042,35.043011,34.965393,33.684315,32.996124,31.555805,31.451670,29.801346,29.898396,29.734728,29.651402,30.844032,30.053579,29.465715,29.797607,30.025743,29.669643,29.299297,30.257536,29.700901,29.542891,31.691963,34.166092,35.519711,35.083809,34.974312,35.362972,34.507477,
33.471874,33.103764,34.492317,34.527534,34.690823,34.752567,35.613087,34.676266,34.103535,31.210436,30.876890,29.903629,30.533581,29.814299,30.476486,30.317810,30.021536,29.863836,29.769650,29.703615,29.780603,30.019096,29.404810,30.001093,30.188116,33.224419,33.800163,35.317280,35.078205,36.482384,
35.234932,34.978931,34.867256,35.064987,35.281227,35.230232,35.259289,35.083416,34.635693,35.542957,35.614059,31.853098,31.364258,29.786497,29.921522,30.255171,29.849384,30.452324,30.042599,29.736336,30.028849,29.816181,29.547615,29.537355,30.002766,29.929615,30.043802,32.593044,33.990944,35.553532,
35.364029,34.561729,36.252148,35.041500,34.712952,34.724964,35.289490,35.935913,35.569515,35.272633,35.817547,35.445282,35.569942,32.016716,30.955177,30.603487,30.104607,30.109179,29.824963,29.915388,29.816927,29.469576,30.034851,30.104675,29.862240,29.856619,29.539978,29.523264,29.876308,31.381310,
32.963490,34.966473,35.476376,35.671581,35.654423,35.130768,34.991806,35.327049,35.032814,35.497482,35.715199,35.276897,35.672832,35.499878,35.575603,31.823814,31.048864,30.119349,30.561926,30.875475,29.927982,30.058186,29.882174,29.607641,30.251472,30.052780,29.553158,29.557384,29.457729,29.795660,
29.478170,29.928938,32.299774,34.415218,35.018600,35.351013,35.636173,35.374035,35.161896,35.246452,35.848503,36.202412,35.723236,34.755775,35.102516,35.231533,35.530571,32.096180,31.031857,30.212738,29.815725,29.897425,30.116806,30.212532,30.792463,29.779476,29.893263,29.760498,29.900221,30.248545,
29.799126,29.528423,29.963467,30.036573,29.926020,31.808561,33.425022,36.108452,35.534931,35.283348,35.616695,35.959675,35.981495,35.971260,35.681622,35.081394,35.192375,34.709564,35.591793,31.162937,30.651421,30.126072,29.982489,30.289963,29.935642,29.484997,29.946280,29.849260,30.358604,30.562708,
29.728052,29.588160,29.995047,29.802187,29.447811,29.581106,29.747078,30.400469,32.341225,35.020535,36.541882,35.483341,35.788380,35.678303,36.010071,36.626423,35.384724,35.108875,35.001595,34.889576,34.876366,31.162891,30.516249,30.219873,29.814800,30.749279,29.990110,29.898378,30.490925,29.957872,
30.042242,29.994278,29.553961,30.236063,29.563148,29.877455,29.551201,29.713213,29.791931,30.150269,31.319578,35.301083,35.054108,35.941132,35.674313,36.400726,36.220665,35.983143,36.248146,36.214397,36.363438,35.631176,35.090679,30.752977,30.461384,29.742481,30.137491,30.102514,28.290979,29.591667,
29.672857,29.823454,29.661844,29.545959,29.941406,29.219667,29.816172,29.649904,29.423286,29.608559,29.611916,29.740639,31.032679,34.412640,35.208286,35.884430,36.081642,36.125725,36.216583,36.310204,35.914783,36.279846,36.510159,35.585140,33.878525,30.536989,30.453133,29.845497,29.812941,30.151926,
29.966724,30.328518,30.212074,30.092100,29.768299,29.948242,29.652149,29.457760,29.744308,29.456697,29.649361,29.736805,28.499578,30.140648,30.503004,34.715698,34.584217,35.656910,35.659832,35.670086,35.771759,35.526474,36.120697,36.169987,36.210056,35.018471,34.267387,32.360340,31.394144,30.283535,
30.111923,29.988897,30.383509,30.244726,30.009436,28.952450,30.052336,29.697035,29.860157,29.413933,29.759481,29.932318,29.530575,29.787077,29.600149,29.877132,30.049713,33.285236,34.903492,35.601048,35.680439,35.558037,35.336216,35.131886,34.968597,34.490410,35.369781,34.227779,34.229904,32.629860,
32.496052,30.382954,30.340776,30.338923,30.150995,30.307499,30.410322,30.228249,30.116241,29.638229,30.019041,29.840868,29.939632,29.614403,29.879114,29.857929,29.913263,31.942949,33.113129,35.090935,35.276627,35.651634,35.244732,35.570637,35.514740,34.449360,35.311104,34.834583,34.785351,35.048225,
35.298717,34.094040,33.261524,32.994980,32.861629,33.268528,33.819786,30.240597,29.990622,29.870834,30.424391,29.751329,30.152893,29.897278,30.435659,29.564898,29.537609,29.743263,31.286535,33.060127,34.018021,35.307545,35.928196,35.490273,35.445309,35.626991,35.519154,35.052029,35.177189,35.399738,
35.822407,35.320961,35.689850,33.769344,34.018810,33.894356,35.570854,34.102001,34.233429,30.222151,30.132812,30.430403,30.015600,29.737064,29.904163,29.941334,30.009123,29.907103,29.808104,31.739523,32.106396,32.762123,33.216309,34.993397,35.553715,35.248356,35.708889,35.510681,35.593075,35.511066,
36.413120,35.933678,36.416039,36.000542,35.376503,33.190861,33.493649,34.241016,34.172115,34.584034,34.319748,30.860155,30.170086,30.058989,30.125769,30.088377,29.929113,29.814045,30.206425,29.118309,30.547298,32.336330,33.400879,32.468079,32.649548,33.709805,33.894958,34.250042,34.229904,33.882778,
34.423611,34.403744,34.887779,36.002701,35.902077,35.960785,35.264378,33.250889,33.691711,33.759762,33.965618,33.751312,34.094242,30.881582,30.921249,30.969425,29.857355,30.671698,31.749584,33.012306,33.056740,33.741997,33.754776,34.046505,32.778145,33.817608,34.238117,34.701591,34.220093,34.362137,
34.215816,34.393341,34.302555,34.043175,34.107567,33.923306,34.018215,35.085987,34.207722,33.374298,33.747631,33.205704,34.102360,34.654442,35.118324,31.081171,31.064100,30.640053,30.944107,32.583111,34.110058,34.158962,33.938618,34.074863,33.820190,34.209820,34.226067,33.996681,33.592789,34.336464,
34.351791,34.029461,34.649178,34.331161,34.184895,33.877117,34.889751,33.661694,33.707085,33.702087,33.866566,33.549461,33.617970,33.610279,33.578182,34.359665,34.729172};

/* declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param);
static void esp_eddystone_show_inform(const esp_eddystone_result_t* res, const esp_ble_gap_cb_param_t* scan_result);

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

static void IRAM_ATTR TCA9534_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void TCA9534HandlerTask(void* pvParameters)
{
    int32_t wp_level = 0;
    int32_t cd_level = 0;
    uint32_t io_num;

    peripherals_struct* device_peripherals = (peripherals_struct*) pvParameters;

    TCA9534_get_level(&(device_peripherals->IAQ_TCA9534), TCA9534_SD_CD, &cd_level);
    TCA9534_get_level(&(device_peripherals->IAQ_TCA9534), TCA9534_SD_WP, &wp_level);

    TCA9534_set_level(&(device_peripherals->IAQ_TCA9534), TCA9534_WARN_LED, !cd_level);

    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) 
        {
            if(io_num == GPIO_TCA9534_INT)
            {
                /*Delay added to ignore any bouncing on the mechanical parts generating the interrupt*/
                vTaskDelay(100 / portTICK_PERIOD_MS);

                printf("\nTCA9534 interrupt received\n");
                if (TCA9534_get_level(&(device_peripherals->IAQ_TCA9534), TCA9534_SD_CD, &cd_level) != ESP_OK)
                {
                    printf("\nError getting card detect level");
                }
                else
                {
                    printf("\nCD level is set to %d\n", (int)cd_level);
                    TCA9534_set_level(&(device_peripherals->IAQ_TCA9534), TCA9534_WARN_LED, !cd_level);
                }

                if (TCA9534_get_level(&(device_peripherals->IAQ_TCA9534), TCA9534_SD_WP, &wp_level) != ESP_OK)
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

static void esp_eddystone_show_inform(const esp_eddystone_result_t* res, const esp_ble_gap_cb_param_t* scan_result)
{
    switch(res->common.frame_type)
    {
        case EDDYSTONE_FRAME_TYPE_TLM: {
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

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param)
{
    esp_err_t err;

    switch(event)
    {
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
            uint32_t duration = 0;
            esp_ble_gap_start_scanning(duration);
            break;
        }
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
            if((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG,"Scan start failed: %s", esp_err_to_name(err));
            }
            else {
                ESP_LOGI(TAG,"Start scanning...");
            }
            break;
        }
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t* scan_result = (esp_ble_gap_cb_param_t*)param;
            switch(scan_result->scan_rst.search_evt)
            {
                case ESP_GAP_SEARCH_INQ_RES_EVT: {
                    esp_eddystone_result_t eddystone_res;
                    memset(&eddystone_res, 0, sizeof(eddystone_res));
                    esp_err_t ret = esp_eddystone_decode(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len, &eddystone_res);
                    if (ret) {
                        // error:The received data is not an eddystone frame packet or a correct eddystone frame packet.
                        // just return
                        return;
                    } else {   
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
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:{
            if((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG,"Scan stop failed: %s", esp_err_to_name(err));
            }
            else {
                ESP_LOGI(TAG,"Stop scan successfully");
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
    
    ESP_LOGI(TAG,"Register callback");
    /*<! register the scan callback function to the gap module */
    if((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK) {
        ESP_LOGE(TAG,"gap register error: %s", esp_err_to_name(status));
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


static void receiveCommand(char* cmd)
{
    /*Work more on this*/
    ESP_LOGI(TAG,"%s", cmd);
}
 
//Queue all commands from all websockets and have a single decoding handler to avoid race conditions
static void myWebsocketRecv(Websock *ws, char *data, int len, int flags) 
{
    receiveCommand(data);
}

//Websocket connected. Install reception handler and send welcome message.
static void myStatusWebsocketConnect(Websock *ws) 
{
	ws->recvCb=myWebsocketRecv;
	cgiWebsocketSend(&httpdFreertosInstance.httpdInstance,
	                 ws, "Hi, Status websocket!", 14, WEBSOCK_FLAG_NONE);
}

//Websocket connected. Install reception handler and send welcome message.
static void myInfaredWebsocketConnect(Websock *ws) 
{
	ws->recvCb=myWebsocketRecv;
	cgiWebsocketSend(&httpdFreertosInstance.httpdInstance,
	                 ws, "Hi, Infared websocket!", 14, WEBSOCK_FLAG_NONE);
}

HttpdBuiltInUrl builtInUrls[]={
	ROUTE_CGI_ARG("*", cgiRedirectApClientToHostname, "esp.nonet"),
	ROUTE_REDIRECT("/", "/index.html"),
	ROUTE_WS("/status.cgi", myStatusWebsocketConnect),
	ROUTE_WS("/infared.cgi", myInfaredWebsocketConnect),
	//ROUTE_WS("/bluetooth.cgi", myBluetoothWebsocketConnect),

	ROUTE_FILESYSTEM(),

	ROUTE_END()
};

char* simulateStatusValues(char *status_string, peripherals_struct* device_peripherals)
{
    int32_t cd_level;
    TCA9534_get_level(&(device_peripherals->IAQ_TCA9534), TCA9534_SD_CD, &cd_level);
    get_temp_humidity(&(device_peripherals->IAQ_HIH6030));
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_number(root_object, "HUM",  (int)(device_peripherals->IAQ_HIH6030.humidity));
    json_object_set_number(root_object, "TEMP", (int)(device_peripherals->IAQ_HIH6030.temperature));
    if((device_peripherals->IAQ_ZH03.PM_2_5) != ESP_FAIL || (device_peripherals->IAQ_ZH03.PM_2_5) != ESP_ERR_TIMEOUT)
    {
        json_object_set_number(root_object, "PM25", device_peripherals->IAQ_ZH03.PM_2_5);
    }
    //json_object_set_number(root_object, "TIME", rand()%100);
    json_object_set_number(root_object, "RTC_BATT", rand()%100);
    //json_object_set_number(root_object, "MAIN_BATT", rand()%100);
    json_object_set_number(root_object, "DEVICE_TIME", unix_time);
    json_object_set_string(root_object, "SD_CARD", cd_level == TCA9534_HIGH?"CONNECTED":"DISCONNECTED");
    status_string = json_serialize_to_string(root_value);
    ESP_LOGI(TAG, "%s", status_string);
    json_value_free(root_value);

    ESP_LOGI(TAG,"status string length is %d bytes", strlen(status_string));

    return status_string;
}

char* simulateInfaredValues(char* base64_dst, float* data)
{   
    int ret;
    size_t base64_length=0;

    ret=mbedtls_base64_encode(NULL, 0, &base64_length, (unsigned char *)data, 768*sizeof(float));
    if(ret==MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL)
    {
        ESP_LOGI(TAG,"Buffer for allocation of infared is too small, length needed is %d bytes", base64_length);
        base64_dst=(char*)malloc((base64_length*sizeof(size_t))+1);
    }
    else if(ret==MBEDTLS_ERR_BASE64_INVALID_CHARACTER)
    {
        ESP_LOGI(TAG,"Buffer for allocation of infared is too small, length needed is %d bytes", base64_length);
        ESP_LOGI(TAG,"Invalid character found");
    }
    else
    {
        ESP_LOGI(TAG,"Function returned succesful");
    }
    base64_dst=(char*)malloc((base64_length*sizeof(size_t))+1);
    ret=mbedtls_base64_encode((unsigned char*)base64_dst, base64_length, &base64_length, (unsigned char *)data, 768*sizeof(float));
    if(ret==MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL)
    {
        ESP_LOGI(TAG,"Buffer for allocation of infared is too small, length needed is %d bytes", base64_length);
        return NULL;
    }
    else if(ret==MBEDTLS_ERR_BASE64_INVALID_CHARACTER)
    {
        ESP_LOGI(TAG,"Buffer for allocation of infared is too small, length needed is %d bytes", base64_length);
        ESP_LOGI(TAG,"Invalid character found");
        return NULL;
    }
    ESP_LOGI(TAG,"Function returned successful");

    return base64_dst;
}
void uartSensorTask(void *pvParameters)
{
    peripherals_struct* device_peripherals = (peripherals_struct*) pvParameters;

	while(1) 
    {
    get_particulate_reading(&(device_peripherals->IAQ_ZH03));
	}
    vTaskDelete(NULL);
}

void statusPageWsBroadcastTask(void *pvParameters)
{
    peripherals_struct* device_peripherals = (peripherals_struct*) pvParameters;

    int connections;
    char *status_string = NULL;

	xTaskCreate(uartSensorTask, "uartSensorTask", 4096, device_peripherals, 3, &xuartSensorTaskHandle);

	while(1) {
        
        status_string=simulateStatusValues(status_string, device_peripherals);
        if(status_string != NULL)
        {  
		    connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance,
		                                      "/status.cgi",status_string, strlen(status_string),
		                                      WEBSOCK_FLAG_NONE);
            json_free_serialized_string(status_string);
            ESP_LOGD(TAG,"Broadcast sent to %d connections", connections);
        }
        else
        {
            ESP_LOGD(TAG,"Status page JSON assembler returned NULL");
        }
        unix_time++;
		vTaskDelay(1000/portTICK_RATE_MS);
	}
    vTaskDelete(NULL);
}

void infaredPageWsBroadcastTask(void *pvParameters)
{
    int connections;
    char* base64_string = NULL;
    char* json_string = NULL;
    char* data_txt_chunk = NULL;
    char* json_identifier =NULL;
    int size_to_do;
    int chunk_size;
    int size;
    int offset;
	while(1) 
    {


        base64_string=simulateInfaredValues(base64_string, camera_data_float);

        if(base64_string != NULL)
        {
            //ESP_LOG_BUFFER_HEXDUMP("Base64 data:", base64_string, strlen(base64_string)+1, ESP_LOG_DEBUG);
            size=strlen(base64_string);
            size_to_do=size;
            for(int i=0; size_to_do>0; i++)
            {
                JSON_Value *root_value = json_value_init_object();
                JSON_Object *root_object = json_value_get_object(root_value);
                chunk_size = (size_to_do < HTTPD_MAX_SENDBUFF_LEN) ? size_to_do : 1024;
                offset = i * 1024;
                json_identifier = (size_to_do <= 1024) ? "CAMERA_END" : "CAMERA_CONT";
                data_txt_chunk=(char*)malloc(HTTPD_MAX_SENDBUFF_LEN);
                memset(data_txt_chunk,0, chunk_size*sizeof(char));
                strlcpy(data_txt_chunk, base64_string+offset, chunk_size+1);
                ESP_LOGI(TAG, "Allocated string chunk length is %d", strlen(data_txt_chunk));
                //ESP_LOG_BUFFER_HEXDUMP("Raw data chunk:", data_txt_chunk, strlen(data_txt_chunk)+1, ESP_LOG_DEBUG);                                                        
                if(json_object_set_string(root_object, json_identifier, data_txt_chunk) != JSONSuccess)
                {
                    ESP_LOGI(TAG, "Setting json string failed");
                    free(data_txt_chunk);
                    json_value_free(root_value); 
                    json_free_serialized_string(json_string);
                    break;
                }
                json_string = json_serialize_to_string(root_value);
                if(json_string == NULL)
                {
                    ESP_LOGI(TAG, "Allocation of json string failed");
                    free(data_txt_chunk);
                    json_value_free(root_value); 
                    json_free_serialized_string(json_string);
                    break;
                }
                //ESP_LOG_BUFFER_HEXDUMP("JSON string:", json_string, strlen(json_string)+1, ESP_LOG_DEBUG);
                connections = cgiWebsockBroadcast(&httpdFreertosInstance.httpdInstance,
                                              "/infared.cgi", json_string, strlen(json_string),
                                              WEBSOCK_FLAG_NONE);
                ESP_LOGD(TAG,"Sent chunk %d of raw data size %d of a maximum %d byte raw payload to %d connections", i, chunk_size, strlen(base64_string), connections);
                ESP_LOGD(TAG,"json data size for this chunk is %d", strlen(json_string));
                free(data_txt_chunk);
                json_value_free(root_value); 
                json_free_serialized_string(json_string);
                size_to_do -= chunk_size; //Subtract the quantity of data already transferred.
                if(size_to_do<1024)
                {
                    if ((offset+chunk_size) != size)
                    {
                        ESP_LOGE(TAG, "Error: Burst size mismatch");
                    }
                }
            }
            free(base64_string);
        }
        else
        {
            ESP_LOGD(TAG,"Infared page JSON assembler returned NULL");
        }
        vTaskDelay(1000/portTICK_RATE_MS);
	}
    vTaskDelete(NULL);
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_AP_START:
    {
        tcpip_adapter_ip_info_t ap_ip_info;
        if (tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ap_ip_info) == 0) {
            ESP_LOGI(TAG, "~~~~~~~~~~~");
            ESP_LOGI(TAG, "IP:" IPSTR, IP2STR(&ap_ip_info.ip));
            ESP_LOGI(TAG, "MASK:" IPSTR, IP2STR(&ap_ip_info.netmask));
            ESP_LOGI(TAG, "GW:" IPSTR, IP2STR(&ap_ip_info.gw));
            ESP_LOGI(TAG, "~~~~~~~~~~~");
        }
    }
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:" MACSTR" join,AID=%d\n",
                MAC2STR(event->event_info.sta_connected.mac),
                event->event_info.sta_connected.aid);
        xEventGroupSetBits(wifi_ap_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station " MACSTR"left the Wifi chat,AID=%d\n",
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

void ICACHE_FLASH_ATTR init_wifi(void) {
    
    uint8_t mac[6];
    char   * ssid;
    char   * password;
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    asprintf(&ssid, "%s-%02X%02X%02X", "Kaiote-AQ", mac[3], mac[4], mac[5]);
    asprintf(&password, "%02X%02X%02X%02X%02X%02X",mac[3], mac[4], mac[5],mac[3], mac[4], mac[5]);

	wifi_ap_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK( esp_event_loop_init(wifi_event_handler, NULL) );

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_AP) );
	wifi_config_t ap_config;
	strcpy((char*)(&ap_config.ap.ssid), ssid);
	ap_config.ap.ssid_len = strlen(ssid);
	ap_config.ap.channel = 1;
    strcpy((char*)(&ap_config.ap.password), password);
	ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
	ap_config.ap.ssid_hidden = 0;
	ap_config.ap.max_connection = 1;
	ap_config.ap.beacon_interval = 100;

	esp_wifi_set_config(WIFI_IF_AP, &ap_config);

	ESP_ERROR_CHECK( esp_wifi_start() );
}

void webServerTask(void *pvParameters)
{
    peripherals_struct* device_peripherals = (peripherals_struct*) pvParameters;

	espFsInit((void*)(webpages_espfs_start));

	tcpip_adapter_init();

	httpdFreertosInit(&httpdFreertosInstance, builtInUrls, WEB_PORT, connectionMemory, MAX_CONNECTIONS, HTTPD_FLAG_NONE);

	httpdFreertosStart(&httpdFreertosInstance);

    /*Allow a 2 second delay to allow stabilisation, avoiding brown out*/
	vTaskDelay(2000/portTICK_RATE_MS);

	init_wifi();

    /*Start these webpage based websocket broadcasts dependant if there are any users on the page*/
	xTaskCreate(statusPageWsBroadcastTask, "statusPageTask", 4096, device_peripherals, 3, &xStatusBroadcastHandle);
	xTaskCreate(infaredPageWsBroadcastTask, "infaredPageTask", 16384, NULL, 2, &xInfaredBroadcastHandle);

    while(1)
    {

    }
    vTaskDelete(NULL);
}

esp_err_t setupTempHumiditySensor(HIH6030* HIH6030_inst, SemaphoreHandle_t* data_mutex, SemaphoreHandle_t* bus_mutex)
{
    *bus_mutex = xSemaphoreCreateMutex();
    *data_mutex = xSemaphoreCreateMutex();

    esp_err_t ret = HIH6030_init(HIH6030_inst, 0x27, I2C_MASTER_NUM, I2C_MASTER_FREQ_HZ, I2C_MASTER_SDA_IO, GPIO_PULLUP_DISABLE, I2C_MASTER_SCL_IO, GPIO_PULLUP_DISABLE, 
                                                                                                                               data_mutex, bus_mutex);
    return ret;
}

esp_err_t setupGpioExpander(TCA9534* TCA9534_inst, SemaphoreHandle_t* data_mutex, SemaphoreHandle_t* bus_mutex)
{
    *bus_mutex = xSemaphoreCreateMutex();
    *data_mutex = xSemaphoreCreateMutex();

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_TCA9534_INT, TCA9534_isr_handler, (void*)GPIO_TCA9534_INT);

    esp_err_t ret = TCA9534_init(TCA9534_inst, 0x27, I2C_MASTER_NUM, I2C_MASTER_FREQ_HZ, I2C_MASTER_SDA_IO, GPIO_PULLUP_DISABLE, I2C_MASTER_SCL_IO, GPIO_PULLUP_DISABLE, 
                                                                                                                               data_mutex, bus_mutex);
    if(ret != ESP_OK)
    {
        /*Do something*/
    }
    ret = TCA9534_set_pin_direction(TCA9534_inst, TCA9534_SD_WP, TCA9534_INPUT);
    if( ret != ESP_OK)
    {
        printf("\nError setting WP pin direction, returned %d", (int)ret);
    }
    ret = TCA9534_set_pin_direction(TCA9534_inst, TCA9534_SD_CD, TCA9534_INPUT);
    if(ret != ESP_OK)
    {
        printf("\nError setting CD pin direction, returned %d", (int)ret);
    }
    ret = TCA9534_set_pin_direction(TCA9534_inst, TCA9534_WARN_LED, TCA9534_OUTPUT);
    if(ret != ESP_OK)
    {
        printf("\nError setting LED pin direction, returned %d", (int)ret);
    }
    return ret;
}

/*

void acquisitionTask(void *pvParameters)
{
    
}
*/

void app_main()
{
    esp_err_t ret;

    SemaphoreHandle_t i2c_bus_mutex = NULL;
    SemaphoreHandle_t HIH6030_data_mutex = NULL;
    SemaphoreHandle_t TCA9534_data_mutex = NULL;

    peripherals_struct device_peripherals;

    ret=setupGpioExpander(&(device_peripherals.IAQ_TCA9534), &TCA9534_data_mutex, &i2c_bus_mutex);
    if(ret != ESP_OK)
    {
        /*Do something*/
    }
    ret=setupTempHumiditySensor(&(device_peripherals.IAQ_HIH6030), &HIH6030_data_mutex, &i2c_bus_mutex);
    if(ret != ESP_OK)
    {
        /*Do something*/
    }
    ZH03B_init();
    ret=powerOnSelfTest();
    if(ret != ESP_OK)
    {
        /*Do something*/
    }
    vTaskDelay(2000/portTICK_RATE_MS);

    /*Erasing NVS for use by bluetooth and Wifi*/
	ret = nvs_flash_init();
	if(   ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_LOGI(TAG, "Erasing NVS...");
		nvs_flash_erase();
		ret = nvs_flash_init();
	}
    /*Reboot if unsuccessful*/
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    //esp_bt_controller_init(&bt_cfg);
 
    esp_eddystone_init();

    /*<! set scan parameters */
    esp_ble_gap_set_scan_params(&ble_scan_params);
    /*Set up supervisor task to perform task setup and synchronisation*/
    xTaskCreate(webServerTask, "webServerTask", 4096, &device_peripherals, 1, &xWebServerTaskHandle);
    xTaskCreate(TCA9534HandlerTask, "TCA9534Interrupt", 2048, &device_peripherals, 1, NULL);

    while(1)
    {

    }
    vTaskDelete(NULL);
}
