#ifndef __HTC_2K_H
#define __HTC_2K_H

#include "./SYSTEM/sys/sys.h"
#include "stm32f4xx_hal.h"

// ================= 1. 硬件引脚定义 =================
#define HTC_CLK_PORT    GPIOB
#define HTC_CLK_PIN     GPIO_PIN_6
#define HTC_DIO_PORT    GPIOB
#define HTC_DIO_PIN     GPIO_PIN_7

#define HTC_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()

// ================= 2. 宏定义操作 =================
#define HTC_CLK(x)      HAL_GPIO_WritePin(HTC_CLK_PORT, HTC_CLK_PIN, (x)?GPIO_PIN_SET:GPIO_PIN_RESET)
#define HTC_DIO(x)      HAL_GPIO_WritePin(HTC_DIO_PORT, HTC_DIO_PIN, (x)?GPIO_PIN_SET:GPIO_PIN_RESET)
#define HTC_READ_DIO()  HAL_GPIO_ReadPin(HTC_DIO_PORT, HTC_DIO_PIN)

// ================= 3. 图标控制结构体 (严格对应厂家 TM1637.h) =================
// 每一个位对应屏幕上的一个图标
typedef struct
{
    uint8_t Clock   : 1; // Bit 0: 闹钟
    uint8_t Light   : 1; // Bit 1: 灯光 (Light)
    uint8_t Set     : 1; // Bit 2: 设置 (Set)
    uint8_t Heat    : 1; // Bit 3: 加热 (Heat)
    uint8_t Fan     : 1; // Bit 4: 风扇 (Fan)
    uint8_t Def     : 1; // Bit 5: 除霜 (Def)
    uint8_t Humi    : 1; // Bit 6: 湿度 (Humi)
    uint8_t Ref     : 1; // Bit 7: 制冷 (Ref)
} icon_bits_t;

typedef union
{
    uint8_t      byte;
    icon_bits_t  bits;
} icon_type_t;

// ================= 4. 特殊字符索引 (源自厂家 TM1637.h) =================
#define ZM_FH         23  // 负号 (-) 的索引
#define ZM_NULL       24  // 空

// 全局变量声明
extern icon_type_t g_IconSet; 

// ================= 5. 函数声明 =================
void HTC2K_Init(void);
void HTC2K_ShowTemp(float temp); 
uint8_t HTC2K_ReadKeys(void);    

#endif



