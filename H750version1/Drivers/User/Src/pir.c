#include "pir.h"
#include "dcmi_ov5640.h"
#include <stdio.h>

// 外部变量引用
extern DCMI_HandleTypeDef hdcmi;
extern uint16_t Camera_Buffer[];
extern void SystemClock_Config(void);

// 内部计时器
volatile uint32_t last_motion_tick = 0;

/**
 * @brief 初始化 PIR 传感器引脚 (PA0 EXTI)
 */
void PIR_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE(); // 必须开启 SYSCFG 时钟，EXTI 才能工作
    
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING; 
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    
    last_motion_tick = HAL_GetTick();
}

/**
 * @brief 外部中断回调函数
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        /* 软件过滤：等待 100ms 再次确认电平。
           如果演示现场干扰很大（莫名其妙重置），可以尝试把 100 改成 200 或 500。
           如果觉得反应太迟钝，可以把这一段去掉。 */
        HAL_Delay(100);
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
            last_motion_tick = HAL_GetTick();
        }
    }
}

extern UART_HandleTypeDef huart1; // 假设你用的是 huart1，如果不是请修改

/**
 * @brief 进入低功耗模式
 */
void System_Enter_Sleep(void) {
    // 确保串口最后一条消息发完
    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET);
    printf("\r\n[PIR] No motion detected. Entering STOP mode...\r\n");
    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET);
    
    // 1. 停止摄像头采样并关闭硬件电源
    HAL_DCMI_Stop(&hdcmi);
    OV5640_PWDN_ON;     // 摄像头硬件进入省电模式
    LCD_Backlight_OFF;  // 关闭屏幕背光
    HAL_Delay(5);
    
    // 2. 进入 STOP 模式 (等待 PA0 中断唤醒)
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    
    // --- 【此处被唤醒】 ---
    
    // 3. 恢复系统时钟
    SystemClock_Config();
    HAL_Delay(10);
    
    // 4. 恢复硬件电源
    OV5640_PWDN_OFF;    // 唤醒摄像头
    LCD_Backlight_ON;   // 开启屏幕背光
    
    last_motion_tick = HAL_GetTick(); 
}

/**
 * @brief 检查是否超时并进入休眠
 */
uint8_t PIR_Check_And_Sleep(void) {
    /* --- 演示调节说明 ---
       如果你想加快进入休眠的速度（比如演示时），请修改下面的数字：
       5000  代表 5 秒无人触发即休眠
       15000 代表 15 秒无人触发即休眠
       30000 代表 30 秒无人触发即休眠（默认）
    */
    if (HAL_GetTick() - last_motion_tick > 30000) { 
        System_Enter_Sleep();
        
        // 醒来后，重新开启 DCMI
        __HAL_DCMI_ENABLE_IT(&hdcmi, DCMI_IT_FRAME);
        HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_CONTINUOUS, (uint32_t)Camera_Buffer, Display_BufferSize);
        
        return 1; 
    }
    return 0; 
}
