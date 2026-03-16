#include "task_XKC_Y20_V.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sys_state.h"
#include "task_rs485_log.h"
#include "main.h"
#include <stdio.h>
#include "bsp_rs485.h"

// ��Ӧ Arduino �� int liquidLevel = 0;
int liquidLevel = 0; 

void Task_XKC_Y20_V_Process(void const *argument) {
    vTaskDelay(pdMS_TO_TICKS(500)); 
    
    char msg[64];

    // =========================================================
    // ���ռ�ǿ�нӹ� PD7 Ӳ�����á�
    // ���� CubeMX ��ô�䣬�����Ƿ� LCD �ٳ֣�������ǿ�ж�ؿ���Ȩ��
    // =========================================================
    __HAL_RCC_GPIOD_CLK_ENABLE(); // 1. ǿ�п��� GPIOD ʱ�ӣ����û����һ�䣬���Զ��� 0��
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_7;       // ���� PD7
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // ǿ����Ϊ��������ģʽ
    GPIO_InitStruct.Pull = GPIO_NOPULL;     // ǿ����Ϊ���գ������߲����ͣ�
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct); // ������Ч��
    // =========================================================

    for(;;) {
        // 1. �����ȡ
        if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_7) == GPIO_PIN_SET) {
            liquidLevel = 1; 
        } else {
            liquidLevel = 0; 
        }

        sprintf(msg, "liquidLevel= %d\r\n", liquidLevel);
        BSP_RS485_SendString(msg);

        // 3. ˳�ָ��µ�ϵͳ���ݺڰ壨Ϊ�˼������� GET ָ�
        SysVarData_t sensor_data = {0};
        SysState_GetSensor(&sensor_data);
        sensor_data.VAR_LIQUID_LEVEL = liquidLevel;
        SysState_UpdateSensor(&sensor_data);

        // 4. ��Ӧ delay(500);
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}



