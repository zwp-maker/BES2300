
cmd_utils/boot_struct/built-in.a := ( /usr/bin/printf 'create utils/boot_struct/built-in.a\n addmod utils/boot_struct/boot_struct.o\n save\nend' | arm-none-eabi-ar -M )
