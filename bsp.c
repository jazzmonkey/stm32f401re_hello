/**
 * @file bsp.c
 *
 * @brief Implementation of the BSP
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdlib.h>
#include "bsp.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define BSP_EXTI_PB_USER_PRIO                       (0xF)
#define BSP_TIM2_PREPRIO                            (0x4)
#define USART2_IRQ_PREPRIO                          (0xE)

#define BSP_TIM2_STATE_RESET                        (0x0)
#define BSP_TIM2_STATE_FIRST_CB                     (0x1)
#define BSP_TIM2_STATE_TIMEOUT                      (0x2)

#define BSP_UART_TX_BUFFER_SIZE_BYTES               (1024)
#define BSP_UART_RX_BUFFER_SIZE_BYTES               (128)

typedef struct
{
    uint32_t size;
    uint32_t in_index;
    uint32_t out_index;
    uint32_t level;
    uint32_t level_pending;
    uint8_t *buffer;
} bsp_char_fifo_t;

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static volatile int32_t bsp_irq_count = 0;

static bsp_callback_t bsp_timer_cb = NULL;
static void *bsp_timer_cb_arg = NULL;
static uint8_t bsp_tim2_state = BSP_TIM2_STATE_RESET;
static volatile uint32_t timer_callback_counter = 0;

static bsp_callback_t bsp_user_pb_cb = NULL;
static void* bsp_user_pb_cb_arg = NULL;

static bsp_callback_t bsp_getchar_cb = NULL;
static void* bsp_getchar_cb_arg = NULL;

uint8_t bsp_uart_tx_buffer[BSP_UART_TX_BUFFER_SIZE_BYTES] = {0};
bsp_char_fifo_t bsp_uart_tx_fifo = {0};
uint8_t bsp_uart_rx_buffer[BSP_UART_RX_BUFFER_SIZE_BYTES] = {0};
bsp_char_fifo_t bsp_uart_rx_fifo = {0};

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
TIM_HandleTypeDef tim_drv_handle;
EXTI_HandleTypeDef exti_user_pb_handle;
UART_HandleTypeDef uart_drv_handle;

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
static void bsp_error_handler(void)
{
    while(1);

    return;
}

static void bsp_system_clock_config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /**
      * @brief  System Clock Configuration
      *         The system Clock is configured as follow :
      *            System Clock source            = PLL (HSI)
      *            SYSCLK(Hz)                     = 84000000
      *            HCLK(Hz)                       = 84000000
      *            AHB Prescaler                  = 1
      *            APB1 Prescaler                 = 2
      *            APB2 Prescaler                 = 1
      *            HSI Frequency(Hz)              = 16000000
      *            PLL_M                          = 16
      *            PLL_N                          = 336
      *            PLL_P                          = 4
      *            PLL_Q                          = 7
      *            VDD(V)                         = 3.3
      *            Main regulator output voltage  = Scale2 mode
      *            Flash Latency(WS)              = 2
      * @param  None
      * @retval None
      */

    /* Enable Power Control clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is
       clocked below the maximum system frequency, to update the voltage scaling value
       regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    /* Enable HSI Oscillator and activate PLL with HSI as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        bsp_error_handler();
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
       clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | \
                                   RCC_CLOCKTYPE_HCLK | \
                                   RCC_CLOCKTYPE_PCLK1 | \
                                   RCC_CLOCKTYPE_PCLK2);
    //RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        bsp_error_handler();
    }

    return;
}

static void bsp_tim2_init(void)
{
    uint32_t uwPrescalerValue;
    /*##-1- Configure the TIM peripheral #######################################*/
     /* -----------------------------------------------------------------------
       In this example TIM2 input clock (TIM2CLK) is set to 2 * APB1 clock (PCLK1),
       since APB1 prescaler is different from 1.
         TIM2CLK = 2 * PCLK1
         PCLK1 = HCLK / 2
         => TIM2CLK = HCLK = SystemCoreClock
       To get TIM2 counter clock at 10 KHz, the Prescaler is computed as following:
       Prescaler = (TIM2CLK / TIM2 counter clock) - 1
       Prescaler = (SystemCoreClock /10 KHz) - 1

       Note:
        SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f4xx.c file.
        Each time the core clock (HCLK) changes, user had to update SystemCoreClock
        variable value. Otherwise, any configuration based on this variable will be incorrect.
        This variable is updated in three ways:
         1) by calling CMSIS function SystemCoreClockUpdate()
         2) by calling HAL API function HAL_RCC_GetSysClockFreq()
         3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
     ----------------------------------------------------------------------- */

    /* Compute the prescaler value to have TIM2 counter clock equal to 10 KHz */
    uwPrescalerValue = (uint32_t) ((SystemCoreClock / 10000) - 1);

    /* Set TIMx instance */
    tim_drv_handle.Instance = TIM2;

    /* Initialize TIM2 peripheral as follow:
         + Period = 10000 - 1
         + Prescaler = ((SystemCoreClock/2)/10000) - 1
         + ClockDivision = 0
         + Counter direction = Up
    */
    tim_drv_handle.Init.Period = 10000 - 1;
    tim_drv_handle.Init.Prescaler = uwPrescalerValue;
    tim_drv_handle.Init.ClockDivision = 0;
    tim_drv_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    tim_drv_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    return;
}

static void bsp_tim2_start(uint32_t delay_100us)
{
    tim_drv_handle.Init.Period = delay_100us;
    if(HAL_TIM_Base_Init(&tim_drv_handle) != HAL_OK)
    {
        bsp_error_handler();
    }

    if(HAL_TIM_Base_Start_IT(&tim_drv_handle) != HAL_OK)
    {
        bsp_error_handler();
    }

    return;
}

static void bsp_tim2_stop(void)
{
    if(HAL_TIM_Base_Stop_IT(&tim_drv_handle) != HAL_OK)
    {
        bsp_error_handler();
    }

    if(HAL_TIM_Base_DeInit(&tim_drv_handle) != HAL_OK)
    {
        bsp_error_handler();
    }

    return;
}

static void bsp_exti_user_pb_cb(void)
{
    if (bsp_user_pb_cb != NULL)
    {
        bsp_user_pb_cb(BSP_STATUS_OK, bsp_user_pb_cb_arg);
    }

    bsp_irq_count++;

    return;
}

static void bsp_uart_init(void)
{
    uart_drv_handle.Instance          = USART2;
    uart_drv_handle.Init.BaudRate     = 115200;
    uart_drv_handle.Init.WordLength   = UART_WORDLENGTH_8B;
    uart_drv_handle.Init.StopBits     = UART_STOPBITS_1;
    uart_drv_handle.Init.Parity       = UART_PARITY_NONE;
    uart_drv_handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    uart_drv_handle.Init.Mode         = UART_MODE_TX_RX;
    uart_drv_handle.Init.OverSampling = UART_OVERSAMPLING_16;

    if(HAL_UART_Init(&uart_drv_handle) != HAL_OK)
    {
        bsp_error_handler();
    }

    bsp_uart_tx_fifo.buffer = bsp_uart_tx_buffer;
    bsp_uart_tx_fifo.size = BSP_UART_TX_BUFFER_SIZE_BYTES;

    bsp_uart_rx_fifo.buffer = bsp_uart_rx_buffer;
    bsp_uart_rx_fifo.size = BSP_UART_RX_BUFFER_SIZE_BYTES;

    // Setup UART to Receive
    HAL_UART_Receive_IT(&uart_drv_handle, (bsp_uart_rx_fifo.buffer + bsp_uart_rx_fifo.in_index), 1);

    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    return;
}

/***********************************************************************************************************************
 * MCU HAL FUNCTIONS
 *
 * @warning - Function names below are expected in STM32 HAL code, do not change
 **********************************************************************************************************************/
void HAL_MspInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    EXTI_ConfigTypeDef exti_config = {0};

    // Enable clocks to ports used
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Configure the LD2 GPO
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure the User PB GPI
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // Configure EXTI for User PB GPI
    exti_config.Line = EXTI_LINE_13;
    exti_config.Mode = EXTI_MODE_INTERRUPT;
    exti_config.Trigger = EXTI_TRIGGER_FALLING;
    exti_config.GPIOSel = EXTI_GPIOC;
    HAL_EXTI_SetConfigLine(&exti_user_pb_handle, &exti_config);
    HAL_EXTI_RegisterCallback(&exti_user_pb_handle, HAL_EXTI_COMMON_CB_ID, &bsp_exti_user_pb_cb);
    HAL_NVIC_SetPriority((IRQn_Type) EXTI15_10_IRQn, BSP_EXTI_PB_USER_PRIO, 0x00);
    HAL_NVIC_EnableIRQ((IRQn_Type) EXTI15_10_IRQn);

    return;
}

void HAL_MspDeInit(void)
{
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_13);

    __HAL_RCC_GPIOA_CLK_DISABLE();
    __HAL_RCC_GPIOC_CLK_DISABLE();

    return;
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        __HAL_RCC_TIM2_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM2_IRQn, BSP_TIM2_PREPRIO, 0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn);
    }

    return;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        bsp_tim2_state++;
        if (bsp_tim2_state == BSP_TIM2_STATE_TIMEOUT)
        {
            if(HAL_TIM_Base_Stop_IT(&tim_drv_handle) != HAL_OK)
            {
                bsp_error_handler();
            }

            if (bsp_timer_cb != NULL)
            {
                bsp_timer_cb(BSP_STATUS_OK, bsp_timer_cb_arg);
                bsp_timer_cb = NULL;
                bsp_timer_cb_arg = NULL;
            }
        }
    }

    bsp_irq_count++;

    return;
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    __HAL_RCC_GPIOA_CLK_ENABLE();

    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitStruct.Pin       = GPIO_PIN_2;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(USART2_IRQn, USART2_IRQ_PREPRIO, 1);
    HAL_NVIC_EnableIRQ(USART2_IRQn);

    return;
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    __HAL_RCC_USART2_FORCE_RESET();
    __HAL_RCC_USART2_RELEASE_RESET();

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2);
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_3);

    HAL_NVIC_DisableIRQ(USART2_IRQn);

    return;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
    if (UartHandle->Instance == USART2)
    {
        // Increment out_index by number of chars transferred
        bsp_uart_tx_fifo.level -= uart_drv_handle.TxXferSize;
        bsp_uart_tx_fifo.out_index += uart_drv_handle.TxXferSize;
        if (bsp_uart_tx_fifo.out_index >= BSP_UART_TX_BUFFER_SIZE_BYTES)
        {
            bsp_uart_tx_fifo.out_index = 0;
        }

        // Check if buffer is empty.  if not, send either:
        // 1.  number of chars until end of FIFO buffer, or
        // 2.  total number of chars remaining
        if (bsp_uart_tx_fifo.level > 0)
        {
            uint32_t tx_size = 0;

            if (bsp_uart_tx_fifo.in_index < bsp_uart_tx_fifo.out_index)
            {
                tx_size = BSP_UART_TX_BUFFER_SIZE_BYTES - bsp_uart_tx_fifo.out_index;
            }
            else
            {
                tx_size = bsp_uart_tx_fifo.in_index - bsp_uart_tx_fifo.out_index;
            }

            HAL_UART_Transmit_IT(&uart_drv_handle, (bsp_uart_tx_fifo.buffer + bsp_uart_tx_fifo.out_index), tx_size);
        }
    }

    bsp_irq_count++;

    return;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
    if (UartHandle->Instance == USART2)
    {
        // Update fifo
        bsp_uart_rx_fifo.in_index++;
        bsp_uart_rx_fifo.in_index %= bsp_uart_rx_fifo.size;
        bsp_uart_rx_fifo.level++;

        // If fifo is not full
        if (bsp_uart_rx_fifo.level < bsp_uart_rx_fifo.size)
        {
            HAL_UART_Receive_IT(&uart_drv_handle, (bsp_uart_rx_fifo.buffer + bsp_uart_rx_fifo.in_index), 1);
        }

        if (bsp_getchar_cb != NULL)
        {
            bsp_getchar_cb(BSP_STATUS_OK, bsp_getchar_cb_arg);
        }
    }

    bsp_irq_count++;

    return;
}

 void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle)
{
    if (UartHandle->Instance == USART2)
    {
        bsp_error_handler();
    }

    bsp_irq_count++;

    return;
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t bsp_init(void)
{
    HAL_Init();
    bsp_system_clock_config();
    bsp_tim2_init();
    bsp_uart_init();

    bsp_set_gpio(BSP_GPIO_ID_LD2, BSP_GPIO_LOW);

    return BSP_STATUS_OK;
}

uint32_t bsp_set_timer(uint32_t duration_ms, bsp_callback_t cb, void *cb_arg)
{
    bsp_timer_cb = cb;
    bsp_timer_cb_arg = cb_arg;

    bsp_tim2_stop();

    bsp_tim2_state = BSP_TIM2_STATE_RESET;

    bsp_tim2_start(duration_ms * 10);
    if (cb == NULL)
    {
        uint8_t temp_tim2_state = BSP_TIM2_STATE_RESET;
        while (temp_tim2_state != BSP_TIM2_STATE_TIMEOUT)
        {
            __disable_irq();
            temp_tim2_state = bsp_tim2_state;
            __enable_irq();
        }
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_set_gpio(uint32_t gpio_id, uint8_t gpio_state)
{
    switch (gpio_id)
    {
        case BSP_GPIO_ID_LD2:
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, (GPIO_PinState) gpio_state);
            break;

        default:
            break;
    }

    return BSP_STATUS_OK;
}

void bsp_sleep(void)
{
    __disable_irq();
    bsp_irq_count--;

    if (bsp_irq_count <= 0)
    {
        bsp_irq_count = 0;
        __enable_irq();
        __WFI();
    }
    else
    {
        __enable_irq();
    }

    return;
}

uint32_t bsp_register_user_pb_cb(bsp_callback_t cb, void *cb_arg)
{
    bsp_user_pb_cb = cb;
    bsp_user_pb_cb_arg = cb_arg;

    return BSP_STATUS_OK;
}

int __io_putchar(int ch)
{
    int ret = ch;

    // If buffer is not full
    if (bsp_uart_tx_fifo.level < bsp_uart_tx_fifo.size)
    {
        uint32_t hal_ret;

        // Add ch to buffer
        bsp_uart_tx_fifo.buffer[bsp_uart_tx_fifo.in_index] = ch;

        //If UART is not currently transmitting, kick off transmiting of buffer with just this single byte
        hal_ret = HAL_UART_GetState(&uart_drv_handle);
        if ((hal_ret == HAL_UART_STATE_READY) || (hal_ret == HAL_UART_STATE_BUSY_RX))
        {
            hal_ret = HAL_UART_Transmit_IT(&uart_drv_handle, &(bsp_uart_tx_fifo.buffer[bsp_uart_tx_fifo.in_index]), 1);
            if (hal_ret == HAL_OK)
            {
                bsp_uart_tx_fifo.in_index++;
                bsp_uart_tx_fifo.level++;
            }
            else
            {
                errno = EIO;
                ret = EOF;
            }
        }
        else
        {
            bsp_uart_tx_fifo.in_index++;
            bsp_uart_tx_fifo.level++;
        }
    }
    else
    {
        errno = EIO;
        ret = EOF;
    }

    return ret;
}

int __io_getchar(void)
{
    int32_t ret = EOF;

    __disable_irq();

    if (bsp_uart_rx_fifo.level > 0)
    {
        ret = bsp_uart_rx_fifo.buffer[bsp_uart_rx_fifo.out_index++];
        bsp_uart_rx_fifo.out_index %= bsp_uart_rx_fifo.size;
        bsp_uart_rx_fifo.level--;
    }
    else
    {
        errno = 0;
    }

    __enable_irq();

    return ret;
}

uint32_t bsp_register_getchar_cb(bsp_callback_t cb, void *cb_arg)
{
    bsp_getchar_cb = cb;
    bsp_getchar_cb_arg = cb_arg;

    return BSP_STATUS_OK;
}