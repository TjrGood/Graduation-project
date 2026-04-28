#ifndef __ESP32_IOT_H
#define __ESP32_IOT_H

#include "main.h"

// --- 配置信息 ---
#define WIFI_SSID     "14-3"
#define WIFI_PASS     "42811901"

#define ONENET_PRODUCT_ID  "dQ596939LD"
#define ONENET_DEVICE_NAME "Camera01"
#define ONENET_TOKEN       "version=2018-10-31&res=products%2FdQ596939LD%2Fdevices%2FCamera01&et=2000000000&method=sha1&sign=%2ByI7tSAuJoUOkPTclz6QkWSS0Ms%3D"

// --- 函数声明 ---
void ESP32_IoT_Init(void);
uint8_t ESP32_SendAT_Wait(char* cmd, char* expected, uint32_t timeout_ms);
void OneNet_Publish_PersonState(int state);

#endif
