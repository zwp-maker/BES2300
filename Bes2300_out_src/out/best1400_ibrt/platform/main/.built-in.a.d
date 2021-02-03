
cmd_platform/main/built-in.a := ( /usr/bin/printf 'create platform/main/built-in.a\n addmod platform/main/startup_main.o,platform/main/main.o\n addlib platform/main/../../utils/hwtimer_list/built-in.a\nsave\nend' | arm-none-eabi-ar -M )
