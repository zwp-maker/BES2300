
cmd_platform/drivers/btpcm/built-in.a := ( /usr/bin/printf 'create platform/drivers/btpcm/built-in.a\n addmod platform/drivers/btpcm/btpcm.o\n save\nend' | arm-none-eabi-ar -M )
