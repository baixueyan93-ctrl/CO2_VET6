#include "task_adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <math.h>     // ���������ͷ�ļ������� log() ����
#include "main.h"
#include "sys_state.h"

extern ADC_HandleTypeDef hadc1; // ���� CubeMX ���ɵ� ADC1 ���

// ����ȫ�����飬DMA ���Զ��� PC4 �� PC5 �� ADC ԭʼֵʵʱ���˵�����
uint16_t adc_buffer[2] = {0};   
float g_temp_10k = 0.0f;
float g_temp_50k = 0.0f;
/* ==========================================================
 * ����ʦ���10K NTC ת������ (�ٶ���Ӧ INU4)
 * ========================================================== */
int16_t adc_to_temperature_10k(uint16_t adc_value) {
    const float V_REF = 3.3f;          
    const uint16_t ADC_MAX = 4095;     
    const float R_REF = 10000.0f;       // ��·���ϵĲο���ѹ���� (10k��)
    const float R_NTC_NOMINAL = 10000.0f;// ��10K̽ͷ��25��ʱ�ı����ֵ
    const float T_NOMINAL = 298.15f;   
    const float B_VALUE = 3950.0f;     

    if (adc_value > ADC_MAX) adc_value = ADC_MAX;
    
    float voltage = (adc_value * V_REF) / ADC_MAX;
    if (voltage < 0.001f) return 1000;  // �ӽ�0V����������
    
    float r_ntc = ( voltage / (V_REF - voltage)) * R_REF;
    float inv_t = (1.0f / T_NOMINAL) + (log(r_ntc / R_NTC_NOMINAL) / B_VALUE);
    float temp_celsius = (1.0f / inv_t) - 273.15f;

    // ��ʦ�ľ��裺����10��ת������������洢�ʹ���
    int32_t temp_scaled = (int32_t)(temp_celsius * 10.0f + 0.5f);  
    if (temp_scaled < -1000) return -1000;
    else if (temp_scaled > 2000) return 2000;
    else return (int16_t)temp_scaled;
}

/* ==========================================================
 * ����ʦ���50K NTC ת������ (�ٶ���Ӧ INU5)
 * ========================================================== */
int16_t adc_to_temperature_50k(uint16_t adc_value) {
    const float V_REF = 3.3f;          
    const uint16_t ADC_MAX = 4095;     
    const float R_REF = 50000.0f;       // ��·���ѹ����̶�Ϊ50k��
    const float R_NTC_NOMINAL = 50000.0f;// ��50K̽ͷ��25��ʱ�ı����ֵ
    const float T_NOMINAL = 298.15f;   
    const float B_VALUE = 3950.0f;     

    if (adc_value > ADC_MAX) adc_value = ADC_MAX;
    
    float voltage = (adc_value * V_REF) / ADC_MAX;
    if (voltage < 0.001f) return 1000;  
    
    float r_ntc = ( voltage / (V_REF - voltage)) * R_REF;
    float inv_t = (1.0f / T_NOMINAL) + (log(r_ntc / R_NTC_NOMINAL) / B_VALUE);
    float temp_celsius = (1.0f / inv_t) - 273.15f;

    int32_t temp_scaled = (int32_t)(temp_celsius * 10.0f + 0.5f);  
    if (temp_scaled < -1000) return -1000;
    else if (temp_scaled > 2000) return 2000;
    else return (int16_t)temp_scaled;
}

/* ==========================================================
 * �������̣߳�����GET��ӡһ���¶�
 * ========================================================== */
void Task_ADC_Process(void const *argument) {
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, 2);
    vTaskDelay(pdMS_TO_TICKS(500)); 
    
    for(;;) {
        // ��������¶�
        int16_t t_10k_raw = adc_to_temperature_10k(adc_buffer[0]); 
        int16_t t_50k_raw = adc_to_temperature_50k(adc_buffer[1]); 
        
        // 2. �������޸ġ���Ҫ��ӡ�ˣ�ֱ�Ӱ���ʵ�¶ȴ浽��ȫ�ֹ���塱�ϣ�
        g_temp_10k = t_10k_raw / 10.0f;
        g_temp_50k = t_50k_raw / 10.0f;
        
			  SysVarData_t temp_sensor_data;
        // 1. �ȰѺڰ���ԭ�������ݶ���������ֹ���ǵ�ѹ����Һλ����������д�����ݣ�
        SysState_GetSensor(&temp_sensor_data); 
        
        // 2. �����Ǹղ�������¶ȸ��½�ȥ
        temp_sensor_data.VAR_EVAP_TEMP    = g_temp_10k; // 10K 对应蒸发温度
        temp_sensor_data.VAR_EXHAUST_TEMP = g_temp_50k; // 50K 对应排气温度
        
        // 3. ��ȫ��д��ϵͳ�ڰ壨��ᴥ�����ײ�д�� Mutex �����������԰�ȫ��
        SysState_UpdateSensor(&temp_sensor_data);
			
			
        // 3. ��Ȼ����ӡ�ˣ����¿��Կ�һ�㣬���� 100ms ĬĬˢ��һ��
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}



