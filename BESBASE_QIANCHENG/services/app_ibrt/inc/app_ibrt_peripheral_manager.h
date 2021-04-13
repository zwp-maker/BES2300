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
#ifndef __APP_IBRT_PERIPHERAL_MANAGER_H__
#define __APP_IBRT_PERIPHERAL_MANAGER_H__

typedef enum {
    //UI class:
    APP_IBRT_PERIPHERAL_MANAGER_OPENCASE_CMD = 0x01,
    APP_IBRT_PERIPHERAL_MANAGER_CLOSECASE_CMD = 0x02,
    APP_IBRT_PERIPHERAL_MANAGER_RESET_CMD = 0x03,
    APP_IBRT_PERIPHERAL_MANAGER_POWEROFF_CMD = 0x04,
    APP_IBRT_PERIPHERAL_MANAGER_PAIRING_TWS_CMD = 0x05,
    APP_IBRT_PERIPHERAL_MANAGER_PAIRING_SINGLE_CMD = 0x06,
    APP_IBRT_PERIPHERAL_MANAGER_TWS_GET_EP_INFO_CMD = 0x07, // Get earphone information.
    APP_IBRT_PERIPHERAL_MANAGER_TWS_EXC_BATT_CMD = 0x08,    // Exchange battery information.
    // APP_IBRT_PERIPHERAL_MANAGER_BOX_SND_BATT_CMD = 0x09,
    APP_IBRT_PERIPHERAL_MANAGER_CLEAN_RECORD_CMD = 0x09,
    APP_IBRT_PERIPHERAL_MANAGER_RECOVER_FACTOR_CMD = 0x0A,
    APP_IBRT_PERIPHERAL_MANAGER_TWS_PAIRING_PHONE_CMD = 0x0B,
    APP_IBRT_PERIPHERAL_MANAGER_TWS_GET_BTADDRESS_CMD = 0x20,
    APP_IBRT_PERIPHERAL_MANAGER_TWS_SET_PEER_BTADDRESS_CMD = 0x21,
    // APP_IBRT_PERIPHERAL_MANAGER_TWS_SET_PEER_BTADDRESS_LEFT_CMD = 0x22,
    // APP_IBRT_PERIPHERAL_MANAGER_TWS_SET_PEER_BTADDRESS_RIGHT_CMD = 0x23,

    //TEST class:
    APP_IBRT_PERIPHERAL_MANAGER_BTTEST_ENTER_DUT_CMD = 0x30,
    APP_IBRT_PERIPHERAL_MANAGER_BTTEST_HANDSET_PAIRING_CMD = 0x31,
    APP_IBRT_PERIPHERAL_MANAGER_BTTEST_VCO_TEST_CMD = 0x32,
    APP_IBRT_PERIPHERAL_MANAGER_BTTEST_NOSIG_TEST_CMD = 0x33,
    APP_IBRT_PERIPHERAL_MANAGER_DUMP_RECORD_CMD = 0x34,
    
    //FACTORY class:
    
    //AUTO TEST class:
    APP_IBRT_PERIPHERAL_MANAGER_FETH_OUT_LEFT_CMD = 0x80,
    APP_IBRT_PERIPHERAL_MANAGER_FETH_IN_LEFT_CMD = 0x81,
    APP_IBRT_PERIPHERAL_MANAGER_FETH_OUT_RIGHT_CMD = 0x82,
    APP_IBRT_PERIPHERAL_MANAGER_FETH_IN_RIGHT_CMD = 0x83,
    APP_IBRT_PERIPHERAL_MANAGER_WEAR_ON_LEFT_CMD = 0x84,
    APP_IBRT_PERIPHERAL_MANAGER_WEAR_OFF_LEFT_CMD = 0x85,
    APP_IBRT_PERIPHERAL_MANAGER_WEAR_ON_RIGHT_CMD = 0x86,
    APP_IBRT_PERIPHERAL_MANAGER_WEAR_OFF_RIGHT_CMD = 0x87,
    APP_IBRT_PERIPHERAL_MANAGER_ROLE_SWITCH_CMD = 0x88,
    APP_IBRT_PERIPHERAL_MANAGER_PHONE_CALL_CMD = 0x89,
    APP_IBRT_PERIPHERAL_MANAGER_PHONE_REJECT_CMD = 0x8A,
    APP_IBRT_PERIPHERAL_MANAGER_PHONE_HOLD_CMD = 0x8B,
    APP_IBRT_PERIPHERAL_MANAGER_MUSIC_PLAY_CMD = 0x8C,
    APP_IBRT_PERIPHERAL_MANAGER_MUSIC_PAUSE_CMD = 0x8D,
    APP_IBRT_PERIPHERAL_MANAGER_MUSIC_NEXT_CMD = 0x8E,
    APP_IBRT_PERIPHERAL_MANAGER_MUSIC_PRE_CMD = 0x8F,
    APP_IBRT_PERIPHERAL_MANAGER_VOLUME_UP_CMD = 0x90,
    APP_IBRT_PERIPHERAL_MANAGER_VOLUME_DOWMN_CMD = 0x91,
    APP_IBRT_PERIPHERAL_MANAGER_BOX_OPEN_LEFT_CMD = 0x92,
    APP_IBRT_PERIPHERAL_MANAGER_BOX_CLOSE_LEFT_CMD = 0x93,
    APP_IBRT_PERIPHERAL_MANAGER_BOX_OPEN_RIGHT_CMD = 0x94,
    APP_IBRT_PERIPHERAL_MANAGER_BOX_CLOSE_RIGHT_CMD = 0x95,
    APP_IBRT_PERIPHERAL_MANAGER_PUSH_TO_TALK_CMD = 0x96,
    APP_IBRT_PERIPHERAL_MANAGER_TWS_DISCONNECT_CMD = 0x97,
    APP_IBRT_PERIPHERAL_MANAGER_TWS_PHONE_DISCONNECT_CMD = 0x98,
    
    //ANC TUNE class:
    
} app_ibrt_peripheral_manager_cmd_code_e;


typedef enum {
    APP_IBRT_PERIPHERAL_MANAGER_EXECUTE_CUSTOM_FUNCTION_TYPE = 0x00,
    APP_IBRT_PERIPHERAL_MANAGER_EXECUTE_CUSTOM_FUNCTION_WITH_A_PARAMETER_TYPE = 0x01,
    APP_IBRT_PERIPHERAL_MANAGER_EXECUTE_CUSTOM_FUNCTION_WITH_TWO_PARAMETER_TYPE = 0x02,
    APP_IBRT_PERIPHERAL_MANAGER_COMMUNICATION_WITH_CHARGE_BOX_TYPE = 0x03,
    APP_IBRT_PERIPHERAL_MANAGER_IBRT_TEST_TYPE = 0xff,
}app_ibrt_peripheral_manager_type_code_e;

typedef struct {
    uint32_t message_id;
    uint32_t message_ptr;
    uint32_t message_Param0;
    uint32_t message_Param1;
    uint32_t message_Param2;
    const char *command;
    uint8_t *payload;
    uint32_t payload_length;
} TWS_PD_MSG_BODY;

typedef struct {
    TWS_PD_MSG_BODY msg_body;
} TWS_PD_MSG_BLOCK;

typedef int (*app_ibrt_peripheral_manage_receive_func_typedef)(uint8_t *buf, uint32_t len);

int app_ibrt_peripheral_mailbox_put(TWS_PD_MSG_BLOCK* msg_src);
int app_ibrt_peripheral_mailbox_free(TWS_PD_MSG_BLOCK* msg_p);
int app_ibrt_peripheral_mailbox_get(TWS_PD_MSG_BLOCK** msg_p);
void app_ibrt_peripheral_thread_init(void);
void app_ibrt_peripheral_run0(uint32_t ptr);
void app_ibrt_peripheral_run1(uint32_t ptr, uint32_t param0);
void app_ibrt_peripheral_run2(uint32_t ptr, uint32_t param0, uint32_t param1);
int app_ibrt_peripheral_manage_entry(app_ibrt_peripheral_manager_cmd_code_e execute_command, uint8_t *payload, uint32_t payload_length);
int app_ibrt_peripheral_manage_register_receive_cb(app_ibrt_peripheral_manage_receive_func_typedef func);
app_ibrt_peripheral_manage_receive_func_typedef app_ibrt_peripheral_manage_get_receive_cb(void);

#endif
