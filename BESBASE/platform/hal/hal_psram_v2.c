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
#if defined(CHIP_HAS_PSRAM) && (CHIP_PSRAM_CTRL_VER >= 2)

#include "plat_types.h"
#include "plat_addr_map.h"
#include "hal_location.h"
#include "hal_psram.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "pmu.h"
#include "reg_psram_mc_v2.h"
#include "reg_psram_phy_v2.h"

#define PSRAM_RESET
//#define PSRAM_DUAL_8BIT
//#define PSRAM_WRAP_ENABLE

#define TX_FIFO_DEPTH                       8
#define RX_FIFO_DEPTH                       8

// MR0
#define MR0_DRIVE_STR_SHIFT                 0
#define MR0_DRIVE_STR_MASK                  (0x3 << MR0_DRIVE_STR_SHIFT)
#define MR0_DRIVE_STR(n)                    BITFIELD_VAL(MR0_DRIVE_STR, n)
#define MR0_LATENCY_CODE_SHIFT              2
#define MR0_LATENCY_CODE_MASK               (0x7 << MR0_LATENCY_CODE_SHIFT)
#define MR0_LATENCY_CODE(n)                 BITFIELD_VAL(MR0_LATENCY_CODE, n)
#define MR0_LT                              (1 << 5)
#define MR0_FIXED_00_SHIFT                  6
#define MR0_FIXED_00_MASK                   (0x3 << MR0_FIXED_00_SHIFT)
#define MR0_FIXED_00(n)                     BITFIELD_VAL(MR0_FIXED_00, n)

// MR1
#define MR1_VENDOR_ID_SHIFT                 0
#define MR1_VENDOR_ID_MASK                  (0x1F << MR1_VENDOR_ID_SHIFT)
#define MR1_VENDOR_ID(n)                    BITFIELD_VAL(MR1_VENDOR_ID, n)
#define MR1_DENSITY                         (1 << 5)
#define MR1_REV                             (1 << 6)
#define MR1_ULP                             (1 << 7)

// MR2
#define MR2_FIXED_001_SHIFT                 0
#define MR2_FIXED_001_MASK                  (0x7 << MR2_FIXED_001_SHIFT)
#define MR2_FIXED_001(n)                    BITFIELD_VAL(MR2_FIXED_001, n)
#define MR2_DEV_ID_SHIFT                    3
#define MR2_DEV_ID_MASK                     (0x3 << MR2_DEV_ID_SHIFT)
#define MR2_DEV_ID(n)                       BITFIELD_VAL(MR2_DEV_ID, n)
#define MR2_RSVD                            (1 << 5)
#define MR2_FIXED_1                         (1 << 6)
#define MR2_GB                              (1 << 7)

// MR4
#define MR4_PASR_SHIFT                      0
#define MR4_PASR_MASK                       (0x7 << MR4_PASR_SHIFT)
#define MR4_PASR(n)                         BITFIELD_VAL(MR4_PASR, n)
#define MR4_RF                              (1 << 3)
#define MR4_RSVD_SHIFT                      4
#define MR4_RSVD_MASK                       (0x7 << MR4_RSVD_SHIFT)
#define MR4_RSVD(n)                         BITFIELD_VAL(MR4_RSVD, n)
#define MR4_WL                              (1 << 7)

// MR6
#define MR6_RSVD_SHIFT                      0
#define MR6_RSVD_MASK                       (0xF << MR6_RSVD_SHIFT)
#define MR6_RSVD(n)                         BITFIELD_VAL(MR6_RSVD, n)
#define MR6_HALF_SLEEP_SHIFT                4
#define MR6_HALF_SLEEP_MASK                 (0xF << MR6_HALF_SLEEP_SHIFT)
#define MR6_HALF_SLEEP(n)                   BITFIELD_VAL(MR6_HALF_SLEEP, n)

enum PSRAM_CMD_T {
    PSRAM_CMD_SYNC_READ     = 0x00,
    PSRAM_CMD_SYNC_WRITE    = 0x80,
    PSRAM_CMD_4BYTE_READ    = 0x3F,
    PSRAM_CMD_4BYTE_WRITE   = 0xBF,
    PSRAM_CMD_REG_READ      = 0x40,
    PSRAM_CMD_REG_WRITE     = 0xC0,
    PSRAM_CMD_GLOBAL_RESET  = 0xFF,
};

enum CP_FSM_STATE_T {
    CP_FSM_STATE_SELF_REFRESH   = 1,
    CP_FSM_STATE_PD             = 2,
    CP_FSM_STATE_READY          = 4,
};

enum MEMIF_CMD_T {
    MEMIF_NO_CMD        = 0x00,
    MEMIF_WRITE         = 0x01,
    MEMIF_READ          = 0x02,
    MEMIF_MRS           = 0x05,
    MEMIF_MRR           = 0x06,
    MEMIF_REF           = 0x08,
    MEMIF_SREF          = 0x09,
    MEMIF_PD            = 0x10,
    MEMIF_NOP           = 0x20,
    MEMIF_RST           = 0xFF,
    MEMIF_ZQCL          = 0x85,
    MEMIF_ZQCS          = 0x45,
    MEMIF_ZQCRST        = 0x25,
    MEMIF_START_CLOCK   = 0x40,
    MEMIF_STOP_CLOCK    = 0x80,
    MEMIF_NEW_CMD       = 0x7F,
};

static struct PSRAM_MC_T * const psram_mc = (struct PSRAM_MC_T *)PSRAM_CTRL_BASE;
static struct PSRAM_PHY_T * const psram_phy = (struct PSRAM_PHY_T *)(PSRAM_CTRL_BASE + 0x8000);

static const uint32_t psram_cfg_clk = 52000000;
static const uint32_t psram_run_clk = 192000000;

static void psram_chip_timing_config(uint32_t clk, bool psram_first);

int hal_psramip_mc_busy(void)
{
    return !!(psram_mc->REG_404 & PSRAM_ULP_MC_BUSY);
}

int hal_psramip_mc_in_sleep(void)
{
    return GET_BITFIELD(psram_mc->REG_404, PSRAM_ULP_MC_CP_FSM_STATE) == CP_FSM_STATE_PD;
}

int hal_psramip_rx_fifo_empty(void)
{
    return !!(psram_mc->REG_404 & PSRAM_ULP_MC_MGR_RXFIFO_R_EMPTY);
}

int hal_psramip_tx_fifo_full(void)
{
    return !!(psram_mc->REG_404 & PSRAM_ULP_MC_MGR_TXFIFO_W_FULL);
}

uint32_t hal_psramip_get_rx_fifo_len(void)
{
    return GET_BITFIELD(psram_mc->REG_404, PSRAM_ULP_MC_MGR_RXFIFO_FULL_CNT);
}

uint32_t hal_psramip_get_tx_fifo_free_len(void)
{
    return GET_BITFIELD(psram_mc->REG_404, PSRAM_ULP_MC_MGR_TXFIFO_EMPTY_CNT);
}

void hal_psramip_mc_busy_wait(void)
{
    while (hal_psramip_mc_busy());
}

void hal_psramip_flush_tx_fifo(void)
{
    psram_mc->REG_01C = PSRAM_ULP_MC_MGR_TX_FIFO_CLR;
}

void hal_psramip_flush_rx_fifo(void)
{
    psram_mc->REG_01C = PSRAM_ULP_MC_MGR_RX_FIFO_CLR;
}

void hal_psramip_flush_all_fifo(void)
{
    psram_mc->REG_01C = PSRAM_ULP_MC_MGR_TX_FIFO_CLR | PSRAM_ULP_MC_MGR_RX_FIFO_CLR;
}

void hal_psramip_xfer_addr_len(uint32_t addr, uint32_t len)
{
    psram_mc->REG_008 = addr;
    psram_mc->REG_00C = len;
}

void hal_psramip_write_fifo(uint32_t *data, uint32_t len)
{
    for (int i = 0; i < len; i++) {
        psram_mc->REG_014 = *data++;
        data++;
    }
}

void hal_psramip_read_fifo(uint32_t *data, uint32_t len)
{
    for (int i = 0; i < len; i++) {
        *data++ = psram_mc->REG_018;
    }
}

void hal_psramip_set_reg_data_mask(void)
{
#ifdef PSRAM_DUAL_8BIT
    psram_mc->REG_010 = 0xFC;
#else
    psram_mc->REG_010 = 0xFE;
#endif
}

void hal_psramip_set_mem_data_mask(void)
{
    psram_mc->REG_010 = 0;
}

void hal_psramip_set_cmd(enum MEMIF_CMD_T cmd)
{
    psram_mc->REG_004 = cmd;
}

static POSSIBLY_UNUSED void psram_read_reg(uint32_t reg, uint32_t *val)
{
    hal_psramip_mc_busy_wait();
    hal_psramip_flush_all_fifo();
    hal_psramip_xfer_addr_len(reg, 1);
    hal_psramip_set_cmd(MEMIF_MRR);
    while (hal_psramip_rx_fifo_empty());
    hal_psramip_read_fifo(val, 1);
}

static void psram_send_cmd_reg(enum MEMIF_CMD_T cmd, uint32_t reg, uint32_t val)
{
#ifdef PSRAM_DUAL_8BIT
    val &= 0xFF;
    val |= (val << 8);
#endif
    hal_psramip_mc_busy_wait();
    hal_psramip_flush_all_fifo();
    //hal_psramip_set_reg_data_mask();
    hal_psramip_write_fifo(&val, 1);
    hal_psramip_xfer_addr_len(reg, 1);
    hal_psramip_set_cmd(cmd);
    while (hal_psramip_get_tx_fifo_free_len() != TX_FIFO_DEPTH);
    hal_psramip_mc_busy_wait();
    //hal_psramip_set_mem_data_mask();
}

static void psram_write_reg(uint32_t reg, uint32_t val)
{
    psram_send_cmd_reg(MEMIF_MRS, reg, val);
}

static void psram_single_cmd(enum MEMIF_CMD_T cmd)
{
    hal_psramip_mc_busy_wait();
    hal_psramip_flush_all_fifo();
    hal_psramip_set_cmd(cmd);
    hal_psramip_mc_busy_wait();
}

static POSSIBLY_UNUSED void psram_reset(void)
{
    psram_single_cmd(MEMIF_RST);
}

static void psram_set_timing(uint32_t clk)
{
    uint32_t reg;
    uint32_t val;

    reg = 0;
    val = MR0_DRIVE_STR(3);
    if (clk <= 66000000) {
        val |= MR0_LATENCY_CODE(2);
    } else if (clk <= 109000000) {
        val |= MR0_LATENCY_CODE(3);
    } else if (clk <= 133000000) {
        val |= MR0_LATENCY_CODE(4);
    } else if (clk <= 166000000) {
        val |= MR0_LATENCY_CODE(5);
    } else {
        val |= MR0_LATENCY_CODE(6);
    }
    psram_write_reg(reg, val);

    reg = 4;
    val = MR4_PASR(0) | MR4_RF;
    if (clk > 166000000) {
        val |= MR4_WL;
    }
    psram_write_reg(reg, val);
}

static void hal_psram_phy_dll_config(uint32_t clk)
{
    uint32_t phy_clk;
    uint32_t range;

    psram_phy->REG_050 &= ~PSRAM_ULP_PHY_REG_DLL_RESETB | PSRAM_ULP_PHY_REG_DLL_CK_RDY;
    phy_clk = clk * 2;
    if (phy_clk <= 100000000 / 2) {
        range = 3;
    } else if (phy_clk <= 150000000 / 2) {
        range = 2;
    } else if (phy_clk <= 300000000 / 2) {
        range = 1;
    } else {
        range = 0;
    }
    psram_phy->REG_050 = SET_BITFIELD(psram_phy->REG_050, PSRAM_ULP_PHY_REG_DLL_RANGE, range);
    psram_phy->REG_050 |= PSRAM_ULP_PHY_REG_DLL_RESETB | PSRAM_ULP_PHY_REG_DLL_CK_RDY;
}

static void hal_psram_phy_init(uint32_t clk)
{
    psram_phy->REG_048 |= PSRAM_ULP_PHY_REG_LDO_PU | PSRAM_ULP_PHY_REG_LDO_PRECHARGE;
    hal_sys_timer_delay_us(10);
    psram_phy->REG_048 &= ~PSRAM_ULP_PHY_REG_LDO_PRECHARGE;
    psram_phy->REG_04C |= PSRAM_ULP_PHY_REG_PSRAM_PU;
    psram_phy->REG_050 |= PSRAM_ULP_PHY_REG_DLL_PU;
    hal_sys_timer_delay_us(2);
    hal_psram_phy_dll_config(clk);
}

static void hal_psram_mc_set_timing(uint32_t clk)
{
    uint32_t val;

    if (clk <= 166000000) {
        val = PSRAM_ULP_MC_WRITE_LATENCY(0);
    } else {
        val = PSRAM_ULP_MC_WRITE_LATENCY(2);
#if (CHIP_PSRAM_CTRL_VER == 2)
        psram_mc->REG_024 &= ~PSRAM_ULP_MC_ENTRY_SLEEP_IDLE;
#endif
    }
    psram_mc->REG_028 = val;
#if (CHIP_PSRAM_CTRL_VER == 2)
    if (clk <= 66000000) {
        val = PSRAM_ULP_MC_READ_LATENCY(2);
    } else if (clk <= 109000000) {
        val = PSRAM_ULP_MC_READ_LATENCY(3);
    } else if (clk <= 133000000) {
        val = PSRAM_ULP_MC_READ_LATENCY(4);
    } else if (clk <= 166000000) {
        val = PSRAM_ULP_MC_READ_LATENCY(5);
    } else {
        val = PSRAM_ULP_MC_READ_LATENCY(6);
    }
    psram_mc->REG_02C = val;
#else
    // Min latency: 2 cycles
    psram_mc->REG_02C = PSRAM_ULP_MC_READ_LATENCY(2);
#endif
    // tRC >= 55 ns
    val = (clk / 1000000 * 55 + (1000 - 1)) / 1000;
    psram_mc->REG_050 = PSRAM_ULP_MC_T_RC(val);
    val = 2;
    psram_mc->REG_058 = PSRAM_ULP_MC_T_CPHR(val);
    psram_mc->REG_068 = PSRAM_ULP_MC_T_MRR(val);
    val = 4;
    psram_mc->REG_060 = PSRAM_ULP_MC_T_CPHW(val);
#ifdef CHIP_BEST2001
    val += 1;
#endif
    psram_mc->REG_06C = PSRAM_ULP_MC_T_MRS(val);
    // tCEM <= 2.5 us
    val = clk / 1000000 * 25 / 10;
    psram_mc->REG_070 = PSRAM_ULP_MC_T_CEM(val);
    // tRST >= 2 us
    val = clk / 1000000 * 2 + 1;
    psram_mc->REG_074 = PSRAM_ULP_MC_T_RST(val);
    // tHS >= 4 us
    val = clk / 1000000 * 4 + 1;
    psram_mc->REG_080 = PSRAM_ULP_MC_T_HS(val);
    // tXPHS in [60 ns, 4 us]
    val = (clk / 1000000 * 60 + (1000 - 1)) / 1000;
    psram_mc->REG_084 = PSRAM_ULP_MC_T_XPHS(val);
    // tXHS >= 70 us
    val = clk / 1000000 * 70 + 1;
    psram_mc->REG_088 = PSRAM_ULP_MC_T_XHS(val);
    psram_mc->REG_09C = PSRAM_ULP_MC_WR_DMY_CYC(1);
    // NOP dummy cycles, same as tXPHS in [60 ns, 4 us]
    val = (clk / 1000000 * 60 + (1000 - 1)) / 1000;
    psram_mc->REG_0A0 = PSRAM_ULP_MC_STOP_CLK_IN_NOP | PSRAM_ULP_MC_NOP_DMY_CYC(val);
    psram_mc->REG_0A4 = PSRAM_ULP_MC_QUEUE_IDLE_CYCLE(5000);
}

static void hal_psram_init_calib(void)
{
    uint32_t delay;

    while ((psram_phy->REG_058 & PSRAM_ULP_PHY_DLL_LOCK) == 0);

    delay = GET_BITFIELD(psram_phy->REG_058, PSRAM_ULP_PHY_DLL_DLY_IN);
    ASSERT(delay < (PSRAM_ULP_PHY_DLL_DLY_IN_MASK >> PSRAM_ULP_PHY_DLL_DLY_IN_SHIFT),
        "%s: Bad DLL_DLY_IN=0x%X reg=0x%08X", __func__, delay, psram_phy->REG_058);

    delay /= 2;
    psram_phy->REG_054 = PSRAM_ULP_PHY_REG_PSRAM_TX_CEB_DLY(delay) | PSRAM_ULP_PHY_REG_PSRAM_TX_CLK_DLY(delay) |
        PSRAM_ULP_PHY_REG_PSRAM_TX_DQS_DLY(delay) | PSRAM_ULP_PHY_REG_PSRAM_RX_DQS_DLY(delay);
}

static void hal_psram_mc_init(uint32_t clk)
{
#ifdef PSRAM_DUAL_8BIT
    psram_mc->REG_000 = PSRAM_ULP_MC_CHIP_BIT;
#else
    psram_mc->REG_000 = 0;
#endif
    psram_mc->REG_020 = 0;
    psram_mc->REG_024 = PSRAM_ULP_MC_ENTRY_SLEEP_IDLE | PSRAM_ULP_MC_AUTOWAKEUP_EN |
        PSRAM_ULP_MC_PD_MR(6) | PSRAM_ULP_MC_PD_CMD(0xF0);
#ifdef PSRAM_WRAP_ENABLE
    // Burst len: 32 bytes, page: 1K
    psram_mc->REG_034 = PSRAM_ULP_MC_BURST_LENGTH(1) | PSRAM_ULP_MC_PAGE_BOUNDARY(0);
#else
    // Burst len: 1K, page: 1K
    psram_mc->REG_034 = PSRAM_ULP_MC_BURST_LENGTH(4) | PSRAM_ULP_MC_PAGE_BOUNDARY(0);
#endif
    // AHB bus width: 32 bits
    psram_mc->REG_038 = 0;
    // Write buffer level with high priority: 0~7
    psram_mc->REG_03C = PSRAM_ULP_MC_HIGH_PRI_LEVEL(4);
#ifdef PSRAM_WRAP_ENABLE
    psram_mc->REG_040 = PSRAM_ULP_MC_CP_WRAP_EN;
#else
    psram_mc->REG_040 = PSRAM_ULP_MC_WRAP_CRT_RET_EN;
#endif
    psram_mc->REG_044 = 0;
    psram_mc->REG_048 = 0;

    hal_psramip_set_reg_data_mask();

    hal_psram_mc_set_timing(clk);

    psram_mc->REG_400 = PSRAM_ULP_MC_INIT_COMPLETE;

    hal_psram_init_calib();
}

void hal_psram_sleep(void)
{
    hal_psramip_mc_busy_wait();
    if (!hal_psramip_mc_in_sleep()) {
        psram_mc->REG_024 &= ~PSRAM_ULP_MC_ENTRY_SLEEP_IDLE;
        hal_psramip_mc_busy_wait();
        hal_psramip_set_cmd(MEMIF_PD);
        hal_psramip_mc_busy_wait();
    }
}

void hal_psram_wakeup(void)
{
    hal_psramip_mc_busy_wait();
    psram_mc->REG_024 |= PSRAM_ULP_MC_ENTRY_SLEEP_IDLE;
}

static void psram_chip_timing_config(uint32_t clk, bool update_psram_first)
{
    enum HAL_CMU_FREQ_T freq;

    if (clk <= 52000000) {
        freq = HAL_CMU_FREQ_104M;
    } else {
#ifdef HAL_CMU_FREQ_T
        freq = HAL_CMU_FREQ_390M;
#else
        freq = HAL_CMU_FREQ_208M;
#endif
    }

    if (update_psram_first) {
        psram_set_timing(clk);
    }
    hal_cmu_mem_set_freq(freq);
    hal_sys_timer_delay_us(3);
    hal_psram_phy_dll_config(clk);
    hal_psram_init_calib();
    hal_psram_mc_set_timing(clk);
    if (!update_psram_first) {
        psram_set_timing(clk);
    }
}

static void hal_psram_calib(void)
{
}

void hal_psram_init(void)
{
    hal_cmu_mem_set_freq(HAL_CMU_FREQ_104M);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_PSRAM);
    hal_cmu_clock_enable(HAL_CMU_MOD_H_PSRAM);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_PSRAM);
    hal_cmu_reset_clear(HAL_CMU_MOD_H_PSRAM);

    hal_psram_phy_init(psram_cfg_clk);
    hal_sys_timer_delay_us(3);
    hal_psram_mc_init(psram_cfg_clk);

#ifdef PSRAM_RESET
    psram_reset();
    psram_chip_timing_config(psram_run_clk, true);
#else
    uint32_t reg;
    uint32_t val;

    reg = 4;
    psram_read_reg(reg, &val);
    if (val & MR4_WL) {
        psram_chip_timing_config(psram_run_clk, false);
    } else {
        psram_chip_timing_config(psram_run_clk, true);
    }
#endif

    hal_psram_calib();
}

#endif

