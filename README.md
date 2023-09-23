# stm32f401re_hello
ST Nucleo-F401RE - Blink LED, process PB, UART TX

# Development Environment
1.  Ubuntu 22.04
2.  Additional packages:
    - git
    - openocd
    - gcc-arm-none-eabi
    - binutils-arm-none-eabi
    - gdb-multiarch
    - libnewlib-arm-none-eabi
3.  Hardware Target:  STM32 Nucleo-64 with STM32F401RE
    - https://www.st.com/en/evaluation-tools/nucleo-f401re.html
4.  Build commands (from root of cloned repo):
    - make
    - openocd -f ./openocd.cfg
    - gdb-multiarch -x ./gdb.txt

# Revision History

## Added processing PB and blinking LED; added STM32CubeF4 submodule

### Issues Debugged
1.  EXT Interrupt for User PB always firing
- Looked up schematic and made sure PB had pullup/down
- Traced from app_pb_pressed_callback to EXTI interrupt handler - always firing - flag not getting cleared?
- Searched in MCU RM and UG to find exact address of GPIOC.13 input data register
- Validated in GDB that value of input data register reflected state of push button
- Something seemed to get corrupted after calling bsp_sleep, so commented out - bingo

2.  Tied LED blinking to set_timer(), but didn't work
- Set bp at TIM2 IRQ handler - not breaking
- Walk through timer init starting in bsp_init
    - Verify data structures are init correctly
    - Walk through TIM init and start HAL code
    - Verify peripheral address correct
    - Found that writes to timer peripheral were not taking effect
        - Either:
            1.  Not enabled/powerd up, or
            2.  Doesn't have clocks
        - Found that I had to add call to HAL_TIM_Base_Init() in Timer_Init()
- Root cause - first theory
    - HAL_TIM_Base_Stop_IT will set htim->State = HAL_TIM_STATE_READY
    - When HAL_TIM_Base_Init called in Timer_Start, call to HAL_TIM_Base_MspInit() is ignored because only called if (htim->State == HAL_TIM_STATE_RESET)
    - Did not resolve issue
- Root case - confirmed
    - Added counter to HAL_TIM_PeriodElapsedCallback - it's called 2x after call to Timer_Start
    - had to include if (bsp_timer_has_started) logic in IRQ handler
