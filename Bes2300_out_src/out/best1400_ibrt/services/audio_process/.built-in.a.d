
cmd_services/audio_process/built-in.a := ( /usr/bin/printf 'create services/audio_process/built-in.a\n addmod services/audio_process/audio_cfg.o,services/audio_process/audio_process.o,services/audio_process/audio_spectrum.o\n save\nend' | arm-none-eabi-ar -M )
