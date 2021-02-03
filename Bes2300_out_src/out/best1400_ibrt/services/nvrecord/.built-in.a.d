
cmd_services/nvrecord/built-in.a := ( /usr/bin/printf 'create services/nvrecord/built-in.a\n addmod services/nvrecord/nvrecord.o,services/nvrecord/nvrecord_ble.o,services/nvrecord/nvrecord_env.o,services/nvrecord/list_ext.o,services/nvrecord/nvrecord_fp_account_key.o,services/nvrecord/nvrec_config.o\n save\nend' | arm-none-eabi-ar -M )
