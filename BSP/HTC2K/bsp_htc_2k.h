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

// ������ֵ���� (����ԭ���� main.c �Ƶ�����)
#define KEY_CODE_SET   0xF4  
#define KEY_CODE_UP    0xF5  
#define KEY_CODE_DOWN  0xF6  
#define KEY_CODE_RST   0xF7  

// ��������
void BSP_HTC2K_Init(void);
void BSP_HTC2K_TestDisplay(void);    // 开机自检：全亮"888"
void BSP_HTC2K_ShowTemp(float temp);
uint8_t BSP_HTC2K_ReadKeys(void);    

#endif




