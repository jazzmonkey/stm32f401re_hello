/**
 * @file stm32f4xx_it.c
 *
 * @brief Interrupt Service Routines.
 *
 * @copyright
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 */
/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "stm32f4xx_hal.h"
#ifdef USE_CMSIS_OS
#include "cmsis_os.h"
#endif

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
extern TIM_HandleTypeDef tim_drv_handle;
extern EXTI_HandleTypeDef exti_user_pb_handle;
extern UART_HandleTypeDef uart_drv_handle;

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&tim_drv_handle);
}

void EXTI15_10_IRQHandler(void)
{
    if (HAL_EXTI_GetPending(&exti_user_pb_handle, EXTI_TRIGGER_RISING))
    {
        HAL_EXTI_IRQHandler(&exti_user_pb_handle);
    }
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&uart_drv_handle);
}