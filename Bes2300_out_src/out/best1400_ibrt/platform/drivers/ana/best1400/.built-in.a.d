
cmd_platform/drivers/ana/best1400/built-in.a := ( /usr/bin/printf 'create platform/drivers/ana/best1400/built-in.a\n addmod platform/drivers/ana/best1400/pmu_best1400.o,platform/drivers/ana/best1400/analog_best1400.o\n save\nend' | arm-none-eabi-ar -M )
