################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Drv/mtrCtrl/mtrCtrl.c 

OBJS += \
./Core/Drv/mtrCtrl/mtrCtrl.o 

C_DEPS += \
./Core/Drv/mtrCtrl/mtrCtrl.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Drv/mtrCtrl/%.o Core/Drv/mtrCtrl/%.su Core/Drv/mtrCtrl/%.cyclo: ../Core/Drv/mtrCtrl/%.c Core/Drv/mtrCtrl/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F767xx -c -I../Core/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Drv-2f-mtrCtrl

clean-Core-2f-Drv-2f-mtrCtrl:
	-$(RM) ./Core/Drv/mtrCtrl/mtrCtrl.cyclo ./Core/Drv/mtrCtrl/mtrCtrl.d ./Core/Drv/mtrCtrl/mtrCtrl.o ./Core/Drv/mtrCtrl/mtrCtrl.su

.PHONY: clean-Core-2f-Drv-2f-mtrCtrl

