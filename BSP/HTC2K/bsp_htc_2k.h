#ifndef __BSP_HTC_2K_H
#define __BSP_HTC_2K_H

#include "main.h" // ���� HAL ��

// Ӳ�����Ŷ��� (�������� PB6 �� PB7)
#define HTC_CLK_PORT    GPIOB
#define HTC_CLK_PIN     GPIO_PIN_6
#define HTC_DIO_PORT    GPIOB
#define HTC_DIO_PIN     GPIO_PIN_7

#define HTC_CLK(x)      HAL_GPIO_WritePin(HTC_CLK_PORT, HTC_CLK_PIN, (x)?GPIO_PIN_SET:GPIO_PIN_RESET)
#define HTC_DIO(x)      HAL_GPIO_WritePin(HTC_DIO_PORT, HTC_DIO_PIN, (x)?GPIO_PIN_SET:GPIO_PIN_RESET)
#define HTC_READ_DIO()  HAL_GPIO_ReadPin(HTC_DIO_PORT, HTC_DIO_PIN)

// ͼ����ƽṹ�� (��������)
typedef struct {
    uint8_t Clock   : 1; 
    uint8_t Light   : 1; 
    uint8_t Set     : 1; 
    uint8_t Heat    : 1; 
    uint8_t Fan     : 1; 
    uint8_t Def     : 1; 
    uint8_t Humi    : 1; 
    uint8_t Ref     : 1; 
} icon_bits_t;

typedef union {
    uint8_t      byte;
    icon_bits_t  bits;
} icon_type_t;

extern icon_type_t g_IconSet;

// 显示数据：外部直接赋值（与原始 TM1637.C 一致）
extern uint8_t Bai;   // 百位（段码表索引）
extern uint8_t Shi;   // 十位（段码表索引）
extern uint8_t Ge;    // 个位（段码表索引）

// 显示标志
typedef struct {
    uint8_t idot  : 1;  // 十位小数点（GRID3 最低位）
    uint8_t FuHao : 1;  // 个位负号点（GRID4 最低位）
} sys_flag_type_t;

extern sys_flag_type_t sys_flag_t;

// 段码表索引定义（字母/符号）
#define ZM_A          10
#define ZM_b          11
#define ZM_C          12
#define ZM_d          13
#define ZM_E          14
#define ZM_F          15
#define ZM_H          16
#define ZM_L          17
#define ZM_o          18
#define ZM_P          19
#define ZM_r          20
#define ZM_t          21
#define ZM_U          22
#define ZM_FH         23
#define ZM_NULL       24

// 按键值定义
#define KEY_CODE_SET   0xF4
#define KEY_CODE_UP    0xF5
#define KEY_CODE_DOWN  0xF6
#define KEY_CODE_RST   0xF7

// 函数声明
void BSP_HTC2K_Init(void);
void BSP_HTC2K_TestDisplay(void);
void BSP_HTC2K_Display(void);       // 纯显示：把 Bai/Shi/Ge 发给 TM1637
uint8_t BSP_HTC2K_ReadKeys(void);    

#endif




