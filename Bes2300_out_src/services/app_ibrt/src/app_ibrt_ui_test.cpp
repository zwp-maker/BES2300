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
#include "app_tws_ibrt_trace.h"
#include "factory_section.h"
#include "apps.h"
#include "app_battery.h"
#include "app_anc.h"
#include "app_key.h"
#include "app_ibrt_if.h"
#include "app_ibrt_ui_test.h"
#include "app_ibrt_peripheral_manager.h"
#include "a2dp_decoder.h"
#include "app_ibrt_keyboard.h"
#include "nvrecord_env.h"
#include "nvrecord_ble.h"
#include "app_tws_if.h"
#include "besbt.h"
#include "app_bt.h"
#include "app_ibrt_keyboard.h"//  jackdong add
#include "app_factory.h"//  jackdong add
// #include "app_ai_if.h"
#include "app_ibrt_nvrecord.h"
#include "nvrecord_bt.h"
#include "app_factory_bt.h"
#include "med_memory.h"

#ifdef __AI_VOICE__
#include "ai_thread.h"
#include "ai_control.h"
#include "ai_manager.h"
#endif
#if defined(VOICE_DATAPATH)
#include "app_voicepath.h"
#endif
#define _FILE_TAG_ "UI"
#include "color_log.h"
#if defined(IBRT)
#include "btapp.h"
extern struct BT_DEVICE_T  app_bt_device;

bt_bdaddr_t master_ble_addr = {0x76, 0x33, 0x33, 0x22, 0x11, 0x11};
bt_bdaddr_t slave_ble_addr  = {0x77, 0x33, 0x33, 0x22, 0x11, 0x11};
bt_bdaddr_t box_ble_addr    = {0x78, 0x33, 0x33, 0x22, 0x11, 0x11};

#ifdef IBRT_SEARCH_UI
void app_ibrt_battery_callback(APP_BATTERY_MV_T currvolt, uint8_t currlevel,enum APP_BATTERY_STATUS_T curstatus,uint32_t status, union APP_BATTERY_MSG_PRAMS prams);
void app_ibrt_simulate_charger_plug_in_test(void)
{
    union APP_BATTERY_MSG_PRAMS msg_prams;
    msg_prams.charger = APP_BATTERY_CHARGER_PLUGIN;
    app_ibrt_battery_callback(0, 0, APP_BATTERY_STATUS_CHARGING, 1, msg_prams);
}
void app_ibrt_simulate_charger_plug_out_test(void)
{
    union APP_BATTERY_MSG_PRAMS msg_prams;
    msg_prams.charger = APP_BATTERY_CHARGER_PLUGOUT;
    app_ibrt_battery_callback(0, 0, APP_BATTERY_STATUS_CHARGING, 1, msg_prams);
}
void app_ibrt_simulate_charger_plug_box_test(void)
{
    static int count = 0;
    if (count++ % 2 == 0)
    {
        app_ibrt_simulate_charger_plug_in_test();
    }
    else
    {
        app_ibrt_simulate_charger_plug_out_test();
    }
}
void app_ibrt_simulate_charger_plugin_key(APP_KEY_STATUS *status, void *param)
{
    app_ibrt_simulate_charger_plug_in_test();
}
void app_ibrt_simulate_charger_plugout_key(APP_KEY_STATUS *status, void *param)
{
    app_ibrt_simulate_charger_plug_out_test();
}
void app_ibrt_simulate_tws_role_switch(APP_KEY_STATUS *status, void *param)
{
    app_ibrt_ui_tws_switch();
}
#endif
void app_ibrt_ui_audio_play(void)
{
    uint8_t action[] = {IBRT_ACTION_PLAY};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_audio_pause(void)
{
    uint8_t action[] = {IBRT_ACTION_PAUSE};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_audio_play_test(void)
{
    app_ibrt_ui_event_entry(IBRT_AUDIO_PLAY);
}
void app_ibrt_ui_audio_pause_test(void)
{
    app_ibrt_ui_event_entry(IBRT_AUDIO_PAUSE);
}
void app_ibrt_ui_audio_forward_test(void)
{
    uint8_t action[] = {IBRT_ACTION_FORWARD};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_audio_backward_test(void)
{
    uint8_t action[] = {IBRT_ACTION_BACKWARD};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_avrcp_volume_up_test(void)
{
    uint8_t action[] = {IBRT_ACTION_AVRCP_VOLUP};
    app_ibrt_if_start_user_action(action, sizeof(action)); 
}
void app_ibrt_ui_avrcp_volume_down_test(void)
{
    uint8_t action[] = {IBRT_ACTION_AVRCP_VOLDN};
    app_ibrt_if_start_user_action(action, sizeof(action)); 
}
void app_ibrt_ui_hfsco_create_test(void)
{
    uint8_t action[] = {IBRT_ACTION_HFSCO_CREATE};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_hfsco_disc_test(void)
{
    uint8_t action[] = {IBRT_ACTION_HFSCO_DISC};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_call_redial_test(void)
{
    uint8_t action[] = {IBRT_ACTION_REDIAL};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_call_answer_test(void)
{
    uint8_t action[] = {IBRT_ACTION_ANSWER};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_call_hangup_test(void)
{
    uint8_t action[] = {IBRT_ACTION_HANGUP};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_local_volume_up_test(void)
{
    uint8_t action[] = {IBRT_ACTION_LOCAL_VOLUP};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_local_volume_down_test(void)
{
    uint8_t action[] = {IBRT_ACTION_LOCAL_VOLDN};
    app_ibrt_if_start_user_action(action, sizeof(action));
}

void app_ibrt_ui_get_tws_conn_state_test(void)
{
    if(app_tws_ibrt_tws_link_connected())
    {
        TRACE("ibrt_ui_log:TWS CONNECTED");
    }
    else
    {
        TRACE("ibrt_ui_log:TWS DISCONNECTED");
    }
}

void app_ibrt_ui_get_a2dp_state_test(void)
{
   const char* a2dp_state_strings[] = {"IDLE", "CODEC_CONFIGURED", "OPEN","STREAMING","CLOSED","ABORTING"};
   AppIbrtA2dpState a2dp_state;

   AppIbrtStatus  status = app_ibrt_if_get_a2dp_state(&a2dp_state);
   if(APP_IBRT_IF_STATUS_SUCCESS == status)
    {
        TRACE("ibrt_ui_log:a2dp_state=%s",a2dp_state_strings[a2dp_state]);
    }
    else
    {
        TRACE("ibrt_ui_log:get a2dp state error");
    }
}

void app_ibrt_ui_get_avrcp_state_test(void)
{
    const char* avrcp_state_strings[] = {"DISCONNECTED",  "CONNECTED", "PLAYING","PAUSED", "VOLUME_UPDATED"};
    AppIbrtAvrcpState avrcp_state;

    AppIbrtStatus status = app_ibrt_if_get_avrcp_state(&avrcp_state);
    if(APP_IBRT_IF_STATUS_SUCCESS == status)
    {
        TRACE("ibrt_ui_log:avrcp_state=%s",avrcp_state_strings[avrcp_state]);
    }
    else
    {
        TRACE("ibrt_ui_log:get avrcp state error");
    }
}


void app_ibrt_ui_get_hfp_state_test(void)
{
    const char* hfp_state_strings[] = {"SLC_DISCONNECTED",  "CLOSED", "SCO_CLOSED","PENDING","SLC_OPEN", \
                            "NEGOTIATE","CODEC_CONFIGURED","SCO_OPEN","INCOMING_CALL","OUTGOING_CALL","RING_INDICATION"};
    AppIbrtHfpState hfp_state;
    AppIbrtStatus status = app_ibrt_if_get_hfp_state(&hfp_state);

    if(APP_IBRT_IF_STATUS_SUCCESS == status)
    {
        TRACE("ibrt_ui_log:hfp_state=%s",hfp_state_strings[hfp_state]);
    }
    else
    {
        TRACE("ibrt_ui_log:get hfp state error");
    }
}


void app_ibrt_ui_get_call_status_test(void)
{
	TRACE("Start get call status get");

    const char* call_status_strings[] = {"NO_CALL","CALL_ACTIVE","HOLD","INCOMMING","OUTGOING","ALERT"};
    AppIbrtCallStatus call_status;

    AppIbrtStatus status = app_ibrt_if_get_hfp_call_status(&call_status);
    if(APP_IBRT_IF_STATUS_SUCCESS == status)
    {
        TRACE("ibrt_ui_log:call_status=%s",call_status_strings[call_status]);
    }
    else
    {
        TRACE("ibrt_ui_log:get call status error");
    }
}


void app_ibrt_ui_get_ibrt_role_test(void)
{
    ibrt_role_e role = app_ibrt_if_get_ibrt_role();

    if(IBRT_MASTER == role)
    {
        TRACE("ibrt_ui_log:ibrt role is MASTER");
    }
    else if(IBRT_SLAVE == role)
    {
        TRACE("ibrt_ui_log:ibrt role is SLAVE");
    }
    else
    {
        TRACE("ibrt_ui_log:ibrt role is UNKNOW");
    }
}

#if defined(ENHANCED_STACK)
#define IBRT_ENHANCED_STACK_PTS
#include "bt_if.h"
static bt_bdaddr_t pts_bt_addr = {{
#if 0
    0x14, 0x71, 0xda, 0x7d, 0x1a, 0x00
#else
    0x13, 0x71, 0xda, 0x7d, 0x1a, 0x00
#endif
}};
void btif_pts_hf_create_link_with_pts(void)
{
    btif_pts_hf_create_service_link(&pts_bt_addr);
}
void btif_pts_av_create_channel_with_pts(void)
{
    btif_pts_av_create_channel(&pts_bt_addr);
}
void btif_pts_ar_connect_with_pts(void)
{
    btif_pts_ar_connect(&pts_bt_addr);
}
#endif
/*****************************************************************************
 Prototype    : app_ibrt_ui_open_box_event_test
 Description  : app ibrt ui open box test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_open_box_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_OPEN_BOX_TEST_EVENT);
}

void app_ibrt_ui_open_box_event_test2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    TRACE("[%s] nv_role: %d", __func__, p_ibrt_ctrl->nv_role);

    if (p_ibrt_ctrl->nv_role != IBRT_UNKNOW)
    {
        app_ibrt_if_event_entry(IBRT_OPEN_BOX_TEST_EVENT);
    }
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_fetch_out_box_event_test
 Description  : fetch out box test function
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_fetch_out_box_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_FETCH_OUT_TEST_EVENT);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_put_in_box_event_test
 Description  : app ibrt put in box test function
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_put_in_box_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_PUT_IN_TEST_EVENT);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_close_box_event_test
 Description  : app ibrt close box test function
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_close_box_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_CLOSE_BOX_TEST_EVENT);
}

void app_ibrt_ui_close_box_event_test2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    app_ibrt_if_event_entry(IBRT_CLOSE_BOX_TEST_EVENT);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_reconnect_event_test
 Description  : app ibrt reconnect function test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_reconnect_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_RECONNECT_TEST_EVENT);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_ware_up_event_test
 Description  : app ibrt wear up function test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_ware_up_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_WEAR_UP_TEST_EVENT);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_ware_down_event_test
 Description  : app ibrt wear down function test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_ware_down_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_WEAR_DOWN_TEST_EVENT);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_phone_connect_event_test
 Description  : app ibrt ui phone connect event
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/3
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_phone_connect_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_PHONE_CONNECT_TEST_EVENT);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_shut_down_test
 Description  : shut down test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/10
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
extern "C" int app_shutdown(void);
void app_ibrt_ui_shut_down_test(void)
{
    app_shutdown();
}

void app_ibrt_ui_shut_down_test2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    app_shutdown();
}

extern int app_reset(void);
void app_ibrt_ui_reset_test2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    app_reset();
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_tws_swtich_test
 Description  : tws switch test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/10
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
extern bt_status_t btif_me_ibrt_role_switch(uint8_t switch_op);
void app_ibrt_ui_tws_swtich_test(void)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    if (p_ibrt_ctrl->current_role == IBRT_MASTER)
    {
        btif_me_ibrt_role_switch(IBRT_SWITCH);
    }
    else
    {
        TRACE("ibrt_ui_log:local role is ibrt_slave, pls send tws switch in another dev");
    }
}

void app_ibrt_ui_tws_switch_test2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    if (p_ibrt_ctrl->current_role == IBRT_MASTER)
    {
        btif_me_ibrt_role_switch(IBRT_SWITCH);
    }
    else
    {
        TRACE("ibrt_ui_log:local role is ibrt_slave, pls send tws switch in another dev");
    }
}

/*****************************************************************************
 Prototype    : app_ibrt_ui_suspend_ibrt_test
 Description  : suspend ibrt fastack
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/24
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_suspend_ibrt_test(void)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    if (p_ibrt_ctrl->current_role== IBRT_MASTER)
    {
        btif_me_suspend_ibrt();
    }
    else
    {
        TRACE("ibrt_ui_log:local role is ibrt_slave, suspend ibrt failed");
    }
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_resume_ibrt_test
 Description  : resume ibrt test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/27
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_resume_ibrt_test(void)
{
    btif_me_resume_ibrt(1);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_pairing_mode_test
 Description  : pairing mode test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/11/20
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_pairing_mode_test(void)
{
    app_ibrt_ui_event_entry(IBRT_TWS_PAIRING_EVENT);
}

void app_ibrt_ui_pairing_mode_test2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    TRACE("%s %x", __func__, payload[3]);
#ifdef IBRT_SEARCH_UI
    if (payload[3] == 0x11)
    {
        app_ibrt_ui_event_entry(IBRT_TWS_PAIRING_EVENT);
    }
#else

#endif
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_freeman_pairing_mode_test
 Description  : ibrt freeman pairing mode test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/11/21
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_freeman_pairing_mode_test(void)
{
    app_ibrt_ui_event_entry(IBRT_FREEMAN_PAIRING_EVENT);
}

void app_ibrt_ui_freeman_pairing_mode_test2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    app_ibrt_ui_set_enter_pairing_mode(IBRT_FREEMAN_PAIR_MODE_ENTER);
    app_ibrt_ui_set_freeman_enable();
    app_ibrt_ui_judge_scan_type(IBRT_ENTER_PAIRING_MODE_TRIGGER, NO_LINK_TYPE, IBRT_UI_NO_ERROR);
    // app_ibrt_ui_event_entry(IBRT_FREEMAN_PAIRING_EVENT);
}

void app_ibrt_inquiry_start_test(void)
{
    app_bt_start_search();
}

void app_ibrt_ui_clean_record2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    app_ibrt_nvrecord_delete_all_mobile_record();
}

void app_ibrt_ui_dump_record2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    nv_record_all_ddbrec_print();
}

void app_ibrt_ui_enter_signal_mode2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    app_factorymode_bt_signalingtest(NULL, NULL);
}

void app_ibrt_ui_enter_nosignal_mode2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    app_factorymode_bt_nosignalingtest(NULL, NULL);
}

void app_ibrt_ui_threeway_hold_and_answer2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    
    if (p_ibrt_ctrl->current_role== IBRT_MASTER)
    {
        if((app_bt_device.hfchan_call[BT_DEVICE_ID_1] == BTIF_HF_CALL_ACTIVE) && ((app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == BTIF_HF_CALL_SETUP_IN)))
        {
            hfp_handle_key(HFP_KEY_THREEWAY_HOLD_AND_ANSWER);
        }
    }
    else
    {
        TRACE("[%s] this is ibrt slave!!!", __func__);
    }
}

void app_ibrt_ui_recover_factory2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    app_ibrt_nvrecord_rebuild();
}


void app_ibrt_ui_get_bt_address2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    // ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    // TRACE("[%s] nv_role: %d", __func__, p_ibrt_ctrl->nv_role);
    uint8_t *pBt_addr;
    uint8_t *buf = (uint8_t *)malloc(6);
    pBt_addr = factory_section_get_bt_address();

    DUMP8("%02x", payload, payload_length);

    if (payload)
    {
        free(payload);
    }
    // if ((p_ibrt_ctrl->nv_role == IBRT_SLAVE) && (command_code == 0x11))
    {
        buf[0] = payload[3];//single_data.which;
        buf[1] = (uint8_t)command_code;
        memcpy(&buf[2], pBt_addr, 6);
    }

    app_ibrt_peripheral_manage_receive_func_typedef func = app_ibrt_peripheral_manage_get_receive_cb();
    if(func != NULL)
    {
        (*func)(buf, 8);
    }
    else
    {
        TRACE("[%s] app_ibrt_peripheral_manage_receive_cb is null", __func__);
    }
}

void app_ibrt_ui_set_peer_bt_address2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    TRACE("[%s] nv_role: %d", __func__, p_ibrt_ctrl->nv_role);
    DUMP8("%02x", payload, payload_length);

    uint8_t *buf = (uint8_t *)malloc(6);

    memcpy(payload, buf, 6);
    buf[0] = payload[3];//single_data.which;
    buf[1] = (uint8_t)command_code;

    if (payload)
    {
        free(payload);
    }
    
    // if ((p_ibrt_ctrl->nv_role == IBRT_SLAVE) && (command_code == 0x11)) //left
    {
        if(payload != NULL)
        {
            memcpy(p_ibrt_ctrl->peer_addr.address, &payload[5], 6);

            if (buf[0] == 0x11)
            {
                extern void app_tws_ibrt_update_info(ibrt_role_e ibrtRole,bt_bdaddr_t *ibrtPeerAddr);
                extern void app_ibrt_config_the_same_bd_addr(bt_bdaddr_t *ibrtSearchedAddr);
                app_ibrt_config_the_same_bd_addr((bt_bdaddr_t *)p_ibrt_ctrl->peer_addr.address);
                app_tws_ibrt_update_info(IBRT_MASTER, (bt_bdaddr_t *)p_ibrt_ctrl->peer_addr.address);
            }
        }
        else
        {
            TRACE("[%s] payload is null", __func__);
        }        
    }

    app_ibrt_peripheral_manage_receive_func_typedef func = app_ibrt_peripheral_manage_get_receive_cb();
    if(func != NULL)
    {
        (*func)(buf, 6);
    }
    else
    {
        TRACE("[%s] app_ibrt_peripheral_manage_receive_cb is null", __func__);
    }
}

void app_ibrt_ui_disconnect_tws_link2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    
    if (app_tws_ibrt_tws_link_connected())
    {
        btif_hci_disconnect_connection_direct(p_ibrt_ctrl->tws_conhandle,BTIF_BEC_USER_TERMINATED);
    }
}

void app_ibrt_ui_disconnect_mobile_link2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    
    if(IBRT_MASTER == p_ibrt_ctrl->current_role)
    {
        if(app_tws_ibrt_mobile_link_connected())
        {
            btif_hci_disconnect_connection_direct(p_ibrt_ctrl->mobile_conhandle,BTIF_BEC_USER_TERMINATED);
        }
    }
}

void app_ibrt_ui_siri_voice2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    
    if(p_ibrt_ctrl->current_role == IBRT_MASTER)
    {
        if((btif_get_hf_chan_state(app_bt_device.hf_channel[BT_DEVICE_ID_1]) == BTIF_HF_STATE_OPEN))
        {
            if(payload != NULL)
            {
                if(payload[0] == 1)
                {
                    if(btif_hf_is_voice_rec_active(app_bt_device.hf_channel[BT_DEVICE_ID_1]) == false)
                    {
                        btif_hf_enable_voice_recognition(app_bt_device.hf_channel[BT_DEVICE_ID_1], true);
                    }
                }
                else if(payload[0] == 0)
                {
                    if(btif_hf_is_voice_rec_active(app_bt_device.hf_channel[BT_DEVICE_ID_1]) == true)
                    {
                        btif_hf_enable_voice_recognition(app_bt_device.hf_channel[BT_DEVICE_ID_1], false);
                    }
                }
                else
                {
                    TRACE("[%s] argument illegal", __func__);
                }
            }
            else
            {
                TRACE("[%s] payload is null", __func__);
            }
        }
    }
    else
    {
        TRACE("[%s] this is ibrt slave!!!", __func__);
    }
}

void app_ibrt_ui_pairing_mode_refresh2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    app_ibrt_if_pairing_mode_refresh();
}

void app_ibrt_ui_ep_exchange_battery_level2(uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    uint8_t level;
    uint16_t volt;
    uint8_t *buf = (uint8_t *)malloc(6);
    APP_BATTERY_STATUS_T battery;
    TRACE("%s", __func__);
    DUMP8("%02x", payload, payload_length);

    app_battery_get_info(&volt, &level, &battery);

    buf[0] = payload[3];    //which payload[5]; // box battery level
    buf[1] = (uint8_t)command_code;
    buf[2] = (uint8_t)(volt >> 8 & 0x00ff);
    buf[3] = (uint8_t)(volt & 0x00ff);
    buf[4] = level;
    buf[5] = (uint8_t)battery;
    TRACE("Level %d", level);
    free(payload);

    app_ibrt_peripheral_manage_receive_func_typedef func = app_ibrt_peripheral_manage_get_receive_cb();
    if(func != NULL)
    {
        (*func)(buf, 6);
    }
}

const app_uart_handle_t app_ibrt_uart_test_handle[]=
{
    {"open_box_event_test",app_ibrt_ui_open_box_event_test},
    {"fetch_out_box_event_test",app_ibrt_ui_fetch_out_box_event_test},
    {"put_in_box_event_test",app_ibrt_ui_put_in_box_event_test},
    {"close_box_event_test",app_ibrt_ui_close_box_event_test},
    {"reconnect_event_test",app_ibrt_ui_reconnect_event_test},
    {"wear_up_event_test",app_ibrt_ui_ware_up_event_test},
    {"wear_down_event_test",app_ibrt_ui_ware_down_event_test},
    {"shut_down_test",app_ibrt_ui_shut_down_test},
    {"phone_connect_event_test",app_ibrt_ui_phone_connect_event_test},
    {"switch_ibrt_test",app_ibrt_ui_tws_swtich_test},
    {"suspend_ibrt_test",app_ibrt_ui_suspend_ibrt_test},
    {"resume_ibrt_test",app_ibrt_ui_resume_ibrt_test},
    {"conn_second_mobile_test",app_ibrt_ui_choice_connect_second_mobile},
    {"mobile_tws_disc_test",app_ibrt_if_disconnect_mobile_tws_link},
    {"pairing_mode_test",app_ibrt_ui_pairing_mode_test},
    {"freeman_mode_test",app_ibrt_ui_freeman_pairing_mode_test},
    {"audio_play",app_ibrt_ui_audio_play_test},
    {"audio_pause",app_ibrt_ui_audio_pause_test},
    {"audio_forward",app_ibrt_ui_audio_forward_test},
    {"audio_backward",app_ibrt_ui_audio_backward_test},
    {"avrcp_volup",app_ibrt_ui_avrcp_volume_up_test},
    {"avrcp_voldn",app_ibrt_ui_avrcp_volume_down_test},
    {"hfsco_create",app_ibrt_ui_hfsco_create_test},
    {"hfsco_disc",app_ibrt_ui_hfsco_disc_test},
    {"call_redial",app_ibrt_ui_call_redial_test},
    {"call_answer",app_ibrt_ui_call_answer_test},
    {"call_hangup",app_ibrt_ui_call_hangup_test},
    {"volume_up",app_ibrt_ui_local_volume_up_test},
    {"volume_down",app_ibrt_ui_local_volume_down_test},
    {"get_a2dp_state",app_ibrt_ui_get_a2dp_state_test},
    {"get_avrcp_state",app_ibrt_ui_get_avrcp_state_test},
    {"get_hfp_state",app_ibrt_ui_get_hfp_state_test},
    {"get_call_status",app_ibrt_ui_get_call_status_test},
    {"get_ibrt_role",app_ibrt_ui_get_ibrt_role_test},
    {"get_tws_state",app_ibrt_ui_get_tws_conn_state_test},
#ifdef IBRT_SEARCH_UI
    {"plug_in_test",app_ibrt_simulate_charger_plug_in_test},
    {"plug_out_test",app_ibrt_simulate_charger_plug_out_test},
    {"plug_box_test",app_ibrt_simulate_charger_plug_box_test},
#endif
#ifdef IBRT_ENHANCED_STACK_PTS
    {"hf_create_service_link",btif_pts_hf_create_link_with_pts},
    {"hf_disc_service_link",btif_pts_hf_disc_service_link},
    {"hf_create_audio_link",btif_pts_hf_create_audio_link},
    {"hf_disc_audio_link",btif_pts_hf_disc_audio_link},
    {"hf_answer_call",btif_pts_hf_answer_call},
    {"hf_hangup_call",btif_pts_hf_hangup_call},
    {"rfc_register",btif_pts_rfc_register_channel},
    {"rfc_close",btif_pts_rfc_close},
    {"av_create_channel",btif_pts_av_create_channel_with_pts},
    {"av_disc_channel",btif_pts_av_disc_channel},
    {"ar_connect",btif_pts_ar_connect_with_pts},
    {"ar_disconnect",btif_pts_ar_disconnect},
    {"ar_panel_play",btif_pts_ar_panel_play},
    {"ar_panel_pause",btif_pts_ar_panel_pause},
    {"ar_panel_stop",btif_pts_ar_panel_stop},
    {"ar_panel_forward",btif_pts_ar_panel_forward},
    {"ar_panel_backward",btif_pts_ar_panel_backward},
    {"ar_volume_up",btif_pts_ar_volume_up},
    {"ar_volume_down",btif_pts_ar_volume_down},
    {"ar_volume_notify",btif_pts_ar_volume_notify},
    {"ar_volume_change",btif_pts_ar_volume_change},
    {"ar_set_absolute_volume",btif_pts_ar_set_absolute_volume},
#endif
};

const app_single_uart_handle_t app_ibrt_single_uart_test_handle[]=
{
    //UI class:
    {"OPEN_CASE",                       app_ibrt_ui_open_box_event_test2},
    {"CLOSE_CASE",                      app_ibrt_ui_close_box_event_test2},
    {"RESET",                           app_ibrt_ui_reset_test2},
    {"POWER_OFF",                       app_ibrt_ui_shut_down_test2},
    {"PAIRING_TWS",                     app_ibrt_ui_pairing_mode_test2},
    {"PAIRING_SINGLE",                  app_ibrt_ui_freeman_pairing_mode_test2},
    {"TWS_GET_EP_INFO",                 NULL},//wait...
    {"TWS_EXC_BATT",                    app_ibrt_ui_ep_exchange_battery_level2},
    {"CLEAN_RECORD",                    app_ibrt_ui_clean_record2},
    {"RECOVER_FACTOR",                  app_ibrt_ui_recover_factory2},
    {"TWS_PAIRING_PHONE",               app_ibrt_ui_pairing_mode_refresh2},
    {"TWS_GET_BTADDRESS",               app_ibrt_ui_get_bt_address2},
    {"TWS_SET_PEER_BTADDRESS",          app_ibrt_ui_set_peer_bt_address2},
    
    //TEST class:
    {"BTTEST_ENTER_DUT",                app_ibrt_ui_enter_signal_mode2},
    {"BTTEST_HANDSET_PAIRING",          NULL},//wait...
    {"BTTEST_VCO_TEST",                 app_ibrt_ui_enter_nosignal_mode2},
    {"BTTEST_NOSIG_TEST",               app_ibrt_ui_enter_nosignal_mode2},
    {"DUMP_RECORD",                     app_ibrt_ui_dump_record2},
    
    //FACTORY class:
    
    //AUTO TEST class:
    // {"FETH_OUT_LEFT",                   app_ibrt_ui_left_fetch_out_box_event_test2},
    // {"FETH_IN_LEFT",                    app_ibrt_ui_left_put_in_box_event_test2},
    // {"FETH_OUT_RIGHT",                  app_ibrt_ui_right_fetch_out_box_event_test2},
    // {"FETH_IN_RIGHT",                   app_ibrt_ui_right_put_in_box_event_test2},
    // {"WEAR_ON_LEFT",                    app_ibrt_ui_left_ware_up_event_test2},
    // {"WEAR_OFF_LEFT",                   app_ibrt_ui_left_ware_down_event_test2},
    // {"WEAR_ON_RIGHT",                   app_ibrt_ui_right_ware_up_event_test2},
    // {"WEAR_OFF_RIGHT",                  app_ibrt_ui_right_ware_down_event_test2},
    // {"ROLE_SWITCH",                     app_ibrt_ui_tws_switch_test2},
    // {"PHONE_CALL",                      app_ibrt_ui_call_redial_test2},
    // {"PHONE_REJECT",                    app_ibrt_ui_call_hangup_test2},
    // {"PHONE_HOLD",                      app_ibrt_ui_threeway_hold_and_answer2},
    // {"MUSIC_PLAY",                      app_ibrt_ui_audio_play_test2},
    // {"MUSIC_PAUSE",                     app_ibrt_ui_audio_pause_test2},
    // {"MUSIC_NEXT",                      app_ibrt_ui_audio_forward_test2},
    // {"MUSIC_PRE",                       app_ibrt_ui_audio_backward_test2},
    // {"VOLUME_UP",                       app_ibrt_ui_avrcp_volume_up_test2},
    // {"VOLUME_DOWN",                     app_ibrt_ui_avrcp_volume_down_test2},
    // {"BOX_OPEN_LEFT",                   app_ibrt_ui_left_open_box_event_test2},
    // {"BOX_CLOSE_LEFT",                  app_ibrt_ui_left_close_box_event_test2},
    // {"BOX_OPEN_RIGHT",                  app_ibrt_ui_right_open_box_event_test2},
    // {"BOX_CLOSE_RIGHT",                 app_ibrt_ui_right_close_box_event_test2},
    // {"PUSH_TO_TALK",                    app_ibrt_ui_siri_voice2},
    // {"TWS_DISCONNECT",                  app_ibrt_ui_disconnect_tws_link2},
    // {"TWS_PHONE_DISCONNECT",            app_ibrt_ui_disconnect_mobile_link2},
    
    //ANC TUNE class:
};
/*****************************************************************************
 Prototype    : app_ibrt_ui_find_uart_handle
 Description  : find the test cmd handle
 Input        : uint8_t* buf
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
app_uart_test_function_handle app_ibrt_ui_find_uart_handle(unsigned char* buf)
{
    app_uart_test_function_handle p = NULL;
    for(uint8_t i = 0; i<sizeof(app_ibrt_uart_test_handle)/sizeof(app_uart_handle_t); i++)
    {
        if(strncmp((char*)buf, app_ibrt_uart_test_handle[i].string, strlen(app_ibrt_uart_test_handle[i].string))==0 ||
           strstr(app_ibrt_uart_test_handle[i].string, (char*)buf))
        {
            p = app_ibrt_uart_test_handle[i].function;
            break;
        }
    }
    return p;
}


/*****************************************************************************
 Prototype    : app_ibrt_ui_find_uart_handle
 Description  : find the test cmd handle
 Input        : uint8_t* buf
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
app_single_uart_test_function_handle app_ibrt_ui_find_single_uart_handle(unsigned char* buf)
{
    app_single_uart_test_function_handle p = NULL;
    for(uint8_t i = 0; i<sizeof(app_ibrt_single_uart_test_handle)/sizeof(app_single_uart_handle_t); i++)
    {
        if(strcmp((char*)buf, app_ibrt_single_uart_test_handle[i].string) == 0)
        {
            p = app_ibrt_single_uart_test_handle[i].function;
            TRACE("[%s] %s", __func__, app_ibrt_single_uart_test_handle[i].string);
            break;
        }
    }
    return p;
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_test_cmd_handler
 Description  : ibrt ui test cmd handler
 Input        : uint8_t *buf
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
extern "C" int app_ibrt_ui_test_cmd_handler(unsigned char *buf, unsigned int length)
{
    int ret = 0;

    if (buf[length-2] == 0x0d ||
        buf[length-2] == 0x0a)
    {
        buf[length-2] = 0;
    }

    app_uart_test_function_handle handl_function = app_ibrt_ui_find_uart_handle(buf);
    if(handl_function)
    {
        handl_function();
    }
    else
    {
        ret = -1;
        TRACE("can not find handle function");
    }
    return ret;
}


extern "C" int app_ibrt_ui_test_cmd_handler2(const char *command, uint32_t command_code, uint8_t *payload, uint32_t payload_length)
{
    int ret = 0;
    
    app_single_uart_test_function_handle handl_function = app_ibrt_ui_find_single_uart_handle((unsigned char *)command);
    
    if(handl_function)
    {
        (*handl_function)(command_code, payload, payload_length);
    }
    else
    {
        ret = -1;
        TRACE("[%s] can not find handle function", __func__);
    }
    
    return ret;
}


#ifdef BES_AUDIO_DEV_Main_Board_9v0
void app_ibrt_key1(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    TWS_PD_MSG_BLOCK msg;
    msg.msg_body.message_id = 0;
    msg.msg_body.message_ptr = (uint32_t)NULL;
    app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_key2(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    TWS_PD_MSG_BLOCK msg;
    msg.msg_body.message_id = 1;
    msg.msg_body.message_ptr = (uint32_t)NULL;
    app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_key3(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    TWS_PD_MSG_BLOCK msg;
    msg.msg_body.message_id = 2;
    msg.msg_body.message_ptr = (uint32_t)NULL;
    app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_key4(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    TWS_PD_MSG_BLOCK msg;
    msg.msg_body.message_id = 3;
    msg.msg_body.message_ptr = (uint32_t)NULL;
    app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_key5(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    TWS_PD_MSG_BLOCK msg;
    msg.msg_body.message_id = 4;
    msg.msg_body.message_ptr = (uint32_t)NULL;
    app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_key6(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    TWS_PD_MSG_BLOCK msg;
    msg.msg_body.message_id = 5;
    msg.msg_body.message_ptr = (uint32_t)NULL;
    app_ibrt_peripheral_mailbox_put(&msg);
}
#endif
/**
 * @description: 主耳调用的按键事件处理函数
 * @param {APP_KEY_STATUS} *status
 * @param {void} *param
 * @return {*}
 * @author: Jevin
 */
void app_ibrt_ui_test_key(APP_KEY_STATUS *status, void *param)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    //uint8_t shutdown_key = HAL_KEY_EVENT_LONGLONGPRESS;//jack dong   关机按键单独处理
    TRACE("%s %d,%d",__func__, status->code, status->event);

    if(status->event==APP_KEY_EVENT_LONGLONGPRESS&&Com_key==0x02)//单独进入DUT模式  jackdong add
    {
        TRACE(" 5S!!");
        app_voice_report(APP_STATUS_INDICATION_POWERON, 0);//调试音测试
        TRACE("ENTER DUT MODE!!"); 
        app_KeyboardTimer_stop();
        app_factorymode_enter();//使用系统默认的函数     
        return;          
    }

    keyeventsFromSlave=false;//每次进入默认是主耳的按键事件
    if (IBRT_SLAVE == p_ibrt_ctrl->current_role /*&& status->event != shutdown_key*/)//jack dong   关机按键单独处理
    {
        app_ibrt_if_keyboard_notify(status,param);//如果当前角色是从耳且状态不是关机，则发送命令给主耳
    }
    else
    {
#ifdef IBRT_SEARCH_UI
        app_ibrt_search_ui_handle_key(status,param);
#else
        app_ibrt_normal_ui_handle_key(status,param);
#endif
    }
}

void app_ibrt_ui_test_key_io_event(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    switch(status->event)
    {
        case APP_KEY_EVENT_CLICK:
            if (status->code== APP_KEY_CODE_FN1)
            {
                app_ibrt_if_event_entry(IBRT_OPEN_BOX_EVENT);
            }
            else if (status->code== APP_KEY_CODE_FN2)
            {
                app_ibrt_if_event_entry(IBRT_FETCH_OUT_EVENT);
            }
            else
            {
                app_ibrt_if_event_entry(IBRT_WEAR_UP_EVENT);
            }
            break;

        case APP_KEY_EVENT_DOUBLECLICK:
            if (status->code== APP_KEY_CODE_FN1)
            {
                app_ibrt_if_event_entry(IBRT_CLOSE_BOX_EVENT);
            }
            else if (status->code== APP_KEY_CODE_FN2)
            {
                app_ibrt_if_event_entry(IBRT_PUT_IN_EVENT);
            }
            else
            {
                app_ibrt_if_event_entry(IBRT_WEAR_DOWN_EVENT);
            }
            break;

        case APP_KEY_EVENT_LONGPRESS:
            break;

        case APP_KEY_EVENT_TRIPLECLICK:
            break;

        case HAL_KEY_EVENT_LONGLONGPRESS:
            break;

        case APP_KEY_EVENT_ULTRACLICK:
            break;

        case APP_KEY_EVENT_RAMPAGECLICK:
            break;
    }
}

void app_ibrt_ui_test_key_custom_event(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    switch(status->event)
    {
        case APP_KEY_EVENT_CLICK:
            break;

        case APP_KEY_EVENT_DOUBLECLICK:
            break;

        case APP_KEY_EVENT_LONGPRESS:
            break;

        case APP_KEY_EVENT_TRIPLECLICK:
            break;

        case HAL_KEY_EVENT_LONGLONGPRESS:
            break;

        case APP_KEY_EVENT_ULTRACLICK:
            break;

        case APP_KEY_EVENT_RAMPAGECLICK:
            break;
    }
}

#ifdef IS_MULTI_AI_ENABLED
#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
extern "C" bool gva_status_update_start;
extern "C" bool ama_status_update_start;
#endif
#endif
void app_ibrt_ui_test_voice_assistant_key(APP_KEY_STATUS *status, void *param)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    TRACE("%s event %d", __func__, status->event);

    if (p_ibrt_ctrl->current_role != IBRT_MASTER)
    {
        app_ibrt_if_keyboard_notify(status, param);
        TRACE("%s isn't master %d", __func__, p_ibrt_ctrl->current_role);
        return;
    }
#ifdef IS_MULTI_AI_ENABLED
    if (ai_manager_spec_get_status_is_in_invalid())
    {
        TRACE("AI feature has been diabled");
        return;
    }

#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
    if (gva_status_update_start||ama_status_update_start)
    {
        TRACE("device reboot is ongoing...");
        return;
    }
#endif

    if(ai_voicekey_is_enable())
    {
        if (AI_SPEC_GSOUND == ai_manager_get_current_spec())
        {
#ifdef VOICE_DATAPATH
            app_voicepath_key(status, param);
#endif
        }
        else if(AI_SPEC_INIT != ai_manager_get_current_spec())
        {
            if (APP_KEY_EVENT_CLICK == status->event)
                ai_function_handle(CALLBACK_WAKE_UP, status, 0);
        }
    }
#else
#if defined(__AI_VOICE__)
    if(ai_struct.wake_up_type == TYPE__PRESS_AND_HOLD)
    {
        if (APP_KEY_EVENT_FIRST_DOWN == status->event)
        {
            TRACE("%s wake up AI", __func__);
            ai_function_handle(CALLBACK_WAKE_UP, status, 0);
        }
        else if (APP_KEY_EVENT_UP == status->event)
        {
            TRACE("%s stop speech", __func__);
#ifdef __AMA_VOICE__
            ai_function_handle(CALLBACK_ENDPOINT_SPEECH, status, 0);
#elif   defined(__TENCENT_VOICE__)
            ai_function_handle(CALLBACK_REQUEST_FOR_STOP_SPEECH, status, 0);
#else
            ai_function_handle(CALLBACK_STOP_SPEECH, status, 0);
#endif
        }
#ifdef __TENCENT_VOICE__
        else if(APP_KEY_EVENT_DOUBLECLICK == status->event)
        {
            TRACE("%s stop replying", __func__);
            ai_function_handle(CALLBACK_STOP_REPLYING, status, 0);
        }
#endif
    }
    else
    {
        if (APP_KEY_EVENT_CLICK == status->event)
        {
            ai_function_handle(CALLBACK_WAKE_UP, status, 0);
        }
    }
#elif defined(VOICE_DATAPATH)
    app_voicepath_key(status, param);
#endif
#endif
}
extern void app_tws_start_chargerbox_adv(void);

void app_ibrt_ui_test_key_voice(APP_KEY_STATUS *status, void *param)
{
    LOG_D(" ");
    //app_voice_report(APP_STATUS_INDICATION_CHARGENEED, 1);
    //app_voice_report_generic(APP_STATUS_INDICATION_CHARGENEED, 1,false);
//    app_tws_start_chargerbox_adv();
    app_ibrt_if_pairing_mode_refresh();
}
void app_ibrt_ui_test_mtu_change(void)
{
    extern volatile uint8_t avdtp_playback_delay_sbc_mtu;
    extern volatile uint8_t avdtp_playback_delay_aac_mtu;

    /* add code low delay */
   //A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU  default in target.mk
    {
        avdtp_playback_delay_sbc_mtu = 35;
        avdtp_playback_delay_aac_mtu = 4;
        app_ibrt_if_force_audio_retrigger();
    }

    TRACE("set SBC_MTU=%d AAC_MTU=%d\n", avdtp_playback_delay_sbc_mtu, avdtp_playback_delay_aac_mtu);
}

const APP_KEY_HANDLE  app_ibrt_ui_test_key_cfg[] =
{
#if defined(__AI_VOICE__) || defined(GSOUND_ENABLED)
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_FIRST_DOWN}, "google assistant key", app_ibrt_ui_test_voice_assistant_key, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_UP}, "google assistant key", app_ibrt_ui_test_voice_assistant_key, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_LONGPRESS}, "google assistant key", app_ibrt_ui_test_voice_assistant_key, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_CLICK}, "google assistant key", app_ibrt_ui_test_voice_assistant_key, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_DOUBLECLICK}, "google assistant key", app_ibrt_ui_test_voice_assistant_key, NULL},
#endif

#if defined( __BT_ANC_KEY__)&&defined(ANC_APP)
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt anc key",app_anc_key, NULL},
#else
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key", app_ibrt_ui_test_key, NULL},
#endif
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_UP_AFTER_LONGPRESS},"app_ibrt_ui_test_key", app_ibrt_ui_test_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGPRESS},"app_ibrt_ui_test_key", app_ibrt_ui_test_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGLONGPRESS},"app_ibrt_ui_test_key", app_ibrt_ui_test_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGLONGLONGPRESS},"app_ibrt_ui_test_key", app_ibrt_ui_test_key, NULL},//jackdong 增加长长长按按键
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_DOUBLECLICK},"app_ibrt_ui_test_key", app_ibrt_ui_test_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_TRIPLECLICK},"app_ibrt_ui_test_key", app_ibrt_ui_test_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_ULTRACLICK},"app_ibrt_ui_test_key", app_ibrt_ui_test_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_RAMPAGECLICK},"app_ibrt_ui_test_key", app_ibrt_ui_test_key, NULL},

#if defined(CHIP_BEST1400)
#ifdef IBRT_SEARCH_UI
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key", app_ibrt_simulate_charger_plugin_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOUBLECLICK},"app_ibrt_ui_test_key",app_ibrt_simulate_tws_role_switch, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key", app_ibrt_simulate_charger_plugout_key, NULL},
#endif
#elif defined(CHIP_BEST1402)
#ifdef IBRT_SEARCH_UI
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key", app_ibrt_simulate_charger_plugin_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_LONGPRESS},"app_ibrt_ui_test_key",app_ibrt_simulate_tws_role_switch, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOUBLECLICK},"app_ibrt_ui_test_key", app_ibrt_simulate_charger_plugout_key, NULL},
#endif
#else
#ifdef IBRT_SEARCH_UI
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key", app_ibrt_simulate_charger_plugin_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOUBLECLICK},"app_ibrt_ui_test_key",app_ibrt_simulate_tws_role_switch, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key", app_ibrt_simulate_charger_plugout_key, NULL},
#else
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key", app_ibrt_ui_test_key_io_event, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOUBLECLICK},"app_ibrt_ui_test_key", app_ibrt_ui_test_key_io_event, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key", app_ibrt_ui_test_key_io_event, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_DOUBLECLICK},"app_ibrt_ui_test_key", app_ibrt_ui_test_key_io_event, NULL},
#endif
#endif
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key", app_ibrt_ui_test_key_io_event, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_DOUBLECLICK},"app_ibrt_ui_test_key", app_ibrt_ui_test_key_io_event, NULL},
    /*
    #ifdef BES_AUDIO_DEV_Main_Board_9v0
        {{APP_KEY_CODE_FN1,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key", app_ibrt_key1, NULL},
        {{APP_KEY_CODE_FN2,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key", app_ibrt_key2, NULL},
        {{APP_KEY_CODE_FN3,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key", app_ibrt_key3, NULL},
        {{APP_KEY_CODE_FN4,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key", app_ibrt_key4, NULL},
        {{APP_KEY_CODE_FN5,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key", app_ibrt_key5, NULL},
        {{APP_KEY_CODE_FN6,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key", app_ibrt_key6, NULL},
    #endif
    */
};

/*
* customer addr config here
*/
ibrt_pairing_info_t g_ibrt_pairing_info[] =
{
    {{0x51, 0x33, 0x33, 0x22, 0x11, 0x11},{0x50, 0x33, 0x33, 0x22, 0x11, 0x11}},
    {{0x53, 0x33, 0x33, 0x22, 0x11, 0x11},{0x52, 0x33, 0x33, 0x22, 0x11, 0x11}}, /*LJH*/
    {{0x61, 0x33, 0x33, 0x22, 0x11, 0x11},{0x60, 0x33, 0x33, 0x22, 0x11, 0x11}},
    {{0x71, 0x33, 0x33, 0x22, 0x11, 0x11},{0x70, 0x33, 0x33, 0x22, 0x11, 0x11}},
    {{0x81, 0x33, 0x33, 0x22, 0x11, 0x11},{0x80, 0x33, 0x33, 0x22, 0x11, 0x11}},
    {{0x91, 0x33, 0x33, 0x22, 0x11, 0x11},{0x90, 0x33, 0x33, 0x22, 0x11, 0x11}}, /*Customer use*/
    {{0x05, 0x33, 0x33, 0x22, 0x11, 0x11},{0x04, 0x33, 0x33, 0x22, 0x11, 0x11}}, /*Rui*/
    {{0x07, 0x33, 0x33, 0x22, 0x11, 0x11},{0x06, 0x33, 0x33, 0x22, 0x11, 0x11}}, /*zsl*/
    {{0x88, 0xaa, 0x33, 0x22, 0x11, 0x11},{0x87, 0xaa, 0x33, 0x22, 0x11, 0x11}}, /*Lufang*/
    {{0x77, 0x22, 0x66, 0x22, 0x11, 0x11},{0x77, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*xiao*/
    {{0xAA, 0x22, 0x66, 0x22, 0x11, 0x11},{0xBB, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*LUOBIN*/
    {{0x08, 0x33, 0x66, 0x22, 0x11, 0x11},{0x07, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*Yangbin1*/
    {{0x0B, 0x33, 0x66, 0x22, 0x11, 0x11},{0x0A, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*Yangbin2*/
    {{0x35, 0x33, 0x66, 0x22, 0x11, 0x11},{0x34, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*Lulu*/
    {{0xF8, 0x33, 0x66, 0x22, 0x11, 0x11},{0xF7, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*jtx*/
    {{0xd3, 0x53, 0x86, 0x42, 0x71, 0x31},{0xd2, 0x53, 0x86, 0x42, 0x71, 0x31}}, /*shhx*/
    {{0xcc, 0xaa, 0x99, 0x88, 0x77, 0x66},{0xbb, 0xaa, 0x99, 0x88, 0x77, 0x66}}, /*mql*/
    {{0x95, 0x33, 0x69, 0x22, 0x11, 0x11},{0x94, 0x33, 0x69, 0x22, 0x11, 0x11}}, /*wyl*/
    {{0x82, 0x35, 0x68, 0x24, 0x19, 0x17},{0x81, 0x35, 0x68, 0x24, 0x19, 0x17}}, /*hy*/
    {{0x66, 0x66, 0x88, 0x66, 0x66, 0x88},{0x65, 0x66, 0x88, 0x66, 0x66, 0x88}}, /*xdl*/
    {{0x61, 0x66, 0x66, 0x66, 0x66, 0x81},{0x16, 0x66, 0x66, 0x66, 0x66, 0x18}}, /*test1*/
    {{0x62, 0x66, 0x66, 0x66, 0x66, 0x82},{0x26, 0x66, 0x66, 0x66, 0x66, 0x28}}, /*test2*/
    {{0x63, 0x66, 0x66, 0x66, 0x66, 0x83},{0x36, 0x66, 0x66, 0x66, 0x66, 0x38}}, /*test3*/
    {{0x64, 0x66, 0x66, 0x66, 0x66, 0x84},{0x46, 0x66, 0x66, 0x66, 0x66, 0x48}}, /*test4*/
    {{0x65, 0x66, 0x66, 0x66, 0x66, 0x85},{0x56, 0x66, 0x66, 0x66, 0x66, 0x58}}, /*test5*/
    {{0xaa, 0x66, 0x66, 0x66, 0x66, 0x86},{0xaa, 0x66, 0x66, 0x66, 0x66, 0x68}}, /*test6*/
    {{0x67, 0x66, 0x66, 0x66, 0x66, 0x87},{0x76, 0x66, 0x66, 0x66, 0x66, 0x78}}, /*test7*/
    {{0x68, 0x66, 0x66, 0x66, 0x66, 0xa8},{0x86, 0x66, 0x66, 0x66, 0x66, 0x8a}}, /*test8*/
    {{0x69, 0x66, 0x66, 0x66, 0x66, 0x89},{0x86, 0x66, 0x66, 0x66, 0x66, 0x18}}, /*test9*/
    {{0x66, 0x66, 0x66, 0x66, 0x66, 0x66},{0x88, 0x88, 0x88, 0x88, 0x88, 0x88}}, /*freddie*/
    {{0x81, 0xFF, 0xDD, 0xCC, 0xBB, 0xAA},{0x80, 0xFF, 0xDD, 0xCC, 0xBB, 0xAA}}, /*SNOOP TEST*/
};

int app_ibrt_ui_test_config_load(void *config)
{
    ibrt_pairing_info_t *ibrt_pairing_info_lst = g_ibrt_pairing_info;
    uint32_t lst_size = sizeof(g_ibrt_pairing_info)/sizeof(ibrt_pairing_info_t);
    ibrt_config_t *ibrt_config = (ibrt_config_t *)config;
    struct nvrecord_env_t *nvrecord_env;

    nv_record_env_get(&nvrecord_env);
    if(nvrecord_env->ibrt_mode.tws_connect_success == 0)
    {
        app_ibrt_ui_clear_tws_connect_success_last();
    }
    else
    {
        app_ibrt_ui_set_tws_connect_success_last();
    }

    factory_section_original_btaddr_get(ibrt_config->local_addr.address);
    for(uint32_t i =0; i<lst_size; i++)
    {
        if (!memcmp(ibrt_pairing_info_lst[i].master_bdaddr.address, ibrt_config->local_addr.address, BD_ADDR_LEN))
        {
            ibrt_config->nv_role = IBRT_MASTER;
            ibrt_config->audio_chnl_sel = AUDIO_CHANNEL_SELECT_RCHNL;
            memcpy(ibrt_config->peer_addr.address, ibrt_pairing_info_lst[i].slave_bdaddr.address, BD_ADDR_LEN);
            return 0;
        }
        else if (!memcmp(ibrt_pairing_info_lst[i].slave_bdaddr.address, ibrt_config->local_addr.address, BD_ADDR_LEN))
        {
            ibrt_config->nv_role = IBRT_SLAVE;
            ibrt_config->audio_chnl_sel = AUDIO_CHANNEL_SELECT_LCHNL;
            memcpy(ibrt_config->peer_addr.address, ibrt_pairing_info_lst[i].master_bdaddr.address, BD_ADDR_LEN);
            return 0;
        }
    }
    return -1;
}

void app_ibrt_ui_test_key_init(void)
{
    app_KeyboardTimer_init();
    app_key_handle_clear();
    for (uint8_t i=0; i<ARRAY_SIZE(app_ibrt_ui_test_key_cfg); i++)
    {
        app_key_handle_registration(&app_ibrt_ui_test_key_cfg[i]);
    }
#ifdef IBRT
    // for TWS side decision, the last bit is 1:right, 0:left
    if (app_tws_is_unknown_side())
    {
    	TRACE("The Side is unknown_side ++++++++++\n\n");
        app_tws_set_side_from_addr(factory_section_get_bt_address());
    }
#endif
}

void app_ibrt_ui_test_init(void)
{
    TRACE("%s", __func__);

    app_ibrt_ui_box_init(&box_ble_addr);
}

void app_ibrt_ui_sync_status(uint8_t status)
{
#ifdef ANC_APP
    app_anc_status_post(status);
#endif
}

#endif
