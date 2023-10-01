/**
 * @file main.c
 *
 * @brief The main function for CS40L25 System Test Harness
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
#include "bsp.h"
#include <stddef.h>
#include <stdlib.h>

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define APP_STATE_BLINK_LONG_ON     (0)
#define APP_STATE_BLINK_LONG_OFF    (1)
#define APP_STATE_MAX               (2)

#define APP_LD2_SHORT_DELAY_MS      (150)
#define APP_LD2_LONG_DELAY_MS       (650)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static bool bsp_pb_pressed = false;
static volatile bool app_timeout = false;
static volatile bool app_getchar = false;
static bool app_ld2_state_on = false;
static uint32_t app_state = 0;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
void app_pb_pressed_callback(uint32_t status, void *arg)
{
    if (status == BSP_STATUS_OK)
    {
        bsp_pb_pressed = !bsp_pb_pressed;
    }
    else
    {
        exit(1);
    }

    return;
}

void app_timeout_callback(uint32_t status, void *arg)
{
    app_timeout = true;

    return;
}

void app_getchar_callback(uint32_t status, void *arg)
{
    app_getchar = true;

    return;
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

int main(void)
{
    int ret_val = 0;

    bsp_init();
    bsp_register_user_pb_cb(app_pb_pressed_callback, NULL);
    bsp_register_getchar_cb(app_getchar_callback, NULL);
    bsp_set_timer(500, app_timeout_callback, NULL);
    printf("\n\rHello world!\n\r");

    while (1)
    {
        if (bsp_pb_pressed)
        {
            switch (app_state)
            {
                case APP_STATE_BLINK_LONG_ON:
                    break;

                case APP_STATE_BLINK_LONG_OFF:
                    break;

                default:
                    break;
            }

            app_state++;
            app_state %= APP_STATE_MAX;

            bsp_pb_pressed = false;

        }

        if (app_getchar)
        {
            printf("%c", getchar());
            app_getchar = false;
        }

        if (app_timeout)
        {
            uint32_t timer_delay_ms;
            app_timeout = false;

            if (app_ld2_state_on)
            {
                bsp_set_gpio(BSP_GPIO_ID_LD2, BSP_GPIO_HIGH);
                if (app_state == APP_STATE_BLINK_LONG_ON)
                {
                    timer_delay_ms = APP_LD2_LONG_DELAY_MS;
                }
                else
                {
                    timer_delay_ms = APP_LD2_SHORT_DELAY_MS;
                }
            }
            else
            {
                bsp_set_gpio(BSP_GPIO_ID_LD2, BSP_GPIO_LOW);
                if (app_state == APP_STATE_BLINK_LONG_ON)
                {
                    timer_delay_ms = APP_LD2_SHORT_DELAY_MS;
                }
                else
                {
                    timer_delay_ms = APP_LD2_LONG_DELAY_MS;
                }
            }

            bsp_set_timer(timer_delay_ms, app_timeout_callback, NULL);
            app_ld2_state_on = !app_ld2_state_on;

        }

        bsp_sleep();
    }

    exit(1);

    return ret_val;
}
