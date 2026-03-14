#ifndef __BSP_NTC_H
#define __BSP_NTC_H

#include "main.h"
#include <stdint.h>

/* -------------------- 硬件引脚定义 -------------------- */
/* INU4 -> PC3 -> ADC1_IN13  (高温NTC, 10kΩ, 上拉10kΩ) */
/* INU5 -> PC2 -> ADC1_IN12  (低温NTC, 50kΩ, 上拉50kΩ) */

#define NTC_HIGH_ADC_CHANNEL    ADC_CHANNEL_13   /* PC3 - INU4 */
#define NTC_LOW_ADC_CHANNEL     ADC_CHANNEL_12   /* PC2 - INU5 */

/* -------------------- NTC参数 (根据实际NTC数据手册调整) ---- */
/* 高温NTC: 10kΩ @25°C */
#define NTC_HIGH_R0         10000.0f    /* 25°C时的标称阻值 (Ω) */
#define NTC_HIGH_B          3435.0f     /* B值 (K), 根据数据手册调整 */
#define NTC_HIGH_RPULLUP    10000.0f    /* 上拉电阻 (Ω) */

/* 低温NTC: 50kΩ @25°C */
#define NTC_LOW_R0          50000.0f    /* 25°C时的标称阻值 (Ω) */
#define NTC_LOW_B           3950.0f     /* B值 (K), 根据数据手册调整 */
#define NTC_LOW_RPULLUP     50000.0f    /* 上拉电阻 (Ω) */

#define NTC_T0_KELVIN       298.15f     /* 25°C = 298.15K */

/* -------------------- NTC数据结构 -------------------- */
typedef struct {
    float temp_high;    /* 高温NTC温度 (°C) */
    float temp_low;     /* 低温NTC温度 (°C) */
    uint16_t adc_high;  /* 高温NTC原始ADC值 */
    uint16_t adc_low;   /* 低温NTC原始ADC值 */
} NTC_Data_t;

/* -------------------- 函数声明 -------------------- */
void     BSP_NTC_Init(void);
uint16_t BSP_NTC_ReadADC(uint32_t channel);
float    BSP_NTC_CalcTemp(uint16_t adc_val, float r_pullup, float r0, float b_value);
void     BSP_NTC_ReadAll(NTC_Data_t *data);

#endif /* __BSP_NTC_H */
