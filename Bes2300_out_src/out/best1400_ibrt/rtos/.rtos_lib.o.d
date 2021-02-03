
cmd_rtos/rtos_lib.o := arm-none-eabi-gcc -Wl,-r,--whole-archive -nostdlib -nostartfiles  -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nano.specs -o rtos/rtos_lib.o rtos/rtx/TARGET_CORTEX_M/built-in.a
