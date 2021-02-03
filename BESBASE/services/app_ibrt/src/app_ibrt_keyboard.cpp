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
#include "cmsis_os.h"
#include <string.h>
#include "app_ibrt_keyboard.h"
#include "app_tws_ibrt_trace.h"
#include "app_ibrt_if.h"
#include "app_ibrt_ui_test.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_ctrl_thread.h"
#include "btapp.h"
#include "apps.h"
#include "app_tws_if.h"
#include "app_factory.h"

#if defined(IBRT)
extern struct BT_DEVICE_T  app_bt_device;
extern void app_bt_volumedown();
extern void app_bt_volumeup();
extern int hfp_volume_get(enum BT_DEVICE_ID_T id);
extern int a2dp_volume_get(enum BT_DEVICE_ID_T id);
extern void app_otaMode_enter(APP_KEY_STATUS *status, void *param);

#ifdef SUPPORT_SIRI
//extern int app_hfp_siri_report();
extern int app_hfp_siri_voice(bool en);
extern int open_siri_flag;
#endif


void app_ibrt_sync_volume_info()
{
    if(app_tws_ibrt_tws_link_connected())
    {
        ibrt_volume_info_t ibrt_volume_req;
        ibrt_volume_req.a2dp_volume = a2dp_volume_get(BT_DEVICE_ID_1);
        ibrt_volume_req.hfp_volume = hfp_volume_get(BT_DEVICE_ID_1);
        tws_ctrl_send_cmd(APP_TWS_CMD_SYNC_VOLUME_INFO,(uint8_t*)&ibrt_volume_req, sizeof(ibrt_volume_req));
    }
}


extern void app_ibrt_remove_history_paired_device(void);
extern void app_ibrt_remove_phone_paired_device(void);
#ifdef IBRT_SEARCH_UI
void app_ibrt_search_ui_handle_key(APP_KEY_STATUS *status, void *param)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    TRACE("%s %d,%d",__func__, status->code, status->event);
    if (APP_KEY_CODE_GOOGLE != status->code)
    {
        switch(status->event)
        {
            case APP_KEY_EVENT_CLICK:
                bt_key_handle_func_click();
                break;

            case APP_KEY_EVENT_DOUBLECLICK:
                TRACE("double kill");
                if(IBRT_UNKNOW==p_ibrt_ctrl->current_role)
                    app_start_tws_serching_direactly();
                else
                    bt_key_handle_func_doubleclick();
                break;

            case APP_KEY_EVENT_LONGPRESS:
                    app_ibrt_ui_event_entry(IBRT_TWS_PAIRING_EVENT);
                    cosonic_trace(bt_pairing,true,"force pairing");


                // dont use this key for customer release due to
                // it is auto triggered by circuit of 3s high-level voltage.
// #if 0
//                 app_ibrt_ui_judge_scan_type(IBRT_FREEMAN_PAIR_TRIGGER,NO_LINK_TYPE, IBRT_UI_NO_ERROR);
//                 app_ibrt_ui_set_freeman_enable();
// #endif
// #if 0
//                 app_bt_volumeup();
//                 if(IBRT_MASTER==p_ibrt_ctrl->current_role)
//                 {
//                     TRACE("ibrt master sync volume to slave !");
//                     app_ibrt_sync_volume_info();
//                 }
// #endif
// #if 0
//                 if(open_siri_flag == 1)
//                 {
//                     TRACE("open siri");
//                     app_hfp_siri_voice(true);
//                     open_siri_flag = 0;
//                 }
//                 else
//                 {
//                     TRACE("evnet none close siri");
//                     app_hfp_siri_voice(false);
//                 }

// #endif
                break;

            case APP_KEY_EVENT_TRIPLECLICK:
               // app_otaMode_enter(NULL,NULL);
                break;
            case HAL_KEY_EVENT_LONGLONGPRESS:
                TRACE("long long press");
                app_ibrt_remove_history_paired_device();
                app_ibrt_remove_phone_paired_device();               
                app_shutdown();
                break;

            case APP_KEY_EVENT_ULTRACLICK:
                TRACE("ultra kill");
                app_voice_report(APP_STATUS_INDICATION_POWERON, 0);
                app_factorymode_enter(); //使用系统默认的函数               
                break;

            case APP_KEY_EVENT_RAMPAGECLICK:
                TRACE("rampage kill!you are crazy!");
                break;

            case APP_KEY_EVENT_UP:
                break;
        }
    }
}
#endif

void app_ibrt_normal_ui_handle_key(APP_KEY_STATUS *status, void *param)
{
    if (APP_KEY_CODE_GOOGLE != status->code)
    {
        switch(status->event)
        {
            case APP_KEY_EVENT_CLICK:
                TRACE("first blood.");
                bt_key_handle_func_click();
                break;

            case APP_KEY_EVENT_DOUBLECLICK:
                TRACE("double kill");
                app_ibrt_if_enter_freeman_pairing();
                break;

            case APP_KEY_EVENT_LONGPRESS:
                app_ibrt_if_enter_pairing_after_tws_connected();
                break;

            case APP_KEY_EVENT_TRIPLECLICK:
                break;

            case HAL_KEY_EVENT_LONGLONGPRESS:
                TRACE("long long press");
                app_shutdown();
                break;

            case APP_KEY_EVENT_ULTRACLICK:
                TRACE("ultra kill");
                break;

            case APP_KEY_EVENT_RAMPAGECLICK:
                TRACE("rampage kill!you are crazy!");
                break;

            case APP_KEY_EVENT_UP:
                break;
        }
    }
}

int app_ibrt_if_keyboard_notify(APP_KEY_STATUS *status, void *param)
{
    if (app_tws_ibrt_slave_ibrt_link_connected())
    {
        tws_ctrl_send_cmd(APP_TWS_CMD_KEYBOARD_REQUEST, (uint8_t *)status, sizeof(APP_KEY_STATUS));
    }
    return 0;
}

void app_ibrt_send_keyboard_request(uint8_t *p_buff, uint16_t length)
{
    if (app_tws_ibrt_slave_ibrt_link_connected())
    {
        app_ibrt_send_cmd_without_rsp(APP_TWS_CMD_KEYBOARD_REQUEST, p_buff, length);
    }
}

void app_ibrt_keyboard_request_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if (app_tws_ibrt_mobile_link_connected())
    {
#ifdef IBRT_SEARCH_UI
        app_ibrt_search_ui_handle_key((APP_KEY_STATUS *)p_buff, NULL);
#else
        app_ibrt_normal_ui_handle_key((APP_KEY_STATUS *)p_buff, NULL);
#endif

        app_ibrt_ui_test_voice_assistant_key((APP_KEY_STATUS *)p_buff, NULL);
    }
}

void app_ibrt_if_start_user_action(uint8_t *p_buff, uint16_t length)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    TRACE("%s role %d code %d",__func__, p_ibrt_ctrl->current_role, p_buff[0]);

    if (IBRT_SLAVE == p_ibrt_ctrl->current_role)
    {
        app_ibrt_ui_send_user_action(p_buff, length);
    }
    else
    {
        app_ibrt_ui_perform_user_action(p_buff, length);
    }
}

extern void app_bt_volumeup();
extern void app_bt_volumedown();

void app_ibrt_ui_perform_user_action(uint8_t *p_buff, uint16_t length)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (IBRT_MASTER != p_ibrt_ctrl->current_role)
    {
        TRACE("%s not ibrt master, skip cmd %02x\n", __func__, p_buff[0]);
        return;
    }

    switch (p_buff[0])
    {
        case IBRT_ACTION_PLAY:
            a2dp_handleKey(AVRCP_KEY_PLAY);
            break;
        case IBRT_ACTION_PAUSE:
            a2dp_handleKey(AVRCP_KEY_PAUSE);
            break;
        case IBRT_ACTION_FORWARD:
            a2dp_handleKey(AVRCP_KEY_FORWARD);
            break;
        case IBRT_ACTION_BACKWARD:
            a2dp_handleKey(AVRCP_KEY_BACKWARD);
            break;
        case IBRT_ACTION_AVRCP_VOLUP:
            a2dp_handleKey(AVRCP_KEY_VOLUME_UP);
            break;
        case IBRT_ACTION_AVRCP_VOLDN:
            a2dp_handleKey(AVRCP_KEY_VOLUME_DOWN);
            break;
        case IBRT_ACTION_HFSCO_CREATE:
            if (!btif_hf_audio_link_is_up())
            {
                btif_pts_hf_create_audio_link();
            }
            else
            {
                TRACE("%s cannot create audio link\n", __func__);
            }
            break;
        case IBRT_ACTION_HFSCO_DISC:
            if (btif_hf_audio_link_is_up())
            {
                btif_pts_hf_disc_audio_link();
            }
            else
            {
                TRACE("%s cannot disc audio link\n", __func__);
            }
            break;
        case IBRT_ACTION_REDIAL:
            if (btif_hf_service_link_is_up())
            {
                btif_pts_hf_redial_call();
            }
            else
            {
                TRACE("%s cannot redial call\n", __func__);
            }
            break;
        case IBRT_ACTION_ANSWER:
            if (btif_hf_service_link_is_up())
            {
                btif_pts_hf_answer_call();
            }
            else
            {
                TRACE("%s cannot answer call\n", __func__);
            }
            break;
        case IBRT_ACTION_HANGUP:
            if (btif_hf_service_link_is_up())
            {
                btif_pts_hf_hangup_call();
            }
            else
            {
                TRACE("%s cannot hangup call\n", __func__);
            }
            break;
        case IBRT_ACTION_LOCAL_VOLUP:
            app_bt_volumeup();
            app_ibrt_sync_volume_info();
            break;
        case IBRT_ACTION_LOCAL_VOLDN:
            app_bt_volumedown();
            app_ibrt_sync_volume_info();
            break;
        default:
            TRACE("%s unknown user action %d\n", __func__, p_buff[0]);
            break;
    }
}

#endif

