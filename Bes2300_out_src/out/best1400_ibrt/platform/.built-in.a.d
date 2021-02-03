
cmd_platform/built-in.a := ( /usr/bin/printf 'create platform/built-in.a\n addmod \n addlib platform/cmsis/built-in.a\n addlib platform/drivers/built-in.a\n addlib platform/hal/built-in.a\n addlib platform/main/built-in.a\nsave\nend' | arm-none-eabi-ar -M )
