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
#ifndef __ARM_ARCH_ISA_ARM

#include "mpu.h"
#include "cmsis.h"
#include "hal_trace.h"

#ifdef __ARM_ARCH_8M_MAIN__
#define NORM_MEM_WT_RA_ATTR             ARM_MPU_ATTR_MEMORY_(1, 0, 1, 0)
#define NORM_MEM_WB_WA_ATTR             ARM_MPU_ATTR_MEMORY_(1, 1, 1, 1)
#define DEV_MEM_ATTR_OUTER              ARM_MPU_ATTR_MEMORY_(0, 0, 0, 0)

enum MAIR_ATTR_TYPE_T {
    MAIR_ATTR_FLASH,
    MAIR_ATTR_INT_SRAM,
    MAIR_ATTR_EXT_SRAM,
    MAIR_ATTR_DEVICE,
    MAIR_ATTR_4,
    MAIR_ATTR_5,
    MAIR_ATTR_6,
    MAIR_ATTR_7,

    MAIR_ATTR_QTY,
};

static void init_mair_attr(void)
{
    ARM_MPU_SetMemAttr(MAIR_ATTR_FLASH, ARM_MPU_ATTR(NORM_MEM_WT_RA_ATTR, NORM_MEM_WT_RA_ATTR));
    ARM_MPU_SetMemAttr(MAIR_ATTR_INT_SRAM, ARM_MPU_ATTR(NORM_MEM_WT_RA_ATTR, NORM_MEM_WT_RA_ATTR));
    ARM_MPU_SetMemAttr(MAIR_ATTR_EXT_SRAM, ARM_MPU_ATTR(NORM_MEM_WB_WA_ATTR, NORM_MEM_WB_WA_ATTR));
    ARM_MPU_SetMemAttr(MAIR_ATTR_DEVICE, ARM_MPU_ATTR(DEV_MEM_ATTR_OUTER, ARM_MPU_ATTR_DEVICE_nGnRnE));
}
#endif

int mpu_open(void)
{
    int i;

    if ((MPU->TYPE & MPU_TYPE_DREGION_Msk) == 0) {
        return 1;
    }

    ARM_MPU_Disable();

    for (i = 0; i < MPU_ID_QTY; i++) {
        ARM_MPU_ClrRegion(i);
    }

    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);

#ifdef __ARM_ARCH_8M_MAIN__
    init_mair_attr();
#endif

    return 0;
}

int mpu_close(void)
{
    ARM_MPU_Disable();

    return 0;
}

#ifdef __ARM_ARCH_7EM__
static int mpu_set_armv7(enum MPU_ID_T id, uint32_t addr, uint32_t len, enum MPU_ATTR_T attr)
{
    int ret;
    uint32_t rbar;
    uint32_t rasr;
    uint8_t xn;
    uint8_t ap;
    uint8_t size;

    if ((MPU->CTRL & MPU_CTRL_ENABLE_Msk) == 0) {
        ret = mpu_open();
        if (ret) {
            return ret;
        }
    }

    if (id >= MPU_ID_QTY) {
        return 2;
    }
    if (len < 32 || (len & (len - 1))) {
        return 3;
    }
    if (addr & (len - 1)) {
        return 4;
    }
    if (attr >= MPU_ATTR_QTY) {
        return 5;
    }

    if (attr == MPU_ATTR_READ_WRITE_EXEC || attr == MPU_ATTR_READ_EXEC ||
            attr == MPU_ATTR_EXEC) {
        xn = 0;
    } else {
        xn = 1;
    }

    ap = ARM_MPU_AP_NONE;
    if (attr == MPU_ATTR_READ_WRITE_EXEC || attr == MPU_ATTR_READ_WRITE) {
        ap = ARM_MPU_AP_FULL;
    } else if (attr == MPU_ATTR_READ_EXEC || attr == MPU_ATTR_READ) {
        ap = ARM_MPU_AP_RO;
    }

    size = __CLZ(__RBIT(len)) - 1;

    ARM_MPU_Disable();
    rbar = ARM_MPU_RBAR(id, addr);
    // Type Extention: 0
    // Shareable: 1
    // Cacheable: 1
    // Bufferable: 1
    // Subregion: 0
    rasr = ARM_MPU_RASR(xn, ap, 0, 1, 1, 1, 0, size);

    ARM_MPU_SetRegion(rbar, rasr);
    __DSB();
    __ISB();
    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);

    return 0;
}

static int mpu_set_sub_region(enum MPU_ID_T id, int srd_bits)
{
    ARM_MPU_SetSubRegion(id, srd_bits);
    __DSB();
    __ISB();
    return 0;
}

/*
 * sub region is 8 bits, and if one bits is 1, the sub resion will be
 * disabled
 * like 0b00011111, the top 3 sub region will be disabled
 * if it is 0b11110000, the bottom 4 sub region will be disabled.
 */
static int mpu_enable_top_sub_region(uint32_t id, uint32_t rg_end,
                                    uint32_t fr_end, uint32_t sub_rg_sz)
{
    int dis_nums;
    uint8_t srd_bits;
    int ret;

    dis_nums = (rg_end - fr_end) / sub_rg_sz;
    if ((fr_end & (sub_rg_sz - 1)) != 0)
        dis_nums += 1;

    srd_bits = 0xff;
    srd_bits &= ~((1UL << (8 - dis_nums)) - 1);

    ret = mpu_set_sub_region(id, srd_bits);

    return ret;
}

static int mpu_enable_bottom_sub_region(uint32_t id, uint32_t rg_start,
                                    uint32_t fr_start, uint32_t sub_rg_sz)
{
    int dis_nums;
    uint8_t srd_bits;
    int ret;

    dis_nums = (fr_start - rg_start) / sub_rg_sz;
    srd_bits = 0xff;
    srd_bits &= ((1UL << dis_nums) - 1);

    ret = mpu_set_sub_region(id, srd_bits);
    return ret;
}

static uint32_t calc_sub_region_size(uint32_t fr_start)
{
    int lsb_bit;
    uint32_t sz;

    //sub region size aligned to fram start
    lsb_bit = get_lsb_pos(fr_start);
    sz = 1 << lsb_bit;

    return sz;
}

/*
Allocate two mpu sections to protect the fast ram.  The cortext M4
mpu has a lot of restrict, such as one region's start address must
be aligned to the size of region, and the sub region number is fixed
to 8.
The layout like:
          ------------------
          | sub region 8   |
          ------------------
          | sub region 7   |
          | .....          |
          |                |
          ------------------>fast_ram_end
          |////////////////|
          |////////////////|
          |////////////////|
          |////////////////|
          ------------------>mpu region2
          |//sub region 8//|
          |////////////////|
          |////////////////|
          |////////////////|
          ------------------>fast_ram_start aligned to sub region size
          | .....          |
          |                |
          |sub region 1    |
          ------------------
          |sub region 0    |
          ------------------>mpu region1

If the __fast_sram_text_exec_end__ exceed the region2's end, just leave
the rest unprotect.
*/

int mpu_fram_protect_armv7(uint32_t fr_start, uint32_t fr_end)
{
    uint32_t fr_sz;
    uint32_t sub_rg_sz;
    uint32_t rg_sz;
    uint32_t rg_start;
    uint32_t rg_end;
    int ret = 0;
    uint8_t id;

    fr_sz = fr_end - fr_start;

    sub_rg_sz = calc_sub_region_size(fr_start);
    if (sub_rg_sz < 0x80) {
        /*cortex-m4 doesn't support sub region size less than 128 */
        TRACE("no mpu region for fram start %x, end %x, size %x",
                                                    fr_start, fr_end, fr_sz);
        return -1;
    }
    if (sub_rg_sz > 0x4000)
        sub_rg_sz = 0x4000;

    /* according to cortex-m4 spec, the region is divived to 8 sub region*/
    rg_sz = sub_rg_sz * 8;
    /*now we just protect two region */
    if (fr_sz > (rg_sz * 2)) {
        TRACE("Warning fram is too big, mpu can not protect so much");
        TRACE("region_sz %x, fram size %x", rg_sz, fr_sz);
    }

    /*aliged the region start to region size according to cortext m4 spec*/
    rg_start = fr_start & ~(rg_sz - 1);
    ARM_MPU_Disable();
    id = MPU_ID_FRAM_TEXT1;
    ret = mpu_set(id, rg_start, rg_sz, MPU_ATTR_READ_EXEC);
    if (fr_start > rg_start) {
        ret = mpu_enable_bottom_sub_region(id, rg_start, fr_start, sub_rg_sz);
    }

    rg_end = rg_start + rg_sz;
    if (fr_end < rg_end) {
        ret = mpu_enable_top_sub_region(id, rg_end, fr_end, sub_rg_sz);
        goto out;
    }

    /* need another section, and start from next region*/
    rg_start += rg_sz;
    id = MPU_ID_FRAM_TEXT2;
    ret = mpu_set(id, rg_start, rg_sz, MPU_ATTR_READ_EXEC);

    rg_end = rg_start + rg_sz;
    if (fr_end < rg_end) {
        ret = mpu_enable_top_sub_region(id, rg_end, fr_end, sub_rg_sz);
        goto out;
    }

    /* if fr_end large than two section, just pass */

out:
    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
    return ret;
}

#elif defined(__ARM_ARCH_8M_MAIN__)
static int mpu_set_armv8(enum MPU_ID_T id, uint32_t addr, uint32_t len, enum MPU_ATTR_T attr)
{
    int ret;
    uint32_t rbar;
    uint32_t rlar;
    uint8_t xn;
    uint8_t ro;
    uint32_t lock;

    if ((MPU->CTRL & MPU_CTRL_ENABLE_Msk) == 0) {
        ret = mpu_open();
        if (ret) {
            return ret;
        }
    }

    if (id >= MPU_ID_QTY) {
        return 2;
    }
    if (len < 32) {
        return 3;
    }
    if (addr & 0x1F) {
        return 4;
    }
    if (attr >= MPU_ATTR_QTY) {
        return 5;
    }

    if (attr == MPU_ATTR_READ_WRITE_EXEC || attr == MPU_ATTR_READ_EXEC ||
            attr == MPU_ATTR_EXEC) {
        xn = 0;
    } else {
        xn = 1;
    }

    if (attr == MPU_ATTR_READ_WRITE_EXEC || attr == MPU_ATTR_READ_WRITE) {
        ro = 0;
    } else if (attr == MPU_ATTR_READ_EXEC || attr == MPU_ATTR_READ) {
        ro = 1;
    } else {
        // Cannot support no access
        return 6;
    }

    // Sharebility: Outer Shareable
    // Non-privilege Access: Enalbed
    rbar = ARM_MPU_RBAR(addr, 2, ro, 1, xn);
    rlar = ARM_MPU_RLAR((addr + len - 1), MAIR_ATTR_INT_SRAM);

    lock = int_lock();

    ARM_MPU_SetRegion(id, rbar, rlar);
    __DSB();
    __ISB();

    int_unlock(lock);

    return 0;
}

static inline int mpu_fram_protect_armv8(uint32_t fr_start, uint32_t fr_end)
{
    return 0;
}
#endif

int mpu_set(enum MPU_ID_T id, uint32_t addr, uint32_t len, enum MPU_ATTR_T attr)
{
#ifdef __ARM_ARCH_7EM__
    return mpu_set_armv7(id, addr, len, attr);
#elif defined(__ARM_ARCH_8M_MAIN__)
    return mpu_set_armv8(id, addr, len, attr);
#else
    return -1;
#endif
}

int mpu_clear(enum MPU_ID_T id)
{
    uint32_t lock;

    if (id >= MPU_ID_QTY) {
        return 2;
    }

    lock = int_lock();

    ARM_MPU_ClrRegion(id);
    __DSB();
    __ISB();

    int_unlock(lock);

    return 0;
}

int mpu_null_check_enable(void)
{
    int ret;

#ifdef CHIP_BEST1000
    ret = mpu_set(MPU_ID_NULL_POINTER, 0, 0x400, MPU_ATTR_NO_ACCESS);
#else
    ret = mpu_set(MPU_ID_NULL_POINTER, 0, 0x800, MPU_ATTR_NO_ACCESS);
#endif
    return ret;
}

extern uint32_t __fast_sram_text_exec_start__[];
extern uint32_t __fast_sram_text_exec_end__[];

int mpu_fast_ram_protect(void)
{
    uint32_t start  = (uint32_t)__fast_sram_text_exec_start__;
    uint32_t end = (uint32_t)__fast_sram_text_exec_end__;

#ifdef __ARM_ARCH_7EM__
    return mpu_fram_protect_armv7(start, end);
#elif defined(__ARM_ARCH_8M_MAIN__)
    return mpu_fram_protect_armv8(start, end);
#else
    return -1;
#endif
}

#endif
