#include "task_ntc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "bsp_ntc.h"
#include "bsp_rs485.h"
#include <stdio.h>

/* 全局NTC数据, 供其他任务读取 */
static NTC_Data_t g_ntc_data = {0};

/**
 * @brief  获取最新NTC温度数据 (供外部调用)
 */
NTC_Data_t Task_NTC_GetData(void)
{
    return g_ntc_data;
}

/**
 * @brief  NTC温度采集任务主函数
 *         每1秒读取一次两路NTC, 并通过RS485打印结果
 */
void Task_NTC_Process(void const *argument)
{
    char msg[128];

    /* 初始化NTC (ADC + GPIO) */
    BSP_NTC_Init();
    osDelay(100);  /* 等待ADC稳定 */

    BSP_RS485_SendString("\r\n--- NTC Task Started ---\r\n");

    for (;;) {
        /* 读取两路NTC */
        BSP_NTC_ReadAll(&g_ntc_data);

        /* 通过RS485输出温度信息 */
        sprintf(msg, "[NTC] High:%.1fC (ADC:%d) | Low:%.1fC (ADC:%d)\r\n",
                g_ntc_data.temp_high, g_ntc_data.adc_high,
                g_ntc_data.temp_low,  g_ntc_data.adc_low);
        BSP_RS485_SendString(msg);

        /* 每1秒采集一次 */
        osDelay(1000);
    }
}
