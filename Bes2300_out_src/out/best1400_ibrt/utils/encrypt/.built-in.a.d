
cmd_utils/encrypt/built-in.a := ( /usr/bin/printf 'create utils/encrypt/built-in.a\n addmod utils/encrypt/uECC.o,utils/encrypt/_sha256.o,utils/encrypt/aes.o\n save\nend' | arm-none-eabi-ar -M )
