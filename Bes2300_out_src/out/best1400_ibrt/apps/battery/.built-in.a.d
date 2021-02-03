
cmd_apps/battery/built-in.a := ( /usr/bin/printf 'create apps/battery/built-in.a\n addmod apps/battery/app_battery.o\n save\nend' | arm-none-eabi-ar -M )
