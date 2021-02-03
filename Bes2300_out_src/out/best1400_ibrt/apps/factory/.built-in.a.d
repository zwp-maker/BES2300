
cmd_apps/factory/built-in.a := ( /usr/bin/printf 'create apps/factory/built-in.a\n addmod apps/factory/sys_api_cdc_comm.o,apps/factory/app_factory_cdc_comm.o,apps/factory/app_factory_audio.o,apps/factory/app_factory_bt.o,apps/factory/app_factory.o\n save\nend' | arm-none-eabi-ar -M )
