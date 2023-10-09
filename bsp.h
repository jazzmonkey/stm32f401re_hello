/**
 * @file bsp.h
 *
 * @brief Functions and prototypes exported by the BSP module
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

#ifndef BSP_H
#define BSP_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "stm32f4xx_hal.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
/**
 * @brief Return values for all public and most private API calls
 *
 */
#define BSP_STATUS_OK               (0)
#define BSP_STATUS_FAIL             (1)

/**
 * @brief GPIO states
 *
 * @see bsp_set_gpio
 *
 */
#define BSP_GPIO_LOW                (0)
#define BSP_GPIO_HIGH               (1)

#define BSP_PB_ID_USER                  (0)
#define BSP_GPIO_ID_LD2                 (0)

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
/**
 * Callback type for BSP
 *
 * @param [in] status           Result of BSP call
 * @param [in] arg              Argument registered when BSP call was issued
 *
 * @return none
 *
 */
typedef void (*bsp_callback_t)(uint32_t status, void* arg);

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t bsp_init(void);
uint32_t bsp_set_timer(uint32_t duration_ms, bsp_callback_t cb, void *cb_arg);
uint32_t bsp_set_gpio(uint32_t gpio_id, uint8_t gpio_state);
uint32_t bsp_register_user_pb_cb(bsp_callback_t cb, void *cb_arg);
uint32_t bsp_register_getchar_cb(bsp_callback_t cb, void *cb_arg);
void bsp_sleep(void);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // BSP_H
