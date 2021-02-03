
cmd_services/osif/built-in.a := ( /usr/bin/printf 'create services/osif/built-in.a\n addmod services/osif/ddbif_bes.o,services/osif/osif_rtx.o\n save\nend' | arm-none-eabi-ar -M )
