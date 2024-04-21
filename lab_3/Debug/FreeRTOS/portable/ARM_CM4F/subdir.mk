################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FreeRTOS/portable/ARM_CM4F/port.c 

OBJS += \
./FreeRTOS/portable/ARM_CM4F/port.o 

C_DEPS += \
./FreeRTOS/portable/ARM_CM4F/port.d 


# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/portable/ARM_CM4F/%.o FreeRTOS/portable/ARM_CM4F/%.su: ../FreeRTOS/portable/ARM_CM4F/%.c FreeRTOS/portable/ARM_CM4F/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/louis/STM32CubeIDE/workspace_1.11.0/lab_3/FreeRTOS/include" -I"C:/Users/louis/STM32CubeIDE/workspace_1.11.0/lab_3/FreeRTOS/portable/ARM_CM4F" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-FreeRTOS-2f-portable-2f-ARM_CM4F

clean-FreeRTOS-2f-portable-2f-ARM_CM4F:
	-$(RM) ./FreeRTOS/portable/ARM_CM4F/port.d ./FreeRTOS/portable/ARM_CM4F/port.o ./FreeRTOS/portable/ARM_CM4F/port.su

.PHONY: clean-FreeRTOS-2f-portable-2f-ARM_CM4F

