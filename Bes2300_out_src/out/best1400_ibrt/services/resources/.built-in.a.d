
cmd_services/resources/built-in.a := ( /usr/bin/printf 'create services/resources/built-in.a\n addmod services/resources/resources.o\n save\nend' | arm-none-eabi-ar -M )
