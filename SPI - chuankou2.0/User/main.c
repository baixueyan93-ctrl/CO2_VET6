/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK) & 你的名字
 * @version     V2.0
 * @date        2026-01-21
 * @brief       HTC-2K 屏幕 + 内部温度传感器 + 按键扫描 综合实验
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "htc_2k.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h" 

// ================= 1. 按键键值定义 =================
#define KEY_CODE_SET   0xF4  // Set键
#define KEY_CODE_UP    0xF5  // 上键
#define KEY_CODE_DOWN  0xF6  // 下键
#define KEY_CODE_RST   0xF7  // Rst键 (复位键)

// ================= 2. 全局变量 =================
// 上电默认显示 0.0，等待串口数据刷新
float g_env_temp  = 0.0f;    
// 默认设定阈值 -5.0 度
float g_set_limit = -5.0f;   
// 0: 监控模式, 1: 设置模式
uint8_t g_mode    = 0;       

// 闪烁计数器
uint8_t blink_cnt = 0; 

int main(void)
{
    // 硬件初始化
    HAL_Init();                         
    sys_stm32_clock_init(336, 8, 2, 7); 
    delay_init(168);                    
    usart_init(115200);
    led_init();                         
    lcd_init();         
    
    HTC2K_Init(); // 屏幕初始化
    
    printf("System Ready. Waiting for Temp Data...\r\n");
    
    // LCD 提示
    lcd_show_string(30, 50, 200, 16, 16, "Thermostat System", RED);
    lcd_show_string(30, 80, 200, 16, 16, "Mode: MONITOR", BLUE);
    
    uint8_t key_val = 0;

    while (1)
    {
        // ================= 任务1：串口接收 (模拟传感器) =================
        if (g_usart_rx_sta & 0x8000)
        {
            uint8_t len = g_usart_rx_sta & 0x3FFF;
            g_usart_rx_buf[len] = '\0';
            
            float temp_input = atof((char*)g_usart_rx_buf);
            
            // 数据安全过滤
            if (temp_input > -50.0f && temp_input < 100.0f) {
                g_env_temp = temp_input;
                printf(">> Update: %.1f C\r\n", g_env_temp);
            } else {
                printf(">> Error: Data Invalid.\r\n");
            }

            g_usart_rx_sta = 0; 
        }

        // ================= 任务2：按键处理 =================
        key_val = HTC2K_ReadKeys();
        
        if (key_val != 0x00 && key_val != 0xFF)
        {
            printf("Key: 0x%02X\r\n", key_val); 
            LED0_TOGGLE(); 
            
            // --- [Set键] 切换 设置/监控 模式 ---
            if (key_val == KEY_CODE_SET) 
            {
                if (g_mode == 0) {
                    g_mode = 1; // 进设置
                    g_IconSet.bits.Set = 1; // Set灯亮
                    printf(">> Mode: SETTING\r\n");
                    lcd_show_string(30, 80, 200, 16, 16, "Mode: SETTING ", RED);
                } 
                else {
                    g_mode = 0; // 保存并退出
                    g_IconSet.bits.Set = 0; // Set灯灭
                    printf(">> Mode: MONITOR\r\n");
                    lcd_show_string(30, 80, 200, 16, 16, "Mode: MONITOR ", BLUE);
                }
            }
            
            // --- [上/下键] 仅在设置模式有效 ---
            if (g_mode == 1)
            {
                if (key_val == KEY_CODE_UP)   g_set_limit += 0.5f; 
                if (key_val == KEY_CODE_DOWN) g_set_limit -= 0.5f; 
                
                // 限制设置范围 (-30 ~ 30)
                if(g_set_limit > 30.0f) g_set_limit = 30.0f;
                if(g_set_limit < -30.0f) g_set_limit = -30.0f;
                
                printf("Setting: %.1f\r\n", g_set_limit);
            }
            
            // --- [Rst键] 恢复出厂设置 ---
            if (key_val == KEY_CODE_RST)
            {
                // 1. 强制退出设置模式
                g_mode = 0; 
                g_IconSet.bits.Set = 0;
                
                // 2. 【核心功能】重置设定值为默认值 (-5.0)
                g_set_limit = -5.0f; 
                
                printf(">> !!! FACTORY RESET !!! Limit -> -5.0\r\n");
                lcd_show_string(30, 80, 200, 16, 16, "Mode: RESET DONE", BLUE);
            }
            
            delay_ms(200); 
        }

        // ================= 任务3：显示与控制 =================
        
        if (g_mode == 0) 
        {
            // === [监控模式] ===
            
            // 控制逻辑
            if (g_env_temp > g_set_limit) {
                // 环境太热 -> 制冷 (雪花+风扇)
                g_IconSet.bits.Ref = 1;  
                g_IconSet.bits.Fan = 1;  
                g_IconSet.bits.Heat = 0; 
            } 
            else {
                // 环境太冷 -> 加热 (太阳)
                g_IconSet.bits.Ref = 0;
                g_IconSet.bits.Fan = 0;
                g_IconSet.bits.Heat = 1; 
            }
            
            // 显示环境温度
            HTC2K_ShowTemp(g_env_temp);
        }
        else 
        {
            // === [设置模式] ===
            
            // 闪烁提示
            blink_cnt++;
            if(blink_cnt > 10) blink_cnt = 0;
            
            if(blink_cnt < 6) {
                HTC2K_ShowTemp(g_set_limit); 
            } else {
                // 简单的视觉暂留刷新，保持数字显示稳定，只靠Set灯提示
                HTC2K_ShowTemp(g_set_limit); 
            }
        }

        delay_ms(50);
    }
}





