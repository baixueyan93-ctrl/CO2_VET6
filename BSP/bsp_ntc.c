#include "bsp_ntc.h"
#include <math.h>

/* ADC句柄 (本模块内部使用) */
static ADC_HandleTypeDef hadc1;

/**
 * @brief  初始化ADC1, 用于读取NTC (PC2=IN12, PC3=IN13)
 */
void BSP_NTC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 1. 使能时钟 */
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* 2. 配置PC2, PC3为模拟输入 */
    GPIO_InitStruct.Pin  = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* 3. 配置ADC1 */
    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;   /* APB2=84MHz / 4 = 21MHz */
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);
}

/**
 * @brief  读取指定ADC通道的原始值 (轮询模式, 单次转换)
 * @param  channel: ADC_CHANNEL_12 或 ADC_CHANNEL_13
 * @return 12位ADC值 (0~4095), 转换失败返回0
 */
uint16_t BSP_NTC_ReadADC(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel      = channel;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_144CYCLES;  /* 较长采样时间提高精度 */
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        uint16_t val = (uint16_t)HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
        return val;
    }
    HAL_ADC_Stop(&hadc1);
    return 0;
}

/**
 * @brief  多次采样取平均, 提高稳定性
 */
static uint16_t NTC_ReadADC_Avg(uint32_t channel, uint8_t samples)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < samples; i++) {
        sum += BSP_NTC_ReadADC(channel);
    }
    return (uint16_t)(sum / samples);
}

/**
 * @brief  根据ADC值计算NTC温度
 * @param  adc_val:  12位ADC原始值
 * @param  r_pullup: 上拉电阻阻值 (Ω)
 * @param  r0:       NTC在25°C时的标称阻值 (Ω)
 * @param  b_value:  NTC的B值 (K)
 * @return 温度 (°C), 如果ADC值异常返回 -999.0f
 *
 * 电路: VDDA --- R_pullup --- [ADC] --- R_series(5.1k) --- NTC --- VSSA
 * 注意: 5.1kΩ串联电阻对分压有影响, 此处简化处理
 *       V_adc = VDDA * (R_ntc + R_series) / (R_pullup + R_ntc + R_series)
 *       如需更高精度可校准
 */
float BSP_NTC_CalcTemp(uint16_t adc_val, float r_pullup, float r0, float b_value)
{
    /* 防止除零 */
    if (adc_val == 0 || adc_val >= 4095) {
        return -999.0f;
    }

    /* 串联保护电阻 */
    const float r_series = 5100.0f;  /* 5.1kΩ */

    /*
     * 分压公式 (NTC在下臂, 串联电阻在NTC和ADC之间):
     * V_adc / VDDA = (R_ntc + R_series) / (R_pullup + R_ntc + R_series)
     * 令 ratio = adc_val / 4095.0
     * R_ntc + R_series = R_pullup * ratio / (1 - ratio)
     * R_ntc = R_pullup * ratio / (1 - ratio) - R_series
     */
    float ratio = (float)adc_val / 4095.0f;
    float r_ntc = r_pullup * ratio / (1.0f - ratio) - r_series;

    /* NTC阻值应为正值 */
    if (r_ntc <= 0.0f) {
        return -999.0f;
    }

    /* B参数方程: 1/T = 1/T0 + (1/B) * ln(R/R0) */
    float temp_k = 1.0f / (1.0f / NTC_T0_KELVIN + (1.0f / b_value) * logf(r_ntc / r0));
    float temp_c = temp_k - 273.15f;

    return temp_c;
}

/**
 * @brief  一次性读取两路NTC温度
 */
void BSP_NTC_ReadAll(NTC_Data_t *data)
{
    /* 8次采样取平均 */
    data->adc_high = NTC_ReadADC_Avg(NTC_HIGH_ADC_CHANNEL, 8);
    data->adc_low  = NTC_ReadADC_Avg(NTC_LOW_ADC_CHANNEL,  8);

    data->temp_high = BSP_NTC_CalcTemp(data->adc_high,
                                        NTC_HIGH_RPULLUP,
                                        NTC_HIGH_R0,
                                        NTC_HIGH_B);

    data->temp_low  = BSP_NTC_CalcTemp(data->adc_low,
                                        NTC_LOW_RPULLUP,
                                        NTC_LOW_R0,
                                        NTC_LOW_B);
}
