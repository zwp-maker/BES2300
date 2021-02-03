
cmd_platform/drivers/ana/built-in.a := ( /usr/bin/printf 'create platform/drivers/ana/built-in.a\n addmod \n addlib platform/drivers/ana/best1400/built-in.a\nsave\nend' | arm-none-eabi-ar -M )
