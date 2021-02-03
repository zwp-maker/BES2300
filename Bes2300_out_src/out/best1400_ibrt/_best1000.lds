/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
/*************************************************
    ROM: 0x0001_FFFF ~ 0x0000_0000
**************************************************/
/* Total ROM_SIZE: 128 KB; boot ROM: 48 KB */
/*************************************************
    SRAM: 
        0x2003_FFFF ~ 0x2002_0000 (SRAM1 128 KB NC address)
        0x0023_FFFF ~ 0x0022_0000 (SRAM1 128 KB C  address)
        0x2001_FFFF ~ 0x2000_0000 (SRAM0 128 KB NC address)
        0x0021_FFFF ~ 0x0020_0000 (SRAM0 128 KB C  address)
*************************************************/
/************************************************
    Nor FLASH:
        0x3FFF_FFFF ~ 0x3C00_0000 (alias)
        0x3BFF_FFFF ~ 0x3800_0000 (alias)
        0x0FFF_FFFF ~ 0x0C00_0000
        0x0BFF_FFFF ~ 0x0800_0000
************************************************/
/************************************************
    CACHE
************************************************/
/* No data cache */
/************************************************
    AHB0 (APB0): 0x400F_FFFF ~ 0x4000_0000
************************************************/
/* CMU: 0x4000_0FFF ~ 0x4000_0000 */
/* GPIO: 0x4000_1FFF ~ 0x4000_1000 */
/* WDT: 0x4000_2FFF ~ 0x4000_2000 */
/* PWM: 0x4000_3FFF ~ 0x4000_3000 */
/* TIMER0: 0x4000_4FFF ~ 0x4000_4000 */
/* TIMER1: 0x4000_5FFF ~ 0x4000_5000 */
/* I2C0: 0x4000_6FFF ~ 0x4000_6000 */
/* SPI: 0x4000_7FFF ~ 0x4000_7000 */
/* Internal SPI: 0x4000_8FFF ~ 0x4000_8000 */
/* SPI for USB PHY = ISPI */
/* UART0: 0x4000_9FFF ~ 0x4000_9000 */
/* UART1: 0x4000_AFFF ~ 0x4000_A000 */
/* I2S: 0x4000_BFFF ~ 0x4000_B000 */
/* UART2: 0x4000_CFFF ~ 0x4000_C000 */
/* IOMUX: 0x4000_DFFF ~ 0x4000_D000 */
/* BTPCM: 0x4000_EFFF ~ 0x4000_E000 */
/* BT */
/***********************************************
    AHB1: 0x401F_FFFF ~ 0x4010_0000
***********************************************/
/* GPDMA: 0x4013_FFFF ~ 0x4013_0000 */
/* AUDMA = GPDMA */
/* FLASH: 0x4014_FFFF ~ 0x4014_0000 */
/* BT DUMP: 0x4015_FFFF ~ 0x4015_0000 */
/* I2C_SLAVE_BASE: 0x4016_FFFF ~ 0x4016_0000 */
/* USB OTG: 0x401C_FFFF ~ 0x4018_0000 */
/***********************************************
    CODEC: 0x403F_FFFF ~ 0x4030_0000
***********************************************/
/***********************************************
    For Linker Scripts
***********************************************/
/* TODO: Add FLASH_REGION_OFFSET if there are multiple levels of boot loaders */
/* Linker script to configure memory regions. */
/* See plat_addr_map.h and common.mk for the macro definitions */
MEMORY
{
 ROM (rx) : ORIGIN = 0x00000000, LENGTH = 0x0000C000
 FLASH (r) : ORIGIN = (0x3C000000), LENGTH = (0x100000 - ((0x3C000000) - 0x3C000000))
 FLASH_NC (r) : ORIGIN = (((0x3C000000)) - 0x3C000000 + 0x38000000), LENGTH = (0x100000 - ((0x3C000000) - 0x3C000000))
 FLASHX (rx) : ORIGIN = (((0x3C000000)) - 0x3C000000 + 0x0C000000), LENGTH = (0x100000 - ((0x3C000000) - 0x3C000000))
 RAM (rwx) : ORIGIN = 0x20000000, LENGTH = (0x00040000) - 0x5a00
 RAMX (rx) : ORIGIN = 0x00200000, LENGTH = (0x00040000) - 0x5a00
 FRAMX (rwx) : ORIGIN = 0x00200000 + (0x00040000) - 0x5a00, LENGTH = 0x5a00
}
/* Library configurations */
GROUP(libgcc.a libc.a libm.a libnosys.a)
/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __export_fn_rom
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 *   __free_ram
 *   __factory_start
 *   __factory_end
 *   __calib_start
 *   __calib_end
 *   __flash_start
 *   __flash_end
 *   __free_flash
 *   __boot_sram_start_flash__
 *   __boot_sram_end_flash__
 *   __boot_sram_start__
 *   __boot_bss_sram_start__
 *   __boot_bss_sram_end__
 *   __sram_text_data_start_flash__
 *   __sram_text_data_end_flash__
 *   __fast_sram_text_data_start__
 *   __fast_sram_text_data_end__
 *   __fast_sram_text_data_start_flash__
 *   __fast_sram_text_data_end_flash__
 *   __sram_text_data_start__
 *   __sram_bss_start__
 *   __sram_bss_end__
 *   __overlay_text_start__
 *   __overlay_text_exec_start__
 *   __overlay_data_start__
 */
ENTRY(Boot_Loader)
SECTIONS
{
 __export_fn_rom = (ORIGIN(ROM) + LENGTH(ROM) - 40 - 128);
 .boot_struct (ORIGIN(FLASH)) :
 {
  __flash_start = .;
  KEEP(*(.boot_struct))
  . = ALIGN(4);
 } > FLASH
 . = ((.) - 0x3C000000 + 0x0C000000);
 .boot_text_flash (.) : AT (((ADDR(.boot_text_flash)) - 0x0C000000 + 0x3C000000))
 {
  *(.boot_loader)
  *(.boot_text_flash*)
  *(.boot_rodata*)
  . = ALIGN(4);
 } > FLASHX
 . = ((.) - 0x0C000000 + 0x3C000000);
 .got_info (.) :
 {
  __got_info_start = .;
  __got_start = .;
  *(.got)
  . = ALIGN(4);
  __got_end = .;
  __got_plt_start = .;
  *(.got.plt)
  . = ALIGN(4);
  __igot_plt_start = .;
  *(.igot.plt)
  . = ALIGN(4);
  __dynamic_start = .;
  *(.dynamic)
  . = ALIGN(4);
  __got_info_end = .;
 } > FLASH
 .vector_table (ORIGIN(RAM)) (NOLOAD) :
 {
  KEEP(*(.vector_table))
  . = 256;
  . = ALIGN(4);
 } > RAM
 .reboot_param (.) (NOLOAD) :
 {
  KEEP(*(.reboot_param))
  . = 64;
  . = ALIGN(4);
 } > RAM
 .userdata_pool (.) (NOLOAD) :
 {
  *(.userdata_pool)
  . = ALIGN(4);
  __userdata_pool_end__ = .;
 } > RAM
 __boot_sram_start_flash__ = LOADADDR(.got_info) + SIZEOF(.got_info);
 __boot_sram_start__ = __userdata_pool_end__;
 .boot_text_sram (((__boot_sram_start__) - 0x20000000 + 0x00200000)) : AT (__boot_sram_start_flash__)
 {
  /* memcpy.o or libc_nano.a(lib_a-memcpy.o/lib_a-memcpy-stub.o) */
  *:memcpy.o(.text*)
  *:lib_a-memcpy*.o(.text*)
  *:libc_rom.o(.text*)
  /* memset.o or libc_nano.a(lib_a-memset.o) */
  *:memset.o(.text*)
  *:lib_a-memset*.o(.text*)
  *:hal_norflash*.o(.text*)
  *:norflash_*.o(EXCLUDE_FILE(*:norflash_api.o) .text*)
  *(.boot_text_sram*)
  . = ALIGN(4);
  __boot_text_sram_end = .;
 } > RAMX
 .boot_data_sram (((ADDR(.boot_text_sram) + SIZEOF(.boot_text_sram)) - 0x00200000 + 0x20000000)) :
     AT (__boot_sram_start_flash__ + SIZEOF(.boot_text_sram))
 {
  *:hal_norflash*.o(.data* .rodata*)
  *:norflash_*.o(EXCLUDE_FILE(*:norflash_api.o) .data* EXCLUDE_FILE(*:norflash_api.o) .rodata*)
  *:hal_psram.o(.data* .rodata*)
  *(.boot_data*)
  . = ALIGN(4);
  __boot_data_sram_end = .;
 } > RAM
 __boot_sram_end_flash__ = __boot_sram_start_flash__ +
        SIZEOF(.boot_text_sram) + SIZEOF(.boot_data_sram);
 .boot_bss_sram (.) (NOLOAD) : AT (ADDR(.boot_bss_sram))
 {
  __boot_bss_sram_start__ = .;
  *:hal_norflash*.o(.bss*)
  *:norflash_*.o(EXCLUDE_FILE(*:norflash_api.o) .bss*)
  *:hal_psram.o(.bss*)
  *(.boot_bss*)
  . = ALIGN(4);
  __boot_bss_sram_end__ = .;
 } > RAM
 __sram_text_data_start_flash__ = __boot_sram_end_flash__;
 __sram_text_data_start__ = __boot_bss_sram_end__;
 .sram_text (((__sram_text_data_start__) - 0x20000000 + 0x00200000)) : AT (__sram_text_data_start_flash__)
 {
  *:norflash_api.o(.text*)
  *(.sram_text*)
  . = ALIGN(4);
 } > RAMX
 .sram_data (((ADDR(.sram_text) + SIZEOF(.sram_text)) - 0x00200000 + 0x20000000)) :
    AT (__sram_text_data_start_flash__ + SIZEOF(.sram_text))
 {
        *(.nv_extension_pool)
  . = ALIGN(128);
        /*. = ALIGN(4);*/
  *:utils.o(.data* .rodata*)
  *:cqueue.o(.data* .rodata*)
  *:iir_process.o(.data* .rodata*)
  *:sbcplc.o(.data* .rodata*)
  *:sbc.o(.data* .rodata*)
  *:sbc_math.o(.data* .rodata*)
  *:crossover_filter.o(.data* .rodata*)
  *:exp_func_tables.o(.data* .rodata*)
  *:norflash_api.o(.rodata*)
        . = ALIGN(0);
  *(.sram_data*)
        . = ALIGN(4);
 } > RAM
 __sram_text_data_end_flash__ = __sram_text_data_start_flash__ +
        SIZEOF(.sram_text) + SIZEOF(.sram_data);
 .sram_bss (.) (NOLOAD) : AT (ADDR(.sram_bss))
 {
  __sram_bss_start__ = .;
  *:utils.o(.bss*)
  *(.sram_bss*)
  . = ALIGN(4);
  __sram_bss_end__ = .;
 } > RAM
 __fast_sram_text_data_start_flash__ = __sram_text_data_end_flash__;
 .fast_text_sram (ORIGIN(FRAMX)) : AT (__fast_sram_text_data_start_flash__)
 {
  __fast_sram_text_exec_start__ = .;
  *:rom_utils.o(.text*)
  /* memmove.o or libc_nano.a(lib_a-memmove.o) */
  *:memmove.o(.text*)
  *:lib_a-memmove.o(.text*)
  *:utils.o(.text*)
  *:cqueue.o(.text*)
  *:audio_resample_ex.o(.text*)
  *:sbc.o(.text*.sbc_is_stream_info_valid)
  *:sbc.o(.text*.sbc_frame_length_get)
  *:sbc.o(.text*.sbc_4bit_crc_calc)
  *:sbc.o(.text*.sbc_bits_for_mono_alloc)
  *:sbc.o(.text*.sbc_bits_for_stereo_alloc)
  *:sbc.o(.text*.sbc_header_process)
  *:sbc.o(.text*.sbc_factors_process)
  *:sbc.o(.text*.sbc_sample_extract)
  *:sbc.o(.text*.sbc_sbsamples_make)
  *:sbc.o(.text*.sbc_joint_process)
  *:sbc.o(.text*.sbc_4band_synth_filter)
  *:sbc.o(.text*.sbc_8band_synth_filter)
  *:sbc.o(.text*.sbc_decoder_state_reset)
  *:sbc.o(.text*.sbc_frame_mute)
  *:sbc.o(.text*.sbc_frames_decode)
  *:sbc.o(.text*.sbc_samples_quant)
  *:sbc.o(.text*.sbc_decoder_init)
  *:sbc.o(.text*.sbc_encoder_init)
  *:sbc.o(.text*.sbc_frames_encode)
  *:sbc_math.o(.text*)
  *:fixpoint_math.o(.text*)
  *:FDK_bitbuffer.o(.text*)
  *:FDK_tools_rom.o(.text*FDKgetWindowSlope*)
  *:FDK_tools_rom.o(.text*getBitstreamElementList*)
  *:dct.o(.text*dct_IV*)
  *:fft.o(.text*._Z3fftiPlPi)
  *:fft_rad2.o(.text*dit_fft*)
  *:dct.o(.text*getTables*)
  *:crossover_filter.o(.text*)
  *:data_compressor.o(.text*)
  *:drc.o(.text*)
  *:limiter.o(.text*)
  *(.fast_text_sram*)
  . = ALIGN(4);
  __fast_sram_text_exec_end__ = .;
 } > FRAMX
 __fast_sram_text_data_end_flash__ = __fast_sram_text_data_start_flash__ + SIZEOF(.fast_text_sram);
 __fast_sram_text_data_start__ = ((__fast_sram_text_exec_start__) - 0x00200000 + 0x20000000);
 __fast_sram_text_data_end__ = ((__fast_sram_text_exec_end__) - 0x00200000 + 0x20000000);
 __overlay_text_start_flash__ = __fast_sram_text_data_end_flash__;
 __overlay_text_start__ = __fast_sram_text_data_end__;
 __overlay_text_exec_start__ = ((__overlay_text_start__) - 0x20000000 + 0x00200000);
 OVERLAY (__overlay_text_exec_start__) : NOCROSSREFS AT (__overlay_text_start_flash__)
 {
  .overlay_text0
  {
   LONG(0);
   *:arm_biquad_cascade_df1_f32.o(.text*)
   *:arm_rfft_fast_f32.o(.text*)
   *:arm_cfft_f32.o(.text*)
   *:cmsis_fftwrap.o(.text*)
   *:integer_delay.o(.text*)
   *:lc_mmse_ns_float.o(.text*.speech_ns2float_process)
   *:coherence.o(.text*)
   *:echo_suppression.o(.text*.float_cmp)
   *:echo_suppression.o(.text*.es_process)
   *:lcmmse.o(.text*.lcmmse_noise_estimate)
   *:lcmmse.o(.text*.lcmmse_get_speech_prob)
   *:logmmse.o(.text*.filterbank_compute_bank_pflt)
   *:logmmse.o(.text*.filterbank_compute_psd_pflt)
   *:logmmse.o(.text*.logmmse_analysis)
   *:logmmse.o(.text*.logmmse_process)
   *:lpc_plc.o(.text*.WB_PLC_common)
   *:lpc_plc.o(.text*.WB_PLC_erasure)
   *:lpc_plc.o(.text*.apfilterQ1_Q0)
   *:lpc_plc.o(.text*.apfilterQ0_Q0)
   *:lpc_plc.o(.text*.azfilterQ0_Q1)
   *:lpc_plc.o(.text*.decim)
   *:lpc_plc.o(.text*.merit)
   *(.overlay_text0)
   . = ALIGN(4);
  }
  .overlay_text1
  {
   LONG(0);
   *:fixedpoint.o(.text*)
   *:replaygain.o(.text*)
   *:dsp_core.o(.text*)
   *:dsp_misc.o(.text*)
   *:dsp_filter.o(.text*)
   *:dsp_sample_input.o(.text*)
   *:dsp_sample_output.o(.text*)
   *:eq.o(.text*)
   *:pga.o(.text*)
   *(.overlay_a2dp_sbc*)
   *(.overlay_text1)
   . = ALIGN(4);
  }
  .overlay_text2
  {
   LONG(0);
   *:fmdec.o(.text* .data* .rodata* .bss*)
   *(.overlay_text2)
   . = ALIGN(4);
  }
  .overlay_text3
  {
   LONG(0);
   *:fft128dot.o(.text*)
   *(.overlay_text3)
   . = ALIGN(4);
  }
  .overlay_text4
  {
   LONG(0);
   *:block.o(.text*.*CBlock_GetEscape*)
   *:tpdec_lib.o(.text*.*transportDec_GetBitstream*)
   *:tpdec_lib.o(.text*.*transportDec_GetAuBitsRemaining*)
   *:tpdec_latm.o(.text*.*CLatmDemux_GetValue*)
   *:tpdec_latm.o(.text*.*CLatmDemux_Read*)
   *:tpdec_lib.o(.text*.*transportDec_FillData*)
   *:tpdec_lib.o(.text*.*transportDec_ReadAccessUnit*)
   *:tpdec_lib.o(.text*.*transportDec_readHeader*)
   *:block.o(.text*.*ApplyTools*)
   *:aacdec_pns.o(.text*.*CPns_Apply*)
   *:aacdec_tns.o(.text*.*CTns_Apply*)
   *:aacdec_tns.o(.text*.*CTns_Reset*)
   *:aacdec_pns.o(.text*.*CPns_ResetData*)
   *:pulsedata.o(.text*.*CPulseData_Read*)
   *(.overlay_a2dp_aac*)
   *(.overlay_text4)
   . = ALIGN(4);
  }
  .overlay_text5
  {
   LONG(0);
   *:mpa.o(.text*)
   *:bit.o(.text*)
   *:frame.o(.text*)
   *:huffman.o(.text*)
   *:layer12.o(.text*)
   *:layer3.o(.text*)
   *:stream.o(.text*)
   *:synth.o(.text*)
   *(.overlay_text5)
   . = ALIGN(4);
  }
  .overlay_text6
  {
   LONG(0);
   *(.overlay_text6)
   . = ALIGN(4);
  }
  .overlay_text7
  {
   LONG(0);
   *(.overlay_text7)
   . = ALIGN(4);
  }
  .overlay_text_last
  {
   LONG(0);
   . = ALIGN(4);
  }
 } > FRAMX
 .dummy_overlay_text_end (.) (NOLOAD) : AT (ADDR(.dummy_overlay_text_end))
 {
  __overlay_text_exec_end__ = .;
 } > FRAMX
 __fast_sram_end__ = ((__overlay_text_exec_end__) - 0x00200000 + 0x20000000);
 ASSERT(__fast_sram_end__ - __fast_sram_text_data_start__ <= 0x5a00, "fast xram sections too large")
 __free_fram = 0x5a00 - (__fast_sram_end__ - __fast_sram_text_data_start__);
 __overlay_data_start__ = __sram_bss_end__;
 OVERLAY (__overlay_data_start__) : NOCROSSREFS AT (__load_stop_overlay_text_last)
 {
  .overlay_data0
  {
   /* Explicitly place 4 bytes at section start to avoid "section type changed to PROGBITS" warnings */
   LONG(0);
   *(.overlay_data0 .overlay_rodata0)
   . = ALIGN(4);
  }
  .overlay_data1
  {
   LONG(0);
   *:fixedpoint.o(.data* .rodata*)
   *:replaygain.o(.data* .rodata*)
   *:dsp_core.o(.data* .rodata*)
   *:dsp_misc.o(.data* .rodata*)
   *:dsp_filter.o(.data* .rodata*)
   *:dsp_sample_input.o(.data* .rodata*)
   *:dsp_sample_output.o(.data* .rodata*)
   *:eq.o(.data* .rodata*)
   *:pga.o(.data* .rodata*)
   *(.overlay_data1 .overlay_rodata1)
   . = ALIGN(4);
  }
  .overlay_data2
  {
   LONG(0);
   *(.overlay_data2 .overlay_rodata2)
   . = ALIGN(4);
  }
  .overlay_data3
  {
   LONG(0);
   *:fft128dot.o(.data* .rodata*)
   *(.overlay_data3 .overlay_rodata3)
   . = ALIGN(4);
  }
  .overlay_data4
  {
   LONG(0);
   *:block.o(.data* .rodata*)
   *:tpdec_lib.o(.data* .rodata*)
   *:tpdec_latm.o(.data* .rodata*)
   *:aacdec_pns.o(.data* .rodata*)
   *:aacdec_tns.o(.data* .rodata*)
   *:pulsedata.o(.data* .rodata*)
   *:channel.o(.data* .rodata*)
   *:tpdec_asc.o(.data* .rodata*)
   *:aacdecoder.o(.data* .rodata*)
   *:channelinfo.o(.data* .rodata*)
   *:aacdecoder_lib.o(.data* .rodata*)
   *:stereo.o(.data* .rodata*)
   *:fixpoint_math.o(.data* .rodata*)
   *:FDK_bitbuffer.o(.data* .rodata*)
   *(.overlay_data4 .overlay_rodata4)
   . = ALIGN(4);
  }
  .overlay_data5
  {
   LONG(0);
   *:mpa.o(.data* .bss*)
   *:bit.o(.data* .bss*)
   *:frame.o(.data* .bss*)
   *:huffman.o(.data* .bss*)
   *:layer12.o(.data* .bss*)
   *:layer3.o(.data* .bss*)
   *:stream.o(.data* .bss*)
   *:synth.o(.data* .bss*)
   *(.overlay_data5 .overlay_rodata5)
   . = ALIGN(4);
  }
  .overlay_data6
  {
   LONG(0);
   *(.overlay_data6 .overlay_rodata6)
   . = ALIGN(4);
  }
  .overlay_data7
  {
   LONG(0);
   *(.overlay_data7 .overlay_rodata7)
   . = ALIGN(4);
  }
  .overlay_data_last
  {
   LONG(0);
   . = ALIGN(4);
  }
 } > RAM
 .dummy_overlay_data_end (.) (NOLOAD) : AT (ADDR(.dummy_overlay_data_end))
 {
  __overlay_data_end__ = .;
 } > RAM
 ASSERT(__overlay_data_end__ - __overlay_data_start__ <= 0x8000, "overlay data sections too large")
 .text (((__load_stop_overlay_data_last) - 0x3C000000 + 0x0C000000)) : AT (((ADDR(.text)) - 0x0C000000 + 0x3C000000))
 {
  *(.text*)
  *(.flash_text*)
  KEEP(*(.init))
  KEEP(*(.fini))
  /* .ctors */
  *crtbegin.o(.ctors)
  *crtbegin?.o(.ctors)
  *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
  *(SORT(.ctors.*))
  *(.ctors)
  /* .dtors */
  *crtbegin.o(.dtors)
  *crtbegin?.o(.dtors)
  *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
  *(SORT(.dtors.*))
  *(.dtors)
 } > FLASHX
 .ARM.extab (.) : AT (((ADDR(.ARM.extab)) - 0x0C000000 + 0x3C000000))
 {
  *(.ARM.extab* .gnu.linkonce.armextab.*)
 } > FLASHX
 __exidx_start = .;
 /* .ARM.exidx contains R_ARM_PREL31 (+-0x40000000) offset to functions, which means
	 * the session location cannot be too far away from the function addresses */
 .ARM.exidx (.) : AT (((ADDR(.ARM.exidx)) - 0x0C000000 + 0x3C000000))
 {
  *(.ARM.exidx* .gnu.linkonce.armexidx.*)
 } > FLASHX
 __exidx_end = .;
 . = ((.) - 0x0C000000 + 0x3C000000);
 .rodata (.) :
 {
  *(.rodata*)
  *(.flash_rodata*)
        . = ALIGN(4);
        __cust_cmd_hldr_tbl_start = .;
        KEEP(*(.cust_cmd_hldr_tbl))
        __cust_cmd_hldr_tbl_end = .;
  . = ALIGN(4);
  __ai_gatt_server_table_start = .;
  KEEP(*(.ai_gatt_server_table))
  __ai_gatt_server_table_end = .;
  . = ALIGN(4);
  __ai_ble_handler_table_start = .;
  KEEP(*(.ai_ble_handler_table))
  __ai_ble_handler_table_end = .;
  . = ALIGN(4);
  __ai_spp_register_table_start = .;
  KEEP(*(.ai_spp_register_table))
  __ai_spp_register_table_end = .;
  . = ALIGN(4);
  __tota_handler_table_start = .;
  KEEP(*(.tota_handler_table))
  __tota_handler_table_end = .;
  . = ALIGN(4);
  __ai_handler_function_table_start = .;
  KEEP(*(.ai_handler_function_table))
  __ai_handler_function_table_end = .;
  . = ALIGN(4);
  __custom_handler_table_start = .;
  KEEP(*(.custom_handler_table))
  __custom_handler_table_end = .;
  . = ALIGN(4);
  __thirdparty_handler_table_start = .;
  KEEP(*(.thirdparty_handler_table))
  __thirdparty_handler_table_end = .;
  KEEP(*(.eh_frame*))
  *(.note.gnu.build-id)
 } > FLASH
 /* To copy multiple FLASH to RAM sections,
	 * uncomment .copy.table section and,
	 * define __STARTUP_COPY_MULTIPLE in startup_ARMCMx.S */
 /*
	.copy.table (.) :
	{
		. = ALIGN(4);
		__copy_table_start__ = .;
		LONG (__etext)
		LONG (__data_start__)
		LONG (__data_end__ - __data_start__)
		LONG (__etext2)
		LONG (__data2_start__)
		LONG (__data2_end__ - __data2_start__)
		__copy_table_end__ = .;
	} > FLASH
	*/
 /* To clear multiple BSS sections,
	 * uncomment .zero.table section and,
	 * define __STARTUP_CLEAR_BSS_MULTIPLE in startup_ARMCMx.S */
 /*
	.zero.table (.) :
	{
		. = ALIGN(4);
		__zero_table_start__ = .;
		LONG (__bss_start__)
		LONG (__bss_end__ - __bss_start__)
		LONG (__bss2_start__)
		LONG (__bss2_end__ - __bss2_start__)
		__zero_table_end__ = .;
	} > FLASH
	*/
 /* Location counter can end up 2byte aligned with narrow Thumb code but
	   __etext is assumed by startup code to be the LMA of a section in RAM
	   which must be 4byte aligned */
 __etext = ALIGN(4);
 /* The VMA is either the end of overlay_data or the end of sram bss */
 .data : AT (__etext)
 {
  __data_start__ = .;
  *(vtable)
  . = ALIGN(4);
  *(.data*)
  . = ALIGN(4);
  /* preinit data */
  PROVIDE_HIDDEN (__preinit_array_start = .);
  KEEP(*(.preinit_array))
  PROVIDE_HIDDEN (__preinit_array_end = .);
  . = ALIGN(4);
  /* init data */
  PROVIDE_HIDDEN (__init_array_start = .);
  KEEP(*(SORT(.init_array.*)))
  KEEP(*(.init_array))
  PROVIDE_HIDDEN (__init_array_end = .);
  . = ALIGN(4);
  /* finit data */
  PROVIDE_HIDDEN (__fini_array_start = .);
  KEEP(*(SORT(.fini_array.*)))
  KEEP(*(.fini_array))
  PROVIDE_HIDDEN (__fini_array_end = .);
  KEEP(*(.jcr*))
  /* All data end */
  . = ALIGN(4);
  __data_end__ = .;
 } > RAM
 .bss (.) (NOLOAD) : AT (ADDR(.bss))
 {
  . = ALIGN(4);
  __bss_start__ = .;
  *(.bss*)
  *(COMMON)
  . = ALIGN(4);
  __bss_end__ = .;
 } > RAM
 .reboot_custom_param (.) (NOLOAD) :
 {
  KEEP(*(.reboot_custom_param))
  . = ALIGN(4);
  __reboot_custom_param_end__ = .;
 } > RAM
 .heap (.) (NOLOAD) :
 {
  . = ALIGN(8);
  __HeapBase = .;
  __end__ = .;
  end = __end__;
  . += 0x1000;
  . = ALIGN(8);
  __HeapLimit = .;
 } > RAM
 /* .stack_dummy section doesn't contains any symbols. It is only
	 * used for linker to calculate size of stack sections, and assign
	 * values to stack symbols later */
 .stack_dummy (.) (COPY) :
 {
  . = 0xD00;
  . = ALIGN(8);
 } > RAM
 /* Set stack top to end of RAM, and stack limit move down by
	 * size of stack_dummy section */
 __StackTop = ORIGIN(RAM) + LENGTH(RAM) - 0x64;
 __StackLimit = __StackTop - SIZEOF(.stack_dummy);
 PROVIDE(__stack = __StackTop);
 /* Check if data + heap + stack exceeds RAM limit */
 ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
 __free_ram = __StackLimit - __HeapLimit;
 . = __etext + SIZEOF(.data);
 .system_info (.) :
 {
  KEEP(*(.system_info_list_size))
  KEEP(*(.system_info_list))
  . = ALIGN(4);
 } > FLASH
 .build_info (.) :
 {
  KEEP(*(.build_info))
  . = ALIGN(4);
 } > FLASH = 0x00000000
 /* The following section MUST be the last loaded section */
 .code_start_addr (.) :
 {
  LONG(0xBE57341D);
  LONG(ABSOLUTE(__flash_start));
 } > FLASH
 __flash_end = .;
 .coredump_section (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - 0x1000 - 0x1000 -
  0 - 0x1000*2 - 0x1000 -
  0 - 0 - 0 - 0) (NOLOAD) :
 {
  __coredump_section_start = .;
  . = 0;
  __coredump_section_end = .;
 } > FLASH_NC
 .ota_upgrade_log (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - 0x1000 - 0x1000 -
  0 - 0x1000*2 - 0x1000 -
  0 - 0 - 0) (NOLOAD) :
 {
  __ota_upgrade_log_start = .;
  . = 0;
  __ota_upgrade_log_end = .;
 } > FLASH_NC
 .log_dump (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - 0x1000 - 0x1000 -
  0 - 0x1000*2 - 0x1000 -
  0 - 0) (NOLOAD) :
 {
  __log_dump_start = .;
  . = 0;
  __log_dump_end = .;
 } > FLASH_NC
 .crash_dump (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - 0x1000 - 0x1000 -
  0 - 0x1000*2 - 0x1000 -
  0) (NOLOAD) :
 {
  __crash_dump_start = .;
  . = 0;
  __crash_dump_end = .;
 } > FLASH_NC
 .custom_parameter (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - 0x1000 - 0x1000 -
  0 - 0x1000*2 - 0x1000) (NOLOAD) :
 {
  __custom_parameter_start = .;
  . = 0x1000;
  __custom_parameter_end = .;
 } > FLASH_NC
 .userdata (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - 0x1000 - 0x1000 -
  0 - 0x1000*2) (NOLOAD) :
 {
  __userdata_start = .;
  . = 0x1000*2;
  __userdata_end = .;
 } > FLASH_NC
 .audio (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - 0x1000 - 0x1000 -
  0) (NOLOAD) :
 {
  __aud_start = .;
  . = 0;
  __aud_end = .;
 } > FLASH_NC
 .reserved (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - 0x1000 - 0x1000) (NOLOAD) :
 {
  __reserved_start = .;
  . = 0x1000;
  __reserved_end = .;
 } > FLASH_NC
 .factory (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - 0x1000) (NOLOAD) :
 {
  __factory_start = .;
  . = 0x1000;
  __factory_end = .;
 } > FLASH_NC
 __tail_section_start = __ota_upgrade_log_start;
 ASSERT(((__tail_section_start) - 0x38000000 + 0x3C000000) >= __flash_end, "region FLASH overflowed")
 __free_flash = ((__tail_section_start) - 0x38000000 + 0x3C000000) - __flash_end;
}
