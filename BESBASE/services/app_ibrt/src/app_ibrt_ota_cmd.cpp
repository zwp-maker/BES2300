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
#include "string.h"
#include "app_tws_ibrt_trace.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_ibrt_ota_cmd.h"
#if IBRT_OTA
#include "ota_control.h"
#endif

#if defined(IBRT)
#if IBRT_OTA

static void app_ibrt_ota_get_version_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_get_version_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_select_side_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_select_side_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_bp_check_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_bp_check_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_start_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_start_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_config_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_config_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_segment_crc_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_segment_crc_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_image_crc_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_image_crc_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_image_overwrite_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_image_overwrite_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_image_buff_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_image_buff_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_image_buff_cmd_slaveSend(uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_image_buff_cmd_slaveSend_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static app_tws_cmd_instance_t g_ibrt_ota_tws_cmd_handler_table[]=
{
    {
        IBRT_OTA_TWS_GET_VERSION_CMD,                            "OTA_GET_VERSION",
        app_ibrt_ota_get_version_cmd_send,
        app_ibrt_ota_get_version_cmd_send_handler,               0,
        0,                                                      0
    },
    {
        IBRT_OTA_TWS_SELECT_SIDE_CMD,                            "OTA_SELECT_SIDE",
        app_ibrt_ota_select_side_cmd_send,
        app_ibrt_ota_select_side_cmd_send_handler,               0,
        0,                                                      0
    },
    {
        IBRT_OTA_TWS_BP_CHECK_CMD,                               "OTA_BP_CHECK",//Break-point
        app_ibrt_ota_bp_check_cmd_send,
        app_ibrt_ota_bp_check_cmd_send_handler,               0,
        0,                                                      0
    },
    {
        IBRT_OTA_TWS_START_OTA_CMD,                              "OTA_START",
        app_ibrt_ota_start_cmd_send,
        app_ibrt_ota_start_cmd_send_handler,               0,
        0,                                                      0
    },
    {
        IBRT_OTA_TWS_OTA_CONFIG_CMD,                             "OTA_CONFIG",
        app_ibrt_ota_config_cmd_send,
        app_ibrt_ota_config_cmd_send_handler,               0,
        0,                                                      0
    },
    {
        IBRT_OTA_TWS_SEGMENT_CRC_CMD,                            "OTA_SEGMENT_CRC",
        app_ibrt_ota_segment_crc_cmd_send,
        app_ibrt_ota_segment_crc_cmd_send_handler,               0,
        0,                                                      0
    },
    {
        IBRT_OTA_TWS_IMAGE_CRC_CMD,                              "OTA_IMAGE_CRC",
        app_ibrt_ota_image_crc_cmd_send,
        app_ibrt_ota_image_crc_cmd_send_handler,               0,
        0,                                                      0
    },
    {
        IBRT_OTA_TWS_IMAGE_OVERWRITE_CMD,                        "OTA_IMAGE_OVERWRITE",
        app_ibrt_ota_image_overwrite_cmd_send,
        app_ibrt_ota_image_overwrite_cmd_send_handler,               0,
        0,                                                      0
    },
    {
        IBRT_OTA_TWS_IMAGE_BUFF,                                 "OTA_IMAGE_BUFF",
        app_ibrt_ota_image_buff_cmd_send,
        app_ibrt_ota_image_buff_cmd_send_handler,               0,
        0,                                                      0
    },
    {
        IBRT_OTA_TWS_IMAGE_BUFF_SLAVE,                                 "OTA_IMAGE_BUFF_SLAVE",
        app_ibrt_ota_image_buff_cmd_slaveSend,
        app_ibrt_ota_image_buff_cmd_slaveSend_handler,               0,
        0,                                                      0
    },
};

uint32_t ibrt_ota_cmd_type = 0;
uint32_t twsBreakPoint = 0;
uint8_t ibrt_connect_slave = 0;

int app_ibrt_ota_tws_cmd_table_get(void **cmd_tbl, uint16_t *cmd_size)
{
    *cmd_tbl = (void *)&g_ibrt_ota_tws_cmd_handler_table;
    *cmd_size = ARRAY_SIZE(g_ibrt_ota_tws_cmd_handler_table);
    return 0;
}

static void app_ibrt_ota_get_version_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(IBRT_OTA_TWS_GET_VERSION_CMD, p_buff, length);
    TRACE("%s", __func__);
}

static void app_ibrt_ota_get_version_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ibrt_ota_cmd_type == OTA_RSP_VERSION)
    {
        ibrt_ota_send_version_rsp();
        ibrt_ota_cmd_type = 0;
    }
    else if(ibrt_ota_cmd_type == 0)
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_GET_VERSION_CMD, p_buff, length);
    }
    TRACE("%s  %d", __func__, ibrt_ota_cmd_type);
}

static void app_ibrt_ota_select_side_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(IBRT_OTA_TWS_SELECT_SIDE_CMD, p_buff, length);
    TRACE("%s", __func__);
}

static void app_ibrt_ota_select_side_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ibrt_ota_cmd_type == OTA_RSP_SIDE_SELECTION)
    {
        if(1 == p_buff[1])
        {
            ota_control_side_selection_rsp(true);
        }
        else
        {
            ota_control_side_selection_rsp(false);
        }
        ibrt_ota_cmd_type = 0;
    }
    else if(ibrt_ota_cmd_type == 0)
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_SELECT_SIDE_CMD, p_buff, length);
    }
    TRACE("%s", __func__);
}

static void app_ibrt_ota_bp_check_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(IBRT_OTA_TWS_BP_CHECK_CMD, p_buff, length);
    TRACE("%s", __func__);
}

static void app_ibrt_ota_bp_check_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    OTA_RSP_RESUME_VERIFY_T* tRsp = (OTA_RSP_RESUME_VERIFY_T *)p_buff;
    if(ibrt_ota_cmd_type == OTA_RSP_RESUME_VERIFY)
    {
        TRACE("breakPoint %d, tRsp->breakPoint %d", twsBreakPoint, tRsp->breakPoint);
        if(twsBreakPoint == tRsp->breakPoint)
        {
            if(twsBreakPoint == 0)
            {
                LOG_DBG("reset random code:");
                LOG_DUMP("%02x ", (uint8_t *)tRsp, sizeof(OTA_RSP_RESUME_VERIFY_T));
                ota_randomCode_log((uint8_t *)&tRsp->randomCode);
            }
            ota_control_send_resume_response(tRsp->breakPoint, tRsp->randomCode);
        }
        else
        {
            TRACE("tws break-point not synchronized");
            ota_upgradeLog_destroy();
            ota_randomCode_log((uint8_t *)&tRsp->randomCode);
            ota_control_reset_env();
            ota_status_change(true);
            tRsp->breakPoint = 0;
            twsBreakPoint = 0;
            tws_ctrl_send_cmd(IBRT_OTA_TWS_BP_CHECK_CMD, (uint8_t *)tRsp, length);
        }
        ibrt_ota_cmd_type = 0;
    }
    TRACE("%s", __func__);
}

static void app_ibrt_ota_start_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(IBRT_OTA_TWS_START_OTA_CMD, p_buff, length);
    TRACE("%s", __func__);
}

static void app_ibrt_ota_start_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ibrt_ota_cmd_type == OTA_RSP_START)
    {
        ibrt_ota_send_start_response(*p_buff);
        ibrt_ota_cmd_type = 0;
    }
    else if(ibrt_ota_cmd_type == 0)
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_START_OTA_CMD, p_buff, length);
    }
    TRACE("%s", __func__);
}

static void app_ibrt_ota_config_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(IBRT_OTA_TWS_OTA_CONFIG_CMD, p_buff, length);
    TRACE("%s", __func__);
}

static void app_ibrt_ota_config_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ibrt_ota_cmd_type == OTA_RSP_CONFIG)
    {
        if(*p_buff == 1)
        {
            ibrt_ota_send_configuration_response(true);
        }
        else if(*p_buff == OTA_RESULT_ERR_IMAGE_SIZE)
        {
            ibrt_ota_send_result_response(OTA_RESULT_ERR_IMAGE_SIZE);
        }
        else if(*p_buff == OTA_RESULT_ERR_BREAKPOINT)
        {
            ibrt_ota_send_result_response(OTA_RESULT_ERR_BREAKPOINT);
        }
        else
        {
            ibrt_ota_send_configuration_response(false);
        }
        ibrt_ota_cmd_type = 0;
    }
    else if(ibrt_ota_cmd_type == 0)
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_OTA_CONFIG_CMD, p_buff, length);
    }
    TRACE("%s", __func__);
}

static void app_ibrt_ota_segment_crc_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(IBRT_OTA_TWS_SEGMENT_CRC_CMD, p_buff, length);
    TRACE("%s", __func__);
}

static void app_ibrt_ota_segment_crc_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ibrt_ota_cmd_type == OTA_RSP_SEGMENT_VERIFY)
    {
        if(*p_buff == 1)
        {
            ibrt_ota_send_segment_verification_response(true);
        }
        else if(*p_buff == OTA_RESULT_ERR_SEG_VERIFY)
        {
            ota_upgradeLog_destroy();
            ibrt_ota_send_result_response(OTA_RESULT_ERR_SEG_VERIFY);
        }
        else if(*p_buff == OTA_RESULT_ERR_FLASH_OFFSET)
        {
            ibrt_ota_send_result_response(OTA_RESULT_ERR_FLASH_OFFSET);
        }
        else
        {
            ibrt_ota_send_segment_verification_response(false);
        }
        ibrt_ota_cmd_type = 0;
    }
    else
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_SEGMENT_CRC_CMD, p_buff, length);
    }
    TRACE("%s", __func__);
}

static void app_ibrt_ota_image_crc_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(IBRT_OTA_TWS_IMAGE_CRC_CMD, p_buff, length);
    TRACE("%s", __func__);
}

static void app_ibrt_ota_image_crc_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ibrt_ota_cmd_type == OTA_RSP_RESULT)
    {
        if(*p_buff == 1)
        {
            ibrt_ota_send_result_response(true);
        }
        else
        {
            ibrt_ota_send_result_response(false);
        }
        ibrt_ota_cmd_type = 0;
    }
    else
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_IMAGE_CRC_CMD, p_buff, length);
    }
    TRACE("%s", __func__);
}

static void app_ibrt_ota_image_overwrite_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(IBRT_OTA_TWS_IMAGE_OVERWRITE_CMD, p_buff, length);
    TRACE("%s", __func__);
}

static void app_ibrt_ota_image_overwrite_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ibrt_ota_cmd_type == OTA_RSP_IMAGE_APPLY)
    {
        if(*p_buff == 1)
        {
            ota_control_image_apply_rsp(true);
        }
        else
        {
            ota_control_image_apply_rsp(false);
        }
        ibrt_ota_cmd_type = 0;
    }
    else
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_IMAGE_OVERWRITE_CMD, p_buff, length);
    }
    TRACE("%s", __func__);
}

static void app_ibrt_ota_image_buff_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(IBRT_OTA_TWS_IMAGE_BUFF, p_buff, length);
    TRACE("%s", __func__);
}

static void app_ibrt_ota_image_buff_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    ibrt_connect_slave = 0;
    ota_bes_handle_received_data(p_buff, true, length);
    TRACE("%s", __func__);
}

static void app_ibrt_ota_image_buff_cmd_slaveSend(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(IBRT_OTA_TWS_IMAGE_BUFF_SLAVE, p_buff, length);
    TRACE("%s", __func__);
}

static void app_ibrt_ota_image_buff_cmd_slaveSend_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    ibrt_connect_slave = 1;
    ota_bes_handle_received_data(p_buff, true, length);
    TRACE("%s", __func__);
}

#endif
#endif
