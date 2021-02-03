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
#ifndef __APP_IBRT_IF_CUSTOM_CMD__
#define __APP_IBRT_IF_CUSTOM_CMD__

#include "app_ibrt_custom_cmd.h"

typedef enum
{
    APP_IBRT_CUSTOM_CMD_TEST1 = APP_IBRT_CMD_BASE|APP_IBRT_CUSTOM_CMD_PREFIX|0x01,
    APP_IBRT_CUSTOM_CMD_TEST2 = APP_IBRT_CMD_BASE|APP_IBRT_CUSTOM_CMD_PREFIX|0x02,
    //new customer cmd add here
} app_ibrt_custom_cmd_code_e;


/**
 * @description: 主从私有指令接收接口，包括M-S,S-M，都可以使用
 * @param len:有效数据长度,包含头部1字节，total len= head + valid data length
 * @param buff: 包括1字节头部及后续的不定长数据
 * @author: CHT 2021/1/13
 */
// typedef struct
// { 
//     uint8_t buff[15]; // 1 head + max 11 data
//     uint8_t len;      // valid data length;
// } __attribute__((packed))ibrt_cosonic_cmd_packet_t;

 typedef struct
 {
    uint8_t cmd;
    uint8_t current_physical_role	:1;
    uint8_t current_volt	:7;
    uint8_t cosonic_ibrt_buff[14];
    uint8_t length;
} __attribute__((packed))ibrt_cosonic_tws_cmd_packet_t;//主从耳之间的私有协议

typedef struct
{
    uint8_t buff[6];
} __attribute__((packed))ibrt_custom_cmd_test_t;


void app_ibrt_customif_cmd_test(ibrt_custom_cmd_test_t *cmd_test);
void app_ibrt_customif_cosonic_cmd_send(uint8_t *p_buff, uint16_t length);


#endif
