#include "bsp_htc_2k.h"
#include "FreeRTOS.h"
#include "task.h"

// 段码表（与原始TM1637.C完全一致）
const uint8_t SmgTab[] = {
    0xFC, 0x60, 0xDA, 0xF2, 0x66, 0xB6, 0xBE, 0xE0, 0xFE, 0xF6,
    0xEE, 0x3E, 0x9C, 0x7A, 0x9E, 0x8E, 0x6E, 0x1C, 0x3A, 0xCE,
    0x0A, 0x1E, 0x7C, 0x02, 0x00
};

icon_type_t g_IconSet = {0};

// 微秒延时（volatile防止编译器优化掉循环）
static void HTC_DelayUs(volatile uint32_t us) {
    volatile uint32_t delay = (SystemCoreClock / 1000000 / 4) * us;
    while(delay--) { __NOP(); }
}

// ============ 以下完全复刻原始 TM1637.C 的时序 ============

// Start: CLK=H, DIO从H→L（CLK保持高）
static void TM1637_Start(void) {
    HTC_CLK(1);
    HTC_DIO(1);
    HTC_DelayUs(2);
    HTC_DIO(0);
}

// Stop: DIO在CLK为H时从L→H
static void TM1637_Stop(void) {
    HTC_CLK(0);
    HTC_DelayUs(2);
    HTC_DIO(0);
    HTC_DelayUs(2);
    HTC_CLK(1);
    HTC_DelayUs(2);
    HTC_DIO(1);
}

// ACK: 第9个时钟（开漏模式下释放DIO，让TM1637拉低应答）
static void TM1637_Ask(void) {
    HTC_CLK(0);
    HTC_DIO(1);    // 释放DIO，TM1637会拉低作为ACK
    HTC_DelayUs(5);
    HTC_CLK(1);
    HTC_DelayUs(2);
    HTC_CLK(0);
}

// 写一个字节（LSB先发，与原始代码一致）
static void TM1637_Write_Byte(uint8_t dat) {
    uint8_t i;
    for(i = 0; i < 8; i++) {
        HTC_CLK(0);
        if(dat & 0x01)
            HTC_DIO(1);
        else
            HTC_DIO(0);
        HTC_DelayUs(3);
        dat = dat >> 1;
        HTC_CLK(1);
        HTC_DelayUs(3);
    }
}

// 按键扫描（开漏模式：写1即释放总线，可直接读取，无需切换GPIO方向）
uint8_t BSP_HTC2K_ReadKeys(void) {
    uint8_t rekey = 0, i;
    taskENTER_CRITICAL();
    TM1637_Start();
    TM1637_Write_Byte(0x42);
    TM1637_Ask();
    HTC_DIO(1);  // 释放DIO，让TM1637驱动数据

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
    taskEXIT_CRITICAL();
    return rekey;
}

// 初始化（★关键：必须用开漏+上拉，TM1637是开漏总线协议）
void BSP_HTC2K_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = HTC_CLK_PIN | HTC_DIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;   // 开漏输出
    GPIO_InitStruct.Pull = GPIO_PULLUP;            // 内部上拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(HTC_CLK_PORT, &GPIO_InitStruct);
    HTC_CLK(1);
    HTC_DIO(1);
}

// ★★★ 开机自检：强制显示 "888" 全亮，验证通信是否正常 ★★★
void BSP_HTC2K_TestDisplay(void) {
    // 命令1：设置数据命令 —— 自动地址递增
    TM1637_Start();
    TM1637_Write_Byte(0x40);
    TM1637_Ask();
    TM1637_Stop();

    // 命令2：设置起始地址 + 写4个字节数据
    TM1637_Start();
    TM1637_Write_Byte(0xC0);    // 起始地址
    TM1637_Ask();
    TM1637_Write_Byte(0xFF);    // GRID1: 全部图标点亮
    TM1637_Ask();
    TM1637_Write_Byte(0xFE);    // GRID2: 显示 "8"
    TM1637_Ask();
    TM1637_Write_Byte(0xFF);    // GRID3: 显示 "8."
    TM1637_Ask();
    TM1637_Write_Byte(0xFE);    // GRID4: 显示 "8"
    TM1637_Ask();
    TM1637_Stop();

    // 命令3：显示控制 —— 打开显示，最大亮度
    TM1637_Start();
    TM1637_Write_Byte(0x8F);    // 1000_1111 = 显示ON + 脉冲14/16
    TM1637_Ask();
    TM1637_Stop();
}

// 显示温度
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
    if (temp < 0) byte3_num |= 0x01;

    taskENTER_CRITICAL();
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
    TM1637_Write_Byte(0x8F); TM1637_Ask();
    TM1637_Stop();
    taskEXIT_CRITICAL();
}
