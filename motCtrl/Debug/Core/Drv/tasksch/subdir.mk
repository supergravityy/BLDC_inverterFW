################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Drv/tasksch/tasksch.c \
../Core/Drv/tasksch/tasksch_config.c 

OBJS += \
./Core/Drv/tasksch/tasksch.o \
./Core/Drv/tasksch/tasksch_config.o 

C_DEPS += \
./Core/Drv/tasksch/tasksch.d \
./Core/Drv/tasksch/tasksch_config.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Drv/tasksch/%.o Core/Drv/tasksch/%.su Core/Drv/tasksch/%.cyclo: ../Core/Drv/tasksch/%.c Core/Drv/tasksch/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F767xx -c -I../Core/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Drv-2f-tasksch

clean-Core-2f-Drv-2f-tasksch:
	-$(RM) ./Core/Drv/tasksch/tasksch.cyclo ./Core/Drv/tasksch/tasksch.d ./Core/Drv/tasksch/tasksch.o ./Core/Drv/tasksch/tasksch.su ./Core/Drv/tasksch/tasksch_config.cyclo ./Core/Drv/tasksch/tasksch_config.d ./Core/Drv/tasksch/tasksch_config.o ./Core/Drv/tasksch/tasksch_config.su

.PHONY: clean-Core-2f-Drv-2f-tasksch

