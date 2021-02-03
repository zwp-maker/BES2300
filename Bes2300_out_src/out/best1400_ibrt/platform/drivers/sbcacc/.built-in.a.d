
cmd_platform/drivers/sbcacc/built-in.a := ( /usr/bin/printf 'create platform/drivers/sbcacc/built-in.a\n addmod platform/drivers/sbcacc/sbcaac_dummy.o\n save\nend' | arm-none-eabi-ar -M )
