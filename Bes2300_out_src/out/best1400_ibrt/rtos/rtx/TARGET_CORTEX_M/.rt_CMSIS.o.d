rtos/rtx/TARGET_CORTEX_M/rt_CMSIS.o: \
 ../../rtos/rtx/TARGET_CORTEX_M/rt_CMSIS.c \
 ../../platform/cmsis/inc/cmsis.h ../../platform/hal/plat_addr_map.h \
 ../../platform/hal/best1400/plat_addr_map_best1400.h \
 ../../platform/cmsis/inc/best1400.h ../../platform/cmsis/inc/core_cm4.h \
 /opt/gcc-arm-none-eabi-6_2-2016q4/lib/gcc/arm-none-eabi/6.2.1/include/stdint.h \
 /opt/gcc-arm-none-eabi-6_2-2016q4/arm-none-eabi/include/stdint.h \
 /opt/gcc-arm-none-eabi-6_2-2016q4/arm-none-eabi/include/machine/_default_types.h \
 /opt/gcc-arm-none-eabi-6_2-2016q4/arm-none-eabi/include/sys/features.h \
 /opt/gcc-arm-none-eabi-6_2-2016q4/arm-none-eabi/include/_newlib_version.h \
 /opt/gcc-arm-none-eabi-6_2-2016q4/arm-none-eabi/include/sys/_intsup.h \
 /opt/gcc-arm-none-eabi-6_2-2016q4/arm-none-eabi/include/sys/_stdint.h \
 ../../platform/cmsis/inc/cmsis_version.h \
 ../../platform/cmsis/inc/cmsis_compiler.h \
 ../../platform/cmsis/inc/cmsis_gcc.h \
 ../../platform/cmsis/inc/system_ARMCM.h \
 /opt/gcc-arm-none-eabi-6_2-2016q4/lib/gcc/arm-none-eabi/6.2.1/include/stdbool.h \
 ../../platform/cmsis/inc/cmsis.h \
 ../../rtos/rtx/TARGET_CORTEX_M/rt_TypeDef.h \
 ../../include/rtos/rtx/os_tcb.h \
 ../../rtos/rtx/TARGET_CORTEX_M/RTX_Conf.h \
 ../../rtos/rtx/TARGET_CORTEX_M/rt_System.h \
 ../../rtos/rtx/TARGET_CORTEX_M/rt_Task.h \
 ../../rtos/rtx/TARGET_CORTEX_M/rt_Event.h \
 ../../rtos/rtx/TARGET_CORTEX_M/rt_List.h \
 ../../rtos/rtx/TARGET_CORTEX_M/rt_Time.h \
 ../../rtos/rtx/TARGET_CORTEX_M/rt_Mutex.h \
 ../../rtos/rtx/TARGET_CORTEX_M/rt_Semaphore.h \
 ../../rtos/rtx/TARGET_CORTEX_M/rt_Mailbox.h \
 ../../rtos/rtx/TARGET_CORTEX_M/rt_MemBox.h \
 ../../rtos/rtx/TARGET_CORTEX_M/rt_HAL_CM.h \
 ../../include/rtos/rtx/cmsis_os.h \
 /opt/gcc-arm-none-eabi-6_2-2016q4/lib/gcc/arm-none-eabi/6.2.1/include/stddef.h \
 ../../include/rtos/rtx/os_tcb.h

../../platform/cmsis/inc/cmsis.h:

../../platform/hal/plat_addr_map.h:

../../platform/hal/best1400/plat_addr_map_best1400.h:

../../platform/cmsis/inc/best1400.h:

../../platform/cmsis/inc/core_cm4.h:

/opt/gcc-arm-none-eabi-6_2-2016q4/lib/gcc/arm-none-eabi/6.2.1/include/stdint.h:

/opt/gcc-arm-none-eabi-6_2-2016q4/arm-none-eabi/include/stdint.h:

/opt/gcc-arm-none-eabi-6_2-2016q4/arm-none-eabi/include/machine/_default_types.h:

/opt/gcc-arm-none-eabi-6_2-2016q4/arm-none-eabi/include/sys/features.h:

/opt/gcc-arm-none-eabi-6_2-2016q4/arm-none-eabi/include/_newlib_version.h:

/opt/gcc-arm-none-eabi-6_2-2016q4/arm-none-eabi/include/sys/_intsup.h:

/opt/gcc-arm-none-eabi-6_2-2016q4/arm-none-eabi/include/sys/_stdint.h:

../../platform/cmsis/inc/cmsis_version.h:

../../platform/cmsis/inc/cmsis_compiler.h:

../../platform/cmsis/inc/cmsis_gcc.h:

../../platform/cmsis/inc/system_ARMCM.h:

/opt/gcc-arm-none-eabi-6_2-2016q4/lib/gcc/arm-none-eabi/6.2.1/include/stdbool.h:

../../platform/cmsis/inc/cmsis.h:

../../rtos/rtx/TARGET_CORTEX_M/rt_TypeDef.h:

../../include/rtos/rtx/os_tcb.h:

../../rtos/rtx/TARGET_CORTEX_M/RTX_Conf.h:

../../rtos/rtx/TARGET_CORTEX_M/rt_System.h:

../../rtos/rtx/TARGET_CORTEX_M/rt_Task.h:

../../rtos/rtx/TARGET_CORTEX_M/rt_Event.h:

../../rtos/rtx/TARGET_CORTEX_M/rt_List.h:

../../rtos/rtx/TARGET_CORTEX_M/rt_Time.h:

../../rtos/rtx/TARGET_CORTEX_M/rt_Mutex.h:

../../rtos/rtx/TARGET_CORTEX_M/rt_Semaphore.h:

../../rtos/rtx/TARGET_CORTEX_M/rt_Mailbox.h:

../../rtos/rtx/TARGET_CORTEX_M/rt_MemBox.h:

../../rtos/rtx/TARGET_CORTEX_M/rt_HAL_CM.h:

../../include/rtos/rtx/cmsis_os.h:

/opt/gcc-arm-none-eabi-6_2-2016q4/lib/gcc/arm-none-eabi/6.2.1/include/stddef.h:

../../include/rtos/rtx/os_tcb.h:

cmd_rtos/rtx/TARGET_CORTEX_M/rt_CMSIS.o := arm-none-eabi-gcc -MD -MP -MF rtos/rtx/TARGET_CORTEX_M/.rt_CMSIS.o.d   -I../../rtos/rtx/TARGET_CORTEX_M  -I../../platform/cmsis/inc  -I../../services/audioflinger  -I../../platform/hal  -I../../services/fs/  -I../../services/fs/sd  -I../../services/fs/fat  -I../../services/fs/fat/ChaN -DCHARGER_PLUGINOUT_RESET=0 -DAPP_AUDIO_BUFFER_SIZE=80*1024 -DA2DP_AUDIO_MEMPOOL_SIZE=68*1024 -DAAC_MTU_LIMITER=30 -DSBC_MTU_LIMITER=300 -DTX_7D5MS_AEC_EN -DFAST_XRAM_SECTION_SIZE=0x5a00 -DA2DP_DECODER_VER=2 -DEXTERNAL_WDT -DMEDIA_PLAYER_SUPPORT -DCHIP_BEST1400 -DCHIP_HAS_UART=3 -DCHIP_HAS_I2C=1 -DCHIP_HAS_USB -DLARGE_RAM -DRTOS -DKERNEL_RTX  -I../../include/rtos/rtx/ -D__RTX_CPU_STATISTICS__=1 -DBESBT_STACK_SIZE=1024*4+512 -DDEBUG -DROM_UTILS_ON -DAUD_SECTION_STRUCT_VERSION=2 -DANC_NOISE_TRACKER_CHANNEL_NUM=1 -DNEW_PROFILE_BLOCKED -DUSE_HCIBUFF_TO_CACHE_RTXBUFF -D__BLE_TX_USE_BT_TX_QUEUE__ -D__2M_PACK__ -D_SCO_BTPCM_CHANNEL_ -D__A2DP_PLAYER_USE_BT_TRIGGER__ -DHFP_1_6_ENABLE -DA2DP_AAC_ON -D__ACC_FRAGMENT_COMPATIBLE__ -DBT_XTAL_SYNC_NEW_METHOD -DFIXED_BIT_OFFSET_TARGET -DCVSD_BYPASS -DSCO_DMA_SNAPSHOT -DSCO_OPTIMIZE_FOR_RAM -DAAC_TEXT_PARTIAL_IN_FLASH -DSUPPORT_BATTERY_REPORT -DSUPPORT_SIRI -DTWS_PROMPT_SYNC -DMIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED -DIBRT -DIBRT_BLOCKED -DIBRT_NOT_USE -D__A2DP_AUDIO_SYNC_FIX_DIFF_NOPID__ -DTWS_SYSTEM_ENABLED -DBES_AUD -DIBRT_SEARCH_UI -DSPEECH_TX_DC_FILTER -DHFP_DISABLE_NREC -DSPEECH_TX_AEC2FLOAT -DHFP_DISABLE_NREC -DSPEECH_TX_COMPEXP -DHFP_DISABLE_NREC -DSPEECH_TX_EQ -DHFP_DISABLE_NREC -D__FACTORY_MODE_SUPPORT__ -DNEW_NV_RECORD_ENALBED -DNV_EXTENSION_MIRROR_RAM_SIZE=128  -I../../services/nv_section/userdata_section -DSPEECH_PROCESS_FRAME_MS=15 -DSPEECH_SCO_FRAME_MS=15 -DMULTIPOINT_DUAL_SLAVE -DSPEECH_CODEC_CAPTURE_CHANNEL_NUM=1 -DTILE_DATAPATH=0 -DCUSTOM_INFORMATION_TILE=0  -I../../config/best1400_ibrt  -I../../config/_default_cfg_src_ -DUNALIGNED_ACCESS -Werror -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nano.specs -fno-common -fmessage-length=0 -Wall -fno-exceptions -ffunction-sections -fdata-sections -fomit-frame-pointer -fsigned-char -fno-aggressive-loop-optimizations -fno-isolate-erroneous-paths-dereference -fsingle-precision-constant -Wdouble-promotion -Wfloat-conversion -g -Os -DENHANCED_STACK -Werror=implicit-int -Werror=date-time -Wno-trigraphs -fno-strict-aliasing -Werror-implicit-function-declaration -Wno-format-security  -I../../platform/cmsis/inc  -I../../platform/hal  -I../../utils/hwtimer_list -DOS_CLOCK_NOMINAL=32000 -std=gnu99 -c -o rtos/rtx/TARGET_CORTEX_M/rt_CMSIS.o ../../rtos/rtx/TARGET_CORTEX_M/rt_CMSIS.c
