#include "task_panel.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_htc_2k.h"
#include "sys_state.h"   // 【新增】引入系统黑板
#include "sys_config.h"  // 【新增】引入参数配置 (包含迟滞参数 C_TEMP_HYST_C1)

float g_env_temp  = -5.0f;   // 运行时的真实环境温度
float g_set_limit = -5.0f;   // 默认设定阈值
uint8_t g_mode    = 0;       // 0: 监控模式, 1: 设置模式

void Task_Panel_Process(void const *argument) {
    uint8_t key_val = 0;
    
    // 1. 硬件初始化
    BSP_HTC2K_Init();
    
    // 2. 进入 RTOS 独立线程死循环
    for(;;) {
        // ================= 【问题3修复】去黑板上抄真实温度 =================
        SysVarData_t sensor_data;
        SysState_GetSensor(&sensor_data);
        g_env_temp = sensor_data.VAR_CABINET_TEMP; // 拿到 10K 的真实蒸发器温度！
        
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
            // 【问题13修复】：加入迟滞算法！防止继电器频繁“吧嗒”响
            if (g_env_temp > (g_set_limit + SET_TEMP_HYST_C1)) {
                g_IconSet.bits.Ref = 1;  
                g_IconSet.bits.Fan = 1;  
                g_IconSet.bits.Heat = 0; 
            } else if (g_env_temp < (g_set_limit - SET_TEMP_HYST_C1)) {
                g_IconSet.bits.Ref = 0;
                g_IconSet.bits.Fan = 0;
                g_IconSet.bits.Heat = 1; 
            }
            BSP_HTC2K_ShowTemp(g_env_temp);
        } else {
            // [设置模式] 
            BSP_HTC2K_ShowTemp(g_set_limit); 
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}



