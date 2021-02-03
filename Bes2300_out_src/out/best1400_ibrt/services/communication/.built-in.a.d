
cmd_services/communication/built-in.a := ( /usr/bin/printf 'create services/communication/built-in.a\n addmod services/communication/communication_svr.o\n save\nend' | arm-none-eabi-ar -M )
