#include "peripheral_settings.h"
#include "driver/i2c.h"

struct pinAllocation{

    int sclPin;
    int sdaPin;
    
    int cellularTx;
    int cellularRx;
    int CellularPwrKey;
    
    int particulateSensorRx;
    int particulateSensorRx;
    
    int SDCardCMD;
    int SDCardCLK;
    int SDCardData0;particulateSensor
    int SDCardData1;particulateSensor
    int SDCardData2;particulateSensor
    int SDCardData3;particulateSensor

    int LoadControl;particulateSensor
    int userButton;
    int expanderInterrupt;
    int nRTCReset;
    int BattSense;

};

struct pinAllocation{

    int sclPin;
    int sdaPin;
    
    int cellularTx;
    int cellularRx;
    int CellularPwrKey;
    
    int particulateSensorRx;
    int particulateSensorRx;
    
    int SDCardCMD;
    int SDCardCLK;
    int SDCardData0;particulateSensor
    int SDCardData1;particulateSensor
    int SDCardData2;particulateSensor
    int SDCardData3;particulateSensor

    int LoadControl;particulateSensor
    int userButton;
    int expanderInterrupt;
    int nRTCReset;
    int BattSense;

};



typedef struct
{

    .sclPin=GPIO_NUM_19,
    .sdaPin=GPIO_NUM_21,
    .cellularTx=GPIO_NUM_18,
    .cellularRx=GPIO_NUM_5,
    .CellularPwrKey=GPIO_NUM_27,
    .particulateSensorRx=GPIO_NUM_23,
    .particulateSensorTx=GPIO_NUM_22,
    .SDCardCMD=GPIO_NUM_15,
    .SDCardCLK=GPIO_NUM_14,
    .SDCardData0=GPIO_NUM_2,
    .SDCardData1=GPIO_NUM_4,
    .SDCardData2=GPIO_NUM_12,
    .SDCardData3=GPIO_NUM_13,
    .LoadControl=GPIO_NUM_26,
    .userButton=GPIO_NUM_32,
    .expanderInterrupt=GPIO_NUM_35,
    .RTCReset=GPIO_NUM_25,
    .BattSense=GPIO_NUM_26
} peripheralPins;



}

static void i2cMasterInit(struct pinDef* myPinSettings)
{
    int i2c_port = I2C_NUM_0;
    i2c_config_t i2c_conf;
    i2c_conf.sda_io_num = myPins.sdaPin;
    i2c_conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    i2c_conf.scl_io_num = myPins.sclPin;
    i2c_conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    i2c_conf.master.clk_speed = I2C_SPEED;
    i2c_param_config(i2c_port, &conf);
    i2c_driver_install(i2c_port, conf.mode,
                       I2C_EXAMPLE_SLAVE_RX_BUF_LEN,
                       I2C_EXAMPLE_SLAVE_TX_BUF_LEN, 0);
}

static void cellularInit(struct pinDef* myPinSettings)
{
    int i2c_port = I2C_NUM_0;
    i2c_config_t i2c_conf;
    i2c_conf.sda_io_num = myPins.sdaPin;
    i2c_conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    i2c_conf.scl_io_num = myPins.sclPin;
    i2c_conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    i2c_conf.master.clk_speed = 100000;
    i2c_param_config(i2c_port, &conf);
    i2c_driver_install(i2c_port, conf.mode,
                       I2C_EXAMPLE_SLAVE_RX_BUF_LEN,
                       I2C_EXAMPLE_SLAVE_TX_BUF_LEN, 0);
}

static void particulateSensorInit(struct pinDef* myPinSettings)
{
    int i2c_port = I2C_NUM_0;
    i2c_config_t i2c_conf;
    i2c_conf.sda_io_num = myPins.sdaPin;
    i2c_conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    i2c_conf.scl_io_num = myPins.sclPin;
    i2c_conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    i2c_conf.master.clk_speed = 100000;
    i2c_param_config(i2c_port, &conf);
    i2c_driver_install(i2c_port, conf.mode,
                       I2C_EXAMPLE_SLAVE_RX_BUF_LEN,
                       I2C_EXAMPLE_SLAVE_TX_BUF_LEN, 0);
}

static void simplePeripheralPinsInit(struct pinDef* myPinSettings)
{
    int i2c_port = I2C_NUM_0;
    i2c_config_t i2c_conf;
    i2c_conf.sda_io_num = myPins.sdaPin;
    i2c_conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    i2c_conf.scl_io_num = myPins.sclPin;
    i2c_conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    i2c_conf.master.clk_speed = 100000;
    i2c_param_config(i2c_port, &conf);
    i2c_driver_install(i2c_port, conf.mode,
                       I2C_EXAMPLE_SLAVE_RX_BUF_LEN,
                       I2C_EXAMPLE_SLAVE_TX_BUF_LEN, 0);
}

static esp_err_t init_pin_led_(pinDef* myDef)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    gpio_pad_select_gpio(myDef->);
    gpio_pad_select_gpio(matrixInstanceptr->shift_pin);
    gpio_pad_select_gpio(matrixInstanceptr->latch_pin);
    gpio_pad_select_gpio(matrixInstanceptr->rowclk_pin);
    gpio_pad_select_gpio(matrixInstanceptr->rowrst_pin);
    gpio_set_direction(matrixInstanceptr->serial_pin, GPIO_MODE_OUTPUT);    
    gpio_set_direction(matrixInstanceptr->shift_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(matrixInstanceptr->latch_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(matrixInstanceptr->rowclk_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(matrixInstanceptr->rowrst_pin, GPIO_MODE_OUTPUT);

    return ESP_OK;
}



