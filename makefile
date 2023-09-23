##############################################################################
#
# Makefile for stm32f401re_hello
#
##############################################################################
# Licensed under the Apache License, Version 2.0 (the License); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

##############################################################################
# Variable Assignments
##############################################################################
# Assign paths
REPO_PATH = $(abspath ./)
BUILD_PATH = $(REPO_PATH)/build
STM32CUBEF4_PATH = $(REPO_PATH)/st/STM32CubeF4

# Assign toolchain
CC          = arm-none-eabi-gcc
LD          = arm-none-eabi-ld
AR          = arm-none-eabi-ar
AS          = arm-none-eabi-gcc
OBJCOPY     = arm-none-eabi-objcopy
OBJDUMP     = arm-none-eabi-objdump
SIZE        = arm-none-eabi-size

# Assign flags
DEBUG_OPTIONS = -DDEBUG -g3
CFLAGS =
CFLAGS += -Werror -Wall
CFLAGS += --std=gnu11
CFLAGS += -fno-diagnostics-show-caret
CFLAGS += -fdata-sections -ffunction-sections -fstack-usage
CFLAGS += -frecord-gcc-switches
CFLAGS += -c
CFLAGS += -O0
CFLAGS += $(DEBUG_OPTIONS)
CFLAGS += -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nano.specs
CFLAGS += -DUSE_HAL_DRIVER -DSTM32F401xE
CFLAGS += -DNO_OS

ASMFLAGS =
ASMFLAGS += -c -x assembler-with-cpp
ASMFLAGS += $(DEBUG_OPTIONS)
ASMFLAGS += -mcpu=cortex-m4 -c -x assembler-with-cpp --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb

LDFLAGS =
LDFLAGS += -Wl,-Map="$(BUILD_PATH)/stm32f401re_hello.map"
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -static
LDFLAGS += -Wl,--start-group -lc -lm -Wl,--end-group
LDFLAGS += -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nosys.specs --specs=nano.specs
LDFLAGS += -T"$(STM32CUBEF4_PATH)/Projects/STM32F401RE-Nucleo/Applications/EEPROM/EEPROM_Emulation/SW4STM32/STM32F4xx-Nucleo/STM32F401RETx_FLASH.ld"

# Assign build components
C_SRCS =
C_SRCS += $(REPO_PATH)/main.c
C_SRCS += $(REPO_PATH)/bsp.c
C_SRCS += $(REPO_PATH)/syscalls.c
C_SRCS += $(REPO_PATH)/st/stm32f4xx_it.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c

C_OBJS = $(subst $(REPO_PATH),$(BUILD_PATH),$(patsubst %.c, %.o, $(C_SRCS)))

ASM_SRCS = $(STM32CUBEF4_PATH)/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f401xe.s

ASM_OBJS = $(subst $(REPO_PATH),$(BUILD_PATH),$(patsubst %.s, %.o, $(ASM_SRCS)))

INCLUDES =
INCLUDES += -I.
INCLUDES += -Ist
INCLUDES += -I$(STM32CUBEF4_PATH)/Drivers/CMSIS/Core/Include
INCLUDES += -I$(STM32CUBEF4_PATH)/Drivers/CMSIS/Device/ST/STM32F4xx/Include
INCLUDES += -I$(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Inc

BUILD_PATHS = $(sort $(dir $(C_OBJS)))
BUILD_PATHS += $(sort $(dir $(ASM_OBJS)))

##############################################################################
# Function Assignments
##############################################################################
# Create a target for each .o files, depending on its corresponding .c file
define c_obj_rule
$(1): $(2)
	@echo -------------------------------------------------------------------------------
	@echo COMPILING $(2)
	$(CC) $(CFLAGS) $(INCLUDES) $(2) -o $(1)
endef

# Create a target for each .o file, depending on its corresponding .s file
define asm_obj_rule
$(1): $(2)
	@echo -------------------------------------------------------------------------------
	@echo ASSEMBLING $(2)
	$(AS) $(ASMFLAGS) $(INCLUDES) $(2) -o $(1)
endef

##############################################################################
# Target Rules
##############################################################################
.PHONY: default all clean

default: all

all: stm32f401re_hello

$(BUILD_PATHS):
	mkdir -p $@

$(eval $(call add_build_dir_rules, $(BUILD_PATH), $(BUILD_PATHS)))
$(foreach obj,$(C_OBJS),$(eval $(call c_obj_rule,$(obj),$(subst $(BUILD_PATH),$(REPO_PATH),$(obj:.o=.c)))))
$(foreach obj,$(ASM_OBJS),$(eval $(call asm_obj_rule,$(obj),$(subst $(BUILD_PATH),$(REPO_PATH),$(obj:.o=.s)))))

stm32f401re_hello: $(BUILD_PATHS) $(C_OBJS) $(ASM_OBJS)
	@echo -------------------------------------------------------------------------------
	@echo LINKING $@
	$(CC) $(LDFLAGS) $(ASM_OBJS) $(C_OBJS) -o $(BUILD_PATH)/stm32f401re_hello.elf
	@echo -------------------------------------------------------------------------------
	@echo SIZE of $(BUILD_PATH)/stm32f401re_hello.elf
	$(SIZE) -t $(BUILD_PATH)/stm32f401re_hello.elf

clean:
	rm -rf ./build