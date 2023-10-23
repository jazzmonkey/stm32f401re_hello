/**
 * @file cli.c
 *
 * @brief Implementation of the CLI
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
#include <stdio.h>
#include "cli.h"
#include "bsp.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define CLI_STATE_INIT      (0)
#define CLI_STATE_IDLE      (1)
#define CLI_STATE_BUSY      (2)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint32_t cli_state = CLI_STATE_INIT;
//static char cli_comamds[3] = {'t', 'l', 'e'};
static volatile bool cli_getchar = false;
static char cli_current_command = '0';
static cli_callback_t l_command_cb = NULL;
static void *l_command_cb_arg = NULL;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
static void cli_getchar_callback(uint32_t status, void *arg)
{
    cli_getchar = true;

    return;
}

static void cli_print_commands(void)
{
    printf("Commands:\n\r");
    printf("t = Print temperature (deg C)\n\r");
    printf("l = toggle LED blink pattern\n\r");
    printf("e = Echo print until Return\n\r");
    printf("? = Print command list\n\r");

    return;
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t cli_init(void)
{
    printf("stm32f401re_hello\n\r\n\r");
    cli_print_commands();
    printf("\n\r>");

    bsp_register_getchar_cb(cli_getchar_callback, NULL);

    cli_state = CLI_STATE_IDLE;

    return CLI_OK;
}

uint32_t cli_get_state(uint32_t *state)
{
    *state = cli_state;

    return CLI_OK;
}

uint32_t cli_step(void)
{
    bool temp_bool;
    __disable_irq();
    temp_bool = cli_getchar;
    __enable_irq();

    if (temp_bool)
    {
        cli_getchar = false;
        char temp_char = getchar();

        // Handle BUSY state
        if (cli_state == CLI_STATE_BUSY)
        {
            if (cli_current_command == 'e')
            {
                if  (temp_char == '0')
                {
                    cli_state = CLI_STATE_IDLE;
                }
                else
                {
                    printf("%c", temp_char);
                }
            }
        }

        // Handle IDLE state
        else if (cli_state == CLI_STATE_IDLE)
        {
            printf("\n\r");
            switch (temp_char)
            {
                case 't':
                {
                    uint32_t temp = 0;
                    bsp_read_temp(&temp);
                    printf("temp = %d\n\r", (int) temp);
                    cli_state = CLI_STATE_IDLE;
                    break;
                }

                case 'e':
                    cli_current_command = temp_char;
                    cli_state = CLI_STATE_BUSY;
                    break;

                case 'l':
                    l_command_cb(CLI_OK, l_command_cb_arg);
                    cli_state = CLI_STATE_IDLE;
                    break;

                case '?':
                    cli_print_commands();
                    cli_state = CLI_STATE_IDLE;
                    break;

                default:
                    printf("Unknown command: '%c'\n\r", temp_char);
                    cli_state = CLI_STATE_IDLE;
                    break;
            }
        }

        if (cli_state == CLI_STATE_IDLE)
        {
            printf("\n\r>");
        }
    }

    return CLI_OK;
}

uint32_t cli_command_register_callback(char command, cli_callback_t cb, void *cb_arg)
{
    // Check for valid command
    if (command == 'l')
    {
        l_command_cb = cb;
        l_command_cb_arg = cb_arg;
    }
    else
    {
        return CLI_FAIL;
    }

    // Check for NULL pointers
    if (cb == NULL)
    {
        return CLI_FAIL;
    }

    return CLI_OK;
}