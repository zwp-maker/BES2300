
cmd_apps/main/built-in.a := ( /usr/bin/printf 'create apps/main/built-in.a\n addmod apps/main/apps_tester.o,apps/main/apps.o,apps/main/../../config/_default_cfg_src_/app_status_ind.o\n save\nend' | arm-none-eabi-ar -M )
