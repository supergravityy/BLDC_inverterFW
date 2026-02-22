################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Drv/sensing/sensing.c 

OBJS += \
./Core/Drv/sensing/sensing.o 

C_DEPS += \
./Core/Drv/sensing/sensing.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Drv/sensing/%.o Core/Drv/sensing/%.su Core/Drv/sensing/%.cyclo: ../Core/Drv/sensing/%.c Core/Drv/sensing/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F767xx -c -I../Core/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Drv-2f-sensing

clean-Core-2f-Drv-2f-sensing:
	-$(RM) ./Core/Drv/sensing/sensing.cyclo ./Core/Drv/sensing/sensing.d ./Core/Drv/sensing/sensing.o ./Core/Drv/sensing/sensing.su

.PHONY: clean-Core-2f-Drv-2f-sensing

