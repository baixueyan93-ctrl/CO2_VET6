#include "FreeRTOS.h"
#include "task.h"
#include "bsp_htc_2k.h"
// 如果您想在这个任务里控制外设，可以 #include "bsp_led.h"

// 将您原来的全局变量移到这里 (后续可以放入 sys_state 统一管理)
float g_env_temp  = -5.0f;   // 模拟环境温度
float g_set_limit = -5.0f;   // 默认设定阈值
uint8_t g_mode    = 0;       // 0: 监控模式, 1: 设置模式

void Task_Panel_Process(void const *argument) {
    uint8_t key_val = 0;
    
    // 1. 硬件初始化
    BSP_HTC2K_Init();
    
    // 2. 进入 RTOS 独立线程死循环
    for(;;) {
        // ================= 任务1：按键处理 =================
        key_val = BSP_HTC2K_ReadKeys();
        
        if (key_val != 0x00 && key_val != 0xFF) {
            
            // --- [Set键] 切换 设置/监控 模式 ---
            if (key_val == KEY_CODE_SET) {
                if (g_mode == 0) {
                    g_mode = 1; 
                    g_IconSet.bits.Set = 1; 
                } else {
                    g_mode = 0; 
                    g_IconSet.bits.Set = 0; 
                }
            }
            
            // --- [上/下键] 仅在设置模式有效 ---
            if (g_mode == 1) {
                if (key_val == KEY_CODE_UP)   g_set_limit += 0.5f; 
                if (key_val == KEY_CODE_DOWN) g_set_limit -= 0.5f; 
                if(g_set_limit > 30.0f) g_set_limit = 30.0f;
                if(g_set_limit < -30.0f) g_set_limit = -30.0f;
            }
            
            // --- [Rst键] 恢复出厂设置 ---
            if (key_val == KEY_CODE_RST) {
                g_mode = 0; 
                g_IconSet.bits.Set = 0;
                g_set_limit = -5.0f; 
            }
            
            // 防按键抖动延时
            vTaskDelay(pdMS_TO_TICKS(200)); 
        }

        // ================= 任务2：显示与控制图标 =================
        if (g_mode == 0) {
            // [监控模式] 图标控制逻辑
            if (g_env_temp > g_set_limit) {
                g_IconSet.bits.Ref = 1;  
                g_IconSet.bits.Fan = 1;  
                g_IconSet.bits.Heat = 0; 
            } else {
                g_IconSet.bits.Ref = 0;
                g_IconSet.bits.Fan = 0;
                g_IconSet.bits.Heat = 1; 
            }
            BSP_HTC2K_ShowTemp(g_env_temp);
        } else {
            // [设置模式] 
            BSP_HTC2K_ShowTemp(g_set_limit); 
        }

        // ================= 【极度关键】：让出 CPU！ =================
        // 原来的 delay_ms(50) 变成了绝对安全的 RTOS 延时
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}




