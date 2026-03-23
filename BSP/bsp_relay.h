#ifndef BSP_RELAY_H
#define BSP_RELAY_H

#include "main.h"
#include <stdbool.h>

/* ===========================================================================
 * BSP 继电器/输出控制 + 变频器输入信号 驱动
 *
 * 引脚分配 (请根据 V13 原理图实际修改):
 *   压缩机继电器输出:  PA8   (推挽输出, 高电平有效)
 *   油壳加热器输出:    PA9   (推挽输出, 高电平有效)
 *   变频器过流输入:    PB0   (浮空输入, 高电平=过流)
 *   变频器过热输入:    PB1   (浮空输入, 高电平=过热)
 *   VAC缺相输入:       PB2   (浮空输入, 高电平=缺相)
 * =========================================================================== */

/* --- 输出引脚定义 --- */
#define COMP_RELAY_PORT     GPIOA
#define COMP_RELAY_PIN      GPIO_PIN_8

#define OIL_HEATER_PORT     GPIOA
#define OIL_HEATER_PIN      GPIO_PIN_9

/* --- 输入引脚定义 (变频器/电源状态) --- */
#define INV_OC_PORT         GPIOB       /* 变频器过流 */
#define INV_OC_PIN          GPIO_PIN_0

#define INV_OT_PORT         GPIOB       /* 变频器过热 */
#define INV_OT_PIN          GPIO_PIN_1

#define VAC_PHASE_PORT      GPIOB       /* VAC缺相/错相 */
#define VAC_PHASE_PIN       GPIO_PIN_2

/* === 初始化 === */
void BSP_Relay_Init(void);

/* === 压缩机继电器 === */
void BSP_Comp_On(void);
void BSP_Comp_Off(void);
bool BSP_Comp_IsOn(void);

/* === 油壳加热器 === */
void BSP_OilHeater_On(void);
void BSP_OilHeater_Off(void);
bool BSP_OilHeater_IsOn(void);

/* === 变频器/电源状态读取 === */
bool BSP_INV_IsOverCurrent(void);   /* 变频器过流? */
bool BSP_INV_IsOverHeat(void);      /* 变频器过热? */
bool BSP_VAC_IsPhaseFault(void);    /* VAC错/断相? */

#endif /* BSP_RELAY_H */
