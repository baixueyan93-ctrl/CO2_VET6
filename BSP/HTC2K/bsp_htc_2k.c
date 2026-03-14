#include "bsp_htc_2k.h"

// 厂家原版段码表
const uint8_t SmgTab[] = {
    0xFC, 0x60, 0xDA, 0xF2, 0x66, 0xB6, 0xBE, 0xE0, 0xFE, 0xF6,
    0xEE, 0x3E, 0x9C, 0x7A, 0x9E, 0x8E, 0x6E, 0x1C, 0x3A, 0xCE, 
    0x0A, 0x1E, 0x7C, 0x02, 0x00
};

icon_type_t g_IconSet = {0};

// 【核心改造】：适配 STM32F407 168MHz 的安全微秒延时
// 彻底干掉正点原子的 delay_us，防止与 FreeRTOS 冲突！
static void HTC_DelayUs(uint32_t us) {
    uint32_t delay = (SystemCoreClock / 1000000 / 4) * us; 
    while(delay--) { __NOP(); }
}

// Start信号
static void TM1637_Start(void) {
    HTC_CLK(1); HTC_DIO(1); HTC_DelayUs(2);
    HTC_DIO(0); HTC_DelayUs(2); 
    HTC_CLK(0);
}

// Stop信号
static void TM1637_Stop(void) {
    HTC_CLK(0); HTC_DelayUs(2);
    HTC_DIO(0); HTC_DelayUs(2);
    HTC_CLK(1); HTC_DelayUs(2);
    HTC_DIO(1); 
}

// 等待ACK
static void TM1637_Ask(void) {
    HTC_CLK(0); HTC_DelayUs(5);
    HTC_CLK(1); HTC_DelayUs(2);
    HTC_CLK(0);
}

// 写一个字节
static void TM1637_Write_Byte(uint8_t dat) {
    uint8_t i;
    for(i = 0; i < 8; i++) {
        HTC_CLK(0);
        if(dat & 0x01) HTC_DIO(1); 
        else HTC_DIO(0);
        HTC_DelayUs(3);
        dat = dat >> 1;
        HTC_CLK(1); HTC_DelayUs(3);
    }
}

// 读按键 (您的原版逻辑)
uint8_t BSP_HTC2K_ReadKeys(void) {
    uint8_t rekey = 0, i;
    TM1637_Start();
    TM1637_Write_Byte(0x42); 
    TM1637_Ask();
    HTC_DIO(1); 
    for(i = 0; i < 8; i++) {
        HTC_CLK(0);
        rekey = rekey >> 1; 
        HTC_DelayUs(10);
        HTC_CLK(1);
        if(HTC_READ_DIO()) rekey |= 0x80; 
        HTC_DelayUs(20);
    }
    TM1637_Ask();
    TM1637_Stop();
    return rekey;
}

// 初始化
void BSP_HTC2K_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE(); // 开启时钟
    GPIO_InitStruct.Pin = HTC_CLK_PIN | HTC_DIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; 
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(HTC_CLK_PORT, &GPIO_InitStruct);
    HTC_CLK(1); HTC_DIO(1);
}

// 显示功能 (您的原版核心算法，直接复用！)
void BSP_HTC2K_ShowTemp(float temp) {
    uint8_t byte0_icon, byte1_num, byte2_num, byte3_num;
    uint8_t d1, d2, d3; 
    
    if(temp < -99.9f) temp = -99.9f;
    if(temp > 99.9f) temp = 99.9f;

    int val = (int)(temp * 10); 
    if (val < 0) val = -val;

    d1 = val / 100;       
    d2 = (val / 10) % 10; 
    d3 = val % 10;        
    
    byte0_icon = g_IconSet.byte; 
    byte1_num = SmgTab[d1]; 
    if (byte1_num == 0xFC && val < 100) byte1_num = 0x00; 

    byte2_num = SmgTab[d2] | 0x01; 
    byte3_num = SmgTab[d3];
    if (temp < 0) byte3_num |= 0x01; // 个位控制负号
    
    TM1637_Start();
    TM1637_Write_Byte(0x40); TM1637_Ask();
    TM1637_Stop();
    
    TM1637_Start();
    TM1637_Write_Byte(0xC0); TM1637_Ask();
    TM1637_Write_Byte(byte0_icon); TM1637_Ask();
    TM1637_Write_Byte(byte1_num); TM1637_Ask();
    TM1637_Write_Byte(byte2_num); TM1637_Ask();
    TM1637_Write_Byte(byte3_num); TM1637_Ask();
    TM1637_Stop();
    
    TM1637_Start();
    TM1637_Write_Byte(0x8C); TM1637_Ask();
    TM1637_Stop();
}




