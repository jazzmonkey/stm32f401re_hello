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
#define CLI_STATE_RX_CMD    (2)
#define CLI_STATE_BUSY      (3)

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
typedef uint32_t (*command_handler_t)(void *command);

typedef struct
{
    char command;
    cli_callback_t cb;
    void *cb_arg;
    command_handler_t handler;
} cli_command_t;

typedef struct
{
    uint32_t state;
    cli_command_t *current_command;
    cli_command_t *command_list;
    uint8_t total_commands;
    volatile bool char_is_ready;
    char new_char;
} cli_t;


/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint32_t cli_handle_unknown(void *command);
static uint32_t cli_handle_t(void *command);
static uint32_t cli_handle_l(void *command);
static uint32_t cli_handle_e(void *command);
static uint32_t cli_handle_qm(void *command);

static cli_t cli;
static cli_command_t command_list[] =
{
    {' ', NULL, NULL, cli_handle_unknown},
    {'t', NULL, NULL, cli_handle_t},
    {'l', NULL, NULL, cli_handle_l},
    {'e', NULL, NULL, cli_handle_e},
    {'?', NULL, NULL, cli_handle_qm},
};

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
static void cli_getchar_callback(uint32_t status, void *arg)
{
    cli.char_is_ready = true;

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

static uint32_t cli_handle_unknown(void *command)
{
    printf("Unknown command: '%c'\n\r", cli.new_char);

    cli.state = CLI_STATE_IDLE;

    return CLI_OK;
}

static uint32_t cli_handle_t(void *command)
{
    uint32_t temp = 0;
    bsp_read_temp(&temp);
    printf("temp = %d\n\r", (int) temp);

    cli.state = CLI_STATE_IDLE;

    return CLI_OK;
}

static uint32_t cli_handle_l(void *command)
{
    cli_command_t *cmd = (cli_command_t *) command;
    cmd->cb(CLI_OK, cmd->cb_arg);

    cli.state = CLI_STATE_IDLE;

    return CLI_OK;
}

static uint32_t cli_handle_e(void *command)
{
    if (cli.state == CLI_STATE_RX_CMD)
    {
        cli.state = CLI_STATE_BUSY;
    }
    else
    {
        if (cli.new_char == '0')
        {
            cli.state = CLI_STATE_IDLE;
        }
        else
        {
            printf("%c", cli.new_char);
        }
    }

    return CLI_OK;
}

static uint32_t cli_handle_qm(void *command)
{
    cli_print_commands();

    cli.state = CLI_STATE_IDLE;

    return CLI_OK;
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t cli_init(void)
{
    cli.state = CLI_STATE_IDLE;
    cli.current_command = NULL;
    cli.command_list = command_list;
    cli.total_commands = sizeof(command_list)/sizeof(cli_command_t);
    cli.char_is_ready = false;

    printf("stm32f401re_hello\n\r\n\r");
    cli_print_commands();
    printf("\n\r>");

    bsp_register_getchar_cb(cli_getchar_callback, NULL);

    return CLI_OK;
}

uint32_t cli_get_state(uint32_t *state)
{
    *state = cli.state;

    return CLI_OK;
}

uint32_t cli_step(void)
{
    uint32_t ret = CLI_OK;

    bool temp_bool;
    __disable_irq();
    temp_bool = cli.char_is_ready;
    __enable_irq();

    // Get any new character
    if (temp_bool)
    {
        cli.char_is_ready = false;
        cli.new_char = getchar();

        // If CLI SM is IDLE, then Loop through command list to find matching
        if (cli.state == CLI_STATE_IDLE)
        {
            cli.state = CLI_STATE_RX_CMD;
            cli.current_command = NULL;
            for (int i = 0; i < cli.total_commands; i++)
            {
                if (cli.new_char == cli.command_list[i].command)
                {
                    cli.current_command = &(cli.command_list[i]);
                }
            }

            // Check if the command was found, if not assign Unkown handler
            if (cli.current_command == NULL)
            {
                cli.current_command = &(cli.command_list[0]);
            }

            printf("\n\r");
        }

        // current_command will be guaranteed not to be NULL, but check anyway
        if (cli.current_command != NULL)
        {
            ret = cli.current_command->handler(cli.current_command);
            if (cli.state == CLI_STATE_IDLE)
            {
                printf("\n\r>");
            }
        }
    }

    return ret;
}

uint32_t cli_command_register_callback(char command, cli_callback_t cb, void *cb_arg)
{
    for (int i = 0; i < cli.total_commands; i++)
    {
        if (command == cli.command_list[i].command)
        {
            if (cb == NULL)
            {
                return CLI_FAIL;
            }
            else
            {
                cli.command_list[i].cb = cb;
                cli.command_list[i].cb_arg = cb_arg;
            }
        }
    }

    return CLI_OK;
}