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

uint8_t Bai = 0;
uint8_t Shi = 0;
uint8_t Ge  = 0;
sys_flag_type_t sys_flag_t = {0};

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

// ACK: 第9个时钟（忽略TM1637的应答，与原始代码一致）
static void TM1637_Ask(void) {
    HTC_CLK(0);
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

// DIO方向切换（推挽模式下读按键需要）
static void HTC_DIO_SetInput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = HTC_DIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(HTC_DIO_PORT, &GPIO_InitStruct);
}

static void HTC_DIO_SetOutput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = HTC_DIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(HTC_DIO_PORT, &GPIO_InitStruct);
}

// 按键扫描
uint8_t BSP_HTC2K_ReadKeys(void) {
    uint8_t rekey = 0, i;
    taskENTER_CRITICAL();
    TM1637_Start();
    TM1637_Write_Byte(0x42);
    TM1637_Ask();
    HTC_DIO_SetInput();
    for(i = 0; i < 8; i++) {
        HTC_CLK(0);
        rekey = rekey >> 1;
        HTC_DelayUs(10);
        HTC_CLK(1);
        if(HTC_READ_DIO()) rekey |= 0x80;
        HTC_DelayUs(20);
    }
    HTC_DIO_SetOutput();
    TM1637_Ask();
    TM1637_Stop();
    taskEXIT_CRITICAL();
    return rekey;
}

// 初始化
void BSP_HTC2K_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = HTC_CLK_PIN | HTC_DIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
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

// 纯显示：把 Bai/Shi/Ge 发给 TM1637（与原始 TM1637_display 一致）
void BSP_HTC2K_Display(void) {
    uint8_t Icon = g_IconSet.byte;

    taskENTER_CRITICAL();
    TM1637_Start();
    TM1637_Write_Byte(0x40);
    TM1637_Ask();
    TM1637_Stop();

    TM1637_Start();
    TM1637_Write_Byte(0xC0);
    TM1637_Ask();

    TM1637_Write_Byte(Icon);
    TM1637_Ask();

    TM1637_Write_Byte(SmgTab[Bai]);
    TM1637_Ask();

    if (sys_flag_t.idot)
        TM1637_Write_Byte(SmgTab[Shi] | 0x01);
    else
        TM1637_Write_Byte(SmgTab[Shi]);
    TM1637_Ask();

    if (sys_flag_t.FuHao)
        TM1637_Write_Byte(SmgTab[Ge] | 0x01);
    else
        TM1637_Write_Byte(SmgTab[Ge]);
    TM1637_Ask();

    TM1637_Stop();

    TM1637_Start();
    TM1637_Write_Byte(0x8C);   // 亮度与原始一致（脉冲 11/16）
    TM1637_Ask();
    TM1637_Stop();
    taskEXIT_CRITICAL();
}
