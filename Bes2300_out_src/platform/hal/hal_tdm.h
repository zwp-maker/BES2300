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
#ifndef __HAL_TDM_H__
#define __HAL_TDM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "reg_tdm.h"
#include "hal_i2s.h"

#define TDM_BUF_ALIGN __attribute__((aligned(0x100)))
#define MAX_TDM_FRAME_SIZE   256
#define MAX_TDM_FRAME_NUM    16

enum HAL_TDM_ENABLE_T {
    HAL_TDM_DISABLE,
    HAL_TDM_ENABLE,
    HAL_TDM_ENABLE_NUM,
};

enum HAL_TDM_MODE_T {
    HAL_TDM_MODE_FS_ASSERTED_AT_FIRST,
    HAL_TDM_MODE_FS_ASSERTED_AT_LAST,
    HAL_TDM_MODE_NUM,
};

enum HAL_TDM_FS_EDGE_T {
    HAL_TDM_FS_EDGE_POSEDGE,
    HAL_TDM_FS_EDGE_NEGEDGE,
    HAL_TDM_FS_EDGE_NUM,
};

enum HAL_TDM_FRAME_WIDTH_T {
    HAL_TDM_FRAME_WIDTH_16_CYCLES,
    HAL_TDM_FRAME_WIDTH_32_CYCLES,
    HAL_TDM_FRAME_WIDTH_64_CYCLES,
    HAL_TDM_FRAME_WIDTH_128_CYCLES,
    HAL_TDM_FRAME_WIDTH_256_CYCLES,
    HAL_TDM_FRAME_WIDTH_NUM,
};

enum HAL_TDM_FS_WIDTH_T {
    HAL_TDM_FS_WIDTH_1_CYCLE,
    HAL_TDM_FS_WIDTH_8_CYCLES,
    HAL_TDM_FS_WIDTH_16_CYCLES,
    HAL_TDM_FS_WIDTH_32_CYCLES,
    HAL_TDM_FS_WIDTH_64_CYCLES,
    HAL_TDM_FS_WIDTH_128_CYCLES,
    HAL_TDM_FS_WIDTH_FRAME_LENGTH_1_CYCLES,
    HAL_TDM_FS_WIDTH_NUM,
};

enum HAL_TDM_SLOT_WIDTH_T {
    HAL_TDM_SLOT_WIDTH_32_BIT,
    HAL_TDM_SLOT_WIDTH_16_BIT,
    HAL_TDM_SLOT_WIDTH_NUM,
};

struct HAL_TDM_CONFIG_T {
    enum HAL_TDM_MODE_T mode;
    enum HAL_TDM_FS_EDGE_T edge;
    enum HAL_TDM_FRAME_WIDTH_T frame_width;
    enum HAL_TDM_FS_WIDTH_T fs_width;
    enum HAL_TDM_SLOT_WIDTH_T slot_width;
    uint32_t data_offset;
};

int32_t hal_tdm_open(enum HAL_I2S_ID_T i2s_id,enum AUD_STREAM_T stream,enum HAL_I2S_MODE_T mode);
int32_t hal_tdm_setup_stream(enum HAL_I2S_ID_T i2s_id,
                                       enum AUD_STREAM_T stream,
                                       uint32_t sample_rate,
                                       struct HAL_TDM_CONFIG_T *tdm_cofig);
int32_t hal_tdm_as_i2s_setup_stream(enum HAL_I2S_ID_T i2s_id,
                                       enum AUD_STREAM_T stream,
                                       uint32_t sample_rate);
int32_t hal_tdm_start_stream(enum HAL_I2S_ID_T i2s_id,
                                      enum AUD_STREAM_T stream,
                                      uint16_t frame_size,
                                      uint16_t frame_num,
                                      uint32_t* buffer,
                                      HAL_DMA_IRQ_HANDLER_T irq_handler);
int32_t hal_tdm_stop_stream(enum HAL_I2S_ID_T i2s_id, enum AUD_STREAM_T stream);
int32_t hal_tdm_close(enum HAL_I2S_ID_T i2s_id, enum AUD_STREAM_T stream);

#ifdef __cplusplus
}
#endif

#endif // __HAL_I2S_TDM_H__
