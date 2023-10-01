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
    - putty (for COM port)
3.  Saleae Logic software
4.  Hardware Target:  STM32 Nucleo-64 with STM32F401RE
    - https://www.st.com/en/evaluation-tools/nucleo-f401re.html
5.  Build commands (from root of cloned repo):
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

3.  Added printf capabilities, but nothing out the UART
- Added syscalls.c and overrode _write(), calling into bsp.c:__io_putchar()
- Added calls to UART HAL code and implemented ISR handlers
- Added UART TX FIFO handling
- But nothing captured by Logic16 out of UART
    - Checked that call stack from printf->...->UART HAL TX code is working
    - Checked that UART TX FIFO is being populated correctly with printf() data
    - Found that UART TX IRQ was not firing
    - Sanity check - check that Logic16 is capturing, so set to GPIO for LD2 to capture toggle
    - when i only put BP in UART TX IRQ, it fires
    - I put analog channel of Logic16 and found 1V - contention with USB-COM port attached
    - I still don't capture anything on UART TX
    - Switched to probing off CN3.RX (routed to MCU USART2 TX) instead of CN10.35
    - Found that SB62/SB63 are not populated by default, so PA2/PA3 is not probe-able on CN10

4.  If UART TX buffer smaller than printf() string, only length of buffer is TX

5.  UART RX causes TX to not work
- no IRQs fired
- RX and TX FIFOs in default state
- discovered that HAL_UART_GetState() will return HAL_UART_STATE_BUSY_RX if waiting for RX char, so added that to acceptable states in __io_putchar call to HAL_UART_GetState()

6.  UART RX does not work
- sending from host ttyUSB0 via PuTTy
- no RX IRQ fired
- captured on Logic16 analog - contention on UART RX / PA3
    - PA3 shows 3.3v without ttyUSB0 TX connected
    - capturing from open ttyUSB0 TX shows correct data from PuTTy
- HAL GPIO configuration for PA3 looks correct
- VCC from USB-UART dongle is 2.7V
- Switched PuTTy to ttyACM0 - Virtual COM port exposed by ST-Link USB that also routes to PA2/3 for USART2 peripheral
- still no UART RX IRQ
- removed erroneous HAL GPIO configuration for PA3

7.  UART RX still does not work
- UART RX IRQ is firing and capturing correct character from putty
- stepped through code to find that getchar() was always returning -1, even though UART RX IRQ was firing and __io_getchar was updating the FIFO correctly
- looked like things were being buffered from syscalls to getchar/scanf
- had to call setvbuf and set to unbuffered for stdin and stdout

