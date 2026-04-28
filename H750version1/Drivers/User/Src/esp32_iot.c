#include "esp32_iot.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

static char esp_rx_buf[1024];
static char pub_json_buf[256]; 
static char at_cmd_buf[256];

/**
 * @brief 标准发送函数：依靠 ATE0 解决长指令冲突
 */
uint8_t ESP32_SendAT_Wait(char* cmd, char* expected, uint32_t timeout_ms) {
    memset(esp_rx_buf, 0, sizeof(esp_rx_buf));
    uint16_t ptr = 0;
    
    // 清除错误，清空 FIFO
    __HAL_UART_CLEAR_OREFLAG(&huart2);
    
    // 直接发送指令
    HAL_UART_Transmit(&huart2, (uint8_t*)cmd, strlen(cmd), 200);
    
    uint32_t start_time = HAL_GetTick();
    while (HAL_GetTick() - start_time < timeout_ms) {
        uint8_t ch;
        // 增加接收频率，减小超时时间
        if (HAL_UART_Receive(&huart2, &ch, 1, 5) == HAL_OK) {
            if (ptr < sizeof(esp_rx_buf) - 1) {
                esp_rx_buf[ptr++] = ch;
                HAL_UART_Transmit(&huart1, &ch, 1, 1); // 实时显示回复
            }
        }
        if (expected && strstr(esp_rx_buf, expected)) return 1;
    }
    return 0;
}

void ESP32_IoT_Init(void) {
    printf("\r\n[IoT] Hard Resetting ESP32...\r\n");
    ESP32_SendAT_Wait("AT+RST\r\n", "ready", 3000);
    HAL_Delay(2000);

    printf("\r\n[IoT] Step 1: Disabling Echo (ATE0)...\r\n");
    // 不停尝试 ATE0，直到 OK
    while(1) {
        if(ESP32_SendAT_Wait("ATE0\r\n", "OK", 500)) break;
        HAL_Delay(100);
    }
    printf("\r\n[IoT Success] Echo Disabled. Now handling long commands safely.\r\n");

    // 2. 连接 WiFi
    char wifi_cmd[128];
    sprintf(wifi_cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASS);
    printf("[IoT] Connecting WiFi...\r\n");
    while(!ESP32_SendAT_Wait(wifi_cmd, "WIFI GOT IP", 10000));
    printf("\r\n[IoT Success] WiFi Connected!\r\n");

    // 3. 配置 MQTT (由于回显已关，这里发 200 字节也不会丢包)
    char mqtt_cfg[512];
    sprintf(mqtt_cfg, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n", 
            ONENET_DEVICE_NAME, ONENET_PRODUCT_ID, ONENET_TOKEN);
    printf("[IoT] Configuring MQTT (Long String)...\r\n");
    while(!ESP32_SendAT_Wait(mqtt_cfg, "OK", 5000));
    HAL_Delay(1000); // 呼吸延时

    // 4. 连接 OneNet 服务器
    printf("[IoT] Connecting OneNet Server...\r\n");
    while(1) {
        // 关键：预期的字符串改为 OK
        if(ESP32_SendAT_Wait("AT+MQTTCONN=0,\"mqtts.heclouds.com\",1883,0\r\n", "OK", 10000)) {
            // 进一步检查：如果回复里有 DISCONNECTED，说明 Token 还是不对
            if(strstr(esp_rx_buf, "DISCONNECTED")) {
                printf("\r\n[IoT Error] Auth Rejected! Retrying...\r\n");
                HAL_Delay(5000);
                continue; 
            }
            break; // 成功
        }
        HAL_Delay(2000);
    }
    HAL_Delay(2000); 
    
    // 5. 跳过订阅 (有些 ESP32 固件对 $sys 订阅支持不稳，且上报不需要订阅)
    printf("\r\n[IoT Success] MQTT CONNECTED! Ready for Data Upload.\r\n");
}

/**
 * @brief 正式上报数据到 OneNet
 */
void OneNet_Publish_PersonState(int state) {
    memset(pub_json_buf, 0, sizeof(pub_json_buf));
    memset(at_cmd_buf, 0, sizeof(at_cmd_buf));
    
    // 构造 OneJson 标准格式
    sprintf(pub_json_buf, "{\"id\":\"123\",\"version\":\"1.0\",\"params\":{\"PersonState\":{\"value\":%d}}}", state);
    
    // 构造上报指令 (注意 Topic 不需要引号，AT 内部处理)
    sprintf(at_cmd_buf, "AT+MQTTPUBRAW=0,\"$sys/%s/%s/thing/property/post\",%d,0,0\r\n", 
            ONENET_PRODUCT_ID, ONENET_DEVICE_NAME, (int)strlen(pub_json_buf));
    
    // 1. 发送 PUBRAW，等待 '>'
    if (ESP32_SendAT_Wait(at_cmd_buf, ">", 2000)) {
        // 2. 看到 '>' 后发送 JSON 数据
        HAL_UART_Transmit(&huart2, (uint8_t*)pub_json_buf, strlen(pub_json_buf), 1000);
        printf("\r\n[IoT] Data Sent: %s\r\n", pub_json_buf);
        
        // 3. 等待返回 OK (表示服务器收到了)
        if(ESP32_SendAT_Wait("", "OK", 3000)) {
            printf("[IoT] Upload Success!\r\n");
        }
    } else {
        // 如果失败，可能是 busy 或连接断开
        printf("[IoT] Upload Failed (No >)\r\n");
    }
}
