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
#include "app_ibrt_customif_cmd.h"

extern "C" int app_shutdown(void);
#if defined(IBRT)
/*
* custom cmd handler add here, this is just a example
*/

#define app_ibrt_custom_cmd_rsp_timeout_handler_null   (0)
#define app_ibrt_custom_cmd_rsp_handler_null           (0)
#define app_ibrt_custom_cmd_rx_handler_null            (0)

static void app_ibrt_customif_cosonic_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_customif_cosonic_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);


static void app_ibrt_customif_test1_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_customif_test1_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_customif_test2_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_customif_test2_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_customif_test2_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_customif_test2_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static const app_tws_cmd_instance_t g_ibrt_custom_cmd_handler_table[]=
{
    {
        APP_IBRT_CUSTOM_CMD_TEST1,                              "TWS_CMD_TEST1",
        app_ibrt_customif_test1_cmd_send,
        app_ibrt_customif_test1_cmd_send_handler,			0,
        app_ibrt_custom_cmd_rsp_timeout_handler_null,           app_ibrt_custom_cmd_rsp_handler_null
    },
    {
        APP_IBRT_CUSTOM_CMD_TEST2,                              "TWS_CMD_TEST2",
        app_ibrt_customif_test2_cmd_send,
        app_ibrt_customif_test2_cmd_send_handler,               RSP_TIMEOUT_DEFAULT,
        app_ibrt_customif_test2_cmd_send_rsp_timeout_handler,   app_ibrt_customif_test2_cmd_send_rsp_handler
    },
    {
        APP_IBRT_CUSTOM_COSONIC_CMD,                            "COSONIC_TWS_CMD",
		app_ibrt_customif_cosonic_cmd_send,	
		app_ibrt_customif_cosonic_cmd_send_handler,				0,
		app_ibrt_custom_cmd_rsp_timeout_handler_null,			app_ibrt_custom_cmd_rsp_handler_null
    },
};


extern uint8_t LR_flags;
/**
 * @description: 主从耳通讯命令号
 * @author: Jevin
 */
enum ibrt_cosonic_tws_cmd_packet_e {
    cosonic_power_off_cmd=0x00,
};
/**
 * @description: 主从耳通讯命令实体
 * @author: Jevin
 */
ibrt_cosonic_tws_cmd_packet_t M_S_sync_power_off_cmd={cosonic_power_off_cmd,(uint8_t)LR_flags,};


int app_ibrt_customif_cmd_table_get(void **cmd_tbl, uint16_t *cmd_size)
{
    *cmd_tbl = (void *)&g_ibrt_custom_cmd_handler_table;
    *cmd_size = ARRAY_SIZE(g_ibrt_custom_cmd_handler_table);
    return 0;
}

void app_ibrt_customif_cmd_test(ibrt_custom_cmd_test_t *cmd_test)
{
    tws_ctrl_send_cmd(APP_IBRT_CUSTOM_CMD_TEST1, (uint8_t*)cmd_test, sizeof(ibrt_custom_cmd_test_t));
    tws_ctrl_send_cmd(APP_IBRT_CUSTOM_CMD_TEST2, (uint8_t*)cmd_test, sizeof(ibrt_custom_cmd_test_t));
}

static void app_ibrt_customif_test1_cmd_send(uint8_t *p_buff, uint16_t length)
{
	app_ibrt_send_cmd_without_rsp(APP_IBRT_CUSTOM_CMD_TEST1,p_buff,length);
	TRACE("%s", __func__);
}
static void app_ibrt_customif_test1_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
	TRACE("%s", __func__);
}

static void app_ibrt_customif_test2_cmd_send(uint8_t *p_buff, uint16_t length)
{
	app_ibrt_send_cmd_with_rsp(APP_IBRT_CUSTOM_CMD_TEST2,p_buff,length);
    TRACE("%s", __func__);
}

static void app_ibrt_customif_test2_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    tws_ctrl_send_rsp(APP_IBRT_CUSTOM_CMD_TEST2, rsp_seq, NULL, 0);
    TRACE("%s", __func__);

}

static void app_ibrt_customif_test2_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE("%s", __func__);

}

static void app_ibrt_customif_test2_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE("%s", __func__);
}

/**
 * @description: 主从TWS私有指令发送接口，包括M-S,S-M，都可以使用
 * @author: CHT 2021/1/13
 */
static void app_ibrt_customif_cosonic_cmd_send(uint8_t *p_buff, uint16_t length)
{
	TRACE("%s", __func__);
	app_ibrt_send_cmd_without_rsp(APP_IBRT_CUSTOM_COSONIC_CMD, p_buff, length);
}
static void app_ibrt_customif_cosonic_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
	TRACE("%s", __func__);
	ibrt_cosonic_tws_cmd_packet_t * pRcv =(ibrt_cosonic_tws_cmd_packet_t*) p_buff;
	uint8_t head_type = 0;
	//uint8_t valid_data_len = 0;
	
	if(NULL == pRcv || length != sizeof(ibrt_cosonic_tws_cmd_packet_t))
	{
    	TRACE("COSONIC: CMD RCV INVALID,Length:%d",length);
		return ;
	}

	//valid_data_len = pRcv->length;
	head_type = pRcv->cmd;
	
    switch(head_type)
	{
		case cosonic_power_off_cmd:
            app_shutdown();
			TRACE("COSONIC:sync_power_off");
			break;
		default:
			TRACE("COSONIC:default");
			break;
	}   
}

void synchronized_power_off(void)
{
    if(app_tws_ibrt_tws_link_connected()){
        tws_ctrl_send_cmd(APP_IBRT_CUSTOM_COSONIC_CMD, (uint8_t *)&M_S_sync_power_off_cmd, sizeof(ibrt_cosonic_tws_cmd_packet_t));
    }
    app_shutdown();
}

#endif
