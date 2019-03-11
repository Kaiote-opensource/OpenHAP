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
#ifndef __PERIPHERAL_SETTINGS_H__
#define __PERIPHERAL_SETTINGS_H__


#define I2C_BAUDRATE 100000U
#define CELL_BAUDRATE 115200U
#define PM_BAUDRATE 115200U

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


#endif