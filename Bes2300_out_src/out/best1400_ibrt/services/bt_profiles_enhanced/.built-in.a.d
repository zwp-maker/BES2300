
cmd_services/bt_profiles_enhanced/built-in.a := ( /usr/bin/printf 'create services/bt_profiles_enhanced/built-in.a\n addmod \n addlib services/bt_profiles_enhanced/ibrt_libbt_profiles_sbc_enc_romaac_2m.a\nsave\nend' | arm-none-eabi-ar -M )
