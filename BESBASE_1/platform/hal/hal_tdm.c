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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hal_i2s.h"
#include "hal_trace.h"
#include "hal_dma.h"
#include "hal_tdm.h"

static struct HAL_DMA_DESC_T i2s_0_rx_dma_desc[MAX_TDM_FRAME_NUM];
static struct HAL_DMA_DESC_T i2s_0_tx_dma_desc[MAX_TDM_FRAME_NUM];

static struct HAL_DMA_DESC_T i2s_1_rx_dma_desc[MAX_TDM_FRAME_NUM];
static struct HAL_DMA_DESC_T i2s_1_tx_dma_desc[MAX_TDM_FRAME_NUM];


static const char * const invalid_id = "Invalid I2S ID: %d\n";
static inline uint32_t _tdm_get_reg_base(enum HAL_I2S_ID_T id)
{
    ASSERT(id < HAL_I2S_ID_QTY, invalid_id, id);
    switch(id) {
        case HAL_I2S_ID_0:
            return (I2S0_BASE | I2SIP_TDM_CTRL_REG_OFFSET);
            break;
#if defined(CHIP_HAS_I2S) && (CHIP_HAS_I2S > 1)
        case HAL_I2S_ID_1:
            return (I2S1_BASE | I2SIP_TDM_CTRL_REG_OFFSET);
            break;
#endif
        default:
            break;
    }
	return 0;
}

#if 0
static void tdm_get_config(enum HAL_I2S_ID_T i2s_id,struct HAL_TDM_CONFIG_T *tdm_config)
{
    volatile uint32_t *base_addr;
    uint32_t val;

    base_addr = (uint32_t*)_tdm_get_reg_base(i2s_id);
    val = *base_addr;

    if((val & (TDM_MODE_FS_ASSERTED_AT_LAST << TDM_MODE_FS_ASSERTED_SHIFT)) == (TDM_MODE_FS_ASSERTED_AT_LAST << TDM_MODE_FS_ASSERTED_SHIFT))
    {
        tdm_config->mode = HAL_TDM_MODE_FS_ASSERTED_AT_LAST;
    }
    else
    {
        tdm_config->mode = HAL_TDM_MODE_FS_ASSERTED_AT_FIRST;
    }

    if((val & (TDM_FS_EDGE_NEGEDGE << TDM_FS_EDGE_SHIFT)) == (TDM_FS_EDGE_NEGEDGE << TDM_FS_EDGE_SHIFT))
    {
        tdm_config->edge = HAL_TDM_FS_EDGE_NEGEDGE;
    }
    else
    {
        tdm_config->edge = HAL_TDM_FS_EDGE_POSEDGE;
    }

    if((val & (TDM_FRAME_WIDTH_16_CYCLES << TDM_FRAME_WIDTH_SHIFT)) == (TDM_FRAME_WIDTH_16_CYCLES << TDM_FRAME_WIDTH_SHIFT))
    {
        tdm_config->frame_width = HAL_TDM_FRAME_WIDTH_16_CYCLES;
    }
    else if((val & (TDM_FRAME_WIDTH_32_CYCLES << TDM_FRAME_WIDTH_SHIFT)) == (TDM_FRAME_WIDTH_32_CYCLES << TDM_FRAME_WIDTH_SHIFT))
    {
        tdm_config->frame_width = HAL_TDM_FRAME_WIDTH_32_CYCLES;
    }
    else if((val & (TDM_FRAME_WIDTH_64_CYCLES << TDM_FRAME_WIDTH_SHIFT)) == (TDM_FRAME_WIDTH_64_CYCLES << TDM_FRAME_WIDTH_SHIFT))
    {
        tdm_config->frame_width = HAL_TDM_FRAME_WIDTH_64_CYCLES;
    }
    else if((val & (TDM_FRAME_WIDTH_128_CYCLES << TDM_FRAME_WIDTH_SHIFT)) == (TDM_FRAME_WIDTH_128_CYCLES << TDM_FRAME_WIDTH_SHIFT))
    {
        tdm_config->frame_width = HAL_TDM_FRAME_WIDTH_128_CYCLES;
    }
    else if((val & (TDM_FRAME_WIDTH_256_CYCLES << TDM_FRAME_WIDTH_SHIFT)) == (TDM_FRAME_WIDTH_256_CYCLES << TDM_FRAME_WIDTH_SHIFT))
    {
        tdm_config->frame_width = HAL_TDM_FRAME_WIDTH_256_CYCLES;
    }
    else
    {
        tdm_config->frame_width = HAL_TDM_FRAME_WIDTH_256_CYCLES;
    }

    if((val & (TDM_FS_WIDTH_8_CYCLES << TDM_FS_WIDTH_SHIFT)) == (TDM_FS_WIDTH_8_CYCLES << TDM_FS_WIDTH_SHIFT))
    {
        tdm_config->frame_width = HAL_TDM_FS_WIDTH_8_CYCLES;
    }
    else if((val & (TDM_FS_WIDTH_16_CYCLES << TDM_FS_WIDTH_SHIFT)) == (TDM_FS_WIDTH_16_CYCLES << TDM_FS_WIDTH_SHIFT))
    {
        tdm_config->frame_width = HAL_TDM_FS_WIDTH_16_CYCLES;
    }
    else if((val & (TDM_FS_WIDTH_32_CYCLES << TDM_FS_WIDTH_SHIFT)) == (TDM_FS_WIDTH_32_CYCLES << TDM_FS_WIDTH_SHIFT))
    {
        tdm_config->frame_width = HAL_TDM_FS_WIDTH_32_CYCLES;
    }
    else if((val & (TDM_FS_WIDTH_64_CYCLES << TDM_FS_WIDTH_SHIFT)) == (TDM_FS_WIDTH_64_CYCLES << TDM_FS_WIDTH_SHIFT))
    {
        tdm_config->frame_width = HAL_TDM_FS_WIDTH_64_CYCLES;
    }
    else if((val & (TDM_FS_WIDTH_128_CYCLES << TDM_FS_WIDTH_SHIFT)) == (TDM_FS_WIDTH_128_CYCLES << TDM_FS_WIDTH_SHIFT))
    {
        tdm_config->frame_width = HAL_TDM_FS_WIDTH_128_CYCLES;
    }
    else if((val & (TDM_FS_WIDTH_1_CYCLE << TDM_FS_WIDTH_SHIFT)) == (TDM_FS_WIDTH_1_CYCLE << TDM_FS_WIDTH_SHIFT))
    {
        tdm_config->frame_width = HAL_TDM_FS_WIDTH_FRAME_LENGTH_1_CYCLES;
    }
    else
    {
        tdm_config->frame_width = HAL_TDM_FS_WIDTH_FRAME_LENGTH_1_CYCLES;
    }

    if((val & (TDM_SLOT_WIDTH_16_BIT << TDM_SLOT_WIDTH_SHIFT)) == (TDM_SLOT_WIDTH_16_BIT << TDM_SLOT_WIDTH_SHIFT))
    {
        tdm_config->edge = HAL_TDM_SLOT_WIDTH_16_BIT;
    }
    else
    {
        tdm_config->edge = HAL_TDM_SLOT_WIDTH_32_BIT;
    }

    tdm_config->data_offset = ((val >> TDM_DATA_OFFSET_SHIT) & 0x7);
}
#endif

static void tdm_set_config(enum HAL_I2S_ID_T i2s_id,struct HAL_TDM_CONFIG_T *tdm_config)
{
    volatile uint32_t *base_addr;
    uint32_t val = 0;

    base_addr = (uint32_t*)_tdm_get_reg_base(i2s_id);

    if(tdm_config->mode == HAL_TDM_MODE_FS_ASSERTED_AT_LAST) {
        val |= (TDM_MODE_FS_ASSERTED_AT_LAST << TDM_MODE_FS_ASSERTED_SHIFT);
    }

    if(tdm_config->edge == HAL_TDM_FS_EDGE_NEGEDGE) {
        val |= (TDM_FS_EDGE_NEGEDGE << TDM_FS_EDGE_SHIFT);
    }

    if(tdm_config->frame_width == HAL_TDM_FRAME_WIDTH_16_CYCLES) {
        val |= (TDM_FRAME_WIDTH_16_CYCLES << TDM_FRAME_WIDTH_SHIFT);
    }
    else if(tdm_config->frame_width == HAL_TDM_FRAME_WIDTH_32_CYCLES) {
        val |= (TDM_FRAME_WIDTH_32_CYCLES << TDM_FRAME_WIDTH_SHIFT);
    }
    else if(tdm_config->frame_width == HAL_TDM_FRAME_WIDTH_64_CYCLES) {
        val |= (TDM_FRAME_WIDTH_64_CYCLES << TDM_FRAME_WIDTH_SHIFT);
    }
    else if(tdm_config->frame_width == HAL_TDM_FRAME_WIDTH_128_CYCLES) {
        val |= (TDM_FRAME_WIDTH_128_CYCLES << TDM_FRAME_WIDTH_SHIFT);
    }
    else {
        val |= (TDM_FRAME_WIDTH_256_CYCLES << TDM_FRAME_WIDTH_SHIFT);
    }

    if(tdm_config->fs_width == HAL_TDM_FS_WIDTH_8_CYCLES) {
        val |= (TDM_FS_WIDTH_8_CYCLES << TDM_FS_WIDTH_SHIFT);
    }
    else if(tdm_config->fs_width == HAL_TDM_FS_WIDTH_16_CYCLES)
    {
        val |= (TDM_FS_WIDTH_16_CYCLES << TDM_FS_WIDTH_SHIFT);
    }
    else if(tdm_config->fs_width == HAL_TDM_FS_WIDTH_32_CYCLES)
    {
        val |= (TDM_FS_WIDTH_32_CYCLES << TDM_FS_WIDTH_SHIFT);
    }
    else if(tdm_config->fs_width == HAL_TDM_FS_WIDTH_64_CYCLES)
    {
        val |= (TDM_FS_WIDTH_64_CYCLES << TDM_FS_WIDTH_SHIFT);
    }
    else if(tdm_config->fs_width == HAL_TDM_FS_WIDTH_128_CYCLES)
    {
        val |= (TDM_FS_WIDTH_128_CYCLES << TDM_FS_WIDTH_SHIFT);
    }
    else if(tdm_config->fs_width == HAL_TDM_FS_WIDTH_FRAME_LENGTH_1_CYCLES)
    {
        val |= (TDM_FS_WIDTH_FRAME_LENGTH_1_CYCLES << TDM_FS_WIDTH_SHIFT);
    }
    else
    {
        val |= (TDM_FS_WIDTH_1_CYCLE << TDM_FS_WIDTH_SHIFT);
    }

    if(tdm_config->slot_width == HAL_TDM_SLOT_WIDTH_16_BIT)
    {
        val |= (TDM_SLOT_WIDTH_16_BIT << TDM_SLOT_WIDTH_SHIFT);
    }

    if(tdm_config->data_offset >= TDM_DATA_OFFSET_MIN
        && tdm_config->data_offset <= TDM_DATA_OFFSET_MAX)
    {
        val |= (tdm_config->data_offset << TDM_DATA_OFFSET_SHIT);
    }

    *base_addr = val;
}

static void tdm_get_default_config(struct HAL_TDM_CONFIG_T *tdm_config)
{
    tdm_config->mode = HAL_TDM_MODE_FS_ASSERTED_AT_FIRST;
    tdm_config->edge = HAL_TDM_FS_EDGE_POSEDGE;
    tdm_config->frame_width = HAL_TDM_FRAME_WIDTH_256_CYCLES;
    tdm_config->fs_width = HAL_TDM_FS_WIDTH_1_CYCLE;
    tdm_config->slot_width = HAL_TDM_SLOT_WIDTH_32_BIT;
    tdm_config->data_offset = 0;
}

static void tdm_get_i2s_config(struct HAL_TDM_CONFIG_T *tdm_config)
{
    tdm_config->mode = HAL_TDM_MODE_FS_ASSERTED_AT_LAST;
    tdm_config->edge = HAL_TDM_FS_EDGE_NEGEDGE;
    tdm_config->frame_width = HAL_TDM_FRAME_WIDTH_32_CYCLES;
    tdm_config->fs_width = HAL_TDM_FS_WIDTH_16_CYCLES;
    tdm_config->slot_width = HAL_TDM_SLOT_WIDTH_16_BIT;
    tdm_config->data_offset = 0;
}

static void tdm_enable(enum HAL_I2S_ID_T i2s_id,bool enable)
{
    volatile uint32_t *base_addr;
    uint32_t val = 0;

    base_addr = (uint32_t*)_tdm_get_reg_base(i2s_id);
    val = *base_addr;

    if(enable)
    {
        val |= (TDM_ENABLE << TDM_ENABLE_SHIFT);
    }
    else
    {
        val &= ~(TDM_ENABLE << TDM_ENABLE_SHIFT);
    }
    *base_addr = val;
}

int32_t hal_tdm_open(enum HAL_I2S_ID_T i2s_id,enum AUD_STREAM_T stream,enum HAL_I2S_MODE_T mode)
{
    int ret;
    struct HAL_TDM_CONFIG_T tdm_config;

    TRACE("hal_tdm_open:i2s_id = %d, stream = %d, mode = %d.",
           i2s_id, stream, mode);
    ASSERT(i2s_id < HAL_I2S_ID_QTY,"i2s_id = %d!",i2s_id);

    // i2s open playback and capture.
    ret = hal_i2s_open(i2s_id, stream, mode);
    if(ret)
    {
        TRACE("hal_i2s_open failed.ret = %d.", ret);
    }
    tdm_enable(i2s_id,false);
    tdm_get_default_config(&tdm_config);
    tdm_set_config(i2s_id, &tdm_config);
    TRACE("hal_tdm_open done.");
    return ret;
}

int32_t hal_tdm_setup_stream(enum HAL_I2S_ID_T i2s_id,
                                       enum AUD_STREAM_T stream,
                                       uint32_t sample_rate,
                                       struct HAL_TDM_CONFIG_T *tdm_cofig)
{
    struct HAL_I2S_CONFIG_T i2s_cfg;
    int ret;

    TRACE("hal_tdm_setup_stream:i2s_id = %d, stream = %d, sample_rate = %d.",
        i2s_id, stream, sample_rate);
    ASSERT(i2s_id < HAL_I2S_ID_QTY,"i2s_id = %d!",i2s_id);

    // i2s setup stream playback and capture.
    memset(&i2s_cfg, 0, sizeof(i2s_cfg));
    i2s_cfg.use_dma = true;
    i2s_cfg.master_clk_wait = stream == AUD_STREAM_PLAYBACK ? true : false;
    i2s_cfg.chan_sep_buf = false;
    i2s_cfg.bits = 16;
    i2s_cfg.channel_num = 2;
    i2s_cfg.channel_map = AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1;
    i2s_cfg.sample_rate = sample_rate;
    ret = hal_i2s_setup_stream(i2s_id, stream, &i2s_cfg);
    if(ret)
    {
        TRACE("hal_i2s_setup_stream playback failed.ret = %d.", ret);
    }
    else
    {
        tdm_enable(i2s_id,false);
        tdm_set_config(i2s_id,tdm_cofig);
    }
    TRACE("hal_tdm_setup_stream done.");
    return ret;
}

int32_t hal_tdm_as_i2s_setup_stream(enum HAL_I2S_ID_T i2s_id,
                                       enum AUD_STREAM_T stream,
                                       uint32_t sample_rate)
{
    struct HAL_I2S_CONFIG_T i2s_cfg;
    struct HAL_TDM_CONFIG_T tdm_cfg;
    int ret;

    TRACE("hal_tdm_setup_stream:i2s_id = %d, stream = %d, sample_rate = %d.",
        i2s_id, stream, sample_rate);
    ASSERT(i2s_id < HAL_I2S_ID_QTY,"i2s_id = %d!",i2s_id);

    // i2s setup stream playback and capture.
    memset(&i2s_cfg, 0, sizeof(i2s_cfg));
    i2s_cfg.use_dma = true;
    i2s_cfg.master_clk_wait = stream == AUD_STREAM_PLAYBACK ? true : false;
    i2s_cfg.chan_sep_buf = false;
    i2s_cfg.bits = 16;
    i2s_cfg.channel_num = 2;
    i2s_cfg.channel_map = AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1;
    i2s_cfg.sample_rate = sample_rate;
    ret = hal_i2s_setup_stream(i2s_id, stream, &i2s_cfg);
    if(ret)
    {
        TRACE("hal_i2s_setup_stream playback failed.ret = %d.", ret);
    }
    else
    {
        tdm_enable(i2s_id,false);
        tdm_get_i2s_config(&tdm_cfg);
        tdm_set_config(i2s_id,&tdm_cfg);
    }
    TRACE("hal_tdm_setup_stream done.");
    return ret;
}

int32_t hal_tdm_start_stream(enum HAL_I2S_ID_T i2s_id,
                                      enum AUD_STREAM_T stream,
                                      uint16_t frame_size,
                                      uint16_t frame_num,
                                      uint32_t* buffer,
                                      HAL_DMA_IRQ_HANDLER_T irq_handler)
{
    int i;
    struct HAL_DMA_CH_CFG_T dma_cfg;
    struct HAL_DMA_DESC_T *dma_desc;
    int ret;

    TRACE("hal_tdm_start_stream:i2s_id = %d stream = %d, fram_size = %d, fram_num = %d, buffer = 0x%x irq_handler = 0x%x.",
           i2s_id, stream, frame_size, frame_num, (uint32_t) buffer, (uint32_t)irq_handler);
    ASSERT(i2s_id < HAL_I2S_ID_QTY,"i2s_id = %d!",i2s_id);

    tdm_enable(i2s_id,true);
    memset(&dma_cfg, 0, sizeof(dma_cfg));
    if(stream == AUD_STREAM_PLAYBACK)
    {
        dma_desc = i2s_id == HAL_I2S_ID_0 ? &i2s_0_tx_dma_desc[0] : &i2s_1_tx_dma_desc[0];

        dma_cfg.dst = 0; //useless
        dma_cfg.dst_bsize = HAL_DMA_BSIZE_4;
        dma_cfg.dst_periph = i2s_id == HAL_I2S_ID_0 ? HAL_AUDMA_I2S0_TX : HAL_AUDMA_I2S1_TX;
        dma_cfg.dst_width = HAL_DMA_WIDTH_HALFWORD;
        dma_cfg.handler = irq_handler;
        dma_cfg.src_bsize = HAL_DMA_BSIZE_4;
        dma_cfg.src_tsize = frame_size/2;
        dma_cfg.src_width = HAL_DMA_WIDTH_HALFWORD;
        dma_cfg.try_burst = 1;
        dma_cfg.type = HAL_DMA_FLOW_M2P_DMA;
        dma_cfg.ch = hal_audma_get_chan(dma_cfg.dst_periph, HAL_DMA_HIGH_PRIO);

        for (i = 0; i < frame_num; i++)
        {
            dma_cfg.src = (uint32_t)buffer + (frame_size*i);
            ret = hal_audma_init_desc(&dma_desc[i],
                    &dma_cfg,
                    &dma_desc[(i + 1) % frame_num],
                    1);
            if(ret)
            {
                TRACE("hal_audma_init_desc failed.ret = %d.", ret);
                goto __func_fail;
            }
        }
    }
    else
    {
        dma_desc = i2s_id == HAL_I2S_ID_0 ? &i2s_0_rx_dma_desc[0] : &i2s_1_rx_dma_desc[0];

        dma_cfg.dst_bsize = HAL_DMA_BSIZE_4;
        dma_cfg.dst_width = HAL_DMA_WIDTH_HALFWORD;
        dma_cfg.handler = irq_handler;
        dma_cfg.src = 0; // useless
        dma_cfg.src_bsize = HAL_DMA_BSIZE_4;
        dma_cfg.src_periph = i2s_id == HAL_I2S_ID_0 ? HAL_AUDMA_I2S0_RX : HAL_AUDMA_I2S1_RX;
        dma_cfg.src_tsize = frame_size/2;
        dma_cfg.src_width = HAL_DMA_WIDTH_HALFWORD;
        dma_cfg.try_burst = 1;
        dma_cfg.type = HAL_DMA_FLOW_P2M_DMA;
        dma_cfg.ch = hal_audma_get_chan(dma_cfg.src_periph, HAL_DMA_HIGH_PRIO);

        for (i = 0; i < frame_num; i++) {
            dma_cfg.dst = (uint32_t)buffer + (frame_size*i);
            ret = hal_audma_init_desc(&dma_desc[i],
                       &dma_cfg,
                       &dma_desc[(i + 1) % frame_num],
                       (i + 1) % (frame_num / 2) == 0);
            if(ret)
            {
                TRACE("hal_audma_init_desc failed.ret = %d.", ret);
                goto __func_fail;
            }
        }
    }


    ret = hal_audma_sg_start(dma_desc, &dma_cfg);
    if(ret)
    {
        TRACE("hal_audma_sg_start failed.ret = %d.", ret);
        goto __func_fail;
    }    // rx dma init/start.


    // i2s start stream playback and capture.
    ret = hal_i2s_start_stream(i2s_id, stream);
    if(ret)
    {
        TRACE("hal_i2s_start_stream failed.ret = %d.", ret);
        goto __func_fail;
    }
    TRACE("hal_tdm_start_stream done.");
    return 0;

__func_fail:
    TRACE("hal_tdm_start_stream failed!");
    return ret;
}

int32_t hal_tdm_stop_stream(enum HAL_I2S_ID_T i2s_id, enum AUD_STREAM_T stream)
{
    tdm_enable(i2s_id,false);
    return hal_i2s_stop_stream(i2s_id,stream);
}

int32_t hal_tdm_close(enum HAL_I2S_ID_T i2s_id, enum AUD_STREAM_T stream)
{
    tdm_enable(i2s_id,false);
    return hal_i2s_close(i2s_id,stream);
}


