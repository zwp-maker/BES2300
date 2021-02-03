
cmd_services/overlay/built-in.a := ( /usr/bin/printf 'create services/overlay/built-in.a\n addmod services/overlay/app_overlay.o\n save\nend' | arm-none-eabi-ar -M )
