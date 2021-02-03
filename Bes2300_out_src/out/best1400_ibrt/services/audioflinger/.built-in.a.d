
cmd_services/audioflinger/built-in.a := ( /usr/bin/printf 'create services/audioflinger/built-in.a\n addmod services/audioflinger/audioflinger.o\n save\nend' | arm-none-eabi-ar -M )
