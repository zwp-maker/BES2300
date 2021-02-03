
cmd_services/audio_dump/built-in.a := ( /usr/bin/printf 'create services/audio_dump/built-in.a\n addmod services/audio_dump/src/audio_dump.o\n save\nend' | arm-none-eabi-ar -M )
