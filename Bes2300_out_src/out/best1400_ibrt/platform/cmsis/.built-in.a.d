
cmd_platform/cmsis/built-in.a := ( /usr/bin/printf 'create platform/cmsis/built-in.a\n addmod platform/cmsis/system_ARMCM.o,platform/cmsis/patch_armv7m.o,platform/cmsis/patch.o,platform/cmsis/system_utils.o,platform/cmsis/cmsis_nvic.o,platform/cmsis/mpu.o,platform/cmsis/system_cp.o,platform/cmsis/retarget.o\n addlib platform/cmsis/DSP_Lib/built-in.a\nsave\nend' | arm-none-eabi-ar -M )
