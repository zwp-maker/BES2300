
cmd_services/bt_if_enhanced/built-in.a := ( /usr/bin/printf 'create services/bt_if_enhanced/built-in.a\n addmod \n addlib services/bt_if_enhanced/ibrt_libbt_api_sbc_enc_aac_2m.a\nsave\nend' | arm-none-eabi-ar -M )
