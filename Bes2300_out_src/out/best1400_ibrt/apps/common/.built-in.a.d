
cmd_apps/common/built-in.a := ( /usr/bin/printf 'create apps/common/built-in.a\n addmod apps/common/app_thread.o,apps/common/app_utils.o,apps/common/app_spec_ostimer.o\n save\nend' | arm-none-eabi-ar -M )
