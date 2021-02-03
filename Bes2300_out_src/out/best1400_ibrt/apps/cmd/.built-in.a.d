
cmd_apps/cmd/built-in.a := ( /usr/bin/printf 'create apps/cmd/built-in.a\n addmod apps/cmd/app_cmd.o\n save\nend' | arm-none-eabi-ar -M )
