#include "bsp_relay.h"

/* ===========================================================================
 * BSP 继电器/输出控制 + 变频器输入信号 驱动
 * =========================================================================== */

void BSP_Relay_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能时钟 (PA, PB) */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* --- 输出引脚: 压缩机继电器 PA8, 油壳加热器 PA9 --- */
    HAL_GPIO_WritePin(COMP_RELAY_PORT, COMP_RELAY_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(OIL_HEATER_PORT, OIL_HEATER_PIN, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin   = COMP_RELAY_PIN | OIL_HEATER_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* --- 输入引脚: 变频器过流 PB0, 过热 PB1, VAC缺相 PB2 --- */
    GPIO_InitStruct.Pin   = INV_OC_PIN | INV_OT_PIN | VAC_PHASE_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;  /* 默认低电平, 高电平表示故障 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* === 压缩机继电器 === */
void BSP_Comp_On(void)
{
    HAL_GPIO_WritePin(COMP_RELAY_PORT, COMP_RELAY_PIN, GPIO_PIN_SET);
}

void BSP_Comp_Off(void)
{
    HAL_GPIO_WritePin(COMP_RELAY_PORT, COMP_RELAY_PIN, GPIO_PIN_RESET);
}

bool BSP_Comp_IsOn(void)
{
    return (HAL_GPIO_ReadPin(COMP_RELAY_PORT, COMP_RELAY_PIN) == GPIO_PIN_SET);
}

/* === 油壳加热器 === */
void BSP_OilHeater_On(void)
{
    HAL_GPIO_WritePin(OIL_HEATER_PORT, OIL_HEATER_PIN, GPIO_PIN_SET);
}

void BSP_OilHeater_Off(void)
{
    HAL_GPIO_WritePin(OIL_HEATER_PORT, OIL_HEATER_PIN, GPIO_PIN_RESET);
}

bool BSP_OilHeater_IsOn(void)
{
    return (HAL_GPIO_ReadPin(OIL_HEATER_PORT, OIL_HEATER_PIN) == GPIO_PIN_SET);
}

/* === 变频器/电源状态读取 (高电平=故障) === */
bool BSP_INV_IsOverCurrent(void)
{
    return (HAL_GPIO_ReadPin(INV_OC_PORT, INV_OC_PIN) == GPIO_PIN_SET);
}

bool BSP_INV_IsOverHeat(void)
{
    return (HAL_GPIO_ReadPin(INV_OT_PORT, INV_OT_PIN) == GPIO_PIN_SET);
}

bool BSP_VAC_IsPhaseFault(void)
{
    return (HAL_GPIO_ReadPin(VAC_PHASE_PORT, VAC_PHASE_PIN) == GPIO_PIN_SET);
}
