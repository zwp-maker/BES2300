
cmd_utils/xyzmodem/built-in.a := ( /usr/bin/printf 'create utils/xyzmodem/built-in.a\n addmod utils/xyzmodem/xyzmodem.o\n save\nend' | arm-none-eabi-ar -M )
