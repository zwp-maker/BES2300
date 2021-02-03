
cmd_apps/audioplayers/a2dp_decoder/built-in.a := ( /usr/bin/printf 'create apps/audioplayers/a2dp_decoder/built-in.a\n addmod apps/audioplayers/a2dp_decoder/a2dp_decoder.o,apps/audioplayers/a2dp_decoder/a2dp_decoder_sbc.o,apps/audioplayers/a2dp_decoder/a2dp_decoder_aac_lc.o\n save\nend' | arm-none-eabi-ar -M )
