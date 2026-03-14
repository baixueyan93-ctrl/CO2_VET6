#include "task_XKC_Y20_V.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sys_state.h"       // 引入数据黑板
#include "task_rs485_log.h"  // 引入RS485串口发送
#include "main.h"            // 引入 PD7 (SW53_SENSOR_Pin) 的定义
#include <stdio.h>
#include "bsp_rs485.h"



void Task_XKC_Y20_V_Process(void const *argument) {
    // 记录上一次的状态，只在状态发生【变化】时才打印，防止串口被刷屏
    uint8_t last_state = 255; 
    
    for(;;) {
        uint8_t current_state = 0;
        
        // ==================================================
        // 1. 极简读取：直接读 PD7 的引脚电平！
        // ==================================================
        // 如果读到高电平 (硬件降压后的 ~3V)
        if (HAL_GPIO_ReadPin(SW53_SENSOR_GPIO_Port, SW53_SENSOR_Pin) == GPIO_PIN_SET) {
            current_state = 1; // 传感器输出高，判定为有水/开关开启
        } 
        // 如果读到低电平 (0V)
        else {
            current_state = 0; // 传感器输出低，判定为缺水/开关关闭
        }
        
        // ==================================================
        // 2. 状态变化时，直接通过 RS485 打印出来证明开关好用！
        // ==================================================
        if (current_state != last_state) {
            char debug_msg[64];
            if (current_state == 1) {
                sprintf(debug_msg, "\r\n[SW53 TEST] -> High Level (1) : Water Detected!\r\n");
            } else {
                sprintf(debug_msg, "\r\n[SW53 TEST] -> Low Level (0) : No Water!\r\n");
            }
            
            // 直接发给电脑串口助手看现象
            BSP_RS485_SendString(debug_msg); 
            
            // 更新记忆
            last_state = current_state;
            
            // 顺手更新到系统黑板里 (保证 GET 指令也能查到)
            SysSensorData_t new_data = {0};
            SysState_GetSensor(&new_data); 
            new_data.Liquid_Level = current_state; 
            SysState_UpdateSensor(&new_data); 
        }

        // 每 50 毫秒扫描一次引脚
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}



