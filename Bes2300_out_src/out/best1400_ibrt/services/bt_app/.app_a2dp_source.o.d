services/bt_app/app_a2dp_source.o: \
 ../../services/bt_app/app_a2dp_source.cpp

cmd_services/bt_app/app_a2dp_source.o := arm-none-eabi-g++ -MD -MP -MF services/bt_app/.app_a2dp_source.o.d   -I../../services/bt_app  -I../../platform/cmsis/inc  -I../../services/audioflinger  -I../../platform/hal  -I../../services/fs/  -I../../services/fs/sd  -I../../services/fs/fat  -I../../services/fs/fat/ChaN -DCHARGER_PLUGINOUT_RESET=0 -DAPP_AUDIO_BUFFER_SIZE=80*1024 -DA2DP_AUDIO_MEMPOOL_SIZE=68*1024 -DAAC_MTU_LIMITER=30 -DSBC_MTU_LIMITER=300 -DTX_7D5MS_AEC_EN -DFAST_XRAM_SECTION_SIZE=0x5a00 -DA2DP_DECODER_VER=2 -DEXTERNAL_WDT -DMEDIA_PLAYER_SUPPORT -DCHIP_BEST1400 -DCHIP_HAS_UART=3 -DCHIP_HAS_I2C=1 -DCHIP_HAS_USB -DLARGE_RAM -DRTOS -DKERNEL_RTX  -I../../include/rtos/rtx/ -D__RTX_CPU_STATISTICS__=1 -DBESBT_STACK_SIZE=1024*4+512 -DDEBUG -DROM_UTILS_ON -DAUD_SECTION_STRUCT_VERSION=2 -DANC_NOISE_TRACKER_CHANNEL_NUM=1 -DNEW_PROFILE_BLOCKED -DUSE_HCIBUFF_TO_CACHE_RTXBUFF -D__BLE_TX_USE_BT_TX_QUEUE__ -D__2M_PACK__ -D_SCO_BTPCM_CHANNEL_ -D__A2DP_PLAYER_USE_BT_TRIGGER__ -DHFP_1_6_ENABLE -DA2DP_AAC_ON -D__ACC_FRAGMENT_COMPATIBLE__ -DBT_XTAL_SYNC_NEW_METHOD -DFIXED_BIT_OFFSET_TARGET -DCVSD_BYPASS -DSCO_DMA_SNAPSHOT -DSCO_OPTIMIZE_FOR_RAM -DAAC_TEXT_PARTIAL_IN_FLASH -DSUPPORT_BATTERY_REPORT -DSUPPORT_SIRI -DTWS_PROMPT_SYNC -DMIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED -DIBRT -DIBRT_BLOCKED -DIBRT_NOT_USE -D__A2DP_AUDIO_SYNC_FIX_DIFF_NOPID__ -DTWS_SYSTEM_ENABLED -DBES_AUD -DIBRT_SEARCH_UI -DSPEECH_TX_DC_FILTER -DHFP_DISABLE_NREC -DSPEECH_TX_AEC2FLOAT -DHFP_DISABLE_NREC -DSPEECH_TX_COMPEXP -DHFP_DISABLE_NREC -DSPEECH_TX_EQ -DHFP_DISABLE_NREC -D__FACTORY_MODE_SUPPORT__ -DNEW_NV_RECORD_ENALBED -DNV_EXTENSION_MIRROR_RAM_SIZE=128  -I../../services/nv_section/userdata_section -DSPEECH_PROCESS_FRAME_MS=15 -DSPEECH_SCO_FRAME_MS=15 -DMULTIPOINT_DUAL_SLAVE -DSPEECH_CODEC_CAPTURE_CHANNEL_NUM=1 -DTILE_DATAPATH=0 -DCUSTOM_INFORMATION_TILE=0  -I../../config/best1400_ibrt  -I../../config/_default_cfg_src_ -DUNALIGNED_ACCESS -Werror -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nano.specs -fno-common -fmessage-length=0 -Wall -fno-exceptions -ffunction-sections -fdata-sections -fomit-frame-pointer -fsigned-char -fno-aggressive-loop-optimizations -fno-isolate-erroneous-paths-dereference -fsingle-precision-constant -Wdouble-promotion -Wfloat-conversion -g -Os -DENHANCED_STACK -Werror=implicit-int -Werror=date-time -Wno-trigraphs -fno-strict-aliasing -Werror-implicit-function-declaration -Wno-format-security  -I../../services/multimedia/audio/codec/fdkaac_codec/libAACdec/include  -I../../services/multimedia/audio/codec/fdkaac_codec/libAACenc/include  -I../../services/multimedia/audio/codec/fdkaac_codec/libFDK/include  -I../../services/multimedia/audio/codec/fdkaac_codec/libMpegTPDec/include  -I../../services/multimedia/audio/codec/fdkaac_codec/libMpegTPEnc/include  -I../../services/multimedia/audio/codec/fdkaac_codec/libPCMutils/include  -I../../services/multimedia/audio/codec/fdkaac_codec/libSBRdec/include  -I../../services/multimedia/audio/codec/fdkaac_codec/libSBRenc/include  -I../../services/multimedia/audio/codec/fdkaac_codec/libSYS/include  -I../../services/audio_process  -I../../services/hw_dsp/inc  -I../../services/fs/fat  -I../../services/fs/sd  -I../../services/fs/fat/ChaN  -I../../services/bt_if_enhanced/inc  -I../../services/multimedia/speech/inc  -I../../services/bone_sensor/lis25ba  -I../../services/overlay  -I../../thirdparty/tile/tile_common/tile_storage  -I../../services/nvrecord  -I../../services/resources  -I../../services/multimedia/rbcodec  -I../../services/multimedia/audio/process/resample/include  -I../../services/multimedia/audio/process/filters/include  -I../../services/multimedia/audio/process/drc/include  -I../../services/multimedia/audio/process/anc/include  -I../../services/nv_section/aud_section  -I../../services/nv_section/userdata_section  -I../../services/nv_section/include  -I../../services/voicepath/  -I../../services/voicepath/gsound/gsound_target  -I../../services/voicepath/gsound/gsound_custom/inc  -I../../services/voicepath/gsound/gsound_target_api_read_only  -I../../platform/drivers/uarthci  -I../../platform/drivers/ana  -I../../platform/cmsis  -I../../platform/drivers/bt  -I../../utils/cqueue  -I../../utils/heap  -I../../services/audioflinger  -I../../utils/lockcqueue  -I../../utils/intersyshci  -I../../apps/anc/inc  -I../../apps/key  -I../../apps/main  -I../../apps/common  -I../../apps/audioplayers  -I../../apps/audioplayers/a2dp_decoder  -I../../apps/battery  -I../../apps/common  -I../../apps/factory  -I../../services/app_ibrt/inc  -I../../services/ble_app  -I../../utils/hwtimer_list  -I../../services/tws_ibrt/inc  -I../../services/ble_stack/ble_ip  -I../../services/ble_stack/hl/api  -I../../services/ble_stack/app/api/  -I../../services/ble_stack/common/api/  -I../../services/ble_stack/hl/inc/  -I../../services/ble_stack/ke/api  -I../../services/voicepath  -I../../thirdparty/userapi  -I../../services/ble_app/app_gfps  -I../../services/ble_app/app_main  -I../../thirdparty/audio_codec_lib/liblhdc-dec/inc  -I../../services/ai_voice/manager  -I../../services/ai_voice/audio  -I../../services/ai_voice/transport  -I../../services/interconnection/red  -I../../services/interconnection/green  -I../../services/interconnection/umm_malloc  -I../../services/bt_app  -I../../services/multimedia/audio/codec/sbc/inc  -I../../services/multimedia/audio/codec/sbc/src/inc  -I../../services/bt_app/a2dp_codecs/include  -I../../services/osif  -I../../services/ibrt/inc -std=gnu++98 -fno-rtti -c -o services/bt_app/app_a2dp_source.o ../../services/bt_app/app_a2dp_source.cpp
