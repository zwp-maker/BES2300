
cmd_platform/drivers/codec/built-in.a := ( /usr/bin/printf 'create platform/drivers/codec/built-in.a\n addmod platform/drivers/codec/codec_tlv32aic32.o\n addlib platform/drivers/codec/best1400/built-in.a\nsave\nend' | arm-none-eabi-ar -M )
