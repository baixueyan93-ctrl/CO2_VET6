#include "htc_2k.h"
#include "./SYSTEM/delay/delay.h" 

// ================= 厂家原版段码表 =================
const uint8_t SmgTab[] =
{
    0xFC,/*0*/ 0x60,/*1*/ 0xDA,/*2*/ 0xF2,/*3*/ 0x66,/*4*/
    0xB6,/*5*/ 0xBE,/*6*/ 0xE0,/*7*/ 0xFE,/*8*/ 0xF6,/*9*/
    0xEE,/*A*/ 0x3E,/*b*/ 0x9C,/*C*/ 0x7A,/*d*/ 0x9E,/*E*/ 0x8E,/*F*/
    0x6E,/*H*/ 0x1C,/*L*/ 0x3A,/*c*/ 0xCE,/*P*/ 0x0A,/*r*/ 0x1E,/*t*/ 0x7C,/*U*/
    0x02,/*-*/ 0x00,/*空*/
};

icon_type_t g_IconSet = {0};

// 延时函数
static void TM1637_DelayUs(uint8_t i) {
    delay_us(i); 
}

// Start信号
static void TM1637_Start(void) {
    HTC_CLK(1); HTC_DIO(1); TM1637_DelayUs(2);
    HTC_DIO(0); TM1637_DelayUs(2); 
    HTC_CLK(0);
}

// Stop信号
static void TM1637_Stop(void) {
    HTC_CLK(0); TM1637_DelayUs(2);
    HTC_DIO(0); TM1637_DelayUs(2);
    HTC_CLK(1); TM1637_DelayUs(2);
    HTC_DIO(1); 
}

// 等待ACK
static void TM1637_Ask(void) {
    HTC_CLK(0); TM1637_DelayUs(5);
    HTC_CLK(1); TM1637_DelayUs(2);
    HTC_CLK(0);
}

// 写一个字节
static void TM1637_Write_Byte(uint8_t dat) {
    uint8_t i;
    for(i = 0; i < 8; i++) {
        HTC_CLK(0);
        if(dat & 0x01) HTC_DIO(1); 
        else HTC_DIO(0);
        TM1637_DelayUs(3);
        dat = dat >> 1;
        HTC_CLK(1); TM1637_DelayUs(3);
    }
}

// 读按键
uint8_t HTC2K_ReadKeys(void) {
    uint8_t rekey = 0, i;
    TM1637_Start();
    TM1637_Write_Byte(0x42); 
    TM1637_Ask();
    HTC_DIO(1); 
    
    for(i = 0; i < 8; i++) {
        HTC_CLK(0);
        rekey = rekey >> 1; 
        TM1637_DelayUs(10);
        HTC_CLK(1);
        if(HTC_READ_DIO()) rekey |= 0x80; 
        TM1637_DelayUs(20);
    }
    TM1637_Ask();
    TM1637_Stop();
    return rekey;
}

// 初始化
void HTC2K_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    HTC_CLK_ENABLE();
    GPIO_InitStruct.Pin = HTC_CLK_PIN | HTC_DIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; 
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(HTC_CLK_PORT, &GPIO_InitStruct);
    HTC_CLK(1); HTC_DIO(1);
}

// ================= 核心：根据厂家源码修正的逻辑 =================
void HTC2K_ShowTemp(float temp) {
    uint8_t byte0_icon, byte1_num, byte2_num, byte3_num;
    uint8_t d1, d2, d3; 
    
    // 1. 范围限制
    if(temp < -99.9f) temp = -99.9f;
    if(temp > 99.9f) temp = 99.9f;

    // 2. 提取数字 (取绝对值，保证三个数字位都用来显示数值)
    int val = (int)(temp * 10); 
    if (val < 0) val = -val;

    d1 = val / 100;        // 百位 
    d2 = (val / 10) % 10;  // 十位
    d3 = val % 10;         // 个位 

    // 3. 构建数据 (Icon -> Bai -> Shi -> Ge)
    
    // --- Byte 0 (0xC0): 图标 ---
    byte0_icon = g_IconSet.byte; 

    // --- Byte 1 (0xC1): 百位 (左边数字) ---
    byte1_num = SmgTab[d1]; 
    if (byte1_num == 0xFC && val < 100) byte1_num = 0x00; // 消隐百位0

    // --- Byte 2 (0xC2): 十位 (中间数字) + 小数点 ---
    // 厂家代码: if(sys_flag_t.idot == ON) SmgTab[Shi] | 0x01;
    // 这说明 0xC2 的 Bit0 是小数点
    byte2_num = SmgTab[d2] | 0x01; 

    // --- Byte 3 (0xC3): 个位 (右边数字) + 负号 ---
    byte3_num = SmgTab[d3];
    
    if (temp < 0) {
        // 【真相大白】：厂家代码显示 FuHao (负号) 是在 Ge (个位) 上 | 0x01
        // 虽然负号在最左边，但它的电路控制脚接在最右边数字的 Bit 0 上！
        byte3_num |= 0x01; 
    }


    // ================= 4. 发送数据 =================
    
    TM1637_Start();
    TM1637_Write_Byte(0x40); 
    TM1637_Ask();
    TM1637_Stop();
    
    TM1637_Start();
    TM1637_Write_Byte(0xC0); 
    TM1637_Ask();
    
    // 0xC0: 图标
    TM1637_Write_Byte(byte0_icon);
    TM1637_Ask();
    
    // 0xC1: 百位
    TM1637_Write_Byte(byte1_num);
    TM1637_Ask();
    
    // 0xC2: 十位 + 小数点
    TM1637_Write_Byte(byte2_num);
    TM1637_Ask();
    
    // 0xC3: 个位 + 负号 (在这里点亮！)
    TM1637_Write_Byte(byte3_num);
    TM1637_Ask();
    
    TM1637_Stop();
    
    // 开显示
    TM1637_Start();
    TM1637_Write_Byte(0x8C); 
    TM1637_Ask();
    TM1637_Stop();
}


