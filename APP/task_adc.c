#include "task_adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <math.h>     // 必须包含此头文件，用于 log() 计算
#include "main.h"
#include "sys_state.h"

extern ADC_HandleTypeDef hadc1; // 声明 CubeMX 生成的 ADC1 句柄

// 定义全局数组，DMA 会自动把 PC4 和 PC5 的 ADC 原始值实时搬运到这里
uint16_t adc_buffer[2] = {0};   
float g_temp_10k = 0.0f;
float g_temp_50k = 0.0f;
/* ==========================================================
 * 【老师风格】10K NTC 转换函数 (假定对应 INU4)
 * ========================================================== */
int16_t adc_to_temperature_10k(uint16_t adc_value) {
    const float V_REF = 3.3f;          
    const uint16_t ADC_MAX = 4095;     
    const float R_REF = 10000.0f;       // 电路板上的参考分压电阻 (10kΩ)
    const float R_NTC_NOMINAL = 10000.0f;// 【10K探头】25℃时的标称阻值
    const float T_NOMINAL = 298.15f;   
    const float B_VALUE = 3950.0f;     

    if (adc_value > ADC_MAX) adc_value = ADC_MAX;
    
    float voltage = (adc_value * V_REF) / ADC_MAX;
    if (voltage < 0.001f) return 1000;  // 接近0V，视作断线
    
    float r_ntc = ( voltage / (V_REF - voltage)) * R_REF;
    float inv_t = (1.0f / T_NOMINAL) + (log(r_ntc / R_NTC_NOMINAL) / B_VALUE);
    float temp_celsius = (1.0f / inv_t) - 273.15f;

    // 老师的精髓：乘以10倍转成整数，方便存储和传输
    int32_t temp_scaled = (int32_t)(temp_celsius * 10.0f + 0.5f);  
    if (temp_scaled < -1000) return -1000;
    else if (temp_scaled > 2000) return 2000;
    else return (int16_t)temp_scaled;
}

/* ==========================================================
 * 【老师风格】50K NTC 转换函数 (假定对应 INU5)
 * ========================================================== */
int16_t adc_to_temperature_50k(uint16_t adc_value) {
    const float V_REF = 3.3f;          
    const uint16_t ADC_MAX = 4095;     
    const float R_REF = 50000.0f;       // 电路板分压电阻固定为50kΩ
    const float R_NTC_NOMINAL = 50000.0f;// 【50K探头】25℃时的标称阻值
    const float T_NOMINAL = 298.15f;   
    const float B_VALUE = 3950.0f;     

    if (adc_value > ADC_MAX) adc_value = ADC_MAX;
    
    float voltage = (adc_value * V_REF) / ADC_MAX;
    if (voltage < 0.001f) return 1000;  
    
    float r_ntc = ( voltage / (V_REF - voltage)) * R_REF;
    float inv_t = (1.0f / T_NOMINAL) + (log(r_ntc / R_NTC_NOMINAL) / B_VALUE);
    float temp_celsius = (1.0f / inv_t) - 273.15f;

    // 精确处理负数的四舍五入！
    int32_t temp_scaled = (temp_celsius >= 0) ? 
                          (int32_t)(temp_celsius * 10.0f + 0.5f) : 
                          (int32_t)(temp_celsius * 10.0f - 0.5f);
    if (temp_scaled < -1000) return -1000;
    else if (temp_scaled > 2000) return 2000;
    else return (int16_t)temp_scaled;
}

/* ==========================================================
 * 主任务线程：输入GET打印一次温度
 * ========================================================== */
void Task_ADC_Process(void const *argument) {
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, 2);
    vTaskDelay(pdMS_TO_TICKS(500)); 
    
    for(;;) {
        // 算出整数温度
        int16_t t_10k_raw = adc_to_temperature_10k(adc_buffer[0]); 
        int16_t t_50k_raw = adc_to_temperature_50k(adc_buffer[1]); 
        
        // 2. 【核心修改】不要打印了，直接把真实温度存到“全局公告板”上！
        g_temp_10k = t_10k_raw / 10.0f;
        g_temp_50k = t_50k_raw / 10.0f;
        
			  SysSensorData_t temp_sensor_data;
        // 1. 先把黑板上原来的数据读出来（防止覆盖掉压力、液位等其他任务写的数据）
        SysState_GetSensor(&temp_sensor_data); 
        
        // 2. 把咱们刚测出来的温度更新进去
        temp_sensor_data.V_EVAP_T = g_temp_10k; // 10K 对应蒸发
        temp_sensor_data.V_EXH_T  = g_temp_50k; // 50K 对应排气
        
        // 3. 安全地写回系统黑板（这会触发您底层写的 Mutex 互斥锁，绝对安全）
        SysState_UpdateSensor(&temp_sensor_data);
			
			
        // 3. 既然不打印了，测温可以快一点，比如 100ms 默默刷新一次
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}



