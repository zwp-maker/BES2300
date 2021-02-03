
cmd_config/built-in.a := ( /usr/bin/printf 'create config/built-in.a\n addmod config/best1400_ibrt/tgt_hardware.o\n save\nend' | arm-none-eabi-ar -M )
