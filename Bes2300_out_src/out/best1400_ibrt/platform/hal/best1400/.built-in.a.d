
cmd_platform/hal/best1400/built-in.a := ( /usr/bin/printf 'create platform/hal/best1400/built-in.a\n addmod platform/hal/best1400/hal_analogif_best1400.o,platform/hal/best1400/hal_cmu_best1400.o,platform/hal/best1400/hal_codec_best1400.o,platform/hal/best1400/hal_iomux_best1400.o\n save\nend' | arm-none-eabi-ar -M )
