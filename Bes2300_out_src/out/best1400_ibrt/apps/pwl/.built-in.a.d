
cmd_apps/pwl/built-in.a := ( /usr/bin/printf 'create apps/pwl/built-in.a\n addmod apps/pwl/app_pwl.o\n save\nend' | arm-none-eabi-ar -M )
