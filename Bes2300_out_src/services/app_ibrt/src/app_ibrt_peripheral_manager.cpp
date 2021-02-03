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
#include <string.h>
#include "cmsis_os.h"
#include "app_tws_ibrt_trace.h"
#include "bluetooth.h"
#include "hci_api.h"
#include "me_api.h"
#include "app_tws_ibrt.h"
#include "app_tws_besaud.h"
#include "app_vendor_cmd_evt.h"
#include "tws_role_switch.h"
#include "l2cap_api.h"
#include "rfcomm_api.h"
#include "conmgr_api.h"
#include "bt_if.h"
#include "app_ibrt_if.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_ibrt.h"
#include "bt_drv_interface.h"
#include "bt_drv_reg_op.h"
#include "app_ibrt_peripheral_manager.h"
#include "app_ibrt_ui.h"
#if defined(IBRT)
osThreadId app_ibrt_peripheral_tid;
static void app_ibrt_peripheral_thread(void const *argument);
osThreadDef(app_ibrt_peripheral_thread, osPriorityHigh,1, 1024,"app_ibrt_peripheral_thread");

static app_ibrt_peripheral_manage_receive_func_typedef app_ibrt_peripheral_manage_receive_cb = NULL;

#define TWS_PERIPHERAL_DEVICE_MAILBOX_MAX (10)
osMailQDef (app_ibrt_peripheral_mailbox, TWS_PERIPHERAL_DEVICE_MAILBOX_MAX, TWS_PD_MSG_BLOCK);
static osMailQId app_ibrt_peripheral_mailbox = NULL;
static uint8_t app_ibrt_peripheral_mailbox_cnt = 0;

static int app_ibrt_peripheral_mailbox_init(void)
{
    app_ibrt_peripheral_mailbox = osMailCreate(osMailQ(app_ibrt_peripheral_mailbox), NULL);
    if (app_ibrt_peripheral_mailbox == NULL)  {
        TRACE("Failed to Create app_ibrt_peripheral_mailbox\n");
        return -1;
    }
    app_ibrt_peripheral_mailbox_cnt = 0;
    return 0;
}

int app_ibrt_peripheral_mailbox_put(TWS_PD_MSG_BLOCK* msg_src)
{
    if(!msg_src){
        TRACE("msg_src is a null pointer in app_ibrt_peripheral_mailbox_put!");
        return -1;
    }
    if(app_ibrt_peripheral_tid==NULL)
    {
        TRACE("app_ibrt_peripheral_thread not ready!  can't use it's mailbox");
        return -1;
    }
    osStatus status;
    TWS_PD_MSG_BLOCK *msg_p = NULL;
    msg_p = (TWS_PD_MSG_BLOCK*)osMailAlloc(app_ibrt_peripheral_mailbox, 0);
    ASSERT(msg_p, "osMailAlloc error");
    msg_p->msg_body.message_id = msg_src->msg_body.message_id;
    msg_p->msg_body.message_ptr = msg_src->msg_body.message_ptr;
    msg_p->msg_body.message_Param0 = msg_src->msg_body.message_Param0;
    msg_p->msg_body.message_Param1 = msg_src->msg_body.message_Param1;
    msg_p->msg_body.message_Param2 = msg_src->msg_body.message_Param2;
    msg_p->msg_body.command = msg_src->msg_body.command;
    msg_p->msg_body.payload = msg_src->msg_body.payload;
    msg_p->msg_body.payload_length = msg_src->msg_body.payload_length;

    status = osMailPut(app_ibrt_peripheral_mailbox, msg_p);
    if (osOK == status)
        app_ibrt_peripheral_mailbox_cnt++;
    return (int)status;
}

int app_ibrt_peripheral_mailbox_free(TWS_PD_MSG_BLOCK* msg_p)
{
    if(!msg_p){
        TRACE("msg_p is a null pointer in app_ibrt_peripheral_mailbox_free!");
        return -1;
    }
    osStatus status;

    status = osMailFree(app_ibrt_peripheral_mailbox, msg_p);
    if (osOK == status)
        app_ibrt_peripheral_mailbox_cnt--;

    return (int)status;
}

int app_ibrt_peripheral_mailbox_get(TWS_PD_MSG_BLOCK** msg_p)
{
    if(!msg_p){
        TRACE("msg_p is a null pointer in app_ibrt_peripheral_mailbox_get!");
        return -1;
    }

    osEvent evt;
    evt = osMailGet(app_ibrt_peripheral_mailbox, osWaitForever);
    if (evt.status == osEventMail) {
        *msg_p = (TWS_PD_MSG_BLOCK *)evt.value.p;
        return 0;
    }
    return -1;
}

/*****************************************************************************
 Prototype    : app_ibrt_peripheral_thread
 Description  : peripheral manager thread
 Input        : void const *argument
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/18
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
typedef void (*app_ibrt_peripheral_cb0)(void);
typedef void (*app_ibrt_peripheral_cb1)(void *);
typedef void (*app_ibrt_peripheral_cb2)(void *, void *);

void app_ibrt_peripheral_thread(void const *argument)
{
    while(1){
        TWS_PD_MSG_BLOCK *msg_p = NULL;
        if ((!app_ibrt_peripheral_mailbox_get(&msg_p))&&(!argument)) {
            switch(msg_p->msg_body.message_id){
                case APP_IBRT_PERIPHERAL_MANAGER_EXECUTE_CUSTOM_FUNCTION_TYPE:
                    if(msg_p->msg_body.message_ptr){
                        ((app_ibrt_peripheral_cb0)(msg_p->msg_body.message_ptr))();
                    }
                    break;
                case APP_IBRT_PERIPHERAL_MANAGER_EXECUTE_CUSTOM_FUNCTION_WITH_A_PARAMETER_TYPE:
                    if(msg_p->msg_body.message_ptr){
                        ((app_ibrt_peripheral_cb1)(msg_p->msg_body.message_ptr))((void *)msg_p->msg_body.message_Param0);
                    }
                    break;
                case APP_IBRT_PERIPHERAL_MANAGER_EXECUTE_CUSTOM_FUNCTION_WITH_TWO_PARAMETER_TYPE:
                    if(msg_p->msg_body.message_ptr){
                        ((app_ibrt_peripheral_cb2)(msg_p->msg_body.message_ptr))((void *)msg_p->msg_body.message_Param0,
                                                                                 (void *)msg_p->msg_body.message_Param1);
                    }
                    break;
                case APP_IBRT_PERIPHERAL_MANAGER_COMMUNICATION_WITH_CHARGE_BOX_TYPE:
                    app_ibrt_ui_test_cmd_handler2(msg_p->msg_body.command, msg_p->msg_body.message_Param0, msg_p->msg_body.payload, msg_p->msg_body.payload_length);
                    break;    
                case APP_IBRT_PERIPHERAL_MANAGER_IBRT_TEST_TYPE: {// ibrt test
                    char ibrt_cmd[20] = {0};
                    memcpy(ibrt_cmd+0, &msg_p->msg_body.message_Param0, 4);
                    memcpy(ibrt_cmd+4, &msg_p->msg_body.message_Param1, 4);
                    memcpy(ibrt_cmd+8, &msg_p->msg_body.message_Param2, 4);
                    TRACE("ibrt_ui_log: %s\n", ibrt_cmd);
                    app_ibrt_ui_test_cmd_handler((unsigned char*)ibrt_cmd, strlen(ibrt_cmd)+1);
                    }
                    break;
                default:
                    break;
            }
            app_ibrt_peripheral_mailbox_free(msg_p);
        }
    }
}

extern "C" void app_ibrt_peripheral_perform_test(const char* ibrt_cmd)
{
    TWS_PD_MSG_BLOCK msg;
    msg.msg_body.message_id = APP_IBRT_PERIPHERAL_MANAGER_IBRT_TEST_TYPE;
    memcpy(&msg.msg_body.message_Param0, ibrt_cmd+0, 4);
    memcpy(&msg.msg_body.message_Param1, ibrt_cmd+4, 4);
    memcpy(&msg.msg_body.message_Param2, ibrt_cmd+8, 4);
    app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_peripheral_run0(uint32_t ptr)
{
    TWS_PD_MSG_BLOCK msg;
    msg.msg_body.message_id = APP_IBRT_PERIPHERAL_MANAGER_EXECUTE_CUSTOM_FUNCTION_TYPE;
    msg.msg_body.message_ptr = ptr;
    app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_peripheral_run1(uint32_t ptr, uint32_t param0)
{
    TWS_PD_MSG_BLOCK msg;
    msg.msg_body.message_id = APP_IBRT_PERIPHERAL_MANAGER_EXECUTE_CUSTOM_FUNCTION_WITH_A_PARAMETER_TYPE;
    msg.msg_body.message_ptr = ptr;
    msg.msg_body.message_Param0 = param0;
    app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_peripheral_run2(uint32_t ptr, uint32_t param0, uint32_t param1)
{
    TWS_PD_MSG_BLOCK msg;
    msg.msg_body.message_id = APP_IBRT_PERIPHERAL_MANAGER_EXECUTE_CUSTOM_FUNCTION_WITH_TWO_PARAMETER_TYPE;
    msg.msg_body.message_ptr = ptr;
    msg.msg_body.message_Param0 = param0;
    msg.msg_body.message_Param1 = param1;
    app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_peripheral_thread_init(void)
{
    if (app_ibrt_peripheral_mailbox_init())
        return;

    app_ibrt_peripheral_tid = osThreadCreate(osThread(app_ibrt_peripheral_thread), NULL);
    if (app_ibrt_peripheral_tid == NULL)  {
        TRACE("Failed to Create app_ibrt_peripheral_thread\n");
        return;
    }

    return;
}


app_ibrt_peripheral_manage_receive_func_typedef app_ibrt_peripheral_manage_get_receive_cb(void)
{
    return app_ibrt_peripheral_manage_receive_cb;
}


int app_ibrt_peripheral_manage_register_receive_cb(app_ibrt_peripheral_manage_receive_func_typedef func)
{
    if(func == NULL)
        return -1;
    
    app_ibrt_peripheral_manage_receive_cb = func;
    
    TRACE("[%s] register receive callback success\n", __func__);
    
    return 0;
}


int app_ibrt_peripheral_manage_entry(app_ibrt_peripheral_manager_cmd_code_e execute_command, uint8_t *payload, uint32_t payload_length)
{
    int temp = 0;
    TWS_PD_MSG_BLOCK msg = {0};
    msg.msg_body.message_id = APP_IBRT_PERIPHERAL_MANAGER_COMMUNICATION_WITH_CHARGE_BOX_TYPE;
    
    TRACE("[%s] enter... excute_command: 0x%02x", __func__, execute_command);
    
    switch(execute_command)
    {
        //UI class:
        case APP_IBRT_PERIPHERAL_MANAGER_OPENCASE_CMD:
        {
            const static char *p = "OPEN_CASE";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_OPENCASE_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_CLOSECASE_CMD:
        {
            const static char *p = "CLOSE_CASE";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_CLOSECASE_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_RESET_CMD:
        {
            const static char *p = "RESET";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_RESET_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_POWEROFF_CMD:
        {
            const static char *p = "POWER_OFF";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_POWEROFF_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_PAIRING_TWS_CMD:
        {
            const static char *p = "PAIRING_TWS";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_PAIRING_TWS_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_PAIRING_SINGLE_CMD:
        {
            const static char *p = "PAIRING_SINGLE";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_PAIRING_SINGLE_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_TWS_GET_EP_INFO_CMD:
        {
            const static char *p = "TWS_GET_EP_INFO";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_TWS_GET_EP_INFO_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_TWS_EXC_BATT_CMD:
        {
            const static char *p = "TWS_EXC_BATT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_TWS_EXC_BATT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_CLEAN_RECORD_CMD:
        {
            const static char *p = "CLEAN_RECORD";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_CLEAN_RECORD_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_RECOVER_FACTOR_CMD:
        {
            const static char *p = "RECOVER_FACTOR";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_RECOVER_FACTOR_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_TWS_PAIRING_PHONE_CMD:
        {
            const static char *p = "TWS_PAIRING_PHONE";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_TWS_PAIRING_PHONE_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_TWS_GET_BTADDRESS_CMD:
        {
            const static char *p = "TWS_GET_BTADDRESS";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_TWS_GET_BTADDRESS_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_TWS_SET_PEER_BTADDRESS_CMD:
        {
            const static char *p = "TWS_SET_PEER_BTADDRESS";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_TWS_SET_PEER_BTADDRESS_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        //TEST class:
        case APP_IBRT_PERIPHERAL_MANAGER_BTTEST_ENTER_DUT_CMD:
        {
            const static char *p = "BTTEST_ENTER_DUT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_BTTEST_ENTER_DUT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_BTTEST_HANDSET_PAIRING_CMD:
        {
            const static char *p = "BTTEST_HANDSET_PAIRING";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_BTTEST_HANDSET_PAIRING_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_BTTEST_VCO_TEST_CMD:
        {
            const static char *p = "BTTEST_VCO_TEST";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_BTTEST_VCO_TEST_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_BTTEST_NOSIG_TEST_CMD:
        {
            const static char *p = "BTTEST_NOSIG_TEST";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_BTTEST_NOSIG_TEST_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_DUMP_RECORD_CMD:
        {
            const static char *p = "DUMP_RECORD";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_DUMP_RECORD_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        //FACTORY class:
        
        
        //AUTO TEST class:
        case APP_IBRT_PERIPHERAL_MANAGER_FETH_OUT_LEFT_CMD:
        {
            const static char *p = "FETH_OUT_LEFT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_FETH_OUT_LEFT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_FETH_IN_LEFT_CMD:
        {
            const static char *p = "FETH_IN_LEFT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_FETH_IN_LEFT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_FETH_OUT_RIGHT_CMD:
        {
            const static char *p = "FETH_OUT_RIGHT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_FETH_OUT_RIGHT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_FETH_IN_RIGHT_CMD:
        {
            const static char *p = "FETH_IN_RIGHT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_FETH_IN_RIGHT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_WEAR_ON_LEFT_CMD:
        {
            const static char *p = "WEAR_ON_LEFT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_WEAR_ON_LEFT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_WEAR_OFF_LEFT_CMD:
        {
            const static char *p = "WEAR_OFF_LEFT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_WEAR_OFF_LEFT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_WEAR_ON_RIGHT_CMD:
        {
            const static char *p = "WEAR_ON_RIGHT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_WEAR_ON_RIGHT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_WEAR_OFF_RIGHT_CMD:
        {
            const static char *p = "WEAR_OFF_RIGHT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_WEAR_OFF_RIGHT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_ROLE_SWITCH_CMD:
        {
            const static char *p = "ROLE_SWITCH";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_ROLE_SWITCH_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_PHONE_CALL_CMD:
        {
            const static char *p = "PHONE_CALL";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_PHONE_CALL_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_PHONE_REJECT_CMD:
        {
            const static char *p = "PHONE_REJECT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_PHONE_REJECT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_PHONE_HOLD_CMD:
        {
            const static char *p = "PHONE_HOLD";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_PHONE_HOLD_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_MUSIC_PLAY_CMD:
        {
            const static char *p = "MUSIC_PLAY";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_MUSIC_PLAY_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_MUSIC_PAUSE_CMD:
        {
            const static char *p = "MUSIC_PAUSE";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_MUSIC_PAUSE_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_MUSIC_NEXT_CMD:
        {
            const static char *p = "MUSIC_NEXT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_MUSIC_NEXT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_MUSIC_PRE_CMD:
        {
            const static char *p = "MUSIC_PRE";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_MUSIC_PRE_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_VOLUME_UP_CMD:
        {
            const static char *p = "VOLUME_UP";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_VOLUME_UP_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_VOLUME_DOWMN_CMD:
        {
            const static char *p = "VOLUME_DOWN";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_VOLUME_DOWMN_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_BOX_OPEN_LEFT_CMD:
        {
            const static char *p = "BOX_OPEN_LEFT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_BOX_OPEN_LEFT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_BOX_CLOSE_LEFT_CMD:
        {
            const static char *p = "BOX_CLOSE_LEFT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_BOX_CLOSE_LEFT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_BOX_OPEN_RIGHT_CMD:
        {
            const static char *p = "BOX_OPEN_RIGHT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_BOX_OPEN_RIGHT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_BOX_CLOSE_RIGHT_CMD:
        {
            const static char *p = "BOX_CLOSE_RIGHT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_BOX_CLOSE_RIGHT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_PUSH_TO_TALK_CMD:
        {
            const static char *p = "PUSH_TO_TALK";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_PUSH_TO_TALK_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_TWS_DISCONNECT_CMD:
        {
            const static char *p = "TWS_DISCONNECT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_TWS_DISCONNECT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        case APP_IBRT_PERIPHERAL_MANAGER_TWS_PHONE_DISCONNECT_CMD:
        {
            const static char *p = "TWS_PHONE_DISCONNECT";
            msg.msg_body.message_Param0 = (uint32_t)APP_IBRT_PERIPHERAL_MANAGER_TWS_PHONE_DISCONNECT_CMD;
            msg.msg_body.command = p;
            msg.msg_body.payload = payload;
            msg.msg_body.payload_length = payload_length;
            app_ibrt_peripheral_mailbox_put(&msg);
            temp = 0;
        }
        break;
        
        //ANC TUNE class:
        
        default:
        temp = -1;
        break;
    }
    
    TRACE("[%s] exit... return_status: %d", __func__, temp);
    
    return temp;
}
#endif

