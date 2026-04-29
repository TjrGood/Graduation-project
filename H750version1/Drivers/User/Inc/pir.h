#ifndef __PIR_H
#define __PIR_H

#include "main.h"

// --- 外部变量声明 ---
extern volatile uint32_t last_motion_tick;

// --- 函数声明 ---
void PIR_Init(void);
uint8_t PIR_Check_And_Sleep(void);
void System_Enter_Sleep(void);

#endif
