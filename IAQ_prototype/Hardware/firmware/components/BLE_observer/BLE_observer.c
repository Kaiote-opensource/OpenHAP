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

/*
 This file initialises the BLE controller and sets hardware registers to assume
 an observer role, protocol is eddystone. On receiving an advertisement, a callback 
 is fired to a routine that decodes the advert from the User's tag and collects metadata
 about the tag nature. This data and metadata is then delivered to the calling
 task.

 The period of observation is settable. The advertisement interval of the tag
 advised to be four times or more faster than the observation period.The choice
 may affect reliability and power consumption of the tag.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_gap_ble_api.h"
