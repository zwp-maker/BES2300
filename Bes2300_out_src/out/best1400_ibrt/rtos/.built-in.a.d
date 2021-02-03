
cmd_rtos/built-in.a := ( /usr/bin/printf 'create rtos/built-in.a\n addmod rtos/rtos_lib.o\n save\nend' | arm-none-eabi-ar -M )
