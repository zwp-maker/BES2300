
cmd_services/bt_app/built-in.a := ( /usr/bin/printf 'create services/bt_app/built-in.a\n addmod services/bt_app/besmain.o,services/bt_app/app_ring_merge.o,services/bt_app/app_hfp.o,services/bt_app/app_a2dp_source.o,services/bt_app/app_spp.o,services/bt_app/app_fp_rfcomm.o,services/bt_app/app_a2dp.o,services/bt_app/app_hsp.o,services/bt_app/app_media_player.o,services/bt_app/app_bt_hid.o,services/bt_app/app_bqb_new_profile.o,services/bt_app/app_bt_stream.o,services/bt_app/app_bt.o,services/bt_app/app_keyhandle.o,services/bt_app/app_bt_func.o,services/bt_app/app_bt_media_manager.o,services/bt_app/app_bqb.o,services/bt_app/app_sec.o,services/bt_app/audio_prompt_sbc.o\n addlib services/bt_app/a2dp_codecs/built-in.a\nsave\nend' | arm-none-eabi-ar -M )
