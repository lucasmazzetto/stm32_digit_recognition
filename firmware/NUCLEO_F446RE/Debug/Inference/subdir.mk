################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/mnt/Shared/Projects/stm32_digit_recognition/external/quantized_digit_recognition/src/convnet.c \
/mnt/Shared/Projects/stm32_digit_recognition/external/quantized_digit_recognition/src/nn.c \
/mnt/Shared/Projects/stm32_digit_recognition/external/quantized_digit_recognition/src/params.c 

OBJS += \
./Inference/convnet.o \
./Inference/nn.o \
./Inference/params.o 

C_DEPS += \
./Inference/convnet.d \
./Inference/nn.d \
./Inference/params.d 


# Each subdirectory must supply rules for building sources it contributes
Inference/convnet.o: /mnt/Shared/Projects/stm32_digit_recognition/external/quantized_digit_recognition/src/convnet.c Inference/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I/mnt/Shared/Projects/stm32_digit_recognition/firmware/NUCLEO_F446RE/../../external/quantized_digit_recognition/include -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Inference/nn.o: /mnt/Shared/Projects/stm32_digit_recognition/external/quantized_digit_recognition/src/nn.c Inference/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I/mnt/Shared/Projects/stm32_digit_recognition/firmware/NUCLEO_F446RE/../../external/quantized_digit_recognition/include -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Inference/params.o: /mnt/Shared/Projects/stm32_digit_recognition/external/quantized_digit_recognition/src/params.c Inference/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I/mnt/Shared/Projects/stm32_digit_recognition/firmware/NUCLEO_F446RE/../../external/quantized_digit_recognition/include -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Inference

clean-Inference:
	-$(RM) ./Inference/convnet.cyclo ./Inference/convnet.d ./Inference/convnet.o ./Inference/convnet.su ./Inference/nn.cyclo ./Inference/nn.d ./Inference/nn.o ./Inference/nn.su ./Inference/params.cyclo ./Inference/params.d ./Inference/params.o ./Inference/params.su

.PHONY: clean-Inference

