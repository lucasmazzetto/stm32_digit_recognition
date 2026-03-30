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
./Inference/Src/convnet.o \
./Inference/Src/nn.o \
./Inference/Src/params.o 

C_DEPS += \
./Inference/Src/convnet.d \
./Inference/Src/nn.d \
./Inference/Src/params.d 


# Each subdirectory must supply rules for building sources it contributes
Inference/Src/convnet.o: /mnt/Shared/Projects/stm32_digit_recognition/external/quantized_digit_recognition/src/convnet.c Inference/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I/mnt/Shared/Projects/stm32_digit_recognition/firmware/NUCLEO_F446RE/../../external/quantized_digit_recognition/include -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Inference/Src/nn.o: /mnt/Shared/Projects/stm32_digit_recognition/external/quantized_digit_recognition/src/nn.c Inference/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I/mnt/Shared/Projects/stm32_digit_recognition/firmware/NUCLEO_F446RE/../../external/quantized_digit_recognition/include -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Inference/Src/params.o: /mnt/Shared/Projects/stm32_digit_recognition/external/quantized_digit_recognition/src/params.c Inference/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I/mnt/Shared/Projects/stm32_digit_recognition/firmware/NUCLEO_F446RE/../../external/quantized_digit_recognition/include -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Inference-2f-Src

clean-Inference-2f-Src:
	-$(RM) ./Inference/Src/convnet.cyclo ./Inference/Src/convnet.d ./Inference/Src/convnet.o ./Inference/Src/convnet.su ./Inference/Src/nn.cyclo ./Inference/Src/nn.d ./Inference/Src/nn.o ./Inference/Src/nn.su ./Inference/Src/params.cyclo ./Inference/Src/params.d ./Inference/Src/params.o ./Inference/Src/params.su

.PHONY: clean-Inference-2f-Src

