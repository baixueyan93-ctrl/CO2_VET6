
#ifndef __TM1637_H
#define __TM1637_H

#include "main.h"

extern uint8_t Bai;
extern uint8_t Shi;
extern uint8_t Ge;

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
#define ZM_NULL        24

#define ON               1//符号亮
#define OFF              0//符号灭

//------------------------------------------------------------------------------
// 符号显示结构体，每个位代表一个符号，可以组合使用，8个位组成一个字节共同体
typedef struct
{
    uint8_t Clock     : 1;
    uint8_t Light     : 1;
    uint8_t Set       : 1;
    uint8_t Heat       : 1;
    uint8_t Fan       : 1;
    uint8_t Def       : 1;
    uint8_t Humi      : 1;
    uint8_t Ref       : 1;
} byte_icon;

typedef union
{
    uint8_t    byte;
    byte_icon  bit;
} icon_type_t;
extern  icon_type_t      icon_set_t; // 风速图标配置及显示


// 数码管IC 驱动引脚
#define TM1637_PORT             GPIOD
#define TM1637_CLK_PIN          GPIO_PIN_3
#define TM1637_DIO_PIN          GPIO_PIN_4

#define TM1637_CLK_HIGH     GPIO_WriteHigh(TM1637_PORT, TM1637_CLK_PIN)
#define TM1637_CLK_LOW      GPIO_WriteLow(TM1637_PORT, TM1637_CLK_PIN)
#define TM1637_DIO_HIGH     GPIO_WriteHigh(TM1637_PORT, TM1637_DIO_PIN)
#define TM1637_DIO_LOW      GPIO_WriteLow(TM1637_PORT,TM1637_DIO_PIN)

#define  TM1637_DIO_IN()	   GPIO_ReadInputPin(TM1637_PORT,TM1637_DIO_PIN)

#define DATA_COMMAND_Z      0x40
#define DATA_COMMAND_G      0x44
#define ADDR_START          0xc0// 首地址命令
#define DISP_CLOSE          0x80
#define DISP_OPEN           0x8C// 灰度调整 最大亮度

void    TM1637_init( void );
void    TM1637_display( void );
unsigned char TM1637_key_scan( void );






#endif