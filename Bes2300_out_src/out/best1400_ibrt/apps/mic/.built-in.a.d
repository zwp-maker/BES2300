
cmd_apps/mic/built-in.a := ( /usr/bin/printf 'create apps/mic/built-in.a\n addmod apps/mic/app_mic.o\n save\nend' | arm-none-eabi-ar -M )
