/**
 * @file cli.h
 *
 * @brief Functions and prototypes exported by the CLI module
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

#ifndef CLI_H
#define CLI_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
/**
 * @brief Return values for all public and most private API calls
 *
 */
#define CLI_OK      (0)
#define CLI_FAIL    (1)

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
/**
 * Callback type for CLI
 *
 * @param [in] status           Result of CLI call
 * @param [in] arg              Argument registered when CLI call was issued
 *
 * @return none
 *
 */
typedef void (*cli_callback_t)(uint32_t status, void* arg);

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t cli_init(void);
uint32_t cli_get_state(uint32_t *state);
uint32_t cli_step(void);
uint32_t cli_command_register_callback(char command, cli_callback_t cb, void *cb_arg);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CLI_H
