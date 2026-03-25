#include "task_panel.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_htc_2k.h"
#include "sys_state.h"   // ������������ϵͳ�ڰ�
#include "sys_config.h"  // ������������������� (�������Ͳ��� C_TEMP_HYST_C1)

float g_env_temp  = -5.0f;   // ����ʱ����ʵ�����¶�
float g_set_limit = -5.0f;   // Ĭ���趨��ֵ
uint8_t g_mode    = 0;       // 0: ���ģʽ, 1: ����ģʽ

static float    s_last_disp_temp = -999.0f;  // 上次显示的温度
static uint8_t  s_last_disp_icon = 0xFF;     // 上次显示的图标

void Task_Panel_Process(void const *argument) {
    uint8_t key_val = 0;
    
    // 1. 硬件初始化
    BSP_HTC2K_Init();
    vTaskDelay(pdMS_TO_TICKS(100));  // 等TM1637上电稳定

    // ★ 开机自检：强制全亮显示 "888"，持续2秒
    BSP_HTC2K_TestDisplay();
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 2. 进入 RTOS 主线程大循环
    for(;;) {
        // ================= ������3�޸���ȥ�ڰ��ϳ���ʵ�¶� =================
        SysVarData_t sensor_data;
        SysState_GetSensor(&sensor_data);
        g_env_temp = sensor_data.VAR_CABINET_TEMP; // �õ� 10K ����ʵ�������¶ȣ�
        
        // ================= ����1���������� =================
        key_val = BSP_HTC2K_ReadKeys();
        
        if (key_val != 0x00 && key_val != 0xFF) {
            
            // --- [Set��] �л� ����/��� ģʽ ---
            if (key_val == KEY_CODE_SET) {
                if (g_mode == 0) {
                    g_mode = 1;
                    g_IconSet.bits.Set = 1;
                } else {
                    g_mode = 0;
                    g_IconSet.bits.Set = 0;
                }
                s_last_disp_temp = -999.0f; // 强制刷新
            }
            
            // --- [��/�¼�] ��������ģʽ��Ч ---
            if (g_mode == 1) {
                if (key_val == KEY_CODE_UP)   g_set_limit += 0.5f; 
                if (key_val == KEY_CODE_DOWN) g_set_limit -= 0.5f; 
                if(g_set_limit > 30.0f) g_set_limit = 30.0f;
                if(g_set_limit < -30.0f) g_set_limit = -30.0f;
            }
            
            // --- [Rst��] �ָ��������� ---
            if (key_val == KEY_CODE_RST) {
                g_mode = 0;
                g_IconSet.bits.Set = 0;
                g_set_limit = -5.0f;
                s_last_disp_temp = -999.0f; // 强制刷新
            }
            
            // ������������ʱ
            vTaskDelay(pdMS_TO_TICKS(200)); 
        }

        // ================= ����2����ʾ�����ͼ�� =================
        if (g_mode == 0) {
            // [���ģʽ] ͼ������߼�
            // ������13�޸�������������㷨����ֹ�̵���Ƶ������ડ���
            if (g_env_temp > (g_set_limit + SET_TEMP_HYST_C1)) {
                g_IconSet.bits.Ref = 1;  
                g_IconSet.bits.Fan = 1;  
                g_IconSet.bits.Heat = 0; 
            } else if (g_env_temp < (g_set_limit - SET_TEMP_HYST_C1)) {
                g_IconSet.bits.Ref = 0;
                g_IconSet.bits.Fan = 0;
                g_IconSet.bits.Heat = 1; 
            }
            float cur = g_env_temp;
            if (cur != s_last_disp_temp || g_IconSet.byte != s_last_disp_icon) {
                BSP_HTC2K_ShowTemp(cur);
                s_last_disp_temp = cur;
                s_last_disp_icon = g_IconSet.byte;
            }
        } else {
            // [����ģʽ]
            float cur = g_set_limit;
            if (cur != s_last_disp_temp || g_IconSet.byte != s_last_disp_icon) {
                BSP_HTC2K_ShowTemp(cur);
                s_last_disp_temp = cur;
                s_last_disp_icon = g_IconSet.byte;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}



